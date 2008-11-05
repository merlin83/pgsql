/*-------------------------------------------------------------------------
 *
 * trigfuncs.c
 *    Builtin functions for useful trigger support.
 *
 *
 * Portions Copyright (c) 1996-2008, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $PostgreSQL: pgsql/src/backend/utils/adt/trigfuncs.c,v 1.3 2008/11/05 18:49:27 adunstan Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/htup.h"
#include "commands/trigger.h"
#include "utils/builtins.h"


/*
 * suppress_redundant_updates_trigger
 *
 * This trigger function will inhibit an update from being done
 * if the OLD and NEW records are identical.
 */
Datum
suppress_redundant_updates_trigger(PG_FUNCTION_ARGS)
{
    TriggerData *trigdata = (TriggerData *) fcinfo->context;
    HeapTuple   newtuple, oldtuple, rettuple;
	HeapTupleHeader newheader, oldheader;

    /* make sure it's called as a trigger */
    if (!CALLED_AS_TRIGGER(fcinfo))
        ereport(ERROR,
				(errcode(ERRCODE_E_R_I_E_TRIGGER_PROTOCOL_VIOLATED),
				 errmsg("suppress_redundant_updates_trigger: must be called as trigger")));
	
    /* and that it's called on update */
    if (! TRIGGER_FIRED_BY_UPDATE(trigdata->tg_event))
        ereport(ERROR,
				(errcode(ERRCODE_E_R_I_E_TRIGGER_PROTOCOL_VIOLATED),
				 errmsg("suppress_redundant_updates_trigger: must be called on update")));

    /* and that it's called before update */
    if (! TRIGGER_FIRED_BEFORE(trigdata->tg_event))
        ereport(ERROR,
				(errcode(ERRCODE_E_R_I_E_TRIGGER_PROTOCOL_VIOLATED),
				 errmsg("suppress_redundant_updates_trigger: must be called before update")));

    /* and that it's called for each row */
    if (! TRIGGER_FIRED_FOR_ROW(trigdata->tg_event))
        ereport(ERROR,
				(errcode(ERRCODE_E_R_I_E_TRIGGER_PROTOCOL_VIOLATED),
				 errmsg("suppress_redundant_updates_trigger: must be called for each row")));

	/* get tuple data, set default result */
	rettuple  = newtuple = trigdata->tg_newtuple;
	oldtuple = trigdata->tg_trigtuple;

	newheader = newtuple->t_data;
	oldheader = oldtuple->t_data;

 	if (oldheader->t_infomask & HEAP_HASOID)
	{
		Oid oldoid = HeapTupleHeaderGetOid(oldheader);
		HeapTupleHeaderSetOid(newheader, oldoid);
	}

	/* if the tuple payload is the same ... */
    if (newtuple->t_len == oldtuple->t_len &&
		newheader->t_hoff == oldheader->t_hoff &&
		(HeapTupleHeaderGetNatts(newheader) == 
		 HeapTupleHeaderGetNatts(oldheader)) &&
		((newheader->t_infomask & ~HEAP_XACT_MASK) == 
		 (oldheader->t_infomask & ~HEAP_XACT_MASK)) &&
		memcmp(((char *)newheader) + offsetof(HeapTupleHeaderData, t_bits),
			   ((char *)oldheader) + offsetof(HeapTupleHeaderData, t_bits),
			   newtuple->t_len - offsetof(HeapTupleHeaderData, t_bits)) == 0)
	{
		/* ... then suppress the update */
		rettuple = NULL;
	}
	
	
    return PointerGetDatum(rettuple);
}
