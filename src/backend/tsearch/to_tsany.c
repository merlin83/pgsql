/*-------------------------------------------------------------------------
 *
 * to_tsany.c
 *		to_ts* function definitions
 *
 * Portions Copyright (c) 1996-2007, PostgreSQL Global Development Group
 *
 *
 * IDENTIFICATION
 *	  $PostgreSQL: pgsql/src/backend/tsearch/to_tsany.c,v 1.3 2007/09/10 12:36:40 teodor Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "catalog/namespace.h"
#include "tsearch/ts_cache.h"
#include "tsearch/ts_utils.h"
#include "utils/builtins.h"
#include "utils/syscache.h"


Datum
get_current_ts_config(PG_FUNCTION_ARGS)
{
	PG_RETURN_OID(getTSCurrentConfig(true));
}

/*
 * to_tsvector
 */
static int
compareWORD(const void *a, const void *b)
{
	if (((ParsedWord *) a)->len == ((ParsedWord *) b)->len)
	{
		int			res = strncmp(
								  ((ParsedWord *) a)->word,
								  ((ParsedWord *) b)->word,
								  ((ParsedWord *) b)->len);

		if (res == 0)
			return (((ParsedWord *) a)->pos.pos > ((ParsedWord *) b)->pos.pos) ? 1 : -1;
		return res;
	}
	return (((ParsedWord *) a)->len > ((ParsedWord *) b)->len) ? 1 : -1;
}

static int
uniqueWORD(ParsedWord * a, int4 l)
{
	ParsedWord *ptr,
			   *res;
	int			tmppos;

	if (l == 1)
	{
		tmppos = LIMITPOS(a->pos.pos);
		a->alen = 2;
		a->pos.apos = (uint16 *) palloc(sizeof(uint16) * a->alen);
		a->pos.apos[0] = 1;
		a->pos.apos[1] = tmppos;
		return l;
	}

	res = a;
	ptr = a + 1;

	qsort((void *) a, l, sizeof(ParsedWord), compareWORD);
	tmppos = LIMITPOS(a->pos.pos);
	a->alen = 2;
	a->pos.apos = (uint16 *) palloc(sizeof(uint16) * a->alen);
	a->pos.apos[0] = 1;
	a->pos.apos[1] = tmppos;

	while (ptr - a < l)
	{
		if (!(ptr->len == res->len &&
			  strncmp(ptr->word, res->word, res->len) == 0))
		{
			res++;
			res->len = ptr->len;
			res->word = ptr->word;
			tmppos = LIMITPOS(ptr->pos.pos);
			res->alen = 2;
			res->pos.apos = (uint16 *) palloc(sizeof(uint16) * res->alen);
			res->pos.apos[0] = 1;
			res->pos.apos[1] = tmppos;
		}
		else
		{
			pfree(ptr->word);
			if (res->pos.apos[0] < MAXNUMPOS - 1 && res->pos.apos[res->pos.apos[0]] != MAXENTRYPOS - 1)
			{
				if (res->pos.apos[0] + 1 >= res->alen)
				{
					res->alen *= 2;
					res->pos.apos = (uint16 *) repalloc(res->pos.apos, sizeof(uint16) * res->alen);
				}
				if (res->pos.apos[0] == 0 || res->pos.apos[res->pos.apos[0]] != LIMITPOS(ptr->pos.pos))
				{
					res->pos.apos[res->pos.apos[0] + 1] = LIMITPOS(ptr->pos.pos);
					res->pos.apos[0]++;
				}
			}
		}
		ptr++;
	}

	return res + 1 - a;
}

/*
 * make value of tsvector, given parsed text
 */
TSVector
make_tsvector(ParsedText *prs)
{
	int4		i,
				j,
				lenstr = 0,
				totallen;
	TSVector	in;
	WordEntry  *ptr;
	char	   *str,
			   *cur;

	prs->curwords = uniqueWORD(prs->words, prs->curwords);
	for (i = 0; i < prs->curwords; i++)
	{
		lenstr += SHORTALIGN(prs->words[i].len);

		if (prs->words[i].alen)
			lenstr += sizeof(uint16) + prs->words[i].pos.apos[0] * sizeof(WordEntryPos);
	}

	totallen = CALCDATASIZE(prs->curwords, lenstr);
	in = (TSVector) palloc0(totallen);
	SET_VARSIZE(in, totallen);
	in->size = prs->curwords;

	ptr = ARRPTR(in);
	cur = str = STRPTR(in);
	for (i = 0; i < prs->curwords; i++)
	{
		ptr->len = prs->words[i].len;
		if (cur - str > MAXSTRPOS)
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("string is too long for tsvector")));
		ptr->pos = cur - str;
		memcpy((void *) cur, (void *) prs->words[i].word, prs->words[i].len);
		pfree(prs->words[i].word);
		cur += SHORTALIGN(prs->words[i].len);
		if (prs->words[i].alen)
		{
			WordEntryPos *wptr;

			ptr->haspos = 1;
			*(uint16 *) cur = prs->words[i].pos.apos[0];
			wptr = POSDATAPTR(in, ptr);
			for (j = 0; j < *(uint16 *) cur; j++)
			{
				WEP_SETWEIGHT(wptr[j], 0);
				WEP_SETPOS(wptr[j], prs->words[i].pos.apos[j + 1]);
			}
			cur += sizeof(uint16) + prs->words[i].pos.apos[0] * sizeof(WordEntryPos);
			pfree(prs->words[i].pos.apos);
		}
		else
			ptr->haspos = 0;
		ptr++;
	}
	pfree(prs->words);
	return in;
}

Datum
to_tsvector_byid(PG_FUNCTION_ARGS)
{
	Oid			cfgId = PG_GETARG_OID(0);
	text	   *in = PG_GETARG_TEXT_P(1);
	ParsedText	prs;
	TSVector	out;

	prs.lenwords = (VARSIZE(in) - VARHDRSZ) / 6;		/* just estimation of
														 * word's number */
	if (prs.lenwords == 0)
		prs.lenwords = 2;
	prs.curwords = 0;
	prs.pos = 0;
	prs.words = (ParsedWord *) palloc(sizeof(ParsedWord) * prs.lenwords);

	parsetext(cfgId, &prs, VARDATA(in), VARSIZE(in) - VARHDRSZ);
	PG_FREE_IF_COPY(in, 1);

	if (prs.curwords)
		out = make_tsvector(&prs);
	else
	{
		pfree(prs.words);
		out = palloc(CALCDATASIZE(0, 0));
		SET_VARSIZE(out, CALCDATASIZE(0, 0));
		out->size = 0;
	}

	PG_RETURN_POINTER(out);
}

Datum
to_tsvector(PG_FUNCTION_ARGS)
{
	text	   *in = PG_GETARG_TEXT_P(0);
	Oid			cfgId;

	cfgId = getTSCurrentConfig(true);
	PG_RETURN_DATUM(DirectFunctionCall2(to_tsvector_byid,
										ObjectIdGetDatum(cfgId),
										PointerGetDatum(in)));
}

/*
 * to_tsquery
 */


/*
 * This function is used for morph parsing.
 *
 * The value is passed to parsetext which will call the right dictionary to
 * lexize the word. If it turns out to be a stopword, we push a QI_VALSTOP
 * to the stack.
 *
 * All words belonging to the same variant are pushed as an ANDed list,
 * and different variants are ORred together. 
 */
static void
pushval_morph(Datum opaque, TSQueryParserState state, char *strval, int lenval, int2 weight)
{
	int4		count = 0;
	ParsedText	prs;
	uint32		variant,
				pos,
				cntvar = 0,
				cntpos = 0,
				cnt = 0;
	Oid cfg_id = DatumGetObjectId(opaque); /* the input is actually an Oid, not a pointer */

	prs.lenwords = 4;
	prs.curwords = 0;
	prs.pos = 0;
	prs.words = (ParsedWord *) palloc(sizeof(ParsedWord) * prs.lenwords);

	parsetext(cfg_id, &prs, strval, lenval);

	if (prs.curwords > 0)
	{

		while (count < prs.curwords)
		{
			pos = prs.words[count].pos.pos;
			cntvar = 0;
			while (count < prs.curwords && pos == prs.words[count].pos.pos)
			{
				variant = prs.words[count].nvariant;

				cnt = 0;
				while (count < prs.curwords && pos == prs.words[count].pos.pos && variant == prs.words[count].nvariant)
				{

					pushValue(state, prs.words[count].word, prs.words[count].len, weight);
					pfree(prs.words[count].word);
					if (cnt)
						pushOperator(state, OP_AND);
					cnt++;
					count++;
				}

				if (cntvar)
					pushOperator(state, OP_OR);
				cntvar++;
			}

			if (cntpos)
				pushOperator(state, OP_AND);

			cntpos++;
		}

		pfree(prs.words);

	}
	else
		pushStop(state);
}

Datum
to_tsquery_byid(PG_FUNCTION_ARGS)
{
	Oid			cfgid = PG_GETARG_OID(0);
	text	   *in = PG_GETARG_TEXT_P(1);
	TSQuery		query;
	QueryItem  *res;
	int4		len;

	query = parse_tsquery(TextPGetCString(in), pushval_morph, ObjectIdGetDatum(cfgid), false);

	if (query->size == 0)
		PG_RETURN_TSQUERY(query);

	res = clean_fakeval(GETQUERY(query), &len);
	if (!res)
	{
		SET_VARSIZE(query, HDRSIZETQ);
		query->size = 0;
		PG_RETURN_POINTER(query);
	}
	memcpy((void *) GETQUERY(query), (void *) res, len * sizeof(QueryItem));
	pfree(res);
	PG_RETURN_TSQUERY(query);
}

Datum
to_tsquery(PG_FUNCTION_ARGS)
{
	text	   *in = PG_GETARG_TEXT_P(0);
	Oid			cfgId;

	cfgId = getTSCurrentConfig(true);
	PG_RETURN_DATUM(DirectFunctionCall2(to_tsquery_byid,
										ObjectIdGetDatum(cfgId),
										PointerGetDatum(in)));
}

Datum
plainto_tsquery_byid(PG_FUNCTION_ARGS)
{
	Oid			cfgid = PG_GETARG_OID(0);
	text	   *in = PG_GETARG_TEXT_P(1);
	TSQuery		query;
	QueryItem  *res;
	int4		len;

	query = parse_tsquery(TextPGetCString(in), pushval_morph, ObjectIdGetDatum(cfgid), true);

	if (query->size == 0)
		PG_RETURN_TSQUERY(query);

	res = clean_fakeval(GETQUERY(query), &len);
	if (!res)
	{
		SET_VARSIZE(query, HDRSIZETQ);
		query->size = 0;
		PG_RETURN_POINTER(query);
	}
	memcpy((void *) GETQUERY(query), (void *) res, len * sizeof(QueryItem));
	pfree(res);
	PG_RETURN_POINTER(query);
}

Datum
plainto_tsquery(PG_FUNCTION_ARGS)
{
	text	   *in = PG_GETARG_TEXT_P(0);
	Oid			cfgId;

	cfgId = getTSCurrentConfig(true);
	PG_RETURN_DATUM(DirectFunctionCall2(plainto_tsquery_byid,
										ObjectIdGetDatum(cfgId),
										PointerGetDatum(in)));
}
