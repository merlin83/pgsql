
/* Module:         convert.c
 *
 * Description:	   This module contains routines related to 
 *                 converting parameters and columns into requested data types.
 *                 Parameters are converted from their SQL_C data types into
 *                 the appropriate postgres type.  Columns are converted from
 *                 their postgres type (SQL type) into the appropriate SQL_C 
 *                 data type.
 *
 * Classes:        n/a
 *
 * API functions:  none
 *
 * Comments:       See "notice.txt" for copyright and license information.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "psqlodbc.h"

#ifndef WIN32
#include "iodbc.h"
#include "isql.h"
#include "isqlext.h"
#else
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#endif

#include <time.h>
#include <math.h>
#include "convert.h"
#include "statement.h"
#include "bind.h"
#include "pgtypes.h"
#include "lobj.h"
#include "connection.h"

#ifndef WIN32
#ifndef HAVE_STRICMP
#define stricmp(s1,s2) strcasecmp(s1,s2)
#define strnicmp(s1,s2,n) strncasecmp(s1,s2,n)
#endif
#ifndef SCHAR
typedef signed char SCHAR;
#endif
#endif

extern GLOBAL_VALUES globals;

/*	How to map ODBC scalar functions {fn func(args)} to Postgres */
/*	This is just a simple substitution */
char *mapFuncs[][2] = {   
	{ "CONCAT",      "textcat" },
	{ "LCASE",       "lower"   },
	{ "LOCATE",      "strpos"  },
	{ "LENGTH",      "textlen" },
	{ "LTRIM",       "ltrim"   },
	{ "RTRIM",       "rtrim"   },
	{ "SUBSTRING",   "substr"  },
	{ "UCASE",       "upper"   },
	{ "NOW",         "now"     },
	{    0,             0      }
};

char *mapFunction(char *func);
unsigned int conv_from_octal(unsigned char *s);
unsigned int conv_from_hex(unsigned char *s);
char *conv_to_octal(unsigned char val);

/********		A Guide for date/time/timestamp conversions    **************

			field_type		fCType				Output
			----------		------				----------
			PG_TYPE_DATE	SQL_C_DEFAULT		SQL_C_DATE
			PG_TYPE_DATE	SQL_C_DATE			SQL_C_DATE
			PG_TYPE_DATE	SQL_C_TIMESTAMP		SQL_C_TIMESTAMP		(time = 0 (midnight))
			PG_TYPE_TIME	SQL_C_DEFAULT		SQL_C_TIME
			PG_TYPE_TIME	SQL_C_TIME			SQL_C_TIME
			PG_TYPE_TIME	SQL_C_TIMESTAMP		SQL_C_TIMESTAMP		(date = current date)
			PG_TYPE_ABSTIME	SQL_C_DEFAULT		SQL_C_TIMESTAMP
			PG_TYPE_ABSTIME	SQL_C_DATE			SQL_C_DATE			(time is truncated)
			PG_TYPE_ABSTIME	SQL_C_TIME			SQL_C_TIME			(date is truncated)
			PG_TYPE_ABSTIME	SQL_C_TIMESTAMP		SQL_C_TIMESTAMP		
******************************************************************************/


/*	This is called by SQLFetch() */
int
copy_and_convert_field_bindinfo(StatementClass *stmt, Int4 field_type, void *value, int col)
{
BindInfoClass *bic = &(stmt->bindings[col]);

	return copy_and_convert_field(stmt, field_type, value, (Int2)bic->returntype, (PTR)bic->buffer,
                                (SDWORD)bic->buflen, (SDWORD *)bic->used, FALSE);
}

/*	This is called by SQLGetData() */
int
copy_and_convert_field(StatementClass *stmt, Int4 field_type, void *value, Int2 fCType, 
					   PTR rgbValue, SDWORD cbValueMax, SDWORD *pcbValue, char multiple)
{
Int4 len = 0;
SIMPLE_TIME st;
time_t t = time(NULL);
struct tm *tim;

	memset(&st, 0, sizeof(SIMPLE_TIME));

	/*	Initialize current date */
	tim = localtime(&t);
	st.m = tim->tm_mon + 1;
	st.d = tim->tm_mday;
	st.y = tim->tm_year + 1900;

	mylog("copy_and_convert: field_type = %d, fctype = %d, value = '%s', cbValueMax=%d\n", field_type, fCType, value, cbValueMax);

	if ( ! value) {
        /* handle a null just by returning SQL_NULL_DATA in pcbValue, */
        /* and doing nothing to the buffer.                           */
        if(pcbValue) {
            *pcbValue = SQL_NULL_DATA;
        }
		return COPY_OK;
	}


	if (stmt->hdbc->DataSourceToDriver != NULL) {
		int length = strlen (value);
		stmt->hdbc->DataSourceToDriver (stmt->hdbc->translation_option,
										SQL_CHAR,
										value, length,
										value, length, NULL,
										NULL, 0, NULL);
	}


	/********************************************************************
		First convert any specific postgres types into more
		useable data.

		NOTE: Conversions from PG char/varchar of a date/time/timestamp 
		value to SQL_C_DATE,SQL_C_TIME, SQL_C_TIMESTAMP not supported 
	*********************************************************************/
	switch(field_type) {
	/*  $$$ need to add parsing for date/time/timestamp strings in PG_TYPE_CHAR,VARCHAR $$$ */
	case PG_TYPE_DATE:
		sscanf(value, "%4d-%2d-%2d", &st.y, &st.m, &st.d);
		break;

	case PG_TYPE_TIME:
		sscanf(value, "%2d:%2d:%2d", &st.hh, &st.mm, &st.ss);
		break;

	case PG_TYPE_ABSTIME:
	case PG_TYPE_DATETIME:
	case PG_TYPE_TIMESTAMP:
		if (strnicmp(value, "invalid", 7) != 0) {
			sscanf(value, "%4d-%2d-%2d %2d:%2d:%2d", &st.y, &st.m, &st.d, &st.hh, &st.mm, &st.ss);

		} else {	/* The timestamp is invalid so set something conspicuous, like the epoch */
			t = 0;
			tim = localtime(&t);
			st.m = tim->tm_mon + 1;
			st.d = tim->tm_mday;
			st.y = tim->tm_year + 1900;
			st.hh = tim->tm_hour;
			st.mm = tim->tm_min;
			st.ss = tim->tm_sec;
		}
		break;

	case PG_TYPE_BOOL: {		/* change T/F to 1/0 */
		char *s = (char *) value;
		if (s[0] == 'T' || s[0] == 't') 
			s[0] = '1';
		else 
			s[0] = '0';
		}
		break;

	/* This is for internal use by SQLStatistics() */
	case PG_TYPE_INT28: {
		// this is an array of eight integers
		short *short_array = (short *)rgbValue;

		len = 16;

		sscanf(value, "%hd %hd %hd %hd %hd %hd %hd %hd",
			&short_array[0],
			&short_array[1],
			&short_array[2],
			&short_array[3],
			&short_array[4],
			&short_array[5],
			&short_array[6],
			&short_array[7]);

		/*  There is no corresponding fCType for this. */
		if(pcbValue)
			*pcbValue = len;

		return COPY_OK;		/* dont go any further or the data will be trashed */
						}

	/* This is a large object OID, which is used to store LONGVARBINARY objects. */
	case PG_TYPE_LO:

		return convert_lo( stmt, value, fCType, rgbValue, cbValueMax, pcbValue, multiple);

	default:

		if (field_type == stmt->hdbc->lobj_type)	/* hack until permanent type available */
			return convert_lo( stmt, value, fCType, rgbValue, cbValueMax, pcbValue, multiple);
	}

	/*  Change default into something useable */
	if (fCType == SQL_C_DEFAULT) {
		fCType = pgtype_to_ctype(stmt, field_type);

		mylog("copy_and_convert, SQL_C_DEFAULT: fCType = %d\n", fCType);
	}


    if(fCType == SQL_C_CHAR) {

		/*	Special character formatting as required */
		/*	These really should return error if cbValueMax is not big enough. */
		switch(field_type) {
		case PG_TYPE_DATE:
		    len = 10;
			if (cbValueMax > len)
				sprintf((char *)rgbValue, "%.4d-%.2d-%.2d", st.y, st.m, st.d);
			break;

		case PG_TYPE_TIME:
			len = 8;
			if (cbValueMax > len)
				sprintf((char *)rgbValue, "%.2d:%.2d:%.2d", st.hh, st.mm, st.ss);
			break;

		case PG_TYPE_ABSTIME:
		case PG_TYPE_DATETIME:
		case PG_TYPE_TIMESTAMP:
			len = 19;
			if (cbValueMax > len)
				sprintf((char *) rgbValue, "%.4d-%.2d-%.2d %.2d:%.2d:%.2d", 
					st.y, st.m, st.d, st.hh, st.mm, st.ss);
			break;

		case PG_TYPE_BOOL:
			len = 1;
			if (cbValueMax > len) {
				strcpy((char *) rgbValue, value);
				mylog("PG_TYPE_BOOL: rgbValue = '%s'\n", rgbValue);
			}
			break;

		/*	Currently, data is SILENTLY TRUNCATED for BYTEA and character data
			types if there is not enough room in cbValueMax because the driver 
			can't handle multiple calls to SQLGetData for these, yet.  Most likely,
			the buffer passed in will be big enough to handle the maximum limit of 
			postgres, anyway.

			LongVarBinary types are handled correctly above, observing truncation
			and all that stuff since there is essentially no limit on the large
			object used to store those.
		*/
		case PG_TYPE_BYTEA:		// convert binary data to hex strings (i.e, 255 = "FF")
			len = convert_pgbinary_to_char(value, rgbValue, cbValueMax);
			break;

		default:
			/*	convert linefeeds to carriage-return/linefeed */
			convert_linefeeds( (char *) value, rgbValue, cbValueMax);
		    len = strlen(rgbValue);

			mylog("    SQL_C_CHAR, default: len = %d, cbValueMax = %d, rgbValue = '%s'\n", len, cbValueMax, rgbValue);
			break;
		}


    } else {

		/*	for SQL_C_CHAR, its probably ok to leave currency symbols in.  But
			to convert to numeric types, it is necessary to get rid of those.
		*/
		if (field_type == PG_TYPE_MONEY)
			convert_money(value);

		switch(fCType) {
		case SQL_C_DATE:
			len = 6;
			{
			DATE_STRUCT *ds = (DATE_STRUCT *) rgbValue;
			ds->year = st.y;
			ds->month = st.m;
			ds->day = st.d;
			}
			break;

		case SQL_C_TIME:
			len = 6;
			{
			TIME_STRUCT *ts = (TIME_STRUCT *) rgbValue;
			ts->hour = st.hh;
			ts->minute = st.mm;
			ts->second = st.ss;
			}
			break;

		case SQL_C_TIMESTAMP:					
			len = 16;
			{
			TIMESTAMP_STRUCT *ts = (TIMESTAMP_STRUCT *) rgbValue;
			ts->year = st.y;
			ts->month = st.m;
			ts->day = st.d;
			ts->hour = st.hh;
			ts->minute = st.mm;
			ts->second = st.ss;
			ts->fraction = 0;
			}
			break;

		case SQL_C_BIT:
			len = 1;
			*((UCHAR *)rgbValue) = atoi(value);
			mylog("SQL_C_BIT: val = %d, cb = %d, rgb=%d\n", atoi(value), cbValueMax, *((UCHAR *)rgbValue));
			break;

		case SQL_C_STINYINT:
		case SQL_C_TINYINT:
			len = 1;
			*((SCHAR *) rgbValue) = atoi(value);
			break;

		case SQL_C_UTINYINT:
			len = 1;
			*((UCHAR *) rgbValue) = atoi(value);
			break;

		case SQL_C_FLOAT:
			len = 4;
			*((SFLOAT *)rgbValue) = (float) atof(value);
			break;

		case SQL_C_DOUBLE:
			len = 8;
			*((SDOUBLE *)rgbValue) = atof(value);
			break;

		case SQL_C_SSHORT:
		case SQL_C_SHORT:
			len = 2;
			*((SWORD *)rgbValue) = atoi(value);
			break;

		case SQL_C_USHORT:
			len = 2;
			*((UWORD *)rgbValue) = atoi(value);
			break;

		case SQL_C_SLONG:
		case SQL_C_LONG:
			len = 4;
			*((SDWORD *)rgbValue) = atol(value);
			break;

		case SQL_C_ULONG:
			len = 4;
			*((UDWORD *)rgbValue) = atol(value);
			break;

		case SQL_C_BINARY:	

			//	truncate if necessary
			//	convert octal escapes to bytes
			len = convert_from_pgbinary(value, rgbValue, cbValueMax);
			mylog("SQL_C_BINARY: len = %d\n", len);
			break;
			
		default:
			return COPY_UNSUPPORTED_TYPE;
		}
	}

    // store the length of what was copied, if there's a place for it
    if(pcbValue)
        *pcbValue = len;

	return COPY_OK;

}


/*	This function inserts parameters into an SQL statements.
	It will also modify a SELECT statement for use with declare/fetch cursors.
	This function no longer does any dynamic memory allocation!
*/
int
copy_statement_with_parameters(StatementClass *stmt)
{
static char *func="copy_statement_with_parameters";
unsigned int opos, npos;
char param_string[128], tmp[256], cbuf[TEXT_FIELD_SIZE+5];
int param_number;
Int2 param_ctype, param_sqltype;
char *old_statement = stmt->statement;
char *new_statement = stmt->stmt_with_params;
SIMPLE_TIME st;
time_t t = time(NULL);
struct tm *tim;
SDWORD used;
char *buffer, *buf;
char in_quote = FALSE;


	if ( ! old_statement) {
		SC_log_error(func, "No statement string", stmt);
		return SQL_ERROR;
	}


	memset(&st, 0, sizeof(SIMPLE_TIME));

	/*	Initialize current date */
	tim = localtime(&t);
	st.m = tim->tm_mon + 1;
	st.d = tim->tm_mday;
	st.y = tim->tm_year + 1900;

	/*	If the application hasn't set a cursor name, then generate one */
	if ( stmt->cursor_name[0] == '\0')
		sprintf(stmt->cursor_name, "SQL_CUR%p", stmt);

	//	For selects, prepend a declare cursor to the statement
	if (stmt->statement_type == STMT_TYPE_SELECT && globals.use_declarefetch) {
		sprintf(new_statement, "declare %s cursor for ", stmt->cursor_name);
		npos = strlen(new_statement);
	}
	else {
		new_statement[0] = '0';
		npos = 0;
	}

    param_number = -1;

    for (opos = 0; opos < strlen(old_statement); opos++) {

		//	Squeeze carriage-returns/linfeed pairs to linefeed only
		if (old_statement[opos] == '\r' && opos+1<strlen(old_statement) && old_statement[opos+1] == '\n') {
			continue;
		}

		//	Handle literals (date, time, timestamp)
		else if (old_statement[opos] == '{') {
			char *esc;
			char *begin = &old_statement[opos + 1];
			char *end = strchr(begin, '}');

			if ( ! end)
				continue;

			*end = '\0';

			esc = convert_escape(begin);
			if (esc) {
				memcpy(&new_statement[npos], esc, strlen(esc));
				npos += strlen(esc);
			}
			else {		/* its not a valid literal so just copy */
				*end = '}';	
				new_statement[npos++] = old_statement[opos];
				continue;
			}

			opos += end - begin + 1;

			*end = '}';

			continue;
		}

		/*	Can you have parameter markers inside of quotes?  I dont think so.
			All the queries I've seen expect the driver to put quotes if needed.
		*/
		else if (old_statement[opos] == '?' && !in_quote)
			;	/* ok */
		else {
			if (old_statement[opos] == '\'')
				in_quote = (in_quote ? FALSE : TRUE);

			new_statement[npos++] = old_statement[opos];
			continue;
		}



		/****************************************************/
		/*       Its a '?' parameter alright                */
		/****************************************************/

		param_number++;

	    if (param_number >= stmt->parameters_allocated)
			break;

		/*	Assign correct buffers based on data at exec param or not */
		if ( stmt->parameters[param_number].data_at_exec) {
			used = stmt->parameters[param_number].EXEC_used ? *stmt->parameters[param_number].EXEC_used : SQL_NTS;
			buffer = stmt->parameters[param_number].EXEC_buffer;
		}
		else {
			used = stmt->parameters[param_number].used ? *stmt->parameters[param_number].used : SQL_NTS;
			buffer = stmt->parameters[param_number].buffer;
		}

		/*	Handle NULL parameter data */
		if (used == SQL_NULL_DATA) {
			strcpy(&new_statement[npos], "NULL");
			npos += 4;
			continue;
		}

		/*	If no buffer, and its not null, then what the hell is it? 
			Just leave it alone then.
		*/
		if ( ! buffer) {
			new_statement[npos++] = '?';
			continue;
		}

		param_ctype = stmt->parameters[param_number].CType;
		param_sqltype = stmt->parameters[param_number].SQLType;
		
		mylog("copy_statement_with_params: from(fcType)=%d, to(fSqlType)=%d\n", param_ctype, param_sqltype);
		
		// replace DEFAULT with something we can use
		if(param_ctype == SQL_C_DEFAULT)
			param_ctype = sqltype_to_default_ctype(param_sqltype);

		buf = NULL;
		param_string[0] = '\0';
		cbuf[0] = '\0';

		
		/*	Convert input C type to a neutral format */
		switch(param_ctype) {
		case SQL_C_BINARY:
		case SQL_C_CHAR:
			buf = buffer;
			break;

		case SQL_C_DOUBLE:
			sprintf(param_string, "%f", 
				 *((SDOUBLE *) buffer));
			break;

		case SQL_C_FLOAT:
			sprintf(param_string, "%f", 
				 *((SFLOAT *) buffer));
			break;

		case SQL_C_SLONG:
		case SQL_C_LONG:
			sprintf(param_string, "%ld",
				*((SDWORD *) buffer));
			break;

		case SQL_C_SSHORT:
		case SQL_C_SHORT:
			sprintf(param_string, "%d",
				*((SWORD *) buffer));
			break;

		case SQL_C_STINYINT:
		case SQL_C_TINYINT:
			sprintf(param_string, "%d",
				*((SCHAR *) buffer));
			break;

		case SQL_C_ULONG:
			sprintf(param_string, "%lu",
				*((UDWORD *) buffer));
			break;

		case SQL_C_USHORT:
			sprintf(param_string, "%u",
				*((UWORD *) buffer));
			break;

		case SQL_C_UTINYINT:
			sprintf(param_string, "%u",
				*((UCHAR *) buffer));
			break;

		case SQL_C_BIT: {
			int i = *((UCHAR *) buffer);
			
			sprintf(param_string, "%d", i ? 1 : 0);
			break;
						}

		case SQL_C_DATE: {
			DATE_STRUCT *ds = (DATE_STRUCT *) buffer;
			st.m = ds->month;
			st.d = ds->day;
			st.y = ds->year;

			break;
						 }

		case SQL_C_TIME: {
			TIME_STRUCT *ts = (TIME_STRUCT *) buffer;
			st.hh = ts->hour;
			st.mm = ts->minute;
			st.ss = ts->second;

			break;
						 }

		case SQL_C_TIMESTAMP: {
			TIMESTAMP_STRUCT *tss = (TIMESTAMP_STRUCT *) buffer;
			st.m = tss->month;
			st.d = tss->day;
			st.y = tss->year;
			st.hh = tss->hour;
			st.mm = tss->minute;
			st.ss = tss->second;

			mylog("m=%d,d=%d,y=%d,hh=%d,mm=%d,ss=%d\n", st.m, st.d, st.y, st.hh, st.mm, st.ss);

			break;

							  }
		default:
			// error
			stmt->errormsg = "Unrecognized C_parameter type in copy_statement_with_parameters";
			stmt->errornumber = STMT_NOT_IMPLEMENTED_ERROR;
			new_statement[npos] = '\0';   // just in case
			SC_log_error(func, "", stmt);
			return SQL_ERROR;
		}

		/*	Now that the input data is in a neutral format, convert it to
			the desired output format (sqltype)
		*/

		switch(param_sqltype) {
		case SQL_CHAR:
		case SQL_VARCHAR:
		case SQL_LONGVARCHAR:

			new_statement[npos++] = '\'';	/*    Open Quote */

			/* it was a SQL_C_CHAR */
			if (buf) {
				convert_special_chars(buf, &new_statement[npos], used);
				npos += strlen(&new_statement[npos]);
			}

			/* it was a numeric type */
			else if (param_string[0] != '\0') {	
				strcpy(&new_statement[npos], param_string);
				npos += strlen(param_string);
			}

			/* it was date,time,timestamp -- use m,d,y,hh,mm,ss */
			else {
				sprintf(tmp, "%.4d-%.2d-%.2d %.2d:%.2d:%.2d",
					st.y, st.m, st.d, st.hh, st.mm, st.ss);

				strcpy(&new_statement[npos], tmp);
				npos += strlen(tmp);
			}

			new_statement[npos++] = '\'';	/*    Close Quote */

			break;

		case SQL_DATE:
			if (buf) {  /* copy char data to time */
				my_strcpy(cbuf, sizeof(cbuf), buf, used);
				parse_datetime(cbuf, &st);
			}

			sprintf(tmp, "'%.4d-%.2d-%.2d'", st.y, st.m, st.d);

			strcpy(&new_statement[npos], tmp);
			npos += strlen(tmp);
			break;

		case SQL_TIME:
			if (buf) {  /* copy char data to time */
				my_strcpy(cbuf, sizeof(cbuf), buf, used);
				parse_datetime(cbuf, &st);
			}

			sprintf(tmp, "'%.2d:%.2d:%.2d'", st.hh, st.mm, st.ss);

			strcpy(&new_statement[npos], tmp);
			npos += strlen(tmp);
			break;

		case SQL_TIMESTAMP:

			if (buf) {
				my_strcpy(cbuf, sizeof(cbuf), buf, used);
				parse_datetime(cbuf, &st);
			}

			sprintf(tmp, "'%.4d-%.2d-%.2d %.2d:%.2d:%.2d'",
				st.y, st.m, st.d, st.hh, st.mm, st.ss);

			strcpy(&new_statement[npos], tmp);
			npos += strlen(tmp);

			break;

		case SQL_BINARY:
		case SQL_VARBINARY:			/* non-ascii characters should be converted to octal */
			new_statement[npos++] = '\'';	/*    Open Quote */

			mylog("SQL_LONGVARBINARY: about to call convert_to_pgbinary, used = %d\n", used);

			npos += convert_to_pgbinary(buf, &new_statement[npos], used);

			new_statement[npos++] = '\'';	/*    Close Quote */
			
			break;
		case SQL_LONGVARBINARY:		
			/*	the oid of the large object -- just put that in for the
				parameter marker -- the data has already been sent to the large object
			*/
			sprintf(param_string, "%d", stmt->parameters[param_number].lobj_oid);
			strcpy(&new_statement[npos], param_string);
			npos += strlen(param_string);

			break;

		//	because of no conversion operator for bool and int4, SQL_BIT
		//	must be quoted (0 or 1 is ok to use inside the quotes)

		default:		/* a numeric type or SQL_BIT */
			if (param_sqltype == SQL_BIT)
				new_statement[npos++] = '\'';	/*    Open Quote */

			if (buf) {
				my_strcpy(&new_statement[npos], sizeof(stmt->stmt_with_params) - npos, buf, used);
				npos += strlen(&new_statement[npos]);
			}
			else {
				strcpy(&new_statement[npos], param_string);
				npos += strlen(param_string);
			}

			if (param_sqltype == SQL_BIT)
				new_statement[npos++] = '\'';	/*    Close Quote */

			break;

		}

	}	/* end, for */

	// make sure new_statement is always null-terminated
	new_statement[npos] = '\0';


	if(stmt->hdbc->DriverToDataSource != NULL) {
		int length = strlen (new_statement);
		stmt->hdbc->DriverToDataSource (stmt->hdbc->translation_option,
										SQL_CHAR,
										new_statement, length,
										new_statement, length, NULL,
										NULL, 0, NULL);
	}


	return SQL_SUCCESS;
}

char *
mapFunction(char *func)
{
int i;

	for (i = 0; mapFuncs[i][0]; i++)
		if ( ! stricmp(mapFuncs[i][0], func))
			return mapFuncs[i][1];

	return NULL;
}

//	This function returns a pointer to static memory!
char *
convert_escape(char *value)
{
char key[32], val[256];
static char escape[1024];
char func[32], the_rest[1024];
char *mapFunc;

	sscanf(value, "%s %[^\r]", key, val);

	mylog("convert_escape: key='%s', val='%s'\n", key, val);

	if ( ! strcmp(key, "d") ||
		 ! strcmp(key, "t") ||
		 ! strcmp(key, "ts")) {

		strcpy(escape, val);
	}
	else if ( ! strcmp(key, "fn")) {
		sscanf(val, "%[^(]%[^\r]", func, the_rest);
		mapFunc = mapFunction(func);
		if ( ! mapFunc)
			return NULL;
		else {
			strcpy(escape, mapFunc);
			strcat(escape, the_rest);
		}

	}
	else {
		return NULL;
	}

	return escape;

}


char *
convert_money(char *s)
{
size_t i = 0, out = 0;

	for (i = 0; i < strlen(s); i++) {
		if (s[i] == '$' || s[i] == ',' || s[i] == ')')
			; // skip these characters
		else if (s[i] == '(')
			s[out++] = '-';
		else
			s[out++] = s[i];
	}
	s[out] = '\0';
	return s;
}



/*	This function parses a character string for date/time info and fills in SIMPLE_TIME */
/*	It does not zero out SIMPLE_TIME in case it is desired to initialize it with a value */
char
parse_datetime(char *buf, SIMPLE_TIME *st)
{
int y,m,d,hh,mm,ss;
int nf;
	
	y = m = d = hh = mm = ss = 0;

	if (buf[4] == '-')	/* year first */
		nf = sscanf(buf, "%4d-%2d-%2d %2d:%2d:%2d", &y,&m,&d,&hh,&mm,&ss);
	else
		nf = sscanf(buf, "%2d-%2d-%4d %2d:%2d:%2d", &m,&d,&y,&hh,&mm,&ss);

	if (nf == 5 || nf == 6) {
		st->y = y;
		st->m = m;
		st->d = d;
		st->hh = hh;
		st->mm = mm;
		st->ss = ss;

		return TRUE;
	}

	if (buf[4] == '-')	/* year first */
		nf = sscanf(buf, "%4d-%2d-%2d", &y, &m, &d);
	else
		nf = sscanf(buf, "%2d-%2d-%4d", &m, &d, &y);

	if (nf == 3) {
		st->y = y;
		st->m = m;
		st->d = d;

		return TRUE;
	}

	nf = sscanf(buf, "%2d:%2d:%2d", &hh, &mm, &ss);
	if (nf == 2 || nf == 3) {
		st->hh = hh;
		st->mm = mm;
		st->ss = ss;

		return TRUE;
	}

	return FALSE;
}

/*	Change linefeed to carriage-return/linefeed */
char *
convert_linefeeds(char *si, char *dst, size_t max)
{
size_t i = 0, out = 0;
static char sout[TEXT_FIELD_SIZE+5];
char *p;

	if (dst)
		p = dst;
	else {
		p = sout;
		max = sizeof(sout);
	}

	p[0] = '\0';

	for (i = 0; i < strlen(si) && out < max; i++) {
		if (si[i] == '\n') {
			/*	Only add the carriage-return if needed */
			if (i > 0 && si[i-1] == '\r') {
				p[out++] = si[i];
				continue;
			}

			p[out++] = '\r';
			p[out++] = '\n';
		}
		else
			p[out++] = si[i];
	}
	p[out] = '\0';
	return p;
}

/*	Change carriage-return/linefeed to just linefeed 
	Plus, escape any special characters.
*/
char *
convert_special_chars(char *si, char *dst, int used)
{
size_t i = 0, out = 0, max;
static char sout[TEXT_FIELD_SIZE+5];
char *p;

	if (dst)
		p = dst;
	else
		p = sout;

	p[0] = '\0';

	if (used == SQL_NTS)
		max = strlen(si);
	else
		max = used;

	for (i = 0; i < max; i++) {
		if (si[i] == '\r' && i+1 < strlen(si) && si[i+1] == '\n') 
			continue;
		if (si[i] == '\'')
			p[out++] = '\\';

		p[out++] = si[i];
	}
	p[out] = '\0';
	return p;
}

/*	!!! Need to implement this function !!!  */
int
convert_pgbinary_to_char(char *value, char *rgbValue, int cbValueMax)
{
	mylog("convert_pgbinary_to_char: value = '%s'\n", value);

	strncpy_null(rgbValue, value, cbValueMax);
	return 0;
}

unsigned int
conv_from_octal(unsigned char *s)
{
int i, y=0;

	for (i = 1; i <= 3; i++) {
		y += (s[i] - 48) * (int) pow(8, 3-i);
	}

	return y;

}

unsigned int
conv_from_hex(unsigned char *s)
{
int i, y=0, val;

	for (i = 1; i <= 2; i++) {

        if (s[i] >= 'a' && s[i] <= 'f')
            val = s[i] - 'a' + 10;
        else if (s[i] >= 'A' && s[i] <= 'F')
            val = s[i] - 'A' + 10;
        else
            val = s[i] - '0';

		y += val * (int) pow(16, 2-i);
	}

	return y;
}

//	convert octal escapes to bytes
int
convert_from_pgbinary(unsigned char *value, unsigned char *rgbValue, int cbValueMax)
{
size_t i;
int o=0;
	
	for (i = 0; i < strlen(value); ) {
		if (value[i] == '\\') {
			rgbValue[o] = conv_from_octal(&value[i]);
			i += 4;
		}
		else {
			rgbValue[o] = value[i++];
		}
		mylog("convert_from_pgbinary: i=%d, rgbValue[%d] = %d, %c\n", i, o, rgbValue[o], rgbValue[o]);
		o++;
	}

	rgbValue[o] = '\0';

	return o;
}


char *
conv_to_octal(unsigned char val)
{
int i;
static char x[6];

	x[0] = '\\';
	x[1] = '\\';
	x[5] = '\0';

	for (i = 4; i > 1; i--) {
		x[i] = (val & 7) + 48;
		val >>= 3;
	}

	return x;
}

//	convert non-ascii bytes to octal escape sequences
int
convert_to_pgbinary(unsigned char *in, char *out, int len)
{
int i, o=0;


	for (i = 0; i < len; i++) {
		mylog("convert_to_pgbinary: in[%d] = %d, %c\n", i, in[i], in[i]);
		if (in[i] < 32 || in[i] > 126) {
			strcpy(&out[o], conv_to_octal(in[i])); 
			o += 5;
		}
		else
			out[o++] = in[i];

	}

	mylog("convert_to_pgbinary: returning %d, out='%.*s'\n", o, o, out);

	return o;
}


void
encode(char *in, char *out)
{
unsigned int i, o = 0;

	for (i = 0; i < strlen(in); i++) {
		if ( in[i] == '+') {
			sprintf(&out[o], "%%2B");
			o += 3;
		}
		else if ( isspace(in[i])) {
			out[o++] = '+';
		}
		else if ( ! isalnum(in[i])) {
			sprintf(&out[o], "%%%02x", in[i]);
			o += 3;
		}
		else
			out[o++] = in[i];
	}
	out[o++] = '\0';
}


void
decode(char *in, char *out)
{
unsigned int i, o = 0;

	for (i = 0; i < strlen(in); i++) { 
		if (in[i] == '+')
			out[o++] = ' ';
		else if (in[i] == '%') {
			sprintf(&out[o++], "%c", conv_from_hex(&in[i]));
			i+=2;
		}
		else
			out[o++] = in[i];
	}
	out[o++] = '\0';
}



/*	1. get oid (from 'value')
	2. open the large object
	3. read from the large object (handle multiple GetData)
	4. close when read less than requested?  -OR-
		lseek/read each time
		handle case where application receives truncated and
		decides not to continue reading.

	CURRENTLY, ONLY LONGVARBINARY is handled, since that is the only
	data type currently mapped to a PG_TYPE_LO.  But, if any other types
	are desired to map to a large object (PG_TYPE_LO), then that would 
	need to be handled here.  For example, LONGVARCHAR could possibly be
	mapped to PG_TYPE_LO someday, instead of PG_TYPE_TEXT as it is now.
*/
int
convert_lo(StatementClass *stmt, void *value, Int2 fCType, PTR rgbValue, 
		   SDWORD cbValueMax, SDWORD *pcbValue, char multiple)
{
Oid oid;
int retval;

	/*	if this is the first call for this column,
		open the large object for reading 
	*/
	if ( ! multiple) {
		oid = atoi(value);
		stmt->lobj_fd = lo_open(stmt->hdbc, oid, INV_READ);
		if (stmt->lobj_fd < 0) {
			stmt->errornumber = STMT_EXEC_ERROR;
			stmt->errormsg = "Couldnt open large object for reading.";
			return COPY_GENERAL_ERROR;
		}
	}

	if (stmt->lobj_fd < 0) {
		stmt->errornumber = STMT_EXEC_ERROR;
		stmt->errormsg = "Large object FD undefined for multiple read.";
		return COPY_GENERAL_ERROR;
	}

	retval = lo_read(stmt->hdbc, stmt->lobj_fd, rgbValue, cbValueMax);
	if (retval < 0) {
		lo_close(stmt->hdbc, stmt->lobj_fd);
		stmt->lobj_fd = -1;

		stmt->errornumber = STMT_EXEC_ERROR;
		stmt->errormsg = "Error reading from large object.";
		return COPY_GENERAL_ERROR;
	}
	else if (retval < cbValueMax)  {	/* success, all done */
		lo_close(stmt->hdbc, stmt->lobj_fd);
		stmt->lobj_fd = -1;	/* prevent further reading */

		if (pcbValue)
			*pcbValue = retval;

		return COPY_OK;
	}
	else {	/* retval == cbVaueMax -- assume truncated */
		if (pcbValue)
			*pcbValue = SQL_NO_TOTAL;

		return COPY_RESULT_TRUNCATED;

	}
}
