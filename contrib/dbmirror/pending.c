/****************************************************************************
 * pending.c
 * $Id: pending.c,v 1.5 2002-09-26 05:24:30 momjian Exp $
 *
 * This file contains a trigger for Postgresql-7.x to record changes to tables
 * to a pending table for mirroring.
 * All tables that should be mirrored should have this trigger hooked up to it.
 *
 *	 Written by Steven Singer (ssinger@navtechinc.com)
 *	 (c) 2001-2002 Navtech Systems Support Inc.
 *	 Released under the GNU Public License version 2. See COPYING.
 *
 *
 *	 This program is distributed in the hope that it will be useful,
 *	 but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	 GNU General Public License for more details.
 *
 *
 ***************************************************************************/
#include <postgres.h>

#include <executor/spi.h>
#include <commands/trigger.h>

enum FieldUsage
{
	PRIMARY = 0, NONPRIMARY, ALL, NUM_FIELDUSAGE
};

int storePending(char *cpTableName, HeapTuple tBeforeTuple,
			 HeapTuple tAfterTuple,
			 TupleDesc tTupdesc,
			 TriggerData *tpTrigdata, char cOp);

int storeKeyInfo(char *cpTableName, HeapTuple tTupleData, TupleDesc tTuplDesc,
			 TriggerData *tpTrigdata);
int storeData(char *cpTableName, HeapTuple tTupleData, TupleDesc tTupleDesc,
		  TriggerData *tpTrigData, int iIncludeKeyData);

int2vector *getPrimaryKey(Oid tblOid);

char *packageData(HeapTuple tTupleData, TupleDesc tTupleDecs,
			TriggerData *tTrigData,
			enum FieldUsage eKeyUsage);

#define BUFFER_SIZE 256
#define MAX_OID_LEN 10


extern Datum recordchange(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(recordchange);


/*****************************************************************************
 * The entry point for the trigger function.
 * The Trigger takes a single SQL 'text' argument indicating the name of the
 * table the trigger was applied to.  If this name is incorrect so will the
 * mirroring.
 ****************************************************************************/
Datum
recordchange(PG_FUNCTION_ARGS)
{
	TriggerData *trigdata;
	TupleDesc	tupdesc;
	HeapTuple	beforeTuple = NULL;
	HeapTuple	afterTuple = NULL;
	HeapTuple	retTuple = NULL;
	char	   *tblname;
	char		op = 0;

	if (fcinfo->context != NULL)
	{

		if (SPI_connect() < 0)
		{
			elog(NOTICE, "storePending could not connect to SPI");
			return -1;
		}
		trigdata = (TriggerData *) fcinfo->context;
		/* Extract the table name */
		tblname = SPI_getrelname(trigdata->tg_relation);
		tupdesc = trigdata->tg_relation->rd_att;
		if (TRIGGER_FIRED_BY_UPDATE(trigdata->tg_event))
		{
			retTuple = trigdata->tg_newtuple;
			beforeTuple = trigdata->tg_trigtuple;
			afterTuple = trigdata->tg_newtuple;
			op = 'u';

		}
		else if (TRIGGER_FIRED_BY_INSERT(trigdata->tg_event))
		{
			retTuple = trigdata->tg_trigtuple;
			afterTuple = trigdata->tg_trigtuple;
			op = 'i';
		}
		else if (TRIGGER_FIRED_BY_DELETE(trigdata->tg_event))
		{
			retTuple = trigdata->tg_trigtuple;
			beforeTuple = trigdata->tg_trigtuple;
			op = 'd';
		}

		if (storePending(tblname, beforeTuple, afterTuple, tupdesc, trigdata, op))
		{
			/* An error occoured. Skip the operation. */
			elog(ERROR, "Operation could not be mirrored");
			return PointerGetDatum(NULL);

		}
#if defined DEBUG_OUTPUT
		elog(NOTICE, "Returning on success");
#endif
		SPI_finish();
		return PointerGetDatum(retTuple);
	}
	else
	{
		/*
		 * Not being called as a trigger.
		 */
		return PointerGetDatum(NULL);
	}
}


/*****************************************************************************
 * Constructs and executes an SQL query to write a record of this tuple change
 * to the pending table.
 *****************************************************************************/
int
storePending(char *cpTableName, HeapTuple tBeforeTuple,
			 HeapTuple tAfterTuple,
			 TupleDesc tTupDesc,
			 TriggerData *tpTrigData, char cOp)
{
	char	   *cpQueryBase = "INSERT INTO \"Pending\" (\"TableName\",\"Op\",\"XID\") VALUES ($1,$2,$3)";

	int			iResult = 0;
	HeapTuple	tCurTuple;

	//Points the current tuple(before or after)
		Datum		saPlanData[4];
	Oid			taPlanArgTypes[3] = {NAMEOID, CHAROID, INT4OID};
	void	   *vpPlan;

	tCurTuple = tBeforeTuple ? tBeforeTuple : tAfterTuple;




	vpPlan = SPI_prepare(cpQueryBase, 3, taPlanArgTypes);
	if (vpPlan == NULL)
		elog(NOTICE, "Error creating plan");
	/* SPI_saveplan(vpPlan); */

	saPlanData[0] = PointerGetDatum(cpTableName);
	saPlanData[1] = CharGetDatum(cOp);
	saPlanData[2] = Int32GetDatum(GetCurrentTransactionId());


	iResult = SPI_execp(vpPlan, saPlanData, NULL, 1);
	if (iResult < 0)
		elog(NOTICE, "storedPending fired (%s) returned %d", cpQueryBase, iResult);


#if defined DEBUG_OUTPUT
	elog(NOTICE, "row successfully stored in pending table");
#endif

	if (cOp == 'd')
	{
		/**
		 * This is a record of a delete operation.
		 * Just store the key data.
		 */
		iResult = storeKeyInfo(cpTableName, tBeforeTuple, tTupDesc, tpTrigData);
	}
	else if (cOp == 'i')
	{
		/**
		 * An Insert operation.
		 * Store all data
		 */
		iResult = storeData(cpTableName, tAfterTuple, tTupDesc, tpTrigData, TRUE);

	}
	else
	{
		/* op must be an update. */
		iResult = storeKeyInfo(cpTableName, tBeforeTuple, tTupDesc, tpTrigData);
		iResult = iResult ? iResult : storeData(cpTableName, tAfterTuple, tTupDesc,
												tpTrigData, TRUE);
	}

#if defined DEBUG_OUTPUT
	elog(NOTICE, "DOne storing keyinfo");
#endif

	return iResult;

}

int
storeKeyInfo(char *cpTableName, HeapTuple tTupleData,
			 TupleDesc tTupleDesc,
			 TriggerData *tpTrigData)
{

	Oid			saPlanArgTypes[1] = {NAMEOID};
	char	   *insQuery = "INSERT INTO \"PendingData\" (\"SeqId\",\"IsKey\",\"Data\") VALUES(currval('\"Pending_SeqId_seq\"'),'t',$1)";
	void	   *pplan;
	Datum		saPlanData[1];
	char	   *cpKeyData;
	int			iRetCode;

	pplan = SPI_prepare(insQuery, 1, saPlanArgTypes);
	if (pplan == NULL)
	{
		elog(NOTICE, "Could not prepare INSERT plan");
		return -1;
	}

	/* pplan = SPI_saveplan(pplan); */
	cpKeyData = packageData(tTupleData, tTupleDesc, tpTrigData, PRIMARY);
	if (cpKeyData == NULL)
	{
		elog(ERROR,"Could not determine primary key data");
		return -1;
	}
#if defined DEBUG_OUTPUT
	elog(NOTICE, cpKeyData);
#endif
	saPlanData[0] = PointerGetDatum(cpKeyData);

	iRetCode = SPI_execp(pplan, saPlanData, NULL, 1);

	if (cpKeyData != NULL)
		SPI_pfree(cpKeyData);

	if (iRetCode != SPI_OK_INSERT)
	{
		elog(NOTICE, "Error inserting row in pendingDelete");
		return -1;
	}
#if defined DEBUG_OUTPUT
	elog(NOTICE, "INSERT SUCCESFULL");
#endif

	return 0;

}




int2vector *
getPrimaryKey(Oid tblOid)
{
	char	   *queryBase;
	char	   *query;
	bool		isNull;
	int2vector *resultKey;
	int2vector *tpResultKey;
	HeapTuple	resTuple;
	Datum		resDatum;
	int			ret;

	queryBase = "SELECT indkey FROM pg_index WHERE indisprimary='t' AND indrelid=";
	query = SPI_palloc(strlen(queryBase) + MAX_OID_LEN + 1);
	sprintf(query, "%s%d", queryBase, tblOid);
	ret = SPI_exec(query, 1);
	if (ret != SPI_OK_SELECT || SPI_processed != 1)
	{
		elog(NOTICE, "Could not select primary index key");
		return NULL;
	}

	resTuple = SPI_tuptable->vals[0];
	resDatum = SPI_getbinval(resTuple, SPI_tuptable->tupdesc, 1, &isNull);

	tpResultKey = (int2vector *) DatumGetPointer(resDatum);
	resultKey = SPI_palloc(sizeof(int2vector));
	memcpy(resultKey, tpResultKey, sizeof(int2vector));

	SPI_pfree(query);
	return resultKey;
}

/******************************************************************************
 * Stores a copy of the non-key data for the row.
 *****************************************************************************/
int
storeData(char *cpTableName, HeapTuple tTupleData, TupleDesc tTupleDesc,
		  TriggerData *tpTrigData, int iIncludeKeyData)
{

	Oid			planArgTypes[1] = {NAMEOID};
	char	   *insQuery = "INSERT INTO \"PendingData\" (\"SeqId\",\"IsKey\",\"Data\") VALUES(currval('\"Pending_SeqId_seq\"'),'f',$1)";
	void	   *pplan;
	Datum		planData[1];
	char	   *cpKeyData;
	int			iRetValue;

	pplan = SPI_prepare(insQuery, 1, planArgTypes);
	if (pplan == NULL)
	{
		elog(NOTICE, "Could not prepare INSERT plan");
		return -1;
	}

	/* pplan = SPI_saveplan(pplan); */
	if (iIncludeKeyData == 0)
		cpKeyData = packageData(tTupleData, tTupleDesc, tpTrigData, NONPRIMARY);
	else
		cpKeyData = packageData(tTupleData, tTupleDesc, tpTrigData, ALL);

	planData[0] = PointerGetDatum(cpKeyData);
	iRetValue = SPI_execp(pplan, planData, NULL, 1);

	if (cpKeyData != 0)
		SPI_pfree(cpKeyData);

	if (iRetValue != SPI_OK_INSERT)
	{
		elog(NOTICE, "Error inserting row in pendingDelete");
		return -1;
	}
#if defined DEBUG_OUTPUT
	elog(NOTICE, "INSERT SUCCESFULL");
#endif

	return 0;

}

/**
 * Packages the data in tTupleData into a string of the format
 * FieldName='value text'  where any quotes inside of value text
 * are escaped with a backslash and any backslashes in value text
 * are esacped by a second back slash.
 *
 * tTupleDesc should be a description of the tuple stored in
 * tTupleData.
 *
 * eFieldUsage specifies which fields to use.
 *	PRIMARY implies include only primary key fields.
 *	NONPRIMARY implies include only non-primary key fields.
 *	ALL implies include all fields.
 */
char *
packageData(HeapTuple tTupleData, TupleDesc tTupleDesc,
			TriggerData *tpTrigData,
			enum FieldUsage eKeyUsage)
{
	int			iNumCols;
	int2vector *tpPKeys = NULL;
	int			iColumnCounter;
	char	   *cpDataBlock;
	int			iDataBlockSize;
	int			iUsedDataBlock;

	iNumCols = tTupleDesc->natts;

	if (eKeyUsage != ALL)
	{
		tpPKeys = getPrimaryKey(tpTrigData->tg_relation->rd_id);
		if (tpPKeys == NULL)
			return NULL;
	}
#if defined DEBUG_OUTPUT
	if (tpPKeys != NULL)
		elog(NOTICE, "Have primary keys");
#endif
	cpDataBlock = SPI_palloc(BUFFER_SIZE);
	iDataBlockSize = BUFFER_SIZE;
	iUsedDataBlock = 0;			/* To account for the null */

	for (iColumnCounter = 1; iColumnCounter <= iNumCols; iColumnCounter++)
	{
		int			iIsPrimaryKey;
		int			iPrimaryKeyIndex;
		char	   *cpUnFormatedPtr;
		char	   *cpFormatedPtr;

		char	   *cpFieldName;
		char	   *cpFieldData;

		if (eKeyUsage != ALL)
		{
			/* Determine if this is a primary key or not. */
			iIsPrimaryKey = 0;
			for (iPrimaryKeyIndex = 0; (*tpPKeys)[iPrimaryKeyIndex] != 0;
				 iPrimaryKeyIndex++)
			{
				if ((*tpPKeys)[iPrimaryKeyIndex] == iColumnCounter)
				{
					iIsPrimaryKey = 1;
					break;
				}
			}
			if (iIsPrimaryKey ? (eKeyUsage != PRIMARY) : (eKeyUsage != NONPRIMARY))
			{
				/**
				 * Don't use.
				 */
#if defined DEBUG_OUTPUT
				elog(NOTICE, "Skipping column");
#endif
				continue;
			}
		}						/* KeyUsage!=ALL */
		cpFieldName = DatumGetPointer(NameGetDatum(&tTupleDesc->attrs
										 [iColumnCounter - 1]->attname));
#if defined DEBUG_OUTPUT
		elog(NOTICE, cpFieldName);
#endif
		while (iDataBlockSize - iUsedDataBlock < strlen(cpFieldName) + 4)
		{
			cpDataBlock = SPI_repalloc(cpDataBlock, iDataBlockSize + BUFFER_SIZE);
			iDataBlockSize = iDataBlockSize + BUFFER_SIZE;
		}
		sprintf(cpDataBlock + iUsedDataBlock, "\"%s\"=", cpFieldName);
		iUsedDataBlock = iUsedDataBlock + strlen(cpFieldName) + 3;
		cpFieldData = SPI_getvalue(tTupleData, tTupleDesc, iColumnCounter);

		cpUnFormatedPtr = cpFieldData;
		cpFormatedPtr = cpDataBlock + iUsedDataBlock;
		if (cpFieldData != NULL)
		{
			*cpFormatedPtr = '\'';
			iUsedDataBlock++;
			cpFormatedPtr++;
		}
		else
		{
			*cpFormatedPtr = ' ';
			iUsedDataBlock++;
			cpFormatedPtr++;
			continue;

		}
#if defined DEBUG_OUTPUT
		elog(NOTICE, cpFieldData);
		elog(NOTICE, "Starting format loop");
#endif
		while (*cpUnFormatedPtr != 0)
		{
			while (iDataBlockSize - iUsedDataBlock < 2)
			{
				cpDataBlock = SPI_repalloc(cpDataBlock, iDataBlockSize + BUFFER_SIZE);
				iDataBlockSize = iDataBlockSize + BUFFER_SIZE;
				cpFormatedPtr = cpDataBlock + iUsedDataBlock;
			}
			if (*cpUnFormatedPtr == '\\' || *cpUnFormatedPtr == '\'')
			{
				*cpFormatedPtr = '\\';
				cpFormatedPtr++;
				iUsedDataBlock++;
			}
			*cpFormatedPtr = *cpUnFormatedPtr;
			cpFormatedPtr++;
			cpUnFormatedPtr++;
			iUsedDataBlock++;
		}

		SPI_pfree(cpFieldData);

		while (iDataBlockSize - iUsedDataBlock < 3)
		{
			cpDataBlock = SPI_repalloc(cpDataBlock, iDataBlockSize + BUFFER_SIZE);
			iDataBlockSize = iDataBlockSize + BUFFER_SIZE;
			cpFormatedPtr = cpDataBlock + iUsedDataBlock;
		}
		sprintf(cpFormatedPtr, "' ");
		iUsedDataBlock = iUsedDataBlock + 2;
#if defined DEBUG_OUTPUT
		elog(NOTICE, cpDataBlock);
#endif

	}							/* for iColumnCounter  */
	if (tpPKeys != NULL)
		SPI_pfree(tpPKeys);
#if defined DEBUG_OUTPUT
	elog(NOTICE, "Returning");
#endif
	memset(cpDataBlock + iUsedDataBlock, 0, iDataBlockSize - iUsedDataBlock);

	return cpDataBlock;

}
