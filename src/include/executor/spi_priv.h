/*-------------------------------------------------------------------------
 *
 * spi.c
 *				Server Programming Interface private declarations
 *
 * $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/include/executor/spi_priv.h,v 1.3 1999-05-13 07:28:58 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef SPI_PRIV_H
#define SPI_PRIV_H

#include "catalog/pg_type.h"
#include "access/printtup.h"

typedef struct
{
	List	   *qtlist;
	uint32		processed;		/* by Executor */
	SPITupleTable *tuptable;
	Portal		portal;			/* portal per procedure */
	MemoryContext savedcxt;
	CommandId	savedId;
} _SPI_connection;

typedef struct
{
	List	   *qtlist;
	List	   *ptlist;
	int			nargs;
	Oid		   *argtypes;
} _SPI_plan;

#define _SPI_CPLAN_CURCXT	0
#define _SPI_CPLAN_PROCXT	1
#define _SPI_CPLAN_TOPCXT	2

#endif /* SPI_PRIV_H */
