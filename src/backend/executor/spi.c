/*-------------------------------------------------------------------------
 *
 * spi.c
 *				Server Programming Interface
 *
 * $Id: spi.c,v 1.46 2000-05-30 04:24:45 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "executor/spi_priv.h"
#include "access/printtup.h"

static Portal _SPI_portal = (Portal) NULL;
static _SPI_connection *_SPI_stack = NULL;
static _SPI_connection *_SPI_current = NULL;
static int	_SPI_connected = -1;
static int	_SPI_curid = -1;

DLLIMPORT uint32 SPI_processed = 0;
DLLIMPORT SPITupleTable *SPI_tuptable;
DLLIMPORT int SPI_result;

static int	_SPI_execute(char *src, int tcount, _SPI_plan *plan);
static int	_SPI_pquery(QueryDesc *queryDesc, EState *state, int tcount);

static int _SPI_execute_plan(_SPI_plan *plan,
				  Datum *Values, char *Nulls, int tcount);

static _SPI_plan *_SPI_copy_plan(_SPI_plan *plan, int location);

static int	_SPI_begin_call(bool execmem);
static int	_SPI_end_call(bool procmem);
static MemoryContext _SPI_execmem(void);
static MemoryContext _SPI_procmem(void);
static bool _SPI_checktuples(void);

#ifdef SPI_EXECUTOR_STATS
extern int	ShowExecutorStats;
extern void ResetUsage(void);
extern void ShowUsage(void);

#endif

/* =================== interface functions =================== */

int
SPI_connect()
{
	char		pname[64];
	PortalVariableMemory pvmem;

	/*
	 * It's possible on startup and after commit/abort. In future we'll
	 * catch commit/abort in some way...
	 */
	strcpy(pname, "<SPI manager>");
	_SPI_portal = GetPortalByName(pname);
	if (!PortalIsValid(_SPI_portal))
	{
		if (_SPI_stack != NULL) /* there was abort */
			free(_SPI_stack);
		_SPI_current = _SPI_stack = NULL;
		_SPI_connected = _SPI_curid = -1;
		SPI_processed = 0;
		SPI_tuptable = NULL;
		_SPI_portal = CreatePortal(pname);
		if (!PortalIsValid(_SPI_portal))
			elog(FATAL, "SPI_connect: global initialization failed");
	}

	/*
	 * When procedure called by Executor _SPI_curid expected to be equal
	 * to _SPI_connected
	 */
	if (_SPI_curid != _SPI_connected)
		return SPI_ERROR_CONNECT;

	if (_SPI_stack == NULL)
	{
		if (_SPI_connected != -1)
			elog(FATAL, "SPI_connect: no connection(s) expected");
		_SPI_stack = (_SPI_connection *) malloc(sizeof(_SPI_connection));
	}
	else
	{
		if (_SPI_connected <= -1)
			elog(FATAL, "SPI_connect: some connection(s) expected");
		_SPI_stack = (_SPI_connection *) realloc(_SPI_stack,
						 (_SPI_connected + 2) * sizeof(_SPI_connection));
	}

	/*
	 * We' returning to procedure where _SPI_curid == _SPI_connected - 1
	 */
	_SPI_connected++;

	_SPI_current = &(_SPI_stack[_SPI_connected]);
	_SPI_current->qtlist = NULL;
	_SPI_current->processed = 0;
	_SPI_current->tuptable = NULL;

	/* Create Portal for this procedure ... */
	snprintf(pname, 64, "<SPI %d>", _SPI_connected);
	_SPI_current->portal = CreatePortal(pname);
	if (!PortalIsValid(_SPI_current->portal))
		elog(FATAL, "SPI_connect: initialization failed");

	/* ... and switch to Portal' Variable memory - procedure' context */
	pvmem = PortalGetVariableMemory(_SPI_current->portal);
	_SPI_current->savedcxt = MemoryContextSwitchTo((MemoryContext) pvmem);

	_SPI_current->savedId = GetScanCommandId();
	SetScanCommandId(GetCurrentCommandId());

	return SPI_OK_CONNECT;

}

int
SPI_finish()
{
	int			res;

	res = _SPI_begin_call(false);		/* live in procedure memory */
	if (res < 0)
		return res;

	/* Restore memory context as it was before procedure call */
	MemoryContextSwitchTo(_SPI_current->savedcxt);
	PortalDrop(&(_SPI_current->portal));

	SetScanCommandId(_SPI_current->savedId);

	/*
	 * After _SPI_begin_call _SPI_connected == _SPI_curid. Now we are
	 * closing connection to SPI and returning to upper Executor and so
	 * _SPI_connected must be equal to _SPI_curid.
	 */
	_SPI_connected--;
	_SPI_curid--;
	if (_SPI_connected == -1)
	{
		free(_SPI_stack);
		_SPI_stack = NULL;
	}
	else
	{
		_SPI_stack = (_SPI_connection *) realloc(_SPI_stack,
						 (_SPI_connected + 1) * sizeof(_SPI_connection));
		_SPI_current = &(_SPI_stack[_SPI_connected]);
	}

	return SPI_OK_FINISH;

}

void
SPI_push(void)
{
	_SPI_curid++;
}

void
SPI_pop(void)
{
	_SPI_curid--;
}

int
SPI_exec(char *src, int tcount)
{
	int			res;

	if (src == NULL || tcount < 0)
		return SPI_ERROR_ARGUMENT;

	res = _SPI_begin_call(true);
	if (res < 0)
		return res;

	res = _SPI_execute(src, tcount, NULL);

	_SPI_end_call(true);
	return res;
}

int
SPI_execp(void *plan, Datum *Values, char *Nulls, int tcount)
{
	int			res;

	if (plan == NULL || tcount < 0)
		return SPI_ERROR_ARGUMENT;

	if (((_SPI_plan *) plan)->nargs > 0 && Values == NULL)
		return SPI_ERROR_PARAM;

	res = _SPI_begin_call(true);
	if (res < 0)
		return res;

	/* copy plan to current (executor) context */
	plan = (void *) _SPI_copy_plan(plan, _SPI_CPLAN_CURCXT);

	res = _SPI_execute_plan((_SPI_plan *) plan, Values, Nulls, tcount);

	_SPI_end_call(true);
	return res;
}

void *
SPI_prepare(char *src, int nargs, Oid *argtypes)
{
	_SPI_plan  *plan;

	if (src == NULL || nargs < 0 || (nargs > 0 && argtypes == NULL))
	{
		SPI_result = SPI_ERROR_ARGUMENT;
		return NULL;
	}

	SPI_result = _SPI_begin_call(true);
	if (SPI_result < 0)
		return NULL;

	plan = (_SPI_plan *) palloc(sizeof(_SPI_plan));		/* Executor context */
	plan->argtypes = argtypes;
	plan->nargs = nargs;

	SPI_result = _SPI_execute(src, 0, plan);

	if (SPI_result >= 0)		/* copy plan to procedure context */
		plan = _SPI_copy_plan(plan, _SPI_CPLAN_PROCXT);
	else
		plan = NULL;

	_SPI_end_call(true);

	return (void *) plan;

}

void *
SPI_saveplan(void *plan)
{
	_SPI_plan  *newplan;

	if (plan == NULL)
	{
		SPI_result = SPI_ERROR_ARGUMENT;
		return NULL;
	}

	SPI_result = _SPI_begin_call(false);		/* don't change context */
	if (SPI_result < 0)
		return NULL;

	newplan = _SPI_copy_plan((_SPI_plan *) plan, _SPI_CPLAN_TOPCXT);

	_SPI_curid--;
	SPI_result = 0;

	return (void *) newplan;

}

HeapTuple
SPI_copytuple(HeapTuple tuple)
{
	MemoryContext oldcxt = NULL;
	HeapTuple	ctuple;

	if (tuple == NULL)
	{
		SPI_result = SPI_ERROR_ARGUMENT;
		return NULL;
	}

	if (_SPI_curid + 1 == _SPI_connected)		/* connected */
	{
		if (_SPI_current != &(_SPI_stack[_SPI_curid + 1]))
			elog(FATAL, "SPI: stack corrupted");
		oldcxt = MemoryContextSwitchTo(_SPI_current->savedcxt);
	}

	ctuple = heap_copytuple(tuple);

	if (oldcxt)
		MemoryContextSwitchTo(oldcxt);

	return ctuple;
}

HeapTuple
SPI_modifytuple(Relation rel, HeapTuple tuple, int natts, int *attnum,
				Datum *Values, char *Nulls)
{
	MemoryContext oldcxt = NULL;
	HeapTuple	mtuple;
	int			numberOfAttributes;
	uint8		infomask;
	Datum	   *v;
	char	   *n;
	bool		isnull;
	int			i;

	if (rel == NULL || tuple == NULL || natts <= 0 || attnum == NULL || Values == NULL)
	{
		SPI_result = SPI_ERROR_ARGUMENT;
		return NULL;
	}

	if (_SPI_curid + 1 == _SPI_connected)		/* connected */
	{
		if (_SPI_current != &(_SPI_stack[_SPI_curid + 1]))
			elog(FATAL, "SPI: stack corrupted");
		oldcxt = MemoryContextSwitchTo(_SPI_current->savedcxt);
	}
	SPI_result = 0;
	numberOfAttributes = rel->rd_att->natts;
	v = (Datum *) palloc(numberOfAttributes * sizeof(Datum));
	n = (char *) palloc(numberOfAttributes * sizeof(char));

	/* fetch old values and nulls */
	for (i = 0; i < numberOfAttributes; i++)
	{
		v[i] = heap_getattr(tuple, i + 1, rel->rd_att, &isnull);
		n[i] = (isnull) ? 'n' : ' ';
	}

	/* replace values and nulls */
	for (i = 0; i < natts; i++)
	{
		if (attnum[i] <= 0 || attnum[i] > numberOfAttributes)
			break;
		v[attnum[i] - 1] = Values[i];
		n[attnum[i] - 1] = (Nulls && Nulls[i] == 'n') ? 'n' : ' ';
	}

	if (i == natts)				/* no errors in *attnum */
	{
		mtuple = heap_formtuple(rel->rd_att, v, n);
		infomask = mtuple->t_data->t_infomask;
		memmove(&(mtuple->t_data->t_oid), &(tuple->t_data->t_oid),
				((char *) &(tuple->t_data->t_hoff) -
				 (char *) &(tuple->t_data->t_oid)));
		mtuple->t_data->t_infomask = infomask;
		mtuple->t_data->t_natts = numberOfAttributes;
	}
	else
	{
		mtuple = NULL;
		SPI_result = SPI_ERROR_NOATTRIBUTE;
	}

	pfree(v);
	pfree(n);

	if (oldcxt)
		MemoryContextSwitchTo(oldcxt);

	return mtuple;
}

int
SPI_fnumber(TupleDesc tupdesc, char *fname)
{
	int			res;

	for (res = 0; res < tupdesc->natts; res++)
	{
		if (strcasecmp(NameStr(tupdesc->attrs[res]->attname), fname) == 0)
			return res + 1;
	}

	return SPI_ERROR_NOATTRIBUTE;
}

char *
SPI_fname(TupleDesc tupdesc, int fnumber)
{

	SPI_result = 0;
	if (tupdesc->natts < fnumber || fnumber <= 0)
	{
		SPI_result = SPI_ERROR_NOATTRIBUTE;
		return NULL;
	}

	return pstrdup(NameStr(tupdesc->attrs[fnumber - 1]->attname));
}

char *
SPI_getvalue(HeapTuple tuple, TupleDesc tupdesc, int fnumber)
{
	Datum		val;
	bool		isnull;
	Oid			foutoid,
				typelem;

	SPI_result = 0;
	if (tuple->t_data->t_natts < fnumber || fnumber <= 0)
	{
		SPI_result = SPI_ERROR_NOATTRIBUTE;
		return NULL;
	}

	val = heap_getattr(tuple, fnumber, tupdesc, &isnull);
	if (isnull)
		return NULL;
	if (!getTypeOutAndElem((Oid) tupdesc->attrs[fnumber - 1]->atttypid,
						   &foutoid, &typelem))
	{
		SPI_result = SPI_ERROR_NOOUTFUNC;
		return NULL;
	}

	return DatumGetCString(OidFunctionCall3(foutoid,
						   val,
						   ObjectIdGetDatum(typelem),
						   Int32GetDatum(tupdesc->attrs[fnumber - 1]->atttypmod)));
}

Datum
SPI_getbinval(HeapTuple tuple, TupleDesc tupdesc, int fnumber, bool *isnull)
{
	Datum		val;

	*isnull = true;
	SPI_result = 0;
	if (tuple->t_data->t_natts < fnumber || fnumber <= 0)
	{
		SPI_result = SPI_ERROR_NOATTRIBUTE;
		return (Datum) NULL;
	}

	val = heap_getattr(tuple, fnumber, tupdesc, isnull);

	return val;
}

char *
SPI_gettype(TupleDesc tupdesc, int fnumber)
{
	HeapTuple	typeTuple;

	SPI_result = 0;
	if (tupdesc->natts < fnumber || fnumber <= 0)
	{
		SPI_result = SPI_ERROR_NOATTRIBUTE;
		return NULL;
	}

	typeTuple = SearchSysCacheTuple(TYPEOID,
				 ObjectIdGetDatum(tupdesc->attrs[fnumber - 1]->atttypid),
									0, 0, 0);

	if (!HeapTupleIsValid(typeTuple))
	{
		SPI_result = SPI_ERROR_TYPUNKNOWN;
		return NULL;
	}

	return pstrdup(NameStr(((Form_pg_type) GETSTRUCT(typeTuple))->typname));
}

Oid
SPI_gettypeid(TupleDesc tupdesc, int fnumber)
{

	SPI_result = 0;
	if (tupdesc->natts < fnumber || fnumber <= 0)
	{
		SPI_result = SPI_ERROR_NOATTRIBUTE;
		return InvalidOid;
	}

	return tupdesc->attrs[fnumber - 1]->atttypid;
}

char *
SPI_getrelname(Relation rel)
{
	return pstrdup(RelationGetRelationName(rel));
}

void *
SPI_palloc(Size size)
{
	MemoryContext oldcxt = NULL;
	void	   *pointer;

	if (_SPI_curid + 1 == _SPI_connected)		/* connected */
	{
		if (_SPI_current != &(_SPI_stack[_SPI_curid + 1]))
			elog(FATAL, "SPI: stack corrupted");
		oldcxt = MemoryContextSwitchTo(_SPI_current->savedcxt);
	}

	pointer = palloc(size);

	if (oldcxt)
		MemoryContextSwitchTo(oldcxt);

	return pointer;
}

void *
SPI_repalloc(void *pointer, Size size)
{
	MemoryContext oldcxt = NULL;

	if (_SPI_curid + 1 == _SPI_connected)		/* connected */
	{
		if (_SPI_current != &(_SPI_stack[_SPI_curid + 1]))
			elog(FATAL, "SPI: stack corrupted");
		oldcxt = MemoryContextSwitchTo(_SPI_current->savedcxt);
	}

	pointer = repalloc(pointer, size);

	if (oldcxt)
		MemoryContextSwitchTo(oldcxt);

	return pointer;
}

void
SPI_pfree(void *pointer)
{
	MemoryContext oldcxt = NULL;

	if (_SPI_curid + 1 == _SPI_connected)		/* connected */
	{
		if (_SPI_current != &(_SPI_stack[_SPI_curid + 1]))
			elog(FATAL, "SPI: stack corrupted");
		oldcxt = MemoryContextSwitchTo(_SPI_current->savedcxt);
	}

	pfree(pointer);

	if (oldcxt)
		MemoryContextSwitchTo(oldcxt);

	return;
}

void
SPI_freetuple(HeapTuple tuple)
{
	MemoryContext oldcxt = NULL;

	if (_SPI_curid + 1 == _SPI_connected)		/* connected */
	{
		if (_SPI_current != &(_SPI_stack[_SPI_curid + 1]))
			elog(FATAL, "SPI: stack corrupted");
		oldcxt = MemoryContextSwitchTo(_SPI_current->savedcxt);
	}

	heap_freetuple(tuple);

	if (oldcxt)
		MemoryContextSwitchTo(oldcxt);

	return;
}

/* =================== private functions =================== */

/*
 * spi_printtup
 *		store tuple retrieved by Executor into SPITupleTable
 *		of current SPI procedure
 *
 */
void
spi_printtup(HeapTuple tuple, TupleDesc tupdesc, DestReceiver *self)
{
	SPITupleTable *tuptable;
	MemoryContext oldcxt;

	/*
	 * When called by Executor _SPI_curid expected to be equal to
	 * _SPI_connected
	 */
	if (_SPI_curid != _SPI_connected || _SPI_connected < 0)
		elog(FATAL, "SPI: improper call to spi_printtup");
	if (_SPI_current != &(_SPI_stack[_SPI_curid]))
		elog(FATAL, "SPI: stack corrupted in spi_printtup");

	oldcxt = _SPI_procmem();	/* switch to procedure memory context */

	tuptable = _SPI_current->tuptable;
	if (tuptable == NULL)
	{
		_SPI_current->tuptable = tuptable = (SPITupleTable *)
			palloc(sizeof(SPITupleTable));
		tuptable->alloced = tuptable->free = 128;
		tuptable->vals = (HeapTuple *) palloc(tuptable->alloced * sizeof(HeapTuple));
		tuptable->tupdesc = CreateTupleDescCopy(tupdesc);
	}
	else if (tuptable->free == 0)
	{
		tuptable->free = 256;
		tuptable->alloced += tuptable->free;
		tuptable->vals = (HeapTuple *) repalloc(tuptable->vals,
								  tuptable->alloced * sizeof(HeapTuple));
	}

	tuptable->vals[tuptable->alloced - tuptable->free] = heap_copytuple(tuple);
	(tuptable->free)--;

	MemoryContextSwitchTo(oldcxt);
	return;
}

/*
 * Static functions
 */

static int
_SPI_execute(char *src, int tcount, _SPI_plan *plan)
{
	List	   *queryTree_list;
	List	   *planTree_list;
	List	   *queryTree_list_item;
	QueryDesc  *qdesc;
	Query	   *queryTree;
	Plan	   *planTree;
	EState	   *state;
	int			nargs = 0;
	Oid		   *argtypes = NULL;
	int			res = 0;
	bool		islastquery;

	/* Increment CommandCounter to see changes made by now */
	CommandCounterIncrement();

	SPI_processed = 0;
	SPI_tuptable = NULL;
	_SPI_current->tuptable = NULL;
	_SPI_current->qtlist = NULL;

	if (plan)
	{
		nargs = plan->nargs;
		argtypes = plan->argtypes;
	}

	queryTree_list = pg_parse_and_rewrite(src, argtypes, nargs, FALSE);

	_SPI_current->qtlist = queryTree_list;

	planTree_list = NIL;

	foreach(queryTree_list_item, queryTree_list)
	{
		queryTree = (Query *) lfirst(queryTree_list_item);
		islastquery = (lnext(queryTree_list_item) == NIL);

		planTree = pg_plan_query(queryTree);
		planTree_list = lappend(planTree_list, planTree);

		if (queryTree->commandType == CMD_UTILITY)
		{
			if (nodeTag(queryTree->utilityStmt) == T_CopyStmt)
			{
				CopyStmt   *stmt = (CopyStmt *) (queryTree->utilityStmt);

				if (stmt->filename == NULL)
					return SPI_ERROR_COPY;
			}
			else if (nodeTag(queryTree->utilityStmt) == T_ClosePortalStmt ||
					 nodeTag(queryTree->utilityStmt) == T_FetchStmt)
				return SPI_ERROR_CURSOR;
			else if (nodeTag(queryTree->utilityStmt) == T_TransactionStmt)
				return SPI_ERROR_TRANSACTION;
			res = SPI_OK_UTILITY;
			if (plan == NULL)
			{
				ProcessUtility(queryTree->utilityStmt, None);
				if (!islastquery)
					CommandCounterIncrement();
				else
					return res;
			}
			else if (islastquery)
				break;
		}
		else if (plan == NULL)
		{
			qdesc = CreateQueryDesc(queryTree, planTree,
									islastquery ? SPI : None);
			state = CreateExecutorState();
			res = _SPI_pquery(qdesc, state, islastquery ? tcount : 0);
			if (res < 0 || islastquery)
				return res;
			CommandCounterIncrement();
		}
		else
		{
			qdesc = CreateQueryDesc(queryTree, planTree,
									islastquery ? SPI : None);
			res = _SPI_pquery(qdesc, NULL, islastquery ? tcount : 0);
			if (res < 0)
				return res;
			if (islastquery)
				break;
		}
	}

	plan->qtlist = queryTree_list;
	plan->ptlist = planTree_list;

	return res;
}

static int
_SPI_execute_plan(_SPI_plan *plan, Datum *Values, char *Nulls, int tcount)
{
	List	   *queryTree_list = plan->qtlist;
	List	   *planTree_list = plan->ptlist;
	List	   *queryTree_list_item;
	QueryDesc  *qdesc;
	Query	   *queryTree;
	Plan	   *planTree;
	EState	   *state;
	int			nargs = plan->nargs;
	int			res = 0;
	bool		islastquery;
	int			k;

	/* Increment CommandCounter to see changes made by now */
	CommandCounterIncrement();

	SPI_processed = 0;
	SPI_tuptable = NULL;
	_SPI_current->tuptable = NULL;
	_SPI_current->qtlist = NULL;

	foreach(queryTree_list_item, queryTree_list)
	{
		queryTree = (Query *) lfirst(queryTree_list_item);
		planTree = lfirst(planTree_list);
		planTree_list = lnext(planTree_list);
		islastquery = (planTree_list == NIL);	/* assume lists are same
												 * len */

		if (queryTree->commandType == CMD_UTILITY)
		{
			ProcessUtility(queryTree->utilityStmt, None);
			if (!islastquery)
				CommandCounterIncrement();
			else
				return SPI_OK_UTILITY;
		}
		else
		{
			qdesc = CreateQueryDesc(queryTree, planTree,
									islastquery ? SPI : None);
			state = CreateExecutorState();
			if (nargs > 0)
			{
				ParamListInfo paramLI = (ParamListInfo) palloc((nargs + 1) *
											  sizeof(ParamListInfoData));

				state->es_param_list_info = paramLI;
				for (k = 0; k < plan->nargs; paramLI++, k++)
				{
					paramLI->kind = PARAM_NUM;
					paramLI->id = k + 1;
					paramLI->isnull = (Nulls && Nulls[k] == 'n');
					paramLI->value = Values[k];
				}
				paramLI->kind = PARAM_INVALID;
			}
			else
				state->es_param_list_info = NULL;
			res = _SPI_pquery(qdesc, state, islastquery ? tcount : 0);
			if (res < 0 || islastquery)
				return res;
			CommandCounterIncrement();
		}
	}

	return res;
}

static int
_SPI_pquery(QueryDesc *queryDesc, EState *state, int tcount)
{
	Query	   *parseTree = queryDesc->parsetree;
	Plan	   *plan = queryDesc->plantree;
	int			operation = queryDesc->operation;
	CommandDest dest = queryDesc->dest;
	TupleDesc	tupdesc;
	bool		isRetrieveIntoPortal = false;
	bool		isRetrieveIntoRelation = false;
	char	   *intoName = NULL;
	int			res;
	Const		tcount_const;
	Node	   *count = NULL;

	switch (operation)
	{
		case CMD_SELECT:
			res = SPI_OK_SELECT;
			if (parseTree->isPortal)
			{
				isRetrieveIntoPortal = true;
				intoName = parseTree->into;
				parseTree->isBinary = false;	/* */

				return SPI_ERROR_CURSOR;

			}
			else if (parseTree->into != NULL)	/* select into table */
			{
				res = SPI_OK_SELINTO;
				isRetrieveIntoRelation = true;
				queryDesc->dest = None; /* */
			}
			break;
		case CMD_INSERT:
			res = SPI_OK_INSERT;
			break;
		case CMD_DELETE:
			res = SPI_OK_DELETE;
			break;
		case CMD_UPDATE:
			res = SPI_OK_UPDATE;
			break;
		default:
			return SPI_ERROR_OPUNKNOWN;
	}

	/* ----------------
	 * Get the query LIMIT tuple count
	 * ----------------
	 */
	if (parseTree->limitCount != NULL)
	{
		/* ----------------
		 * A limit clause in the parsetree overrides the
		 * tcount parameter
		 * ----------------
		 */
		count = parseTree->limitCount;
	}
	else
	{
		/* ----------------
		 * No LIMIT clause in parsetree. Use a local Const node
		 * to put tcount into it
		 * ----------------
		 */
		memset(&tcount_const, 0, sizeof(tcount_const));
		tcount_const.type = T_Const;
		tcount_const.consttype = INT4OID;
		tcount_const.constlen = sizeof(int4);
		tcount_const.constvalue = (Datum) tcount;
		tcount_const.constisnull = FALSE;
		tcount_const.constbyval = TRUE;
		tcount_const.constisset = FALSE;
		tcount_const.constiscast = FALSE;

		count = (Node *) &tcount_const;
	}

	if (state == NULL)			/* plan preparation */
		return res;
#ifdef SPI_EXECUTOR_STATS
	if (ShowExecutorStats)
		ResetUsage();
#endif
	tupdesc = ExecutorStart(queryDesc, state);

	/* Don't work currently */
	if (isRetrieveIntoPortal)
	{
		ProcessPortal(intoName,
					  parseTree,
					  plan,
					  state,
					  tupdesc,
					  None);
		return SPI_OK_CURSOR;
	}

	ExecutorRun(queryDesc, state, EXEC_FOR, parseTree->limitOffset, count);

	_SPI_current->processed = state->es_processed;
	if (operation == CMD_SELECT && queryDesc->dest == SPI)
	{
		if (_SPI_checktuples())
			elog(FATAL, "SPI_select: # of processed tuples check failed");
	}

	ExecutorEnd(queryDesc, state);

#ifdef SPI_EXECUTOR_STATS
	if (ShowExecutorStats)
	{
		fprintf(stderr, "! Executor Stats:\n");
		ShowUsage();
	}
#endif

	if (dest == SPI)
	{
		SPI_processed = _SPI_current->processed;
		SPI_tuptable = _SPI_current->tuptable;
	}
	queryDesc->dest = dest;

	return res;

}

static MemoryContext
_SPI_execmem()
{
	MemoryContext oldcxt;
	PortalHeapMemory phmem;

	phmem = PortalGetHeapMemory(_SPI_current->portal);
	oldcxt = MemoryContextSwitchTo((MemoryContext) phmem);

	return oldcxt;

}

static MemoryContext
_SPI_procmem()
{
	MemoryContext oldcxt;
	PortalVariableMemory pvmem;

	pvmem = PortalGetVariableMemory(_SPI_current->portal);
	oldcxt = MemoryContextSwitchTo((MemoryContext) pvmem);

	return oldcxt;

}

/*
 * _SPI_begin_call
 *
 */
static int
_SPI_begin_call(bool execmem)
{
	if (_SPI_curid + 1 != _SPI_connected)
		return SPI_ERROR_UNCONNECTED;
	_SPI_curid++;
	if (_SPI_current != &(_SPI_stack[_SPI_curid]))
		elog(FATAL, "SPI: stack corrupted");

	if (execmem)				/* switch to the Executor memory context */
	{
		_SPI_execmem();
		StartPortalAllocMode(DefaultAllocMode, 0);
	}

	return 0;
}

static int
_SPI_end_call(bool procmem)
{

	/*
	 * We' returning to procedure where _SPI_curid == _SPI_connected - 1
	 */
	_SPI_curid--;

	_SPI_current->qtlist = NULL;

	if (procmem)				/* switch to the procedure memory context */
	{							/* but free Executor memory before */
		EndPortalAllocMode();
		_SPI_procmem();
	}

	return 0;
}

static bool
_SPI_checktuples()
{
	uint32		processed = _SPI_current->processed;
	SPITupleTable *tuptable = _SPI_current->tuptable;
	bool		failed = false;

	if (processed == 0)
	{
		if (tuptable != NULL)
			failed = true;
	}
	else
/* some tuples were processed */
	{
		if (tuptable == NULL)	/* spi_printtup was not called */
			failed = true;
		else if (processed != (tuptable->alloced - tuptable->free))
			failed = true;
	}

	return failed;
}

static _SPI_plan *
_SPI_copy_plan(_SPI_plan *plan, int location)
{
	_SPI_plan  *newplan;
	MemoryContext oldcxt = NULL;

	if (location == _SPI_CPLAN_PROCXT)
		oldcxt = MemoryContextSwitchTo((MemoryContext)
						  PortalGetVariableMemory(_SPI_current->portal));
	else if (location == _SPI_CPLAN_TOPCXT)
		oldcxt = MemoryContextSwitchTo(TopMemoryContext);

	newplan = (_SPI_plan *) palloc(sizeof(_SPI_plan));
	newplan->qtlist = (List *) copyObject(plan->qtlist);
	newplan->ptlist = (List *) copyObject(plan->ptlist);
	newplan->nargs = plan->nargs;
	if (plan->nargs > 0)
	{
		newplan->argtypes = (Oid *) palloc(plan->nargs * sizeof(Oid));
		memcpy(newplan->argtypes, plan->argtypes, plan->nargs * sizeof(Oid));
	}
	else
		newplan->argtypes = NULL;

	if (location != _SPI_CPLAN_CURCXT)
		MemoryContextSwitchTo(oldcxt);

	return newplan;
}
