/*
 * Routines for handling of 'SET var TO', 'SHOW var' and 'RESET var'
 * statements.
 *
 * $Id: variable.c,v 1.6 1997-04-24 15:41:37 vadim Exp $
 *
 */

#include <stdio.h>
#include <string.h>
#include "postgres.h"
#include "miscadmin.h"
#include "tcop/variable.h"
#include "utils/builtins.h"
#include "optimizer/internal.h"

extern Cost _cpu_page_wight_;
extern Cost _cpu_index_page_wight_;

/*-----------------------------------------------------------------------*/
#if USE_EURODATES
#define DATE_EURO	TRUE
#else
#define DATE_EURO FALSE
#endif

/*-----------------------------------------------------------------------*/
struct PGVariables PGVariables =
	{
		{ DATE_EURO, Date_Postgres }
	};

/*-----------------------------------------------------------------------*/
static const char *get_token(char *buf, int size, const char *str)
	{
	if(!*str)
		return NULL;
		
	/* skip white space */
	while(*str && (*str == ' ' || *str == '\t'))
		str++;
	
	/* copy until we hit white space or comma or end of string */
	while(*str && *str != ' ' && *str != '\t' && *str != ',' && size-- > 1)
		*buf++ = *str++;
	
	*buf = '\0';
	
	/* skip white space and comma*/
	while(*str && (*str == ' ' || *str == '\t' || *str == ','))
		str++;
	
	return str;
	}
	
/*-----------------------------------------------------------------------*/
static bool parse_null(const char *value)
	{
	return TRUE;
	}
	
static bool show_null(const char *value)
	{
	return TRUE;
	}
	
static bool reset_null(const char *value)
	{
	return TRUE;
	}
	
static bool parse_cost_heap (const char *value)
{
    float32 res = float4in ((char*)value);
    
    _cpu_page_wight_ = *res;
    
    return TRUE;
}

static bool show_cost_heap ()
{

    elog (NOTICE, "COST_HEAP is %f", _cpu_page_wight_);
    return TRUE;
}

static bool reset_cost_heap ()
{
    _cpu_page_wight_ = _CPU_PAGE_WEIGHT_;
    return TRUE;
}

static bool parse_cost_index (const char *value)
{
    float32 res = float4in ((char*)value);
    
    _cpu_index_page_wight_ = *res;
    
    return TRUE;
}

static bool show_cost_index ()
{

    elog (NOTICE, "COST_INDEX is %f", _cpu_index_page_wight_);
    return TRUE;
}

static bool reset_cost_index ()
{
    _cpu_index_page_wight_ = _CPU_INDEX_PAGE_WEIGHT_;
    return TRUE;
}

static bool parse_date(const char *value)
	{
	char tok[32];
	int dcnt = 0, ecnt = 0;
	
	while((value = get_token(tok, sizeof(tok), value)) != 0)
		{
		/* Ugh. Somebody ought to write a table driven version -- mjl */
		
		if(!strcasecmp(tok, "iso"))
			{
			DateStyle = USE_ISO_DATES;
			dcnt++;
			}
		else if(!strcasecmp(tok, "sql"))
			{
			DateStyle = USE_SQL_DATES;
			dcnt++;
			}
		else if(!strcasecmp(tok, "postgres"))
			{
			DateStyle = USE_POSTGRES_DATES;
			dcnt++;
			}
		else if(!strncasecmp(tok, "euro", 4))
			{
			EuroDates = TRUE;
			ecnt++;
			}
		else if((!strcasecmp(tok, "us"))
		     || (!strncasecmp(tok, "noneuro", 7)))
			{
			EuroDates = FALSE;
			ecnt++;
			}
		else if(!strcasecmp(tok, "default"))
			{
			DateStyle = USE_POSTGRES_DATES;
			EuroDates = FALSE;
			ecnt++;
			}
		else
			{
			elog(WARN, "Bad value for date (%s)", tok);
			}
		}
	
	if(dcnt > 1 || ecnt > 1)
		elog(NOTICE, "Conflicting settings for date");
		
	return TRUE;
	}
	
static bool show_date()
	{
	char buf[64];

	sprintf( buf, "Date style is %s with%s European conventions",
	  ((DateStyle == USE_ISO_DATES)? "iso": ((DateStyle == USE_ISO_DATES)? "sql": "postgres")),
	  ((EuroDates)? "": "out"));

	elog(NOTICE, buf, NULL);

	return TRUE;
	}
	
static bool reset_date()
	{
	DateStyle = USE_POSTGRES_DATES;
	EuroDates = FALSE;

	return TRUE;
	}
	
/*-----------------------------------------------------------------------*/
struct VariableParsers
	{
	const char *name;
	bool (*parser)(const char *);
	bool (*show)();
	bool (*reset)();
	} VariableParsers[] =
	{
		{ "datestyle",	parse_date,	show_date,	reset_date },
		{ "timezone", 	parse_null,	show_null,	reset_null },
		{ "cost_heap", 	parse_cost_heap,
				show_cost_heap,	reset_cost_heap },
		{ "cost_index",	parse_cost_index,
				show_cost_index,	reset_cost_index },
		{ NULL, NULL, NULL }
	};

/*-----------------------------------------------------------------------*/
bool SetPGVariable(const char *name, const char *value)
	{
	struct VariableParsers *vp;
	
	for(vp = VariableParsers; vp->name; vp++)
		{
		if(!strcasecmp(vp->name, name))
			return (vp->parser)(value);
		}
		
	elog(NOTICE, "Unrecognized variable %s", name);

	return TRUE;
	}

/*-----------------------------------------------------------------------*/
bool GetPGVariable(const char *name)
	{
	struct VariableParsers *vp;
	
	for(vp = VariableParsers; vp->name; vp++)
		{
		if(!strcasecmp(vp->name, name))
			return (vp->show)();
		}
		
	elog(NOTICE, "Unrecognized variable %s", name);

	return TRUE;
	}

/*-----------------------------------------------------------------------*/
bool ResetPGVariable(const char *name)
	{
	struct VariableParsers *vp;
	
	for(vp = VariableParsers; vp->name; vp++)
		{
		if(!strcasecmp(vp->name, name))
			return (vp->reset)();
		}
		
	elog(NOTICE, "Unrecognized variable %s", name);

	return TRUE;
	}
