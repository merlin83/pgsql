
#include "executor/spi.h"		/* this is what you need to work with SPI */
#include "commands/trigger.h"	/* -"- and triggers */

HeapTuple		autoinc(void);

extern int4	nextval(struct varlena * seqin);

HeapTuple
autoinc()
{
	Trigger    *trigger;		/* to get trigger name */
	int			nargs;			/* # of arguments */
	int		   *chattrs;		/* attnums of attributes to change */
	int			chnattrs = 0;	/* # of above */
	Datum	   *newvals;		/* vals of above */
	char	  **args;			/* arguments */
	char	   *relname;		/* triggered relation name */
	Relation	rel;			/* triggered relation */
	HeapTuple	rettuple = NULL;
	TupleDesc	tupdesc;		/* tuple description */
	bool		isnull;
	int			i;

	if (!CurrentTriggerData)
		elog(ERROR, "autoinc: triggers are not initialized");
	if (TRIGGER_FIRED_FOR_STATEMENT(CurrentTriggerData->tg_event))
		elog(ERROR, "autoinc: can't process STATEMENT events");
	if (TRIGGER_FIRED_AFTER(CurrentTriggerData->tg_event))
		elog(ERROR, "autoinc: must be fired before event");
	
	if (TRIGGER_FIRED_BY_INSERT(CurrentTriggerData->tg_event))
		rettuple = CurrentTriggerData->tg_trigtuple;
	else if (TRIGGER_FIRED_BY_UPDATE(CurrentTriggerData->tg_event))
		rettuple = CurrentTriggerData->tg_newtuple;
	else
		elog(ERROR, "autoinc: can't process DELETE events");
	
	rel = CurrentTriggerData->tg_relation;
	relname = SPI_getrelname(rel);
	
	trigger = CurrentTriggerData->tg_trigger;

	nargs = trigger->tgnargs;
	if (nargs <= 0 || nargs % 2 != 0)
		elog(ERROR, "autoinc (%s): even number gt 0 of arguments was expected", relname);
	
	args = trigger->tgargs;
	tupdesc = rel->rd_att;
	
	CurrentTriggerData = NULL;
	
	chattrs = (int *) palloc (nargs/2 * sizeof (int));
	newvals = (Datum *) palloc (nargs/2 * sizeof (Datum));
	
	for (i = 0; i < nargs; )
	{
		struct varlena	   *seqname;
		int					attnum = SPI_fnumber (tupdesc, args[i]);
		int32				val;
		
		if ( attnum < 0 )
			elog(ERROR, "autoinc (%s): there is no attribute %s", relname, args[i]);
		if (SPI_gettypeid (tupdesc, attnum) != INT4OID)
			elog(ERROR, "autoinc (%s): attribute %s must be of INT4 type", 
					relname, args[i]);
		
		val = DatumGetInt32 (SPI_getbinval (rettuple, tupdesc, attnum, &isnull));
		
		if (!isnull && val != 0)
		{
			i += 2;
			continue;
		}
		
		i++;
		chattrs[chnattrs] = attnum;
		seqname = textin (args[i]);
		newvals[chnattrs] = Int32GetDatum (nextval (seqname));
		if ( DatumGetInt32 (newvals[chnattrs]) == 0 )
			newvals[chnattrs] = Int32GetDatum (nextval (seqname));
		pfree (seqname);
		chnattrs++;
		i++;
	}
	
	if (chnattrs > 0)
	{
		rettuple = SPI_modifytuple (rel, rettuple, chnattrs, chattrs, newvals, NULL);
		if ( rettuple == NULL )
			elog (ERROR, "autoinc (%s): %d returned by SPI_modifytuple",
				relname, SPI_result);
	}
	
	pfree (relname);
	pfree (chattrs);
	pfree (newvals);

	return (rettuple);
}
