
/* -----------------------------------------------------------------------
 * formatting.h
 *
 * $Id: formatting.h,v 1.1 2000-01-25 23:53:56 momjian Exp $
 *
 *
 *   The PostgreSQL routines for a DateTime/int/float/numeric formatting, 
 *   inspire with Oracle TO_CHAR() / TO_DATE() / TO_NUMBER() routines.  
 *
 *   1999 Karel Zak "Zakkr"
 *
 * -----------------------------------------------------------------------
 */

#ifndef _FORMATTING_H_
#define _FORMATTING_H_

extern text 	*datetime_to_char(DateTime *dt, text *fmt);
extern text 	*timestamp_to_char(time_t dt, text *fmt);
extern DateTime *to_datetime(text *date_str, text *fmt);
extern time_t	to_timestamp(text *date_str, text *fmt);
extern DateADT  to_date(text *date_str, text *fmt);
extern Numeric	numeric_to_number(text *value, text *fmt);
extern text 	*numeric_to_char(Numeric value, text *fmt);
extern text 	*int4_to_char(int32 value, text *fmt);
extern text 	*int8_to_char(int64 *value, text *fmt);
extern text 	*float4_to_char(float32 value, text *fmt);
extern text 	*float8_to_char(float64 value, text *fmt);

#endif
