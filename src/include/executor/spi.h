/*-------------------------------------------------------------------------
 *
 * spi.h--
 *    
 *
 *-------------------------------------------------------------------------
 */
#ifndef	SPI_H
#define SPI_H

#include <string.h>
#include "postgres.h"
#include "nodes/primnodes.h"
#include "nodes/relation.h"
#include "nodes/execnodes.h"
#include "nodes/plannodes.h"
#include "catalog/pg_proc.h"
#include "parser/parse_query.h"
#include "tcop/pquery.h"
#include "tcop/tcopprot.h"
#include "tcop/utility.h"
#include "tcop/dest.h"
#include "nodes/params.h"
#include "utils/fcache.h"
#include "utils/datum.h"
#include "utils/elog.h"
#include "utils/palloc.h"
#include "utils/syscache.h"
#include "utils/mcxt.h"
#include "utils/portal.h"
#include "catalog/pg_language.h"
#include "access/heapam.h"
#include "access/xact.h"
#include "executor/executor.h"
#include "executor/execdefs.h"

typedef struct {
    uint32	alloced;	/* # of alloced vals */
    uint32	free;		/* # of free vals */
    TupleDesc	tupdesc;	/* tuple descriptor */
    HeapTuple	*vals;		/* tuples */
} SPITupleTable;

#define SPI_ERROR_CONNECT	-1
#define SPI_ERROR_COPY		-2
#define SPI_ERROR_OPUNKNOWN	-3
#define SPI_ERROR_UNCONNECTED	-4
#define SPI_ERROR_CURSOR	-5
#define SPI_ERROR_ARGUMENT	-6
#define SPI_ERROR_PARAM		-7
#define SPI_ERROR_TRANSACTION	-8
#define SPI_ERROR_NOATTRIBUTE	-9
#define SPI_ERROR_NOOUTFUNC	-10
#define SPI_ERROR_TYPUNKNOWN	-11
#define SPI_ERROR_NOENTRY	-12

#define SPI_OK_CONNECT		1
#define SPI_OK_FINISH		2
#define SPI_OK_FETCH		3
#define SPI_OK_UTILITY		4
#define SPI_OK_SELECT		5
#define SPI_OK_SELINTO		6
#define SPI_OK_INSERT		7
#define SPI_OK_DELETE		8
#define SPI_OK_UPDATE		9
#define SPI_OK_CURSOR		10

#define SPI_DSPACE_LOCAL	0
#define SPI_DSPACE_XACT		1
#define SPI_DSPACE_SESSION	2

extern uint32 SPI_processed;
extern SPITupleTable *SPI_tuptable;
extern int SPI_error;

extern int SPI_connect (char *ident);
extern int SPI_finish (void);
extern int SPI_exec (char *src);
extern int SPI_execn (char *src, int tcount);
extern int SPI_execp (int pid, char **values, char *Nulls);
extern int SPI_prepare (char *src, int nargs, Oid *argtypes);
extern int SPI_expplan (int dspace, int start, int count);
extern int SPI_impplan (int dspace, int start, int count);
extern int SPI_expdata (int dspace, int count, void **data, int *len);
extern int SPI_impdata (int dspace, int start, int count, void **data, int **len);

extern int SPI_fnumber (TupleDesc tupdesc, char *fname);
extern char *SPI_getvalue (HeapTuple tuple, TupleDesc tupdesc, int fnumber);
extern char *SPI_getbinval (HeapTuple tuple, TupleDesc tupdesc, int fnumber, bool *isnull);
extern char *SPI_gettype (TupleDesc tupdesc, int fnumber);
extern Oid SPI_gettypeid (TupleDesc tupdesc, int fnumber);
extern char *SPI_getrelname (Relation rel);

#endif /* SPI_H */
