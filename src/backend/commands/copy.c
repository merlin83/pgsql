/*-------------------------------------------------------------------------
 *
 * copy.c--
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/commands/copy.c,v 1.68 1999-01-23 22:27:26 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */

#include <string.h>
#include <unistd.h>

#include <postgres.h>

#include <access/heapam.h>
#include <tcop/dest.h>
#include <fmgr.h>
#include <miscadmin.h>
#include <utils/builtins.h>
#include <utils/acl.h>
#include <sys/stat.h>
#include <catalog/pg_index.h>
#include <utils/syscache.h>
#include <utils/memutils.h>
#include <executor/executor.h>
#include <access/transam.h>
#include <catalog/index.h>
#include <access/genam.h>
#include <catalog/pg_type.h>
#include <catalog/catname.h>
#include <catalog/pg_shadow.h>
#include <commands/copy.h>
#include "commands/trigger.h"
#include <storage/fd.h>
#include <libpq/libpq.h>

#ifdef MULTIBYTE
#include "mb/pg_wchar.h"
#endif

#define ISOCTAL(c) (((c) >= '0') && ((c) <= '7'))
#define VALUE(c) ((c) - '0')


/* non-export function prototypes */
static void CopyTo(Relation rel, bool binary, bool oids, FILE *fp, char *delim);
static void CopyFrom(Relation rel, bool binary, bool oids, FILE *fp, char *delim);
static Oid	GetOutputFunction(Oid type);
static Oid	GetTypeElement(Oid type);
static Oid	GetInputFunction(Oid type);
static Oid	IsTypeByVal(Oid type);
static void GetIndexRelations(Oid main_relation_oid,
				  int *n_indices,
				  Relation **index_rels);

#ifdef COPY_PATCH
static void CopyReadNewline(FILE *fp, int *newline);
static char *CopyReadAttribute(FILE *fp, bool *isnull, char *delim, int *newline);

#else
static char *CopyReadAttribute(FILE *fp, bool *isnull, char *delim);

#endif
static void CopyAttributeOut(FILE *fp, char *string, char *delim, int is_array);
static int	CountTuples(Relation relation);

static int	lineno;

/* 
 * Internal communications functions
 */
inline void CopySendData(void *databuf, int datasize, FILE *fp);
inline void CopySendString(char *str, FILE *fp);
inline void CopySendChar(char c, FILE *fp);
inline void CopyGetData(void *databuf, int datasize, FILE *fp);
inline int CopyGetChar(FILE *fp);
inline int CopyGetEof(FILE *fp);
inline int CopyPeekChar(FILE *fp);
inline void CopyDonePeek(FILE *fp, int c, int pickup);

/*
 * CopySendData sends output data either to the file
 *  specified by fp or, if fp is NULL, using the standard
 *  backend->frontend functions
 *
 * CopySendString does the same for null-terminated strings
 * CopySendChar does the same for single characters
 */
inline void CopySendData(void *databuf, int datasize, FILE *fp) {
  if (!fp)
    pq_putnchar(databuf, datasize);
  else
    fwrite(databuf, datasize, 1, fp);
}
    
inline void CopySendString(char *str, FILE *fp) {
  CopySendData(str,strlen(str),fp);
}

inline void CopySendChar(char c, FILE *fp) {
  CopySendData(&c,1,fp);
}

/*
 * CopyGetData reads output data either from the file
 *  specified by fp or, if fp is NULL, using the standard
 *  backend->frontend functions
 *
 * CopyGetChar does the same for single characters
 * CopyGetEof checks if it's EOF on the input
 */
inline void CopyGetData(void *databuf, int datasize, FILE *fp) {
  if (!fp)
    pq_getnchar(databuf, 0, datasize); 
  else 
    fread(databuf, datasize, 1, fp);
}

inline int CopyGetChar(FILE *fp) {
  if (!fp) 
    return pq_getchar();
  else
    return getc(fp);
}

inline int CopyGetEof(FILE *fp) {
  if (!fp)
    return 0; /* Never return EOF when talking to frontend ? */
  else
    return feof(fp);
}

/*
 * CopyPeekChar reads a byte in "peekable" mode.
 * after each call to CopyPeekChar, a call to CopyDonePeek _must_
 * follow.
 * CopyDonePeek will either take the peeked char off the steam 
 * (if pickup is != 0) or leave it on the stream (if pickup == 0)
 */
inline int CopyPeekChar(FILE *fp) {
  if (!fp) 
    return pq_peekchar();
  else
    return getc(fp);
}

inline void CopyDonePeek(FILE *fp, int c, int pickup) {
  if (!fp) {
    if (pickup) {
      /* We want to pick it up - just receive again into dummy buffer */
      char c;
      pq_getnchar(&c, 0, 1);
    }
    /* If we didn't want to pick it up, just leave it where it sits */
  }
  else {
    if (!pickup) {
      /* We don't want to pick it up - so put it back in there */
      ungetc(c,fp);
    }
    /* If we wanted to pick it up, it's already there */
  }
}
    


/*
 *	 DoCopy executes a the SQL COPY statement.
 */

void
DoCopy(char *relname, bool binary, bool oids, bool from, bool pipe,
	   char *filename, char *delim)
{
/*----------------------------------------------------------------------------
  Either unload or reload contents of class <relname>, depending on <from>.

  If <pipe> is false, transfer is between the class and the file named
  <filename>.  Otherwise, transfer is between the class and our regular
  input/output stream.	The latter could be either stdin/stdout or a
  socket, depending on whether we're running under Postmaster control.

  Iff <binary>, unload or reload in the binary format, as opposed to the
  more wasteful but more robust and portable text format.

  If in the text format, delimit columns with delimiter <delim>.

  When loading in the text format from an input stream (as opposed to
  a file), recognize a "." on a line by itself as EOF.	Also recognize
  a stream EOF.  When unloading in the text format to an output stream,
  write a "." on a line by itself at the end of the data.

  Iff <oids>, unload or reload the format that includes OID information.

  Do not allow a Postgres user without superuser privilege to read from
  or write to a file.

  Do not allow the copy if user doesn't have proper permission to access
  the class.
----------------------------------------------------------------------------*/

	static FILE *fp;			/* static for cleanup */
	static bool file_opened = false;	/* static for cleanup */
	Relation	rel;
	extern char *UserName;		/* defined in global.c */
	const AclMode required_access = from ? ACL_WR : ACL_RD;
	int			result;

	/*
	 * Close previous file opened for COPY but failed with elog(). There
	 * should be a better way, but would not be modular. Prevents file
	 * descriptor leak.  bjm 1998/08/29
	 */
	if (file_opened)
		FreeFile(fp);

	rel = heap_openr(relname);
	if (rel == NULL)
		elog(ERROR, "COPY command failed.  Class %s "
			 "does not exist.", relname);

	result = pg_aclcheck(relname, UserName, required_access);
	if (result != ACLCHECK_OK)
		elog(ERROR, "%s: %s", relname, aclcheck_error_strings[result]);
	/* Above should not return */
	else if (!superuser() && !pipe)
		elog(ERROR, "You must have Postgres superuser privilege to do a COPY "
			 "directly to or from a file.  Anyone can COPY to stdout or "
			 "from stdin.  Psql's \\copy command also works for anyone.");
	/* Above should not return. */
	else
	{
		if (from)
		{						/* copy from file to database */
			if (rel->rd_rel->relkind == RELKIND_SEQUENCE)
				elog(ERROR, "You can't change sequence relation %s", relname);
			if (pipe)
			{
				if (IsUnderPostmaster)
				{
					ReceiveCopyBegin();
					fp = NULL;
				}
				else
					fp = stdin;
			}
			else
			{
#ifndef __CYGWIN32__
				fp = AllocateFile(filename, "r");
#else
				fp = AllocateFile(filename, "rb");
#endif
				if (fp == NULL)
					elog(ERROR, "COPY command, running in backend with "
						 "effective uid %d, could not open file '%s' for "
						 "reading.  Errno = %s (%d).",
						 geteuid(), filename, strerror(errno), errno);
				file_opened = true;
			}
			CopyFrom(rel, binary, oids, fp, delim);
		}
		else
		{						/* copy from database to file */
			if (pipe)
			{
				if (IsUnderPostmaster)
				{
					SendCopyBegin();
					fp = NULL;
				}
				else
					fp = stdout;
			}
			else
			{
				mode_t		oumask;		/* Pre-existing umask value */

				oumask = umask((mode_t) 0);
#ifndef __CYGWIN32__
				fp = AllocateFile(filename, "w");
#else
				fp = AllocateFile(filename, "wb");
#endif
				umask(oumask);
				if (fp == NULL)
					elog(ERROR, "COPY command, running in backend with "
						 "effective uid %d, could not open file '%s' for "
						 "writing.  Errno = %s (%d).",
						 geteuid(), filename, strerror(errno), errno);
				file_opened = true;
			}
			CopyTo(rel, binary, oids, fp, delim);
		}
		if (!pipe)
		{
			FreeFile(fp);
			file_opened = false;
		}
		else if (!from && !binary)
		{
			CopySendData("\\.\n",3,fp);
		}
	}
}



static void
CopyTo(Relation rel, bool binary, bool oids, FILE *fp, char *delim)
{
	HeapTuple	tuple;
	HeapScanDesc scandesc;

	int32		attr_count,
				i;
	Form_pg_attribute *attr;
	FmgrInfo   *out_functions;
	Oid			out_func_oid;
	Oid		   *elements;
	int32	   *typmod;
	Datum		value;
	bool		isnull;			/* The attribute we are copying is null */
	char	   *nulls;

	/*
	 * <nulls> is a (dynamically allocated) array with one character per
	 * attribute in the instance being copied.	nulls[I-1] is 'n' if
	 * Attribute Number I is null, and ' ' otherwise.
	 *
	 * <nulls> is meaningful only if we are doing a binary copy.
	 */
	char	   *string;
	int32		ntuples;
	TupleDesc	tupDesc;

	scandesc = heap_beginscan(rel, 0, SnapshotNow, 0, NULL);

	attr_count = rel->rd_att->natts;
	attr = rel->rd_att->attrs;
	tupDesc = rel->rd_att;

	if (!binary)
	{
		out_functions = (FmgrInfo *) palloc(attr_count * sizeof(FmgrInfo));
		elements = (Oid *) palloc(attr_count * sizeof(Oid));
		typmod = (int32 *) palloc(attr_count * sizeof(int32));
		for (i = 0; i < attr_count; i++)
		{
			out_func_oid = (Oid) GetOutputFunction(attr[i]->atttypid);
			fmgr_info(out_func_oid, &out_functions[i]);
			elements[i] = GetTypeElement(attr[i]->atttypid);
			typmod[i] = attr[i]->atttypmod;
		}
		nulls = NULL;			/* meaningless, but compiler doesn't know
								 * that */
	}
	else
	{
		elements = NULL;
		typmod = NULL;
		out_functions = NULL;
		nulls = (char *) palloc(attr_count);
		for (i = 0; i < attr_count; i++)
			nulls[i] = ' ';

		/* XXX expensive */

		ntuples = CountTuples(rel);
		CopySendData(&ntuples, sizeof(int32), fp);
	}

	while (HeapTupleIsValid(tuple = heap_getnext(scandesc, 0)))
	{

		if (oids && !binary)
		{
		        CopySendString(oidout(tuple->t_data->t_oid),fp);
			CopySendChar(delim[0],fp);
		}

		for (i = 0; i < attr_count; i++)
		{
			value = heap_getattr(tuple, i + 1, tupDesc, &isnull);
			if (!binary)
			{
				if (!isnull)
				{
					string = (char *) (*fmgr_faddr(&out_functions[i]))
						(value, elements[i], typmod[i]);
					CopyAttributeOut(fp, string, delim, attr[i]->attnelems);
					pfree(string);
				}
				else
					CopySendString("\\N", fp);	/* null indicator */

				if (i == attr_count - 1)
					CopySendChar('\n', fp);
				else
				{

					/*
					 * when copying out, only use the first char of the
					 * delim string
					 */
					CopySendChar(delim[0], fp);
				}
			}
			else
			{

				/*
				 * only interesting thing heap_getattr tells us in this
				 * case is if we have a null attribute or not.
				 */
				if (isnull)
					nulls[i] = 'n';
			}
		}

		if (binary)
		{
			int32		null_ct = 0,
						length;

			for (i = 0; i < attr_count; i++)
			{
				if (nulls[i] == 'n')
					null_ct++;
			}

			length = tuple->t_len - tuple->t_data->t_hoff;
			CopySendData(&length, sizeof(int32), fp);
			if (oids)
				CopySendData((char *) &tuple->t_data->t_oid, sizeof(int32), fp);

			CopySendData(&null_ct, sizeof(int32), fp);
			if (null_ct > 0)
			{
				for (i = 0; i < attr_count; i++)
				{
					if (nulls[i] == 'n')
					{
						CopySendData(&i, sizeof(int32), fp);
						nulls[i] = ' ';
					}
				}
			}
			CopySendData((char *) tuple->t_data + tuple->t_data->t_hoff, 
					length, fp);
		}
	}

	heap_endscan(scandesc);
	if (binary)
		pfree(nulls);
	else
	{
		pfree(out_functions);
		pfree(elements);
		pfree(typmod);
	}

	heap_close(rel);
}

static void
CopyFrom(Relation rel, bool binary, bool oids, FILE *fp, char *delim)
{
	HeapTuple	tuple;
	AttrNumber	attr_count;
	Form_pg_attribute *attr;
	FmgrInfo   *in_functions;
	int			i;
	Oid			in_func_oid;
	Datum	   *values;
	char	   *nulls,
			   *index_nulls;
	bool	   *byval;
	bool		isnull;
	bool		has_index;
	int			done = 0;
	char	   *string = NULL,
			   *ptr;
	Relation   *index_rels;
	int32		len,
				null_ct,
				null_id;
	int32		ntuples,
				tuples_read = 0;
	bool		reading_to_eof = true;
	Oid		   *elements;
	int32	   *typmod;
	FuncIndexInfo *finfo,
			  **finfoP = NULL;
	TupleDesc  *itupdescArr;
	HeapTuple	pgIndexTup;
	Form_pg_index *pgIndexP = NULL;
	int		   *indexNatts = NULL;
	char	   *predString;
	Node	  **indexPred = NULL;
	TupleDesc	rtupdesc;
	ExprContext *econtext = NULL;

#ifndef OMIT_PARTIAL_INDEX
	TupleTable	tupleTable;
	TupleTableSlot *slot = NULL;

#endif
	int			natts;
	AttrNumber *attnumP;
	Datum	   *idatum;
	int			n_indices;
	InsertIndexResult indexRes;
	TupleDesc	tupDesc;
	Oid			loaded_oid;
	bool		skip_tuple = false;

	tupDesc = RelationGetDescr(rel);
	attr = tupDesc->attrs;
	attr_count = tupDesc->natts;

	has_index = false;

	/*
	 * This may be a scalar or a functional index.	We initialize all
	 * kinds of arrays here to avoid doing extra work at every tuple copy.
	 */

	if (rel->rd_rel->relhasindex)
	{
		GetIndexRelations(RelationGetRelid(rel), &n_indices, &index_rels);
		if (n_indices > 0)
		{
			has_index = true;
			itupdescArr =
				(TupleDesc *) palloc(n_indices * sizeof(TupleDesc));
			pgIndexP =
				(Form_pg_index *) palloc(n_indices * sizeof(Form_pg_index));
			indexNatts = (int *) palloc(n_indices * sizeof(int));
			finfo = (FuncIndexInfo *) palloc(n_indices * sizeof(FuncIndexInfo));
			finfoP = (FuncIndexInfo **) palloc(n_indices * sizeof(FuncIndexInfo *));
			indexPred = (Node **) palloc(n_indices * sizeof(Node *));
			econtext = NULL;
			for (i = 0; i < n_indices; i++)
			{
				itupdescArr[i] = RelationGetDescr(index_rels[i]);
				pgIndexTup =
					SearchSysCacheTuple(INDEXRELID,
					   ObjectIdGetDatum(RelationGetRelid(index_rels[i])),
										0, 0, 0);
				Assert(pgIndexTup);
				pgIndexP[i] = (Form_pg_index) GETSTRUCT(pgIndexTup);
				for (attnumP = &(pgIndexP[i]->indkey[0]), natts = 0;
					 natts < INDEX_MAX_KEYS && *attnumP != InvalidAttrNumber;
					 attnumP++, natts++);
				if (pgIndexP[i]->indproc != InvalidOid)
				{
					FIgetnArgs(&finfo[i]) = natts;
					natts = 1;
					FIgetProcOid(&finfo[i]) = pgIndexP[i]->indproc;
					*(FIgetname(&finfo[i])) = '\0';
					finfoP[i] = &finfo[i];
				}
				else
					finfoP[i] = (FuncIndexInfo *) NULL;
				indexNatts[i] = natts;
				if (VARSIZE(&pgIndexP[i]->indpred) != 0)
				{
					predString = fmgr(F_TEXTOUT, &pgIndexP[i]->indpred);
					indexPred[i] = stringToNode(predString);
					pfree(predString);
					/* make dummy ExprContext for use by ExecQual */
					if (econtext == NULL)
					{
#ifndef OMIT_PARTIAL_INDEX
						tupleTable = ExecCreateTupleTable(1);
						slot = ExecAllocTableSlot(tupleTable);
						econtext = makeNode(ExprContext);
						econtext->ecxt_scantuple = slot;
						rtupdesc = RelationGetDescr(rel);
						slot->ttc_tupleDescriptor = rtupdesc;

						/*
						 * There's no buffer associated with heap tuples
						 * here, so I set the slot's buffer to NULL.
						 * Currently, it appears that the only way a
						 * buffer could be needed would be if the partial
						 * index predicate referred to the "lock" system
						 * attribute.  If it did, then heap_getattr would
						 * call HeapTupleGetRuleLock, which uses the
						 * buffer's descriptor to get the relation id.
						 * Rather than try to fix this, I'll just disallow
						 * partial indexes on "lock", which wouldn't be
						 * useful anyway. --Nels, Nov '92
						 */
						/* SetSlotBuffer(slot, (Buffer) NULL); */
						/* SetSlotShouldFree(slot, false); */
						slot->ttc_buffer = (Buffer) NULL;
						slot->ttc_shouldFree = false;
#endif	 /* OMIT_PARTIAL_INDEX */
					}
				}
				else
					indexPred[i] = NULL;
			}
		}
	}

	if (!binary)
	{
		in_functions = (FmgrInfo *) palloc(attr_count * sizeof(FmgrInfo));
		elements = (Oid *) palloc(attr_count * sizeof(Oid));
		typmod = (int32 *) palloc(attr_count * sizeof(int32));
		for (i = 0; i < attr_count; i++)
		{
			in_func_oid = (Oid) GetInputFunction(attr[i]->atttypid);
			fmgr_info(in_func_oid, &in_functions[i]);
			elements[i] = GetTypeElement(attr[i]->atttypid);
			typmod[i] = attr[i]->atttypmod;
		}
	}
	else
	{
		in_functions = NULL;
		elements = NULL;
		typmod = NULL;
		CopyGetData(&ntuples, sizeof(int32), fp);
		if (ntuples != 0)
			reading_to_eof = false;
	}

	values = (Datum *) palloc(sizeof(Datum) * attr_count);
	nulls = (char *) palloc(attr_count);
	index_nulls = (char *) palloc(attr_count);
	idatum = (Datum *) palloc(sizeof(Datum) * attr_count);
	byval = (bool *) palloc(attr_count * sizeof(bool));

	for (i = 0; i < attr_count; i++)
	{
		nulls[i] = ' ';
		index_nulls[i] = ' ';
		byval[i] = (bool) IsTypeByVal(attr[i]->atttypid);
	}
	values = (Datum *) palloc(sizeof(Datum) * attr_count);

	lineno = 0;
	while (!done)
	{
	  values = (Datum *) palloc(sizeof(Datum) * attr_count);
		if (!binary)
		{
#ifdef COPY_PATCH
			int			newline = 0;

#endif
			lineno++;
			if (oids)
			{
#ifdef COPY_PATCH
				string = CopyReadAttribute(fp, &isnull, delim, &newline);
#else
				string = CopyReadAttribute(fp, &isnull, delim);
#endif
				if (string == NULL)
					done = 1;
				else
				{
					loaded_oid = oidin(string);
					if (loaded_oid < BootstrapObjectIdData)
						elog(ERROR, "COPY TEXT: Invalid Oid. line: %d", lineno);
				}
			}
			for (i = 0; i < attr_count && !done; i++)
			{
#ifdef COPY_PATCH
				string = CopyReadAttribute(fp, &isnull, delim, &newline);
#else
				string = CopyReadAttribute(fp, &isnull, delim);
#endif
				if (isnull)
				{
					values[i] = PointerGetDatum(NULL);
					nulls[i] = 'n';
				}
				else if (string == NULL)
					done = 1;
				else
				{
					values[i] =
						(Datum) (*fmgr_faddr(&in_functions[i])) (string,
															 elements[i],
															  typmod[i]);

					/*
					 * Sanity check - by reference attributes cannot
					 * return NULL
					 */
					if (!PointerIsValid(values[i]) &&
						!(rel->rd_att->attrs[i]->attbyval))
						elog(ERROR, "copy from line %d: Bad file format", lineno);
				}
			}
#ifdef COPY_PATCH
			if (!done)
				CopyReadNewline(fp, &newline);
#endif
		}
		else
		{						/* binary */
			CopyGetData(&len, sizeof(int32), fp);
			if (CopyGetEof(fp))
				done = 1;
			else
			{
				if (oids)
				{
					CopyGetData(&loaded_oid, sizeof(int32), fp);
					if (loaded_oid < BootstrapObjectIdData)
						elog(ERROR, "COPY BINARY: Invalid Oid line: %d", lineno);
				}
				CopyGetData(&null_ct, sizeof(int32), fp);
				if (null_ct > 0)
				{
					for (i = 0; i < null_ct; i++)
					{
						CopyGetData(&null_id, sizeof(int32), fp);
						nulls[null_id] = 'n';
					}
				}

				string = (char *) palloc(len);
				CopyGetData(string, len, fp);

				ptr = string;

				for (i = 0; i < attr_count; i++)
				{
					if (byval[i] && nulls[i] != 'n')
					{

						switch (attr[i]->attlen)
						{
							case sizeof(char):
								values[i] = (Datum) *(unsigned char *) ptr;
								ptr += sizeof(char);
								break;
							case sizeof(short):
								ptr = (char *) SHORTALIGN(ptr);
								values[i] = (Datum) *(unsigned short *) ptr;
								ptr += sizeof(short);
								break;
							case sizeof(int32):
								ptr = (char *) INTALIGN(ptr);
								values[i] = (Datum) *(uint32 *) ptr;
								ptr += sizeof(int32);
								break;
							default:
								elog(ERROR, "COPY BINARY: impossible size! line: %d", lineno);
								break;
						}
					}
					else if (nulls[i] != 'n')
					{
						ptr = (char *)att_align(ptr, attr[i]->attlen, attr[i]->attalign);
						values[i] = (Datum) ptr;
						ptr = att_addlength(ptr, attr[i]->attlen, ptr);
					}
				}
			}
		}
		if (done)
			continue;

		/*
		 * Does it have any sence ? - vadim 12/14/96
		 *
		 * tupDesc = CreateTupleDesc(attr_count, attr);
		 */
		tuple = heap_formtuple(tupDesc, values, nulls);
		if (oids)
			tuple->t_data->t_oid = loaded_oid;

		skip_tuple = false;
		/* BEFORE ROW INSERT Triggers */
		if (rel->trigdesc &&
			rel->trigdesc->n_before_row[TRIGGER_EVENT_INSERT] > 0)
		{
			HeapTuple	newtuple;

			newtuple = ExecBRInsertTriggers(rel, tuple);

			if (newtuple == NULL)		/* "do nothing" */
				skip_tuple = true;
			else if (newtuple != tuple) /* modified by Trigger(s) */
			{
				pfree(tuple);
				tuple = newtuple;
			}
		}

		if (!skip_tuple)
		{
			/* ----------------
			 * Check the constraints of a tuple
			 * ----------------
			 */

			if (rel->rd_att->constr)
				ExecConstraints("CopyFrom", rel, tuple);

			heap_insert(rel, tuple);

			if (has_index)
			{
				for (i = 0; i < n_indices; i++)
				{
					if (indexPred[i] != NULL)
					{
#ifndef OMIT_PARTIAL_INDEX

						/*
						 * if tuple doesn't satisfy predicate, don't
						 * update index
						 */
						slot->val = tuple;
						/* SetSlotContents(slot, tuple); */
						if (ExecQual((List *) indexPred[i], econtext) == false)
							continue;
#endif	 /* OMIT_PARTIAL_INDEX */
					}
					FormIndexDatum(indexNatts[i],
								(AttrNumber *) &(pgIndexP[i]->indkey[0]),
								   tuple,
								   tupDesc,
								   idatum,
								   index_nulls,
								   finfoP[i]);
					indexRes = index_insert(index_rels[i], idatum, index_nulls,
											&(tuple->t_self), rel);
					if (indexRes)
						pfree(indexRes);
				}
			}
			/* AFTER ROW INSERT Triggers */
			if (rel->trigdesc &&
				rel->trigdesc->n_after_row[TRIGGER_EVENT_INSERT] > 0)
				ExecARInsertTriggers(rel, tuple);
		}

		if (binary)
			pfree(string);

		for (i = 0; i < attr_count; i++)
		{
			if (!byval[i] && nulls[i] != 'n')
			{
				if (!binary)
					pfree((void *) values[i]);
			}
			else if (nulls[i] == 'n')
				nulls[i] = ' ';
		}

		pfree(tuple);
		tuples_read++;

		if (!reading_to_eof && ntuples == tuples_read)
			done = true;
	}
	pfree(values);
	pfree(nulls);
	if (!binary)
	{
		pfree(in_functions);
		pfree(elements);
		pfree(typmod);
	}
	pfree(byval);

	/* comments in execUtils.c */
	if (has_index)
	{
		for (i = 0; i < n_indices; i++)
		{
			if (index_rels[i] == NULL)
				continue;
			if ((index_rels[i])->rd_rel->relam != BTREE_AM_OID && 
				(index_rels[i])->rd_rel->relam != HASH_AM_OID)
				UnlockRelation(index_rels[i], AccessExclusiveLock);
			index_close(index_rels[i]);
		}
	}
	heap_close(rel);
}



static Oid
GetOutputFunction(Oid type)
{
	HeapTuple	typeTuple;

	typeTuple = SearchSysCacheTuple(TYPOID,
									ObjectIdGetDatum(type),
									0, 0, 0);

	if (HeapTupleIsValid(typeTuple))
		return (int) ((Form_pg_type) GETSTRUCT(typeTuple))->typoutput;

	elog(ERROR, "GetOutputFunction: Cache lookup of type %d failed", type);
	return InvalidOid;
}

static Oid
GetTypeElement(Oid type)
{
	HeapTuple	typeTuple;

	typeTuple = SearchSysCacheTuple(TYPOID,
									ObjectIdGetDatum(type),
									0, 0, 0);

	if (HeapTupleIsValid(typeTuple))
		return (int) ((Form_pg_type) GETSTRUCT(typeTuple))->typelem;

	elog(ERROR, "GetOutputFunction: Cache lookup of type %d failed", type);
	return InvalidOid;
}

static Oid
GetInputFunction(Oid type)
{
	HeapTuple	typeTuple;

	typeTuple = SearchSysCacheTuple(TYPOID,
									ObjectIdGetDatum(type),
									0, 0, 0);

	if (HeapTupleIsValid(typeTuple))
		return (int) ((Form_pg_type) GETSTRUCT(typeTuple))->typinput;

	elog(ERROR, "GetInputFunction: Cache lookup of type %d failed", type);
	return InvalidOid;
}

static Oid
IsTypeByVal(Oid type)
{
	HeapTuple	typeTuple;

	typeTuple = SearchSysCacheTuple(TYPOID,
									ObjectIdGetDatum(type),
									0, 0, 0);

	if (HeapTupleIsValid(typeTuple))
		return (int) ((Form_pg_type) GETSTRUCT(typeTuple))->typbyval;

	elog(ERROR, "GetInputFunction: Cache lookup of type %d failed", type);

	return InvalidOid;
}

/*
 * Given the OID of a relation, return an array of index relation descriptors
 * and the number of index relations.  These relation descriptors are open
 * using heap_open().
 *
 * Space for the array itself is palloc'ed.
 */

typedef struct rel_list
{
	Oid			index_rel_oid;
	struct rel_list *next;
} RelationList;

static void
GetIndexRelations(Oid main_relation_oid,
				  int *n_indices,
				  Relation **index_rels)
{
	RelationList *head,
			   *scan;
	Relation	pg_index_rel;
	HeapScanDesc scandesc;
	Oid			index_relation_oid;
	HeapTuple	tuple;
	TupleDesc	tupDesc;
	int			i;
	bool		isnull;

	pg_index_rel = heap_openr(IndexRelationName);
	scandesc = heap_beginscan(pg_index_rel, 0, SnapshotNow, 0, NULL);
	tupDesc = RelationGetDescr(pg_index_rel);

	*n_indices = 0;

	head = (RelationList *) palloc(sizeof(RelationList));
	scan = head;
	head->next = NULL;

	while (HeapTupleIsValid(tuple = heap_getnext(scandesc, 0)))
	{

		index_relation_oid =
			(Oid) DatumGetInt32(heap_getattr(tuple, 2,
											 tupDesc, &isnull));
		if (index_relation_oid == main_relation_oid)
		{
			scan->index_rel_oid =
				(Oid) DatumGetInt32(heap_getattr(tuple,
												 Anum_pg_index_indexrelid,
												 tupDesc, &isnull));
			(*n_indices)++;
			scan->next = (RelationList *) palloc(sizeof(RelationList));
			scan = scan->next;
		}
	}

	heap_endscan(scandesc);
	heap_close(pg_index_rel);

	/* We cannot trust to relhasindex of the main_relation now, so... */
	if (*n_indices == 0)
		return;

	*index_rels = (Relation *) palloc(*n_indices * sizeof(Relation));

	for (i = 0, scan = head; i < *n_indices; i++, scan = scan->next)
	{
		(*index_rels)[i] = index_open(scan->index_rel_oid);
		/* comments in execUtils.c */
		if ((*index_rels)[i] != NULL && 
			((*index_rels)[i])->rd_rel->relam != BTREE_AM_OID &&
			((*index_rels)[i])->rd_rel->relam != HASH_AM_OID)
			LockRelation((*index_rels)[i], AccessExclusiveLock);
	}

	for (i = 0, scan = head; i < *n_indices + 1; i++)
	{
		scan = head->next;
		pfree(head);
		head = scan;
	}
}

#define EXT_ATTLEN 5*8192

/*
   returns 1 is c is in s
*/
static bool
inString(char c, char *s)
{
	int			i;

	if (s)
	{
		i = 0;
		while (s[i] != '\0')
		{
			if (s[i] == c)
				return 1;
			i++;
		}
	}
	return 0;
}

#ifdef COPY_PATCH
/*
 * Reads input from fp until an end of line is seen.
 */

static void
CopyReadNewline(FILE *fp, int *newline)
{
	if (!*newline)
	{
		elog(NOTICE, "CopyReadNewline: line %d - extra fields ignored", lineno);
		while (!CopyGetEof(fp) && (CopyGetChar(fp) != '\n'));
	}
	*newline = 0;
}

#endif

/*
 * Reads input from fp until eof is seen.  If we are reading from standard
 * input, AND we see a dot on a line by itself (a dot followed immediately
 * by a newline), we exit as if we saw eof.  This is so that copy pipelines
 * can be used as standard input.
 */

static char *
#ifdef COPY_PATCH
CopyReadAttribute(FILE *fp, bool *isnull, char *delim, int *newline)
#else
CopyReadAttribute(FILE *fp, bool *isnull, char *delim)
#endif
{
	static char attribute[EXT_ATTLEN];
	char		c;
	int			done = 0;
	int			i = 0;

#ifdef MULTIBYTE
	int			mblen;
	int			encoding;
	unsigned char s[2];
	int			j;

#endif

#ifdef MULTIBYTE
	encoding = pg_get_client_encoding();
	s[1] = 0;
#endif

#ifdef COPY_PATCH
	/* if last delimiter was a newline return a NULL attribute */
	if (*newline)
	{
		*isnull = (bool) true;
		return NULL;
	}
#endif

	*isnull = (bool) false;		/* set default */
	if (CopyGetEof(fp))
		return NULL;

	while (!done)
	{
		c = CopyGetChar(fp);

		if (CopyGetEof(fp))
			return NULL;
		else if (c == '\\')
		{
			c = CopyGetChar(fp);
			if (CopyGetEof(fp))
				return NULL;
			switch (c)
			{
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
					{
						int			val;

						val = VALUE(c);
						c = CopyPeekChar(fp);
						if (ISOCTAL(c))
						{
							val = (val << 3) + VALUE(c);
							CopyDonePeek(fp, c, 1); /* Pick up the character! */
							c = CopyPeekChar(fp);
							if (ISOCTAL(c)) {
							        CopyDonePeek(fp,c,1); /* pick up! */
								val = (val << 3) + VALUE(c);
							}
							else
							{
							        if (CopyGetEof(fp)) {
								        CopyDonePeek(fp,c,1); /* pick up */
									return NULL;
								}
								CopyDonePeek(fp,c,0); /* Return to stream! */
							}
						}
						else
						{
							if (CopyGetEof(fp))
								return NULL;
							CopyDonePeek(fp,c,0); /* Return to stream! */
						}
						c = val & 0377;
					}
					break;
				case 'b':
					c = '\b';
					break;
				case 'f':
					c = '\f';
					break;
				case 'n':
					c = '\n';
					break;
				case 'r':
					c = '\r';
					break;
				case 't':
					c = '\t';
					break;
				case 'v':
					c = '\v';
					break;
				case 'N':
					attribute[0] = '\0';		/* just to be safe */
					*isnull = (bool) true;
					break;
				case '.':
					c = CopyGetChar(fp);
					if (c != '\n')
						elog(ERROR, "CopyReadAttribute - end of record marker corrupted. line: %d", lineno);
					return NULL;
					break;
			}
		}
		else if (inString(c, delim) || c == '\n')
		{
#ifdef COPY_PATCH
			if (c == '\n')
				*newline = 1;
#endif
			done = 1;
		}
		if (!done)
			attribute[i++] = c;
#ifdef MULTIBYTE
		s[0] = c;
		mblen = pg_encoding_mblen(encoding, s);
		mblen--;
		for (j = 0; j < mblen; j++)
		{
			c = CopyGetChar(fp);
			if (CopyGetEof(fp))
				return NULL;
			attribute[i++] = c;
		}
#endif
		if (i == EXT_ATTLEN - 1)
			elog(ERROR, "CopyReadAttribute - attribute length too long. line: %d", lineno);
	}
	attribute[i] = '\0';
#ifdef MULTIBYTE
	return (pg_client_to_server((unsigned char *) attribute, strlen(attribute)));
#else
	return &attribute[0];
#endif
}

static void
CopyAttributeOut(FILE *fp, char *server_string, char *delim, int is_array)
{
	char	   *string;
	char		c;

#ifdef MULTIBYTE
	int			mblen;
	int			encoding;
	int			i;

#endif

#ifdef MULTIBYTE
	string = pg_server_to_client(server_string, strlen(server_string));
	encoding = pg_get_client_encoding();
#else
	string = server_string;
#endif

#ifdef MULTIBYTE
	for (; (mblen = pg_encoding_mblen(encoding, string)) &&
		 ((c = *string) != '\0'); string += mblen)
#else
	for (; (c = *string) != '\0'; string++)
#endif
	{
		if (c == delim[0] || c == '\n' ||
			(c == '\\' && !is_array))
			CopySendChar('\\', fp);
		else if (c == '\\' && is_array)
		{
			if (*(string + 1) == '\\')
			{
				/* translate \\ to \\\\ */
				CopySendChar('\\', fp);
				CopySendChar('\\', fp);
				CopySendChar('\\', fp);
				string++;
			}
			else if (*(string + 1) == '"')
			{
				/* translate \" to \\\" */
				CopySendChar('\\', fp);
				CopySendChar('\\', fp);
			}
		}
#ifdef MULTIBYTE
		for (i = 0; i < mblen; i++)
			CopySendChar(*(string + i), fp);
#else
		CopySendChar(*string, fp);
#endif
	}
}

/*
 * Returns the number of tuples in a relation.	Unfortunately, currently
 * must do a scan of the entire relation to determine this.
 *
 * relation is expected to be an open relation descriptor.
 */
static int
CountTuples(Relation relation)
{
	HeapScanDesc scandesc;
	HeapTuple	tuple;

	int			i;

	scandesc = heap_beginscan(relation, 0, SnapshotNow, 0, NULL);

	i = 0;
	while (HeapTupleIsValid(tuple = heap_getnext(scandesc, 0)))
		i++;
	heap_endscan(scandesc);
	return i;
}
