/*
 * cash.c
 * Written by D'Arcy J.M. Cain
 *
 * Functions to allow input and output of money normally but store
 * and handle it as int4s
 *
 * A slightly modified version of this file and a discussion of the
 * workings can be found in the book "Software Solutions in C" by
 * Dale Schumacher, Academic Press, ISBN: 0-12-632360-7.
 *
 * $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/adt/cash.c,v 1.26 1998-09-01 04:32:26 momjian Exp $
 */

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <locale.h>

#include "postgres.h"
#include "miscadmin.h"
#include "utils/builtins.h"
#include "utils/cash.h"

static const char *num_word(Cash value);

/* when we go to 64 bit values we will have to modify this */
#define CASH_BUFSZ		24

#define TERMINATOR		(CASH_BUFSZ - 1)
#define LAST_PAREN		(TERMINATOR - 1)
#define LAST_DIGIT		(LAST_PAREN - 1)

#ifdef USE_LOCALE
static struct lconv *lconvert = NULL;

#endif

/* cash_in()
 * Convert a string to a cash data type.
 * Format is [$]###[,]###[.##]
 * Examples: 123.45 $123.45 $123,456.78
 *
 * This is currently implemented as a 32-bit integer.
 * XXX HACK It looks as though some of the symbols for
 *	monetary values returned by localeconv() can be multiple
 *	bytes/characters. This code assumes one byte only. - tgl 97/04/14
 * XXX UNHACK Allow the currency symbol to be multi-byte.
 *	- thomas 1998-03-01
 */
Cash *
cash_in(const char *str)
{
	Cash	   *result;

	Cash		value = 0;
	Cash		dec = 0;
	Cash		sgn = 1;
	int			seen_dot = 0;
	const char *s = str;
	int			fpoint;
	char	   *csymbol;
	char		dsymbol,
				ssymbol,
				psymbol,
			   *nsymbol;

#ifdef USE_LOCALE
#ifdef CASHDEBUG
	setlocale(LC_ALL, "");
	lconvert = localeconv();
#endif
	if (lconvert == NULL)
		lconvert = localeconv();

	/* frac_digits in the C locale seems to return CHAR_MAX */
	/* best guess is 2 in this case I think */
	fpoint = ((lconvert->frac_digits != CHAR_MAX) ? lconvert->frac_digits : 2); /* int_frac_digits? */

	dsymbol = ((*lconvert->mon_decimal_point != '\0') ? *lconvert->mon_decimal_point : '.');
	ssymbol = ((*lconvert->mon_thousands_sep != '\0') ? *lconvert->mon_thousands_sep : ',');
	csymbol = ((*lconvert->currency_symbol != '\0') ? lconvert->currency_symbol : "$");
	psymbol = ((*lconvert->positive_sign != '\0') ? *lconvert->positive_sign : '+');
	nsymbol = ((*lconvert->negative_sign != '\0') ? lconvert->negative_sign : "-");
#else
	fpoint = 2;
	dsymbol = '.';
	ssymbol = ',';
	csymbol = "$";
	psymbol = '+';
	nsymbol = "-";
#endif

#ifdef CASHDEBUG
	printf("cashin- precision '%d'; decimal '%c'; thousands '%c'; currency '%s'; positive '%c'; negative '%s'\n",
		   fpoint, dsymbol, ssymbol, csymbol, psymbol, nsymbol);
#endif

	/* we need to add all sorts of checking here.  For now just */
	/* strip all leading whitespace and any leading currency symbol */
	while (isspace(*s))
		s++;
	if (strncmp(s, csymbol, strlen(csymbol)) == 0)
		s += strlen(csymbol);

#ifdef CASHDEBUG
	printf("cashin- string is '%s'\n", s);
#endif

	/* a leading minus or paren signifies a negative number */
	/* again, better heuristics needed */
	if (strncmp(s, nsymbol, strlen(nsymbol)) == 0)
	{
		sgn = -1;
		s += strlen(nsymbol);
#ifdef CASHDEBUG
		printf("cashin- negative symbol; string is '%s'\n", s);
#endif
	}
	else if (*s == '(')
	{
		sgn = -1;
		s++;

	}
	else if (*s == psymbol)
		s++;

#ifdef CASHDEBUG
	printf("cashin- string is '%s'\n", s);
#endif

	while (isspace(*s))
		s++;
	if (strncmp(s, csymbol, strlen(csymbol)) == 0)
		s += strlen(csymbol);

#ifdef CASHDEBUG
	printf("cashin- string is '%s'\n", s);
#endif

	for (;; s++)
	{
		/* we look for digits as int4 as we have less */
		/* than the required number of decimal places */
		if (isdigit(*s) && dec < fpoint)
		{
			value = (value * 10) + *s - '0';

			if (seen_dot)
				dec++;

			/* decimal point? then start counting fractions... */
		}
		else if (*s == dsymbol && !seen_dot)
		{
			seen_dot = 1;

			/* "thousands" separator? then skip... */
		}
		else if (*s == ssymbol)
		{

		}
		else
		{
			/* round off */
			if (isdigit(*s) && *s >= '5')
				value++;

			/* adjust for less than required decimal places */
			for (; dec < fpoint; dec++)
				value *= 10;

			break;
		}
	}

	while (isspace(*s) || *s == '0' || *s == ')')
		s++;

	if (*s != '\0')
		elog(ERROR, "Bad money external representation %s", str);

	if (!PointerIsValid(result = palloc(sizeof(Cash))))
		elog(ERROR, "Memory allocation failed, can't input cash '%s'", str);

	*result = (value * sgn);

#ifdef CASHDEBUG
	printf("cashin- result is %d\n", *result);
#endif

	return result;
}	/* cash_in() */


/* cash_out()
 * Function to convert cash to a dollars and cents representation.
 * XXX HACK This code appears to assume US conventions for
 *	positive-valued amounts. - tgl 97/04/14
 */
const char *
cash_out(Cash *in_value)
{
	Cash		value = *in_value;
	char	   *result;
	char		buf[CASH_BUFSZ];
	int			minus = 0;
	int			count = LAST_DIGIT;
	int			point_pos;
	int			comma_position = 0;
	char		mon_group,
				comma,
				points;
	char	   *csymbol,
				dsymbol,
			   *nsymbol;
	char		convention;

#ifdef USE_LOCALE
	if (lconvert == NULL)
		lconvert = localeconv();

	mon_group = *lconvert->mon_grouping;
	comma = ((*lconvert->mon_thousands_sep != '\0') ? *lconvert->mon_thousands_sep : ',');
	/* frac_digits in the C locale seems to return CHAR_MAX */
	/* best guess is 2 in this case I think */
	points = ((lconvert->frac_digits != CHAR_MAX) ? lconvert->frac_digits : 2); /* int_frac_digits? */
	convention = lconvert->n_sign_posn;
	dsymbol = ((*lconvert->mon_decimal_point != '\0') ? *lconvert->mon_decimal_point : '.');
	csymbol = ((*lconvert->currency_symbol != '\0') ? lconvert->currency_symbol : "$");
	nsymbol = ((*lconvert->negative_sign != '\0') ? lconvert->negative_sign : "-");
#else
	mon_group = 3;
	comma = ',';
	csymbol = "$";
	dsymbol = '.';
	nsymbol = "-";
	points = 2;
	convention = 0;
#endif

	point_pos = LAST_DIGIT - points;

	/* We're playing a little fast and loose with this.  Shoot me. */
	/* Not me, that was the other guy. Haven't fixed it yet - thomas */
	if (!mon_group || mon_group == CHAR_MAX)
		mon_group = 3;

	/* allow more than three decimal points and separate them */
	if (comma)
	{
		point_pos -= (points - 1) / mon_group;
		comma_position = point_pos % (mon_group + 1);
	}

	/* we work with positive amounts and add the minus sign at the end */
	if (value < 0)
	{
		minus = 1;
		value *= -1;
	}

	/* allow for trailing negative strings */
	MemSet(buf, ' ', CASH_BUFSZ);
	buf[TERMINATOR] = buf[LAST_PAREN] = '\0';

	while (value || count > (point_pos - 2))
	{
		if (points && count == point_pos)
			buf[count--] = dsymbol;
		else if (comma && count % (mon_group + 1) == comma_position)
			buf[count--] = comma;

		buf[count--] = (value % 10) + '0';
		value /= 10;
	}

	strncpy((buf + count - strlen(csymbol) + 1), csymbol, strlen(csymbol));
	count -= strlen(csymbol) - 1;

	if (buf[LAST_DIGIT] == ',')
		buf[LAST_DIGIT] = buf[LAST_PAREN];

	/* see if we need to signify negative amount */
	if (minus)
	{
		if (!PointerIsValid(result = palloc(CASH_BUFSZ + 2 - count + strlen(nsymbol))))
			elog(ERROR, "Memory allocation failed, can't output cash", NULL);

		/* Position code of 0 means use parens */
		if (convention == 0)
			sprintf(result, "(%s)", buf + count);
		else if (convention == 2)
			sprintf(result, "%s%s", buf + count, nsymbol);
		else
			sprintf(result, "%s%s", nsymbol, buf + count);
	}
	else
	{
		if (!PointerIsValid(result = palloc(CASH_BUFSZ + 2 - count)))
			elog(ERROR, "Memory allocation failed, can't output cash", NULL);

		strcpy(result, buf + count);
	}

	return result;
}	/* cash_out() */


bool
cash_eq(Cash *c1, Cash *c2)
{
	if (!PointerIsValid(c1) || !PointerIsValid(c2))
		return FALSE;

	return *c1 == *c2;
}	/* cash_eq() */

bool
cash_ne(Cash *c1, Cash *c2)
{
	if (!PointerIsValid(c1) || !PointerIsValid(c2))
		return FALSE;

	return *c1 != *c2;
}	/* cash_ne() */

bool
cash_lt(Cash *c1, Cash *c2)
{
	if (!PointerIsValid(c1) || !PointerIsValid(c2))
		return FALSE;

	return *c1 < *c2;
}	/* cash_lt() */

bool
cash_le(Cash *c1, Cash *c2)
{
	if (!PointerIsValid(c1) || !PointerIsValid(c2))
		return FALSE;

	return *c1 <= *c2;
}	/* cash_le() */

bool
cash_gt(Cash *c1, Cash *c2)
{
	if (!PointerIsValid(c1) || !PointerIsValid(c2))
		return FALSE;

	return *c1 > *c2;
}	/* cash_gt() */

bool
cash_ge(Cash *c1, Cash *c2)
{
	if (!PointerIsValid(c1) || !PointerIsValid(c2))
		return FALSE;

	return *c1 >= *c2;
}	/* cash_ge() */


/* cash_pl()
 * Add two cash values.
 */
Cash *
cash_pl(Cash *c1, Cash *c2)
{
	Cash	   *result;

	if (!PointerIsValid(c1) || !PointerIsValid(c2))
		return NULL;

	if (!PointerIsValid(result = palloc(sizeof(Cash))))
		elog(ERROR, "Memory allocation failed, can't add cash", NULL);

	*result = (*c1 + *c2);

	return result;
}	/* cash_pl() */


/* cash_mi()
 * Subtract two cash values.
 */
Cash *
cash_mi(Cash *c1, Cash *c2)
{
	Cash	   *result;

	if (!PointerIsValid(c1) || !PointerIsValid(c2))
		return NULL;

	if (!PointerIsValid(result = palloc(sizeof(Cash))))
		elog(ERROR, "Memory allocation failed, can't subtract cash", NULL);

	*result = (*c1 - *c2);

	return result;
}	/* cash_mi() */


/* cash_mul_flt8()
 * Multiply cash by float8.
 */
Cash *
cash_mul_flt8(Cash *c, float8 *f)
{
	Cash	   *result;

	if (!PointerIsValid(f) || !PointerIsValid(c))
		return NULL;

	if (!PointerIsValid(result = palloc(sizeof(Cash))))
		elog(ERROR, "Memory allocation failed, can't multiply cash", NULL);

	*result = ((*f) * (*c));

	return result;
}	/* cash_mul_flt8() */


/* flt8_mul_cash()
 * Multiply float8 by cash.
 */
Cash *
flt8_mul_cash(float8 *f, Cash *c)
{
	return cash_mul_flt8(c, f);
}	/* flt8_mul_cash() */


/* cash_div_flt8()
 * Divide cash by float8.
 *
 * XXX Don't know if rounding or truncating is correct behavior.
 * Round for now. - tgl 97/04/15
 */
Cash *
cash_div_flt8(Cash *c, float8 *f)
{
	Cash	   *result;

	if (!PointerIsValid(f) || !PointerIsValid(c))
		return NULL;

	if (!PointerIsValid(result = palloc(sizeof(Cash))))
		elog(ERROR, "Memory allocation failed, can't divide cash", NULL);

	if (*f == 0.0)
		elog(ERROR, "cash_div:  divide by 0.0 error");

	*result = rint(*c / *f);

	return result;
}	/* cash_div_flt8() */

/* cash_mul_flt4()
 * Multiply cash by float4.
 */
Cash *
cash_mul_flt4(Cash *c, float4 *f)
{
	Cash	   *result;

	if (!PointerIsValid(f) || !PointerIsValid(c))
		return NULL;

	if (!PointerIsValid(result = palloc(sizeof(Cash))))
		elog(ERROR, "Memory allocation failed, can't multiply cash", NULL);

	*result = ((*f) * (*c));

	return result;
}	/* cash_mul_flt4() */


/* flt4_mul_cash()
 * Multiply float4 by float4.
 */
Cash *
flt4_mul_cash(float4 *f, Cash *c)
{
	return cash_mul_flt4(c, f);
}	/* flt4_mul_cash() */


/* cash_div_flt4()
 * Divide cash by float4.
 *
 * XXX Don't know if rounding or truncating is correct behavior.
 * Round for now. - tgl 97/04/15
 */
Cash *
cash_div_flt4(Cash *c, float4 *f)
{
	Cash	   *result;

	if (!PointerIsValid(f) || !PointerIsValid(c))
		return NULL;

	if (!PointerIsValid(result = palloc(sizeof(Cash))))
		elog(ERROR, "Memory allocation failed, can't divide cash", NULL);

	if (*f == 0.0)
		elog(ERROR, "cash_div:  divide by 0.0 error");

	*result = rint(*c / *f);

	return result;
}	/* cash_div_flt4() */


/* cash_mul_int4()
 * Multiply cash by int4.
 */
Cash *
cash_mul_int4(Cash *c, int4 i)
{
	Cash	   *result;

	if (!PointerIsValid(c))
		return NULL;

	if (!PointerIsValid(result = palloc(sizeof(Cash))))
		elog(ERROR, "Memory allocation failed, can't multiply cash", NULL);

	*result = ((i) * (*c));

	return result;
}	/* cash_mul_int4() */


/* int4_mul_cash()
 * Multiply int4 by cash.
 */
Cash *
int4_mul_cash(int4 i, Cash *c)
{
	return cash_mul_int4(c, i);
}	/* int4_mul_cash() */


/* cash_div_int4()
 * Divide cash by 4-byte integer.
 *
 * XXX Don't know if rounding or truncating is correct behavior.
 * Round for now. - tgl 97/04/15
 */
Cash *
cash_div_int4(Cash *c, int4 i)
{
	Cash	   *result;

	if (!PointerIsValid(c))
		return NULL;

	if (!PointerIsValid(result = palloc(sizeof(Cash))))
		elog(ERROR, "Memory allocation failed, can't divide cash", NULL);

	if (i == 0)
		elog(ERROR, "cash_idiv:  divide by 0 error");

	*result = rint(*c / i);

	return result;
}	/* cash_div_int4() */


/* cash_mul_int2()
 * Multiply cash by int2.
 */
Cash *
cash_mul_int2(Cash *c, int2 s)
{
	Cash	   *result;

	if (!PointerIsValid(c))
		return NULL;

	if (!PointerIsValid(result = palloc(sizeof(Cash))))
		elog(ERROR, "Memory allocation failed, can't multiply cash", NULL);

	*result = ((s) * (*c));

	return result;
}	/* cash_mul_int2() */


/* int2_mul_cash()
 * Multiply int2 by cash.
 */
Cash *
int2_mul_cash(int2 s, Cash *c)
{
	return cash_mul_int2(c, s);
}	/* int2_mul_cash() */


/* cash_div_int2()
 * Divide cash by int2.
 *
 * XXX Don't know if rounding or truncating is correct behavior.
 * Round for now. - tgl 97/04/15
 */
Cash *
cash_div_int2(Cash *c, int2 s)
{
	Cash	   *result;

	if (!PointerIsValid(c))
		return NULL;

	if (!PointerIsValid(result = palloc(sizeof(Cash))))
		elog(ERROR, "Memory allocation failed, can't divide cash", NULL);

	if (s == 0)
		elog(ERROR, "cash_div:  divide by 0 error");

	*result = rint(*c / s);

	return result;
}	/* cash_div_int2() */


/* cashlarger()
 * Return larger of two cash values.
 */
Cash *
cashlarger(Cash *c1, Cash *c2)
{
	Cash	   *result;

	if (!PointerIsValid(c1) || !PointerIsValid(c2))
		return NULL;

	if (!PointerIsValid(result = palloc(sizeof(Cash))))
		elog(ERROR, "Memory allocation failed, can't return larger cash", NULL);

	*result = ((*c1 > *c2) ? *c1 : *c2);

	return result;
}	/* cashlarger() */


/* cashsmaller()
 * Return smaller of two cash values.
 */
Cash *
cashsmaller(Cash *c1, Cash *c2)
{
	Cash	   *result;

	if (!PointerIsValid(c1) || !PointerIsValid(c2))
		return NULL;

	if (!PointerIsValid(result = palloc(sizeof(Cash))))
		elog(ERROR, "Memory allocation failed, can't return smaller cash", NULL);

	*result = ((*c1 < *c2) ? *c1 : *c2);

	return result;
}	/* cashsmaller() */


/* cash_words_out()
 * This converts a int4 as well but to a representation using words
 * Obviously way North American centric - sorry
 */
const char *
cash_words_out(Cash *value)
{
	static char buf[128];
	char	   *p = buf;
	Cash		m0;
	Cash		m1;
	Cash		m2;
	Cash		m3;

	/* work with positive numbers */
	if (*value < 0)
	{
		*value *= -1;
		strcpy(buf, "minus ");
		p += 6;
	}
	else
		*buf = 0;

	m0 = *value % 100;			/* cents */
	m1 = (*value / 100) % 1000; /* hundreds */
	m2 = (*value / 100000) % 1000;		/* thousands */
	m3 = *value / 100000000 % 1000;		/* millions */

	if (m3)
	{
		strcat(buf, num_word(m3));
		strcat(buf, " million ");
	}

	if (m2)
	{
		strcat(buf, num_word(m2));
		strcat(buf, " thousand ");
	}

	if (m1)
		strcat(buf, num_word(m1));

	if (!*p)
		strcat(buf, "zero");

	strcat(buf, (int) (*value / 100) == 1 ? " dollar and " : " dollars and ");
	strcat(buf, num_word(m0));
	strcat(buf, m0 == 1 ? " cent" : " cents");
	*buf = toupper(*buf);
	return buf;
}	/* cash_words_out() */


/*************************************************************************
 * Private routines
 ************************************************************************/

static const char *
num_word(Cash value)
{
	static char buf[128];
	static const char *small[] = {
		"zero", "one", "two", "three", "four", "five", "six", "seven",
		"eight", "nine", "ten", "eleven", "twelve", "thirteen", "fourteen",
		"fifteen", "sixteen", "seventeen", "eighteen", "nineteen", "twenty",
		"thirty", "fourty", "fifty", "sixty", "seventy", "eighty", "ninety"
	};
	const char **big = small + 18;
	int			tu = value % 100;

	/* deal with the simple cases first */
	if (value <= 20)
		return small[value];

	/* is it an even multiple of 100? */
	if (!tu)
	{
		sprintf(buf, "%s hundred", small[value / 100]);
		return buf;
	}

	/* more than 99? */
	if (value > 99)
	{
		/* is it an even multiple of 10 other than 10? */
		if (value % 10 == 0 && tu > 10)
			sprintf(buf, "%s hundred %s",
					small[value / 100], big[tu / 10]);
		else if (tu < 20)
			sprintf(buf, "%s hundred and %s",
					small[value / 100], small[tu]);
		else
			sprintf(buf, "%s hundred %s %s",
					small[value / 100], big[tu / 10], small[tu % 10]);

	}
	else
	{
		/* is it an even multiple of 10 other than 10? */
		if (value % 10 == 0 && tu > 10)
			sprintf(buf, "%s", big[tu / 10]);
		else if (tu < 20)
			sprintf(buf, "%s", small[tu]);
		else
			sprintf(buf, "%s %s", big[tu / 10], small[tu % 10]);
	}

	return buf;
}	/* num_word() */
