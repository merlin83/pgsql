/*-------------------------------------------------------------------------
 *
 * ts_type.h
 *	  Definitions for the tsvector and tsquery types
 *
 * Copyright (c) 1998-2007, PostgreSQL Global Development Group
 *
 * $PostgreSQL: pgsql/src/include/tsearch/ts_type.h,v 1.2 2007/09/07 15:09:56 teodor Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef _PG_TSTYPE_H_
#define _PG_TSTYPE_H_

#include "fmgr.h"
#include "utils/pg_crc.h"


/*
 * TSVector type.
 * Note, tsvectorsend/recv believe that sizeof(WordEntry) == 4
 */

typedef struct
{
	uint32
				haspos:1,
				len:11,			/* MAX 2Kb */
				pos:20;			/* MAX 1Mb */
} WordEntry;

#define MAXSTRLEN ( (1<<11) - 1)
#define MAXSTRPOS ( (1<<20) - 1)

/*
 * Equivalent to
 * typedef struct {
 *		uint16
 *			weight:2,
 *			pos:14;
 * }
 */

typedef uint16 WordEntryPos;

#define WEP_GETWEIGHT(x)	( (x) >> 14 )
#define WEP_GETPOS(x)		( (x) & 0x3fff )

#define WEP_SETWEIGHT(x,v)  ( (x) = ( (v) << 14 ) | ( (x) & 0x3fff ) )
#define WEP_SETPOS(x,v)		( (x) = ( (x) & 0xc000 ) | ( (v) & 0x3fff ) )

#define MAXENTRYPOS (1<<14)
#define MAXNUMPOS	(256)
#define LIMITPOS(x) ( ( (x) >= MAXENTRYPOS ) ? (MAXENTRYPOS-1) : (x) )

/*
 * Structure of tsvector datatype:
 * 1) standard varlena header
 * 2) int4		size - number of lexemes or WordEntry array, which is the same
 * 3) Array of WordEntry - sorted array, comparison based on word's length
 *							and strncmp(). WordEntry->pos points number of
 *							bytes from end of WordEntry array to start of
 *							corresponding lexeme.
 * 4) Lexeme's storage:
 *	  SHORTALIGNED(lexeme) and position information if it exists
 *	  Position information: first int2 - is a number of positions and it
 *	  follows array of WordEntryPos
 */

typedef struct
{
	int32		vl_len_;		/* varlena header (do not touch directly!) */
	uint32		size;
	char		data[1];
} TSVectorData;

typedef TSVectorData *TSVector;

#define DATAHDRSIZE (VARHDRSZ + sizeof(int4))
#define CALCDATASIZE(x, lenstr) ( (x) * sizeof(WordEntry) + DATAHDRSIZE + (lenstr) )
#define ARRPTR(x)	( (WordEntry*) ( (char*)(x) + DATAHDRSIZE ) )
#define STRPTR(x)	( (char*)(x) + DATAHDRSIZE + ( sizeof(WordEntry) * ((TSVector)(x))->size ) )
#define STRSIZE(x)	( ((TSVector)(x))->len - DATAHDRSIZE - ( sizeof(WordEntry) * ((TSVector)(x))->size ) )
#define _POSDATAPTR(x,e)	(STRPTR(x)+((WordEntry*)(e))->pos+SHORTALIGN(((WordEntry*)(e))->len))
#define POSDATALEN(x,e) ( ( ((WordEntry*)(e))->haspos ) ? (*(uint16*)_POSDATAPTR(x,e)) : 0 )
#define POSDATAPTR(x,e) ( (WordEntryPos*)( _POSDATAPTR(x,e)+sizeof(uint16) ) )

/*
 * fmgr interface macros
 */

#define DatumGetTSVector(X)			((TSVector) PG_DETOAST_DATUM(X))
#define DatumGetTSVectorCopy(X)		((TSVector) PG_DETOAST_DATUM_COPY(X))
#define TSVectorGetDatum(X)			PointerGetDatum(X)
#define PG_GETARG_TSVECTOR(n)		DatumGetTSVector(PG_GETARG_DATUM(n))
#define PG_GETARG_TSVECTOR_COPY(n)	DatumGetTSVectorCopy(PG_GETARG_DATUM(n))
#define PG_RETURN_TSVECTOR(x)		return TSVectorGetDatum(x)

/*
 * I/O
 */
extern Datum tsvectorin(PG_FUNCTION_ARGS);
extern Datum tsvectorout(PG_FUNCTION_ARGS);
extern Datum tsvectorsend(PG_FUNCTION_ARGS);
extern Datum tsvectorrecv(PG_FUNCTION_ARGS);

/*
 * operations with tsvector
 */
extern Datum tsvector_lt(PG_FUNCTION_ARGS);
extern Datum tsvector_le(PG_FUNCTION_ARGS);
extern Datum tsvector_eq(PG_FUNCTION_ARGS);
extern Datum tsvector_ne(PG_FUNCTION_ARGS);
extern Datum tsvector_ge(PG_FUNCTION_ARGS);
extern Datum tsvector_gt(PG_FUNCTION_ARGS);
extern Datum tsvector_cmp(PG_FUNCTION_ARGS);

extern Datum tsvector_length(PG_FUNCTION_ARGS);
extern Datum tsvector_strip(PG_FUNCTION_ARGS);
extern Datum tsvector_setweight(PG_FUNCTION_ARGS);
extern Datum tsvector_concat(PG_FUNCTION_ARGS);
extern Datum tsvector_update_trigger_byid(PG_FUNCTION_ARGS);
extern Datum tsvector_update_trigger_bycolumn(PG_FUNCTION_ARGS);

extern Datum ts_match_vq(PG_FUNCTION_ARGS);
extern Datum ts_match_qv(PG_FUNCTION_ARGS);
extern Datum ts_match_tt(PG_FUNCTION_ARGS);
extern Datum ts_match_tq(PG_FUNCTION_ARGS);

extern Datum ts_stat1(PG_FUNCTION_ARGS);
extern Datum ts_stat2(PG_FUNCTION_ARGS);

extern Datum ts_rank_tt(PG_FUNCTION_ARGS);
extern Datum ts_rank_wtt(PG_FUNCTION_ARGS);
extern Datum ts_rank_ttf(PG_FUNCTION_ARGS);
extern Datum ts_rank_wttf(PG_FUNCTION_ARGS);
extern Datum ts_rankcd_tt(PG_FUNCTION_ARGS);
extern Datum ts_rankcd_wtt(PG_FUNCTION_ARGS);
extern Datum ts_rankcd_ttf(PG_FUNCTION_ARGS);
extern Datum ts_rankcd_wttf(PG_FUNCTION_ARGS);


/*
 * TSQuery
 *
 *
 */

typedef int8 QueryItemType;

/* Valid values for QueryItemType: */
#define QI_VAL 1
#define QI_OPR 2
#define QI_VALSTOP 3	/* This is only used in an intermediate stack representation in parse_tsquery. It's not a legal type elsewhere. */

/*
 * QueryItem is one node in tsquery - operator or operand.
 */
typedef struct
{
	QueryItemType		type;	/* operand or kind of operator (ts_tokentype) */
	int8		weight;			/* weights of operand to search. It's a bitmask of allowed weights.
								 * if it =0 then any weight are allowed */
	int32	valcrc;				/* XXX: pg_crc32 would be a more appropriate data type, 
								 * but we use comparisons to signed integers in the code. 
								 * They would need to be changed as well. */

	/* pointer to text value of operand, must correlate with WordEntry */
	uint32
				istrue:1,		/* use for ranking in Cover */
				length:11,
				distance:20;
} QueryOperand;


/* Legal values for QueryOperator.operator */
#define	OP_NOT	1
#define	OP_AND	2
#define	OP_OR	3

typedef struct 
{
	QueryItemType	type;
	int8		oper;		/* see above */
	int16		left;		/* pointer to left operand. Right operand is
							 * item + 1, left operand is placed
							 * item+item->left */
} QueryOperator;

/*
 * Note: TSQuery is 4-bytes aligned, so make sure there's no fields
 * inside QueryItem requiring 8-byte alignment, like int64.
 */
typedef union
{
	QueryItemType	type;
	QueryOperator operator;
	QueryOperand operand;
} QueryItem;

/*
 * Storage:
 *	(len)(size)(array of QueryItem)(operands as '\0'-terminated c-strings)
 */

typedef struct
{
	int32		vl_len_;		/* varlena header (do not touch directly!) */
	int4		size;			/* number of QueryItems */
	char		data[1];
} TSQueryData;

typedef TSQueryData *TSQuery;

#define HDRSIZETQ	( VARHDRSZ + sizeof(int4) )

/* Computes the size of header and all QueryItems. size is the number of
 * QueryItems, and lenofoperand is the total length of all operands
 */
#define COMPUTESIZE(size, lenofoperand)	( HDRSIZETQ + (size) * sizeof(QueryItem) + (lenofoperand) )

/* Returns a pointer to the first QueryItem in a TSVector */
#define GETQUERY(x)  ((QueryItem*)( (char*)(x)+HDRSIZETQ ))

/* Returns a pointer to the beginning of operands in a TSVector */
#define GETOPERAND(x)	( (char*)GETQUERY(x) + ((TSQuery)(x))->size * sizeof(QueryItem) )

/*
 * fmgr interface macros
 * Note, TSQuery type marked as plain storage, so it can't be toasted
 * but PG_DETOAST_DATUM_COPY is used for simplicity
 */

#define DatumGetTSQuery(X)			((TSQuery) DatumGetPointer(X))
#define DatumGetTSQueryCopy(X)		((TSQuery) PG_DETOAST_DATUM_COPY(X))
#define TSQueryGetDatum(X)			PointerGetDatum(X)
#define PG_GETARG_TSQUERY(n)		DatumGetTSQuery(PG_GETARG_DATUM(n))
#define PG_GETARG_TSQUERY_COPY(n)	DatumGetTSQueryCopy(PG_GETARG_DATUM(n))
#define PG_RETURN_TSQUERY(x)		return TSQueryGetDatum(x)

/*
 * I/O
 */
extern Datum tsqueryin(PG_FUNCTION_ARGS);
extern Datum tsqueryout(PG_FUNCTION_ARGS);
extern Datum tsquerysend(PG_FUNCTION_ARGS);
extern Datum tsqueryrecv(PG_FUNCTION_ARGS);

/*
 * operations with tsquery
 */
extern Datum tsquery_lt(PG_FUNCTION_ARGS);
extern Datum tsquery_le(PG_FUNCTION_ARGS);
extern Datum tsquery_eq(PG_FUNCTION_ARGS);
extern Datum tsquery_ne(PG_FUNCTION_ARGS);
extern Datum tsquery_ge(PG_FUNCTION_ARGS);
extern Datum tsquery_gt(PG_FUNCTION_ARGS);
extern Datum tsquery_cmp(PG_FUNCTION_ARGS);

extern Datum tsquerytree(PG_FUNCTION_ARGS);
extern Datum tsquery_numnode(PG_FUNCTION_ARGS);

extern Datum tsquery_and(PG_FUNCTION_ARGS);
extern Datum tsquery_or(PG_FUNCTION_ARGS);
extern Datum tsquery_not(PG_FUNCTION_ARGS);

extern Datum tsquery_rewrite(PG_FUNCTION_ARGS);
extern Datum tsquery_rewrite_query(PG_FUNCTION_ARGS);
extern Datum ts_rewrite_accum(PG_FUNCTION_ARGS);
extern Datum ts_rewrite_finish(PG_FUNCTION_ARGS);

extern Datum tsq_mcontains(PG_FUNCTION_ARGS);
extern Datum tsq_mcontained(PG_FUNCTION_ARGS);

#endif   /* _PG_TSTYPE_H_ */
