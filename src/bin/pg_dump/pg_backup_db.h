/*
 *	Definitions for pg_backup_db.c
 *
 *	IDENTIFICATION
 *		$Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/bin/pg_dump/pg_backup_db.h,v 1.4 2001-03-22 04:00:13 momjian Exp $
 */

#define BLOB_XREF_TABLE "dump_blob_xref"		/* MUST be lower case */

extern void FixupBlobRefs(ArchiveHandle *AH, char *tablename);
extern int	ExecuteSqlCommand(ArchiveHandle *AH, PQExpBuffer qry, char *desc);
extern int	ExecuteSqlCommandBuf(ArchiveHandle *AH, void *qry, int bufLen);

extern void CreateBlobXrefTable(ArchiveHandle *AH);
extern void InsertBlobXref(ArchiveHandle *AH, int old, int new);
extern void StartTransaction(ArchiveHandle *AH);
extern void StartTransactionXref(ArchiveHandle *AH);
extern void CommitTransaction(ArchiveHandle *AH);
extern void CommitTransactionXref(ArchiveHandle *AH);
