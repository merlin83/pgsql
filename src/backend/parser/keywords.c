/*-------------------------------------------------------------------------
 *
 * keywords.c--
 *	  lexical token lookup for reserved words in postgres SQL
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/parser/keywords.c,v 1.22 1997-11-07 07:02:10 thomas Exp $
 *
 *-------------------------------------------------------------------------
 */
#include <ctype.h>
#include <string.h>
#include "postgres.h"

#include "nodes/pg_list.h"
#include "nodes/parsenodes.h"
#include "parse.h"
#include "utils/elog.h"
#include "parser/keywords.h"
#include "parser/dbcommands.h"	/* createdb, destroydb stop_vacuum */


/*
 * List of (keyword-name, keyword-token-value) pairs.
 *
 * !!WARNING!!: This list must be sorted, because binary
 *		 search is used to locate entries.
 */
static ScanKeyword ScanKeywords[] = {
	/* name					value			*/
	{"abort", ABORT_TRANS},
	{"acl", ACL},
	{"action", ACTION},
	{"add", ADD},
	{"after", AFTER},
	{"aggregate", AGGREGATE},
	{"all", ALL},
	{"alter", ALTER},
	{"analyze", ANALYZE},
	{"and", AND},
	{"append", APPEND},
	{"archIve", ARCHIVE},		/* XXX crooked: I < _ */
	{"arch_store", ARCH_STORE},
	{"archive", ARCHIVE},		/* XXX crooked: i > _ */
	{"as", AS},
	{"asc", ASC},
	{"backward", BACKWARD},
	{"before", BEFORE},
	{"begin", BEGIN_TRANS},
	{"between", BETWEEN},
	{"binary", BINARY},
	{"both", BOTH},
	{"by", BY},
	{"cascade", CASCADE},
	{"cast", CAST},
	{"change", CHANGE},
	{"char", CHAR},
	{"character", CHARACTER},
	{"check", CHECK},
	{"close", CLOSE},
	{"cluster", CLUSTER},
	{"collate", COLLATE},
	{"column", COLUMN},
	{"commit", COMMIT},
	{"constraint", CONSTRAINT},
	{"copy", COPY},
	{"create", CREATE},
	{"cross", CROSS},
	{"current", CURRENT},
	{"current_date", CURRENT_DATE},
	{"current_time", CURRENT_TIME},
	{"current_timestamp", CURRENT_TIMESTAMP},
	{"current_user", CURRENT_USER},
	{"cursor", CURSOR},
	{"database", DATABASE},
	{"day", DAY_P},
	{"decimal", DECIMAL},
	{"declare", DECLARE},
	{"default", DEFAULT},
	{"delete", DELETE},
	{"delimiters", DELIMITERS},
	{"desc", DESC},
	{"distinct", DISTINCT},
	{"do", DO},
	{"double", DOUBLE},
	{"drop", DROP},
	{"end", END_TRANS},
	{"execute", EXECUTE},
	{"exists", EXISTS},
	{"explain", EXPLAIN},
	{"extend", EXTEND},
	{"extract", EXTRACT},
	{"false", FALSE_P},
	{"fetch", FETCH},
	{"float", FLOAT},
	{"for", FOR},
	{"foreign", FOREIGN},
	{"forward", FORWARD},
	{"from", FROM},
	{"full", FULL},
	{"function", FUNCTION},
	{"grant", GRANT},
	{"group", GROUP},
	{"handler", HANDLER},
	{"having", HAVING},
	{"heavy", HEAVY},
	{"hour", HOUR_P},
	{"in", IN},
	{"index", INDEX},
	{"inherits", INHERITS},
	{"inner", INNER_P},
	{"insert", INSERT},
	{"instead", INSTEAD},
	{"interval", INTERVAL},
	{"into", INTO},
	{"is", IS},
	{"isnull", ISNULL},
	{"join", JOIN},
	{"key", KEY},
	{"lancompiler", LANCOMPILER},
	{"language", LANGUAGE},
	{"leading", LEADING},
	{"left", LEFT},
	{"light", LIGHT},
	{"like", LIKE},
	{"listen", LISTEN},
	{"load", LOAD},
	{"local", LOCAL},
	{"location", LOCATION},
	{"match", MATCH},
	{"merge", MERGE},
	{"minute", MINUTE_P},
	{"month", MONTH_P},
	{"move", MOVE},
	{"national", NATIONAL},
	{"natural", NATURAL},
	{"nchar", NCHAR},
	{"new", NEW},
	{"none", NONE},
	{"no", NO},
	{"not", NOT},
	{"nothing", NOTHING},
	{"notify", NOTIFY},
	{"notnull", NOTNULL},
	{"null", NULL_P},
	{"numeric", NUMERIC},
	{"oids", OIDS},
	{"on", ON},
	{"operator", OPERATOR},
	{"option", OPTION},
	{"or", OR},
	{"order", ORDER},
	{"outer", OUTER_P},
	{"partial", PARTIAL},
	{"position", POSITION},
	{"precision", PRECISION},
	{"primary", PRIMARY},
	{"privileges", PRIVILEGES},
	{"procedural", PROCEDURAL},
	{"procedure", PROCEDURE},
	{"public", PUBLIC},
	{"purge", PURGE},
	{"recipe", RECIPE},
	{"references", REFERENCES},
	{"rename", RENAME},
	{"replace", REPLACE},
	{"reset", RESET},
	{"retrieve", RETRIEVE},
	{"returns", RETURNS},
	{"revoke", REVOKE},
	{"right", RIGHT},
	{"rollback", ROLLBACK},
	{"rule", RULE},
	{"second", SECOND_P},
	{"select", SELECT},
	{"sequence", SEQUENCE},
	{"set", SET},
	{"setof", SETOF},
	{"show", SHOW},
	{"stdin", STDIN},
	{"stdout", STDOUT},
	{"store", STORE},
	{"substring", SUBSTRING},
	{"table", TABLE},
	{"time", TIME},
	{"to", TO},
	{"trailing", TRAILING},
	{"transaction", TRANSACTION},
	{"trigger", TRIGGER},
	{"trim", TRIM},
	{"true", TRUE_P},
	{"trusted", TRUSTED},
	{"type", TYPE_P},
	{"union", UNION},
	{"unique", UNIQUE},
	{"update", UPDATE},
	{"using", USING},
	{"vacuum", VACUUM},
	{"values", VALUES},
	{"varchar", VARCHAR},
	{"varying", VARYING},
	{"verbose", VERBOSE},
	{"version", VERSION},
	{"view", VIEW},
	{"where", WHERE},
	{"with", WITH},
	{"work", WORK},
	{"year", YEAR_P},
	{"zone", ZONE},
};

ScanKeyword *
ScanKeywordLookup(char *text)
{
	ScanKeyword *low = &ScanKeywords[0];
	ScanKeyword *high = endof(ScanKeywords) - 1;
	ScanKeyword *middle;
	int			difference;

	while (low <= high)
	{
		middle = low + (high - low) / 2;
		difference = strcmp(middle->name, text);
		if (difference == 0)
			return (middle);
		else if (difference < 0)
			low = middle + 1;
		else
			high = middle - 1;
	}

	return (NULL);
}

#ifdef NOT_USED
char	   *
AtomValueGetString(int atomval)
{
	ScanKeyword *low = &ScanKeywords[0];
	ScanKeyword *high = endof(ScanKeywords) - 1;
	int			keyword_list_length = (high - low);
	int			i;

	for (i = 0; i < keyword_list_length; i++)
		if (ScanKeywords[i].value == atomval)
			return (ScanKeywords[i].name);

	elog(WARN, "AtomGetString called with bogus atom # : %d", atomval);
	return (NULL);
}

#endif
