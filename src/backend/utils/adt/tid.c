/*-------------------------------------------------------------------------
 *
 * tid.c
 *	  Functions for the built-in type tuple id
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/adt/tid.c,v 1.6.2.1 1999-08-02 05:24:57 scrappy Exp $
 *
 * NOTES
 *	  input routine largely stolen from boxin().
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "utils/builtins.h"

#define LDELIM			'('
#define RDELIM			')'
#define DELIM			','
#define NTIDARGS		2

/* ----------------------------------------------------------------
 *		tidin
 * ----------------------------------------------------------------
 */
ItemPointer
tidin(char *str)
{
	char	   *p,
			   *coord[NTIDARGS];
	int			i;
	ItemPointer result;

	BlockNumber blockNumber;
	OffsetNumber offsetNumber;

	if (str == NULL)
		return NULL;

	for (i = 0, p = str; *p && i < NTIDARGS && *p != RDELIM; p++)
		if (*p == DELIM || (*p == LDELIM && !i))
			coord[i++] = p + 1;

	if (i < NTIDARGS - 1)
		return NULL;

	blockNumber = (BlockNumber) atoi(coord[0]);
	offsetNumber = (OffsetNumber) atoi(coord[1]);

	result = (ItemPointer) palloc(sizeof(ItemPointerData));

	ItemPointerSet(result, blockNumber, offsetNumber);

	return result;
}

/* ----------------------------------------------------------------
 *		tidout
 * ----------------------------------------------------------------
 */
char *
tidout(ItemPointer itemPtr)
{
	BlockNumber blockNumber;
	OffsetNumber offsetNumber;
	BlockId		blockId;
	char		buf[32];
	char	   *str;

	blockId = &(itemPtr->ip_blkid);

	blockNumber = BlockIdGetBlockNumber(blockId);
	offsetNumber = itemPtr->ip_posid;

	sprintf(buf, "(%d,%d)", blockNumber, offsetNumber);

	str = (char *) palloc(strlen(buf) + 1);
	strcpy(str, buf);

	return str;
}
