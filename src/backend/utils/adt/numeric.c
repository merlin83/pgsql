/* ----------
 * numeric.c
 *
 *	An exact numeric data type for the Postgres database system
 *
 *	1998 Jan Wieck
 *
 * $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/adt/numeric.c,v 1.26 2000-03-13 02:31:13 tgl Exp $
 *
 * ----------
 */

#include "postgres.h"

#include <ctype.h>
#include <float.h>
#include <math.h>
#include <errno.h>
#include <sys/types.h>

#include "utils/builtins.h"
#include "utils/numeric.h"

/* ----------
 * Uncomment the following to enable compilation of dump_numeric()
 * and dump_var() and to get a dump of any result produced by make_result().
 * ----------
#define NUMERIC_DEBUG
 */


/* ----------
 * Local definitions
 * ----------
 */
#ifndef MIN
#define MIN(a,b) (((a)<(b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) (((a)>(b)) ? (a) : (b))
#endif

#ifndef NAN
#define NAN		(0.0/0.0)
#endif


/* ----------
 * Local data types
 *
 * Note: the first digit of a NumericVar's value is assumed to be multiplied
 * by 10 ** weight.  Another way to say it is that there are weight+1 digits
 * before the decimal point.  It is possible to have weight < 0.
 *
 * The value represented by a NumericVar is determined by the sign, weight,
 * ndigits, and digits[] array.  The rscale and dscale are carried along,
 * but they are just auxiliary information until rounding is done before
 * final storage or display.  (Scales are the number of digits wanted
 * *after* the decimal point.  Scales are always >= 0.)
 *
 * buf points at the physical start of the palloc'd digit buffer for the
 * NumericVar.  digits points at the first digit in actual use (the one
 * with the specified weight).  We normally leave an unused byte or two
 * (preset to zeroes) between buf and digits, so that there is room to store
 * a carry out of the top digit without special pushups.  We just need to
 * decrement digits (and increment weight) to make room for the carry digit.
 *
 * If buf is NULL then the digit buffer isn't actually palloc'd and should
 * not be freed --- see the constants below for an example.
 *
 * NB: All the variable-level functions are written in a style that makes it
 * possible to give one and the same variable as argument and destination.
 * This is feasible because the digit buffer is separate from the variable.
 * ----------
 */
typedef unsigned char NumericDigit;

typedef struct NumericVar
{
	int			ndigits;		/* number of digits in digits[] - can be 0! */
	int			weight;			/* weight of first digit */
	int			rscale;			/* result scale */
	int			dscale;			/* display scale */
	int			sign;			/* NUMERIC_POS, NUMERIC_NEG, or NUMERIC_NAN */
	NumericDigit *buf;			/* start of palloc'd space for digits[] */
	NumericDigit *digits;		/* decimal digits */
} NumericVar;


/* ----------
 * Local data
 * ----------
 */
static int	global_rscale = NUMERIC_MIN_RESULT_SCALE;

/* ----------
 * Some preinitialized variables we need often
 * ----------
 */
static NumericDigit const_zero_data[1] = {0};
static NumericVar const_zero =
{0, 0, 0, 0, NUMERIC_POS, NULL, const_zero_data};

static NumericDigit const_one_data[1] = {1};
static NumericVar const_one =
{1, 0, 0, 0, NUMERIC_POS, NULL, const_one_data};

static NumericDigit const_two_data[1] = {2};
static NumericVar const_two =
{1, 0, 0, 0, NUMERIC_POS, NULL, const_two_data};

static NumericVar const_nan =
{0, 0, 0, 0, NUMERIC_NAN, NULL, NULL};



/* ----------
 * Local functions
 * ----------
 */

#ifdef NUMERIC_DEBUG
static void dump_numeric(char *str, Numeric num);
static void dump_var(char *str, NumericVar *var);
#else
#define dump_numeric(s,n)
#define dump_var(s,v)
#endif

#define digitbuf_alloc(size)  ((NumericDigit *) palloc(size))
#define digitbuf_free(buf)  \
	do { \
		 if ((buf) != NULL) \
			 pfree(buf); \
	} while (0)

#define init_var(v)		memset(v,0,sizeof(NumericVar))
static void alloc_var(NumericVar *var, int ndigits);
static void free_var(NumericVar *var);
static void zero_var(NumericVar *var);

static void set_var_from_str(char *str, NumericVar *dest);
static void set_var_from_num(Numeric value, NumericVar *dest);
static void set_var_from_var(NumericVar *value, NumericVar *dest);
static char *get_str_from_var(NumericVar *var, int dscale);

static Numeric make_result(NumericVar *var);

static void apply_typmod(NumericVar *var, int32 typmod);

static int	cmp_var(NumericVar *var1, NumericVar *var2);
static void add_var(NumericVar *var1, NumericVar *var2, NumericVar *result);
static void sub_var(NumericVar *var1, NumericVar *var2, NumericVar *result);
static void mul_var(NumericVar *var1, NumericVar *var2, NumericVar *result);
static void div_var(NumericVar *var1, NumericVar *var2, NumericVar *result);
static void mod_var(NumericVar *var1, NumericVar *var2, NumericVar *result);
static void ceil_var(NumericVar *var, NumericVar *result);
static void floor_var(NumericVar *var, NumericVar *result);

static void sqrt_var(NumericVar *arg, NumericVar *result);
static void exp_var(NumericVar *arg, NumericVar *result);
static void ln_var(NumericVar *arg, NumericVar *result);
static void log_var(NumericVar *base, NumericVar *num, NumericVar *result);
static void power_var(NumericVar *base, NumericVar *exp, NumericVar *result);

static int	cmp_abs(NumericVar *var1, NumericVar *var2);
static void add_abs(NumericVar *var1, NumericVar *var2, NumericVar *result);
static void sub_abs(NumericVar *var1, NumericVar *var2, NumericVar *result);



/* ----------------------------------------------------------------------
 *
 * Input-, output- and rounding-functions
 *
 * ----------------------------------------------------------------------
 */


/* ----------
 * numeric_in() -
 *
 *	Input function for numeric data type
 * ----------
 */
Numeric
numeric_in(char *str, int dummy, int32 typmod)
{
	NumericVar	value;
	Numeric		res;

	/* ----------
	 * Check for NULL
	 * ----------
	 */
	if (str == NULL)
		return NULL;

	if (strcmp(str, "NULL") == 0)
		return NULL;

	/* ----------
	 * Check for NaN
	 * ----------
	 */
	if (strcmp(str, "NaN") == 0)
		return make_result(&const_nan);

	/* ----------
	 * Use set_var_from_str() to parse the input string
	 * and return it in the packed DB storage format
	 * ----------
	 */
	init_var(&value);
	set_var_from_str(str, &value);

	apply_typmod(&value, typmod);

	res = make_result(&value);
	free_var(&value);

	return res;
}


/* ----------
 * numeric_out() -
 *
 *	Output function for numeric data type
 * ----------
 */
char *
numeric_out(Numeric num)
{
	NumericVar	x;
	char	   *str;

	/* ----------
	 * Handle NULL
	 * ----------
	 */
	if (num == NULL)
		return pstrdup("NULL");

	/* ----------
	 * Handle NaN
	 * ----------
	 */
	if (NUMERIC_IS_NAN(num))
		return pstrdup("NaN");

	/* ----------
	 * Get the number in the variable format.
	 *
	 * Even if we didn't need to change format, we'd still need to copy
	 * the value to have a modifiable copy for rounding.  set_var_from_num()
	 * also guarantees there is extra digit space in case we produce a
	 * carry out from rounding.
	 * ----------
	 */
	init_var(&x);
	set_var_from_num(num, &x);

	str = get_str_from_var(&x, x.dscale);

	free_var(&x);

	return str;
}


/* ----------
 * numeric() -
 *
 *	This is a special function called by the Postgres database system
 *	before a value is stored in a tuples attribute. The precision and
 *	scale of the attribute have to be applied on the value.
 * ----------
 */
Numeric
numeric(Numeric num, int32 typmod)
{
	Numeric		new;
	int32		tmp_typmod;
	int			precision;
	int			scale;
	int			maxweight;
	NumericVar	var;

	/* ----------
	 * Handle NULL
	 * ----------
	 */
	if (num == NULL)
		return NULL;

	/* ----------
	 * Handle NaN
	 * ----------
	 */
	if (NUMERIC_IS_NAN(num))
		return make_result(&const_nan);

	/* ----------
	 * If the value isn't a valid type modifier, simply return a
	 * copy of the input value
	 * ----------
	 */
	if (typmod < (int32) (VARHDRSZ))
	{
		new = (Numeric) palloc(num->varlen);
		memcpy(new, num, num->varlen);
		return new;
	}

	/* ----------
	 * Get the precision and scale out of the typmod value
	 * ----------
	 */
	tmp_typmod = typmod - VARHDRSZ;
	precision = (tmp_typmod >> 16) & 0xffff;
	scale = tmp_typmod & 0xffff;
	maxweight = precision - scale;

	/* ----------
	 * If the number is in bounds and due to the present result scale
	 * no rounding could be necessary, just make a copy of the input
	 * and modify its scale fields.
	 * ----------
	 */
	if (num->n_weight < maxweight && scale >= num->n_rscale)
	{
		new = (Numeric) palloc(num->varlen);
		memcpy(new, num, num->varlen);
		new->n_rscale = scale;
		new->n_sign_dscale = NUMERIC_SIGN(new) |
			((uint16) scale & NUMERIC_DSCALE_MASK);
		return new;
	}

	/* ----------
	 * We really need to fiddle with things - unpack the number into
	 * a variable and let apply_typmod() do it.
	 * ----------
	 */
	init_var(&var);

	set_var_from_num(num, &var);
	apply_typmod(&var, typmod);
	new = make_result(&var);

	free_var(&var);

	return new;
}


/* ----------------------------------------------------------------------
 *
 * Sign manipulation, rounding and the like
 *
 * ----------------------------------------------------------------------
 */


Numeric
numeric_abs(Numeric num)
{
	Numeric		res;

	/* ----------
	 * Handle NULL
	 * ----------
	 */
	if (num == NULL)
		return NULL;

	/* ----------
	 * Handle NaN
	 * ----------
	 */
	if (NUMERIC_IS_NAN(num))
		return make_result(&const_nan);

	/* ----------
	 * Do it the easy way directly on the packed format
	 * ----------
	 */
	res = (Numeric) palloc(num->varlen);
	memcpy(res, num, num->varlen);

	res->n_sign_dscale = NUMERIC_POS | NUMERIC_DSCALE(num);

	return res;
}


Numeric
numeric_uminus(Numeric num)
{
	Numeric		res;

	/* ----------
	 * Handle NULL
	 * ----------
	 */
	if (num == NULL)
		return NULL;

	/* ----------
	 * Handle NaN
	 * ----------
	 */
	if (NUMERIC_IS_NAN(num))
		return make_result(&const_nan);

	/* ----------
	 * Do it the easy way directly on the packed format
	 * ----------
	 */
	res = (Numeric) palloc(num->varlen);
	memcpy(res, num, num->varlen);

	/* ----------
	 * The packed format is known to be totally zero digit trimmed
	 * always. So we can identify a ZERO by the fact that there
	 * are no digits at all.  Do nothing to a zero.
	 * ----------
	 */
	if (num->varlen != NUMERIC_HDRSZ)
	{
		/* Else, flip the sign */
		if (NUMERIC_SIGN(num) == NUMERIC_POS)
			res->n_sign_dscale = NUMERIC_NEG | NUMERIC_DSCALE(num);
		else
			res->n_sign_dscale = NUMERIC_POS | NUMERIC_DSCALE(num);
	}

	return res;
}


Numeric
numeric_sign(Numeric num)
{
	Numeric		res;
	NumericVar	result;

	/* ----------
	 * Handle NULL
	 * ----------
	 */
	if (num == NULL)
		return NULL;

	/* ----------
	 * Handle NaN
	 * ----------
	 */
	if (NUMERIC_IS_NAN(num))
		return make_result(&const_nan);

	init_var(&result);

	/* ----------
	 * The packed format is known to be totally zero digit trimmed
	 * always. So we can identify a ZERO by the fact that there
	 * are no digits at all.
	 * ----------
	 */
	if (num->varlen == NUMERIC_HDRSZ)
		set_var_from_var(&const_zero, &result);
	else
	{
		/* ----------
		 * And if there are some, we return a copy of ONE
		 * with the sign of our argument
		 * ----------
		 */
		set_var_from_var(&const_one, &result);
		result.sign = NUMERIC_SIGN(num);
	}

	res = make_result(&result);
	free_var(&result);

	return res;
}


/* ----------
 * numeric_round() -
 *
 *	Round a value to have 'scale' digits after the decimal point.
 *	We allow negative 'scale', implying rounding before the decimal
 *	point --- Oracle interprets rounding that way.
 * ----------
 */
Numeric
numeric_round(Numeric num, int32 scale)
{
	Numeric		res;
	NumericVar	arg;
	int			i;

	/* ----------
	 * Handle NULL
	 * ----------
	 */
	if (num == NULL)
		return NULL;

	/* ----------
	 * Handle NaN
	 * ----------
	 */
	if (NUMERIC_IS_NAN(num))
		return make_result(&const_nan);

	/* ----------
	 * Limit the scale value to avoid possible overflow in calculations below.
	 * ----------
	 */
	scale = MIN(NUMERIC_MAX_RESULT_SCALE,
				MAX(-NUMERIC_MAX_RESULT_SCALE, scale));

	/* ----------
	 * Unpack the argument and round it at the proper digit position
	 * ----------
	 */
	init_var(&arg);
	set_var_from_num(num, &arg);

	i = arg.weight + scale + 1;

	if (i < arg.ndigits)
	{
		/* If i = 0, the value loses all digits, but could round up if its
		 * first digit is more than 4.  If i < 0 the result must be 0.
		 */
		if (i < 0)
		{
			arg.ndigits = 0;
		}
		else
		{
			int		carry = (arg.digits[i] > 4) ? 1 : 0;

			arg.ndigits = i;

			while (carry)
			{
				carry += arg.digits[--i];
				arg.digits[i] = carry % 10;
				carry /= 10;
			}

			if (i < 0)
			{
				Assert(i == -1); /* better not have added more than 1 digit */
				Assert(arg.digits > arg.buf);
				arg.digits--;
				arg.ndigits++;
				arg.weight++;
			}
		}
	}

	/* ----------
	 * Set result's scale to something reasonable.
	 * ----------
	 */
	scale = MIN(NUMERIC_MAX_DISPLAY_SCALE, MAX(0, scale));
	arg.rscale = scale;
	arg.dscale = scale;

	/* ----------
	 * Return the rounded result
	 * ----------
	 */
	res = make_result(&arg);

	free_var(&arg);
	return res;
}


/* ----------
 * numeric_trunc() -
 *
 *	Truncate a value to have 'scale' digits after the decimal point.
 *	We allow negative 'scale', implying a truncation before the decimal
 *	point --- Oracle interprets truncation that way.
 * ----------
 */
Numeric
numeric_trunc(Numeric num, int32 scale)
{
	Numeric		res;
	NumericVar	arg;

	/* ----------
	 * Handle NULL
	 * ----------
	 */
	if (num == NULL)
		return NULL;

	/* ----------
	 * Handle NaN
	 * ----------
	 */
	if (NUMERIC_IS_NAN(num))
		return make_result(&const_nan);

	/* ----------
	 * Limit the scale value to avoid possible overflow in calculations below.
	 * ----------
	 */
	scale = MIN(NUMERIC_MAX_RESULT_SCALE,
				MAX(-NUMERIC_MAX_RESULT_SCALE, scale));

	/* ----------
	 * Unpack the argument and truncate it at the proper digit position
	 * ----------
	 */
	init_var(&arg);
	set_var_from_num(num, &arg);

	arg.ndigits = MIN(arg.ndigits, MAX(0, arg.weight + scale + 1));

	/* ----------
	 * Set result's scale to something reasonable.
	 * ----------
	 */
	scale = MIN(NUMERIC_MAX_DISPLAY_SCALE, MAX(0, scale));
	arg.rscale = scale;
	arg.dscale = scale;

	/* ----------
	 * Return the truncated result
	 * ----------
	 */
	res = make_result(&arg);

	free_var(&arg);
	return res;
}


/* ----------
 * numeric_ceil() -
 *
 *	Return the smallest integer greater than or equal to the argument
 * ----------
 */
Numeric
numeric_ceil(Numeric num)
{
	Numeric		res;
	NumericVar	result;

	if (num == NULL)
		return NULL;

	if (NUMERIC_IS_NAN(num))
		return make_result(&const_nan);

	init_var(&result);

	set_var_from_num(num, &result);
	ceil_var(&result, &result);

	result.dscale = 0;

	res = make_result(&result);
	free_var(&result);

	return res;
}


/* ----------
 * numeric_floor() -
 *
 *	Return the largest integer equal to or less than the argument
 * ----------
 */
Numeric
numeric_floor(Numeric num)
{
	Numeric		res;
	NumericVar	result;

	if (num == NULL)
		return NULL;

	if (NUMERIC_IS_NAN(num))
		return make_result(&const_nan);

	init_var(&result);

	set_var_from_num(num, &result);
	floor_var(&result, &result);

	result.dscale = 0;

	res = make_result(&result);
	free_var(&result);

	return res;
}


/* ----------------------------------------------------------------------
 *
 * Comparison functions
 *
 * ----------------------------------------------------------------------
 */


int32
numeric_cmp(Numeric num1, Numeric num2)
{
	int			result;
	NumericVar	arg1;
	NumericVar	arg2;

	if (num1 == NULL || num2 == NULL)
		return (int32)0;

	if (NUMERIC_IS_NAN(num1) || NUMERIC_IS_NAN(num2))
		return (int32)0;

	init_var(&arg1);
	init_var(&arg2);

	set_var_from_num(num1, &arg1);
	set_var_from_num(num2, &arg2);

	result = cmp_var(&arg1, &arg2);

	free_var(&arg1);
	free_var(&arg2);

	return (int32)((result == 0) ? 0 : ((result < 0) ? -1 : 1));
}


bool
numeric_eq(Numeric num1, Numeric num2)
{
	int			result;
	NumericVar	arg1;
	NumericVar	arg2;

	if (num1 == NULL || num2 == NULL)
		return FALSE;

	if (NUMERIC_IS_NAN(num1) || NUMERIC_IS_NAN(num2))
		return FALSE;

	init_var(&arg1);
	init_var(&arg2);

	set_var_from_num(num1, &arg1);
	set_var_from_num(num2, &arg2);

	result = cmp_var(&arg1, &arg2);

	free_var(&arg1);
	free_var(&arg2);

	return (result == 0);
}


bool
numeric_ne(Numeric num1, Numeric num2)
{
	int			result;
	NumericVar	arg1;
	NumericVar	arg2;

	if (num1 == NULL || num2 == NULL)
		return FALSE;

	if (NUMERIC_IS_NAN(num1) || NUMERIC_IS_NAN(num2))
		return FALSE;

	init_var(&arg1);
	init_var(&arg2);

	set_var_from_num(num1, &arg1);
	set_var_from_num(num2, &arg2);

	result = cmp_var(&arg1, &arg2);

	free_var(&arg1);
	free_var(&arg2);

	return (result != 0);
}


bool
numeric_gt(Numeric num1, Numeric num2)
{
	int			result;
	NumericVar	arg1;
	NumericVar	arg2;

	if (num1 == NULL || num2 == NULL)
		return FALSE;

	if (NUMERIC_IS_NAN(num1) || NUMERIC_IS_NAN(num2))
		return FALSE;

	init_var(&arg1);
	init_var(&arg2);

	set_var_from_num(num1, &arg1);
	set_var_from_num(num2, &arg2);

	result = cmp_var(&arg1, &arg2);

	free_var(&arg1);
	free_var(&arg2);

	return (result > 0);
}


bool
numeric_ge(Numeric num1, Numeric num2)
{
	int			result;
	NumericVar	arg1;
	NumericVar	arg2;

	if (num1 == NULL || num2 == NULL)
		return FALSE;

	if (NUMERIC_IS_NAN(num1) || NUMERIC_IS_NAN(num2))
		return FALSE;

	init_var(&arg1);
	init_var(&arg2);

	set_var_from_num(num1, &arg1);
	set_var_from_num(num2, &arg2);

	result = cmp_var(&arg1, &arg2);

	free_var(&arg1);
	free_var(&arg2);

	return (result >= 0);
}


bool
numeric_lt(Numeric num1, Numeric num2)
{
	int			result;
	NumericVar	arg1;
	NumericVar	arg2;

	if (num1 == NULL || num2 == NULL)
		return FALSE;

	if (NUMERIC_IS_NAN(num1) || NUMERIC_IS_NAN(num2))
		return FALSE;

	init_var(&arg1);
	init_var(&arg2);

	set_var_from_num(num1, &arg1);
	set_var_from_num(num2, &arg2);

	result = cmp_var(&arg1, &arg2);

	free_var(&arg1);
	free_var(&arg2);

	return (result < 0);
}


bool
numeric_le(Numeric num1, Numeric num2)
{
	int			result;
	NumericVar	arg1;
	NumericVar	arg2;

	if (num1 == NULL || num2 == NULL)
		return FALSE;

	if (NUMERIC_IS_NAN(num1) || NUMERIC_IS_NAN(num2))
		return FALSE;

	init_var(&arg1);
	init_var(&arg2);

	set_var_from_num(num1, &arg1);
	set_var_from_num(num2, &arg2);

	result = cmp_var(&arg1, &arg2);

	free_var(&arg1);
	free_var(&arg2);

	return (result <= 0);
}


/* ----------------------------------------------------------------------
 *
 * Arithmetic base functions
 *
 * ----------------------------------------------------------------------
 */


/* ----------
 * numeric_add() -
 *
 *	Add two numerics
 * ----------
 */
Numeric
numeric_add(Numeric num1, Numeric num2)
{
	NumericVar	arg1;
	NumericVar	arg2;
	NumericVar	result;
	Numeric		res;

	/* ----------
	 * Handle NULL
	 * ----------
	 */
	if (num1 == NULL || num2 == NULL)
		return NULL;

	/* ----------
	 * Handle NaN
	 * ----------
	 */
	if (NUMERIC_IS_NAN(num1) || NUMERIC_IS_NAN(num2))
		return make_result(&const_nan);

	/* ----------
	 * Unpack the values, let add_var() compute the result
	 * and return it. The internals of add_var() will automatically
	 * set the correct result and display scales in the result.
	 * ----------
	 */
	init_var(&arg1);
	init_var(&arg2);
	init_var(&result);

	set_var_from_num(num1, &arg1);
	set_var_from_num(num2, &arg2);

	add_var(&arg1, &arg2, &result);
	res = make_result(&result);

	free_var(&arg1);
	free_var(&arg2);
	free_var(&result);

	return res;
}


/* ----------
 * numeric_sub() -
 *
 *	Subtract one numeric from another
 * ----------
 */
Numeric
numeric_sub(Numeric num1, Numeric num2)
{
	NumericVar	arg1;
	NumericVar	arg2;
	NumericVar	result;
	Numeric		res;

	/* ----------
	 * Handle NULL
	 * ----------
	 */
	if (num1 == NULL || num2 == NULL)
		return NULL;

	/* ----------
	 * Handle NaN
	 * ----------
	 */
	if (NUMERIC_IS_NAN(num1) || NUMERIC_IS_NAN(num2))
		return make_result(&const_nan);

	/* ----------
	 * Unpack the two arguments, let sub_var() compute the
	 * result and return it.
	 * ----------
	 */
	init_var(&arg1);
	init_var(&arg2);
	init_var(&result);

	set_var_from_num(num1, &arg1);
	set_var_from_num(num2, &arg2);

	sub_var(&arg1, &arg2, &result);
	res = make_result(&result);

	free_var(&arg1);
	free_var(&arg2);
	free_var(&result);

	return res;
}


/* ----------
 * numeric_mul() -
 *
 *	Calculate the product of two numerics
 * ----------
 */
Numeric
numeric_mul(Numeric num1, Numeric num2)
{
	NumericVar	arg1;
	NumericVar	arg2;
	NumericVar	result;
	Numeric		res;

	/* ----------
	 * Handle NULL
	 * ----------
	 */
	if (num1 == NULL || num2 == NULL)
		return NULL;

	/* ----------
	 * Handle NaN
	 * ----------
	 */
	if (NUMERIC_IS_NAN(num1) || NUMERIC_IS_NAN(num2))
		return make_result(&const_nan);

	/* ----------
	 * Unpack the arguments, let mul_var() compute the result
	 * and return it. Unlike add_var() and sub_var(), mul_var()
	 * will round the result to the scale stored in global_rscale.
	 * In the case of numeric_mul(), which is invoked for the *
	 * operator on numerics, we set it to the exact representation
	 * for the product (rscale = sum(rscale of arg1, rscale of arg2)
	 * and the same for the dscale).
	 * ----------
	 */
	init_var(&arg1);
	init_var(&arg2);
	init_var(&result);

	set_var_from_num(num1, &arg1);
	set_var_from_num(num2, &arg2);

	global_rscale = arg1.rscale + arg2.rscale;

	mul_var(&arg1, &arg2, &result);

	result.dscale = arg1.dscale + arg2.dscale;

	res = make_result(&result);

	free_var(&arg1);
	free_var(&arg2);
	free_var(&result);

	return res;
}


/* ----------
 * numeric_div() -
 *
 *	Divide one numeric into another
 * ----------
 */
Numeric
numeric_div(Numeric num1, Numeric num2)
{
	NumericVar	arg1;
	NumericVar	arg2;
	NumericVar	result;
	Numeric		res;
	int			res_dscale;

	/* ----------
	 * Handle NULL
	 * ----------
	 */
	if (num1 == NULL || num2 == NULL)
		return NULL;

	/* ----------
	 * Handle NaN
	 * ----------
	 */
	if (NUMERIC_IS_NAN(num1) || NUMERIC_IS_NAN(num2))
		return make_result(&const_nan);

	/* ----------
	 * Unpack the arguments
	 * ----------
	 */
	init_var(&arg1);
	init_var(&arg2);
	init_var(&result);

	set_var_from_num(num1, &arg1);
	set_var_from_num(num2, &arg2);

	/* ----------
	 * The result scale of a division isn't specified in any
	 * SQL standard. For Postgres it is the following (where
	 * SR, DR are the result- and display-scales of the returned
	 * value, S1, D1, S2 and D2 are the scales of the two arguments,
	 * The minimum and maximum scales are compile time options from
	 * numeric.h):
	 *
	 *	DR = MIN(MAX(D1 + D2, MIN_DISPLAY_SCALE), MAX_DISPLAY_SCALE)
	 *	SR = MIN(MAX(MAX(S1 + S2, MIN_RESULT_SCALE), DR + 4), MAX_RESULT_SCALE)
	 *
	 * By default, any result is computed with a minimum of 34 digits
	 * after the decimal point or at least with 4 digits more than
	 * displayed.
	 * ----------
	 */
	res_dscale = MAX(arg1.dscale + arg2.dscale, NUMERIC_MIN_DISPLAY_SCALE);
	res_dscale = MIN(res_dscale, NUMERIC_MAX_DISPLAY_SCALE);
	global_rscale = MAX(arg1.rscale + arg2.rscale,
						NUMERIC_MIN_RESULT_SCALE);
	global_rscale = MAX(global_rscale, res_dscale + 4);
	global_rscale = MIN(global_rscale, NUMERIC_MAX_RESULT_SCALE);

	/* ----------
	 * Do the divide, set the display scale and return the result
	 * ----------
	 */
	div_var(&arg1, &arg2, &result);

	result.dscale = res_dscale;

	res = make_result(&result);

	free_var(&arg1);
	free_var(&arg2);
	free_var(&result);

	return res;
}


/* ----------
 * numeric_mod() -
 *
 *	Calculate the modulo of two numerics
 * ----------
 */
Numeric
numeric_mod(Numeric num1, Numeric num2)
{
	Numeric		res;
	NumericVar	arg1;
	NumericVar	arg2;
	NumericVar	result;

	if (num1 == NULL || num2 == NULL)
		return NULL;

	if (NUMERIC_IS_NAN(num1) || NUMERIC_IS_NAN(num2))
		return make_result(&const_nan);

	init_var(&arg1);
	init_var(&arg2);
	init_var(&result);

	set_var_from_num(num1, &arg1);
	set_var_from_num(num2, &arg2);

	mod_var(&arg1, &arg2, &result);

	result.dscale = result.rscale;
	res = make_result(&result);

	free_var(&result);
	free_var(&arg2);
	free_var(&arg1);

	return res;
}


/* ----------
 * numeric_inc() -
 *
 *	Increment a number by one
 * ----------
 */
Numeric
numeric_inc(Numeric num)
{
	NumericVar	arg;
	Numeric		res;

	/* ----------
	 * Handle NULL
	 * ----------
	 */
	if (num == NULL)
		return NULL;

	/* ----------
	 * Handle NaN
	 * ----------
	 */
	if (NUMERIC_IS_NAN(num))
		return make_result(&const_nan);

	/* ----------
	 * Compute the result and return it
	 * ----------
	 */
	init_var(&arg);

	set_var_from_num(num, &arg);

	add_var(&arg, &const_one, &arg);
	res = make_result(&arg);

	free_var(&arg);

	return res;
}


/* ----------
 * numeric_dec() -
 *
 *	Decrement a number by one
 * ----------
 */
Numeric
numeric_dec(Numeric num)
{
	NumericVar	arg;
	Numeric		res;

	/* ----------
	 * Handle NULL
	 * ----------
	 */
	if (num == NULL)
		return NULL;

	/* ----------
	 * Handle NaN
	 * ----------
	 */
	if (NUMERIC_IS_NAN(num))
		return make_result(&const_nan);

	/* ----------
	 * Compute the result and return it
	 * ----------
	 */
	init_var(&arg);

	set_var_from_num(num, &arg);

	sub_var(&arg, &const_one, &arg);
	res = make_result(&arg);

	free_var(&arg);

	return res;
}


/* ----------
 * numeric_smaller() -
 *
 *	Return the smaller of two numbers
 * ----------
 */
Numeric
numeric_smaller(Numeric num1, Numeric num2)
{
	NumericVar	arg1;
	NumericVar	arg2;
	Numeric		res;

	/* ----------
	 * Handle NULL
	 * ----------
	 */
	if (num1 == NULL || num2 == NULL)
		return NULL;

	/* ----------
	 * Handle NaN
	 * ----------
	 */
	if (NUMERIC_IS_NAN(num1) || NUMERIC_IS_NAN(num2))
		return make_result(&const_nan);

	/* ----------
	 * Unpack the values, and decide which is the smaller one
	 * ----------
	 */
	init_var(&arg1);
	init_var(&arg2);

	set_var_from_num(num1, &arg1);
	set_var_from_num(num2, &arg2);

	if (cmp_var(&arg1, &arg2) <= 0)
		res = make_result(&arg1);
	else
		res = make_result(&arg2);

	free_var(&arg1);
	free_var(&arg2);

	return res;
}


/* ----------
 * numeric_larger() -
 *
 *	Return the larger of two numbers
 * ----------
 */
Numeric
numeric_larger(Numeric num1, Numeric num2)
{
	NumericVar	arg1;
	NumericVar	arg2;
	Numeric		res;

	/* ----------
	 * Handle NULL
	 * ----------
	 */
	if (num1 == NULL || num2 == NULL)
		return NULL;

	/* ----------
	 * Handle NaN
	 * ----------
	 */
	if (NUMERIC_IS_NAN(num1) || NUMERIC_IS_NAN(num2))
		return make_result(&const_nan);

	/* ----------
	 * Unpack the values, and decide which is the larger one
	 * ----------
	 */
	init_var(&arg1);
	init_var(&arg2);

	set_var_from_num(num1, &arg1);
	set_var_from_num(num2, &arg2);

	if (cmp_var(&arg1, &arg2) >= 0)
		res = make_result(&arg1);
	else
		res = make_result(&arg2);

	free_var(&arg1);
	free_var(&arg2);

	return res;
}


/* ----------------------------------------------------------------------
 *
 * Complex math functions
 *
 * ----------------------------------------------------------------------
 */


/* ----------
 * numeric_sqrt() -
 *
 *	Compute the square root of a numeric.
 * ----------
 */
Numeric
numeric_sqrt(Numeric num)
{
	Numeric		res;
	NumericVar	arg;
	NumericVar	result;
	int			res_dscale;

	/* ----------
	 * Handle NULL
	 * ----------
	 */
	if (num == NULL)
		return NULL;

	/* ----------
	 * Handle NaN
	 * ----------
	 */
	if (NUMERIC_IS_NAN(num))
		return make_result(&const_nan);

	/* ----------
	 * Unpack the argument, determine the scales like for divide,
	 * let sqrt_var() do the calculation and return the result.
	 * ----------
	 */
	init_var(&arg);
	init_var(&result);

	set_var_from_num(num, &arg);

	res_dscale = MAX(arg.dscale, NUMERIC_MIN_DISPLAY_SCALE);
	res_dscale = MIN(res_dscale, NUMERIC_MAX_DISPLAY_SCALE);
	global_rscale = MAX(arg.rscale, NUMERIC_MIN_RESULT_SCALE);
	global_rscale = MAX(global_rscale, res_dscale + 4);
	global_rscale = MIN(global_rscale, NUMERIC_MAX_RESULT_SCALE);

	sqrt_var(&arg, &result);

	result.dscale = res_dscale;

	res = make_result(&result);

	free_var(&result);
	free_var(&arg);

	return res;
}


/* ----------
 * numeric_exp() -
 *
 *	Raise e to the power of x
 * ----------
 */
Numeric
numeric_exp(Numeric num)
{
	Numeric		res;
	NumericVar	arg;
	NumericVar	result;
	int			res_dscale;

	/* ----------
	 * Handle NULL
	 * ----------
	 */
	if (num == NULL)
		return NULL;

	/* ----------
	 * Handle NaN
	 * ----------
	 */
	if (NUMERIC_IS_NAN(num))
		return make_result(&const_nan);

	/* ----------
	 * Same procedure like for sqrt().
	 * ----------
	 */
	init_var(&arg);
	init_var(&result);
	set_var_from_num(num, &arg);

	res_dscale = MAX(arg.dscale, NUMERIC_MIN_DISPLAY_SCALE);
	res_dscale = MIN(res_dscale, NUMERIC_MAX_DISPLAY_SCALE);
	global_rscale = MAX(arg.rscale, NUMERIC_MIN_RESULT_SCALE);
	global_rscale = MAX(global_rscale, res_dscale + 4);
	global_rscale = MIN(global_rscale, NUMERIC_MAX_RESULT_SCALE);

	exp_var(&arg, &result);

	result.dscale = res_dscale;

	res = make_result(&result);

	free_var(&result);
	free_var(&arg);

	return res;
}


/* ----------
 * numeric_ln() -
 *
 *	Compute the natural logarithm of x
 * ----------
 */
Numeric
numeric_ln(Numeric num)
{
	Numeric		res;
	NumericVar	arg;
	NumericVar	result;
	int			res_dscale;

	/* ----------
	 * Handle NULL
	 * ----------
	 */
	if (num == NULL)
		return NULL;

	/* ----------
	 * Handle NaN
	 * ----------
	 */
	if (NUMERIC_IS_NAN(num))
		return make_result(&const_nan);

	/* ----------
	 * Same procedure like for sqrt()
	 * ----------
	 */
	init_var(&arg);
	init_var(&result);
	set_var_from_num(num, &arg);

	res_dscale = MAX(arg.dscale, NUMERIC_MIN_DISPLAY_SCALE);
	res_dscale = MIN(res_dscale, NUMERIC_MAX_DISPLAY_SCALE);
	global_rscale = MAX(arg.rscale, NUMERIC_MIN_RESULT_SCALE);
	global_rscale = MAX(global_rscale, res_dscale + 4);
	global_rscale = MIN(global_rscale, NUMERIC_MAX_RESULT_SCALE);

	ln_var(&arg, &result);

	result.dscale = res_dscale;

	res = make_result(&result);

	free_var(&result);
	free_var(&arg);

	return res;
}


/* ----------
 * numeric_log() -
 *
 *	Compute the logarithm of x in a given base
 * ----------
 */
Numeric
numeric_log(Numeric num1, Numeric num2)
{
	Numeric		res;
	NumericVar	arg1;
	NumericVar	arg2;
	NumericVar	result;
	int			res_dscale;

	/* ----------
	 * Handle NULL
	 * ----------
	 */
	if (num1 == NULL || num2 == NULL)
		return NULL;

	/* ----------
	 * Handle NaN
	 * ----------
	 */
	if (NUMERIC_IS_NAN(num1) || NUMERIC_IS_NAN(num2))
		return make_result(&const_nan);

	/* ----------
	 * Initialize things and calculate scales
	 * ----------
	 */
	init_var(&arg1);
	init_var(&arg2);
	init_var(&result);
	set_var_from_num(num1, &arg1);
	set_var_from_num(num2, &arg2);

	res_dscale = MAX(arg1.dscale + arg2.dscale, NUMERIC_MIN_DISPLAY_SCALE);
	res_dscale = MIN(res_dscale, NUMERIC_MAX_DISPLAY_SCALE);
	global_rscale = MAX(arg1.rscale + arg2.rscale, NUMERIC_MIN_RESULT_SCALE);
	global_rscale = MAX(global_rscale, res_dscale + 4);
	global_rscale = MIN(global_rscale, NUMERIC_MAX_RESULT_SCALE);

	/* ----------
	 * Call log_var() to compute and return the result
	 * ----------
	 */
	log_var(&arg1, &arg2, &result);

	result.dscale = res_dscale;

	res = make_result(&result);

	free_var(&result);
	free_var(&arg2);
	free_var(&arg1);

	return res;
}


/* ----------
 * numeric_power() -
 *
 *	Raise m to the power of x
 * ----------
 */
Numeric
numeric_power(Numeric num1, Numeric num2)
{
	Numeric		res;
	NumericVar	arg1;
	NumericVar	arg2;
	NumericVar	result;
	int			res_dscale;

	/* ----------
	 * Handle NULL
	 * ----------
	 */
	if (num1 == NULL || num2 == NULL)
		return NULL;

	/* ----------
	 * Handle NaN
	 * ----------
	 */
	if (NUMERIC_IS_NAN(num1) || NUMERIC_IS_NAN(num2))
		return make_result(&const_nan);

	/* ----------
	 * Initialize things and calculate scales
	 * ----------
	 */
	init_var(&arg1);
	init_var(&arg2);
	init_var(&result);
	set_var_from_num(num1, &arg1);
	set_var_from_num(num2, &arg2);

	res_dscale = MAX(arg1.dscale + arg2.dscale, NUMERIC_MIN_DISPLAY_SCALE);
	res_dscale = MIN(res_dscale, NUMERIC_MAX_DISPLAY_SCALE);
	global_rscale = MAX(arg1.rscale + arg2.rscale, NUMERIC_MIN_RESULT_SCALE);
	global_rscale = MAX(global_rscale, res_dscale + 4);
	global_rscale = MIN(global_rscale, NUMERIC_MAX_RESULT_SCALE);

	/* ----------
	 * Call log_var() to compute and return the result
	 * ----------
	 */
	power_var(&arg1, &arg2, &result);

	result.dscale = res_dscale;

	res = make_result(&result);

	free_var(&result);
	free_var(&arg2);
	free_var(&arg1);

	return res;
}


/* ----------------------------------------------------------------------
 *
 * Type conversion functions
 *
 * ----------------------------------------------------------------------
 */


Numeric
int4_numeric(int32 val)
{
	Numeric		res;
	NumericVar	result;
	char	   *tmp;

	init_var(&result);

	tmp = int4out(val);
	set_var_from_str(tmp, &result);
	res = make_result(&result);

	free_var(&result);
	pfree(tmp);

	return res;
}


int32
numeric_int4(Numeric num)
{
	NumericVar	x;
	char	   *str;
	int32		result;

	if (num == NULL)
		return 0;

	if (NUMERIC_IS_NAN(num))
		elog(ERROR, "Cannot convert NaN to int4");

	/* ----------
	 * Get the number in the variable format so we can round to integer.
	 * ----------
	 */
	init_var(&x);
	set_var_from_num(num, &x);

	str = get_str_from_var(&x, 0); /* dscale = 0 produces rounding */

	free_var(&x);

	result = int4in(str);
	pfree(str);

	return result;
}


Numeric
int8_numeric(int64 *val)
{
	Numeric		res;
	NumericVar	result;
	char	   *tmp;

	init_var(&result);

	tmp = int8out(val);
	set_var_from_str(tmp, &result);
	res = make_result(&result);

	free_var(&result);
	pfree(tmp);

	return res;
}


int64 *
numeric_int8(Numeric num)
{
	NumericVar	x;
	char	   *str;
	int64	   *result;

	if (num == NULL)
		return NULL;

	if (NUMERIC_IS_NAN(num))
		elog(ERROR, "Cannot convert NaN to int8");

	/* ----------
	 * Get the number in the variable format so we can round to integer.
	 * ----------
	 */
	init_var(&x);
	set_var_from_num(num, &x);

	str = get_str_from_var(&x, 0); /* dscale = 0 produces rounding */

	free_var(&x);

	result = int8in(str);
	pfree(str);

	return result;
}


Numeric
int2_numeric(int16 val)
{
	Numeric		res;
	NumericVar	result;
	char	   *tmp;

	init_var(&result);

	tmp = int2out(val);
	set_var_from_str(tmp, &result);
	res = make_result(&result);

	free_var(&result);
	pfree(tmp);

	return res;
}


int16
numeric_int2(Numeric num)
{
	NumericVar	x;
	char	   *str;
	int16		result;

	if (num == NULL)
		return 0;

	if (NUMERIC_IS_NAN(num))
		elog(ERROR, "Cannot convert NaN to int2");

	/* ----------
	 * Get the number in the variable format so we can round to integer.
	 * ----------
	 */
	init_var(&x);
	set_var_from_num(num, &x);

	str = get_str_from_var(&x, 0); /* dscale = 0 produces rounding */

	free_var(&x);

	result = int2in(str);
	pfree(str);

	return result;
}


Numeric
float8_numeric(float64 val)
{
	Numeric		res;
	NumericVar	result;
	char		buf[DBL_DIG + 100];

	if (val == NULL)
		return NULL;

	if (isnan(*val))
		return make_result(&const_nan);

	sprintf(buf, "%.*g", DBL_DIG, *val);

	init_var(&result);

	set_var_from_str(buf, &result);
	res = make_result(&result);

	free_var(&result);

	return res;
}


float64
numeric_float8(Numeric num)
{
	char	   *tmp;
	float64		result;

	if (num == NULL)
		return NULL;

	if (NUMERIC_IS_NAN(num))
	{
		result = (float64) palloc(sizeof(float64data));
		*result = NAN;
		return result;
	}

	tmp = numeric_out(num);
	result = float8in(tmp);
	pfree(tmp);

	return result;
}


Numeric
float4_numeric(float32 val)
{
	Numeric		res;
	NumericVar	result;
	char		buf[FLT_DIG + 100];

	if (val == NULL)
		return NULL;

	if (isnan(*val))
		return make_result(&const_nan);

	sprintf(buf, "%.*g", FLT_DIG, *val);

	init_var(&result);

	set_var_from_str(buf, &result);
	res = make_result(&result);

	free_var(&result);

	return res;
}


float32
numeric_float4(Numeric num)
{
	char	   *tmp;
	float32		result;

	if (num == NULL)
		return NULL;

	if (NUMERIC_IS_NAN(num))
	{
		result = (float32) palloc(sizeof(float32data));
		*result = NAN;
		return result;
	}

	tmp = numeric_out(num);
	result = float4in(tmp);
	pfree(tmp);

	return result;
}


/* ----------------------------------------------------------------------
 *
 * Local functions follow
 *
 * ----------------------------------------------------------------------
 */


#ifdef NUMERIC_DEBUG

/* ----------
 * dump_numeric() - Dump a value in the db storage format for debugging
 * ----------
 */
static void
dump_numeric(char *str, Numeric num)
{
	int			i;

	printf("%s: NUMERIC w=%d r=%d d=%d ", str, num->n_weight, num->n_rscale,
		   NUMERIC_DSCALE(num));
	switch (NUMERIC_SIGN(num))
	{
		case NUMERIC_POS:
			printf("POS");
			break;
		case NUMERIC_NEG:
			printf("NEG");
			break;
		case NUMERIC_NAN:
			printf("NaN");
			break;
		default:
			printf("SIGN=0x%x", NUMERIC_SIGN(num));
			break;
	}

	for (i = 0; i < num->varlen - NUMERIC_HDRSZ; i++)
		printf(" %d %d", (num->n_data[i] >> 4) & 0x0f, num->n_data[i] & 0x0f);
	printf("\n");
}


/* ----------
 * dump_var() - Dump a value in the variable format for debugging
 * ----------
 */
static void
dump_var(char *str, NumericVar *var)
{
	int			i;

	printf("%s: VAR w=%d r=%d d=%d ", str, var->weight, var->rscale,
		   var->dscale);
	switch (var->sign)
	{
		case NUMERIC_POS:
			printf("POS");
			break;
		case NUMERIC_NEG:
			printf("NEG");
			break;
		case NUMERIC_NAN:
			printf("NaN");
			break;
		default:
			printf("SIGN=0x%x", var->sign);
			break;
	}

	for (i = 0; i < var->ndigits; i++)
		printf(" %d", var->digits[i]);

	printf("\n");
}

#endif	 /* NUMERIC_DEBUG */


/* ----------
 * alloc_var() -
 *
 *	Allocate a digit buffer of ndigits digits (plus a spare digit for rounding)
 * ----------
 */
static void
alloc_var(NumericVar *var, int ndigits)
{
	digitbuf_free(var->buf);
	var->buf = digitbuf_alloc(ndigits + 1);
	var->buf[0] = 0;
	var->digits = var->buf + 1;
	var->ndigits = ndigits;
}


/* ----------
 * free_var() -
 *
 *	Return the digit buffer of a variable to the free pool
 * ----------
 */
static void
free_var(NumericVar *var)
{
	digitbuf_free(var->buf);
	var->buf = NULL;
	var->digits = NULL;
	var->sign = NUMERIC_NAN;
}


/* ----------
 * zero_var() -
 *
 *	Set a variable to ZERO.
 *	Note: rscale and dscale are not touched.
 * ----------
 */
static void
zero_var(NumericVar *var)
{
	digitbuf_free(var->buf);
	var->buf = NULL;
	var->digits = NULL;
	var->ndigits = 0;
	var->weight = 0;			/* by convention; doesn't really matter */
	var->sign = NUMERIC_POS;	/* anything but NAN... */
}


/* ----------
 * set_var_from_str()
 *
 *	Parse a string and put the number into a variable
 * ----------
 */
static void
set_var_from_str(char *str, NumericVar *dest)
{
	char	   *cp = str;
	bool		have_dp = FALSE;
	int			i = 0;

	while (*cp)
	{
		if (!isspace(*cp))
			break;
		cp++;
	}

	alloc_var(dest, strlen(cp));
	dest->weight = -1;
	dest->dscale = 0;
	dest->sign = NUMERIC_POS;

	switch (*cp)
	{
		case '+':
			dest->sign = NUMERIC_POS;
			cp++;
			break;

		case '-':
			dest->sign = NUMERIC_NEG;
			cp++;
			break;
	}

	if (*cp == '.')
	{
		have_dp = TRUE;
		cp++;
	}

	if (! isdigit(*cp))
		elog(ERROR, "Bad numeric input format '%s'", str);

	while (*cp)
	{
		if (isdigit(*cp))
		{
			dest->digits[i++] = *cp++ - '0';
			if (!have_dp)
				dest->weight++;
			else
				dest->dscale++;
		}
		else if (*cp == '.')
		{
			if (have_dp)
				elog(ERROR, "Bad numeric input format '%s'", str);
			have_dp = TRUE;
			cp++;
		}
		else
			break;
	}
	dest->ndigits = i;

	/* Handle exponent, if any */
	if (*cp == 'e' || *cp == 'E')
	{
		long	exponent;
		char   *endptr;

		cp++;
		exponent = strtol(cp, &endptr, 10);
		if (endptr == cp)
			elog(ERROR, "Bad numeric input format '%s'", str);
		cp = endptr;
		if (exponent > NUMERIC_MAX_PRECISION ||
			exponent < -NUMERIC_MAX_PRECISION)
			elog(ERROR, "Bad numeric input format '%s'", str);
		dest->weight += (int) exponent;
		dest->dscale -= (int) exponent;
		if (dest->dscale < 0)
			dest->dscale = 0;
	}

	/* Should be nothing left but spaces */
	while (*cp)
	{
		if (!isspace(*cp))
			elog(ERROR, "Bad numeric input format '%s'", str);
		cp++;
	}

	/* Strip any leading zeroes */
	while (dest->ndigits > 0 && *(dest->digits) == 0)
	{
		(dest->digits)++;
		(dest->weight)--;
		(dest->ndigits)--;
	}
	if (dest->ndigits == 0)
		dest->weight = 0;

	dest->rscale = dest->dscale;
}


/*
 * set_var_from_num() -
 *
 *	Parse back the packed db format into a variable
 *
 */
static void
set_var_from_num(Numeric num, NumericVar *dest)
{
	NumericDigit *digit;
	int			i;
	int			n;

	n = num->varlen - NUMERIC_HDRSZ; /* number of digit-pairs in packed fmt */

	alloc_var(dest, n * 2);

	dest->weight = num->n_weight;
	dest->rscale = num->n_rscale;
	dest->dscale = NUMERIC_DSCALE(num);
	dest->sign = NUMERIC_SIGN(num);

	digit = dest->digits;

	for (i = 0; i < n; i++)
	{
		unsigned char digitpair = num->n_data[i];
		*digit++ = (digitpair >> 4) & 0x0f;
		*digit++ = digitpair & 0x0f;
	}
}


/* ----------
 * set_var_from_var() -
 *
 *	Copy one variable into another
 * ----------
 */
static void
set_var_from_var(NumericVar *value, NumericVar *dest)
{
	NumericDigit *newbuf;

	newbuf = digitbuf_alloc(value->ndigits + 1);
	newbuf[0] = 0;				/* spare digit for rounding */
	memcpy(newbuf + 1, value->digits, value->ndigits);

	digitbuf_free(dest->buf);

	memcpy(dest, value, sizeof(NumericVar));
	dest->buf = newbuf;
	dest->digits = newbuf + 1;
}


/* ----------
 * get_str_from_var() -
 *
 *	Convert a var to text representation (guts of numeric_out).
 *	CAUTION: var's contents may be modified by rounding!
 *	Caller must have checked for NaN case.
 *	Returns a palloc'd string.
 * ----------
 */
static char *
get_str_from_var(NumericVar *var, int dscale)
{
	char	   *str;
	char	   *cp;
	int			i;
	int			d;

	/* ----------
	 * Check if we must round up before printing the value and
	 * do so.
	 * ----------
	 */
	i = dscale + var->weight + 1;
	if (i >= 0 && var->ndigits > i)
	{
		int		carry = (var->digits[i] > 4) ? 1 : 0;

		var->ndigits = i;

		while (carry)
		{
			carry += var->digits[--i];
			var->digits[i] = carry % 10;
			carry /= 10;
		}

		if (i < 0)
		{
			Assert(i == -1);	/* better not have added more than 1 digit */
			Assert(var->digits > var->buf);
			var->digits--;
			var->ndigits++;
			var->weight++;
		}
	}
	else
		var->ndigits = MAX(0, MIN(i, var->ndigits));

	/* ----------
	 * Allocate space for the result
	 * ----------
	 */
	str = palloc(MAX(0, dscale) + MAX(0, var->weight) + 4);
	cp = str;

	/* ----------
	 * Output a dash for negative values
	 * ----------
	 */
	if (var->sign == NUMERIC_NEG)
		*cp++ = '-';

	/* ----------
	 * Output all digits before the decimal point
	 * ----------
	 */
	i = MAX(var->weight, 0);
	d = 0;

	while (i >= 0)
	{
		if (i <= var->weight && d < var->ndigits)
			*cp++ = var->digits[d++] + '0';
		else
			*cp++ = '0';
		i--;
	}

	/* ----------
	 * If requested, output a decimal point and all the digits
	 * that follow it.
	 * ----------
	 */
	if (dscale > 0)
	{
		*cp++ = '.';
		while (i >= -dscale)
		{
			if (i <= var->weight && d < var->ndigits)
				*cp++ = var->digits[d++] + '0';
			else
				*cp++ = '0';
			i--;
		}
	}

	/* ----------
	 * terminate the string and return it
	 * ----------
	 */
	*cp = '\0';
	return str;
}


/* ----------
 * make_result() -
 *
 *	Create the packed db numeric format in palloc()'d memory from
 *	a variable.  The var's rscale determines the number of digits kept.
 * ----------
 */
static Numeric
make_result(NumericVar *var)
{
	Numeric		result;
	NumericDigit *digit = var->digits;
	int			weight = var->weight;
	int			sign = var->sign;
	int			n;
	int			i,
				j;

	if (sign == NUMERIC_NAN)
	{
		result = (Numeric) palloc(NUMERIC_HDRSZ);

		result->varlen = NUMERIC_HDRSZ;
		result->n_weight = 0;
		result->n_rscale = 0;
		result->n_sign_dscale = NUMERIC_NAN;

		dump_numeric("make_result()", result);
		return result;
	}

	n = MAX(0, MIN(var->ndigits, var->weight + var->rscale + 1));

	/* truncate leading zeroes */
	while (n > 0 && *digit == 0)
	{
		digit++;
		weight--;
		n--;
	}
	/* truncate trailing zeroes */
	while (n > 0 && digit[n - 1] == 0)
		n--;

	/* If zero result, force to weight=0 and positive sign */
	if (n == 0)
	{
		weight = 0;
		sign = NUMERIC_POS;
	}

	result = (Numeric) palloc(NUMERIC_HDRSZ + (n + 1) / 2);
	result->varlen = NUMERIC_HDRSZ + (n + 1) / 2;
	result->n_weight = weight;
	result->n_rscale = var->rscale;
	result->n_sign_dscale = sign |
		((uint16) var->dscale & NUMERIC_DSCALE_MASK);

	i = 0;
	j = 0;
	while (j < n)
	{
		unsigned char digitpair = digit[j++] << 4;
		if (j < n)
			digitpair |= digit[j++];
		result->n_data[i++] = digitpair;
	}

	dump_numeric("make_result()", result);
	return result;
}


/* ----------
 * apply_typmod() -
 *
 *	Do bounds checking and rounding according to the attributes
 *	typmod field.
 * ----------
 */
static void
apply_typmod(NumericVar *var, int32 typmod)
{
	int			precision;
	int			scale;
	int			maxweight;
	int			i;

	/* Do nothing if we have a default typmod (-1) */
	if (typmod < (int32) (VARHDRSZ))
		return;

	typmod -= VARHDRSZ;
	precision = (typmod >> 16) & 0xffff;
	scale = typmod & 0xffff;
	maxweight = precision - scale;

	/* Round to target scale */
	i = scale + var->weight + 1;
	if (i >= 0 && var->ndigits > i)
	{
		int		carry = (var->digits[i] > 4) ? 1 : 0;

		var->ndigits = i;

		while (carry)
		{
			carry += var->digits[--i];
			var->digits[i] = carry % 10;
			carry /= 10;
		}

		if (i < 0)
		{
			Assert(i == -1);	/* better not have added more than 1 digit */
			Assert(var->digits > var->buf);
			var->digits--;
			var->ndigits++;
			var->weight++;
		}
	}
	else
		var->ndigits = MAX(0, MIN(i, var->ndigits));

	/* ----------
	 * Check for overflow - note we can't do this before rounding,
	 * because rounding could raise the weight.  Also note that the
	 * var's weight could be inflated by leading zeroes, which will
	 * be stripped before storage but perhaps might not have been yet.
	 * In any case, we must recognize a true zero, whose weight doesn't
	 * mean anything.
	 * ----------
	 */
	if (var->weight >= maxweight)
	{
		/* Determine true weight; and check for all-zero result */
		int		tweight = var->weight;

		for (i = 0; i < var->ndigits; i++)
		{
			if (var->digits[i])
				break;
			tweight--;
		}
		
		if (tweight >= maxweight && i < var->ndigits)
			elog(ERROR, "overflow on numeric "
				 "ABS(value) >= 10^%d for field with precision %d scale %d",
				 tweight, precision, scale);
	}

	var->rscale = scale;
	var->dscale = scale;
}


/* ----------
 * cmp_var() -
 *
 *	Compare two values on variable level
 * ----------
 */
static int
cmp_var(NumericVar *var1, NumericVar *var2)
{
	if (var1->ndigits == 0)
	{
		if (var2->ndigits == 0)
			return 0;
		if (var2->sign == NUMERIC_NEG)
			return 1;
		return -1;
	}
	if (var2->ndigits == 0)
	{
		if (var1->sign == NUMERIC_POS)
			return 1;
		return -1;
	}

	if (var1->sign == NUMERIC_POS)
	{
		if (var2->sign == NUMERIC_NEG)
			return 1;
		return cmp_abs(var1, var2);
	}

	if (var2->sign == NUMERIC_POS)
		return -1;

	return cmp_abs(var2, var1);
}


/* ----------
 * add_var() -
 *
 *	Full version of add functionality on variable level (handling signs).
 *	result might point to one of the operands too without danger.
 * ----------
 */
static void
add_var(NumericVar *var1, NumericVar *var2, NumericVar *result)
{
	/* ----------
	 * Decide on the signs of the two variables what to do
	 * ----------
	 */
	if (var1->sign == NUMERIC_POS)
	{
		if (var2->sign == NUMERIC_POS)
		{
			/* ----------
			 * Both are positive
			 * result = +(ABS(var1) + ABS(var2))
			 * ----------
			 */
			add_abs(var1, var2, result);
			result->sign = NUMERIC_POS;
		}
		else
		{
			/* ----------
			 * var1 is positive, var2 is negative
			 * Must compare absolute values
			 * ----------
			 */
			switch (cmp_abs(var1, var2))
			{
				case 0: /* ----------
												 * ABS(var1) == ABS(var2)
												 * result = ZERO
												 * ----------
												 */
					zero_var(result);
					result->rscale = MAX(var1->rscale, var2->rscale);
					result->dscale = MAX(var1->dscale, var2->dscale);
					break;

				case 1: /* ----------
												 * ABS(var1) > ABS(var2)
												 * result = +(ABS(var1) - ABS(var2))
												 * ----------
												 */
					sub_abs(var1, var2, result);
					result->sign = NUMERIC_POS;
					break;

				case -1:		/* ----------
								 * ABS(var1) < ABS(var2)
								 * result = -(ABS(var2) - ABS(var1))
								 * ----------
								 */
					sub_abs(var2, var1, result);
					result->sign = NUMERIC_NEG;
					break;
			}
		}
	}
	else
	{
		if (var2->sign == NUMERIC_POS)
		{
			/* ----------
			 * var1 is negative, var2 is positive
			 * Must compare absolute values
			 * ----------
			 */
			switch (cmp_abs(var1, var2))
			{
				case 0: /* ----------
												 * ABS(var1) == ABS(var2)
												 * result = ZERO
												 * ----------
												 */
					zero_var(result);
					result->rscale = MAX(var1->rscale, var2->rscale);
					result->dscale = MAX(var1->dscale, var2->dscale);
					break;

				case 1: /* ----------
												 * ABS(var1) > ABS(var2)
												 * result = -(ABS(var1) - ABS(var2))
												 * ----------
												 */
					sub_abs(var1, var2, result);
					result->sign = NUMERIC_NEG;
					break;

				case -1:		/* ----------
								 * ABS(var1) < ABS(var2)
								 * result = +(ABS(var2) - ABS(var1))
								 * ----------
								 */
					sub_abs(var2, var1, result);
					result->sign = NUMERIC_POS;
					break;
			}
		}
		else
		{
			/* ----------
			 * Both are negative
			 * result = -(ABS(var1) + ABS(var2))
			 * ----------
			 */
			add_abs(var1, var2, result);
			result->sign = NUMERIC_NEG;
		}
	}
}


/* ----------
 * sub_var() -
 *
 *	Full version of sub functionality on variable level (handling signs).
 *	result might point to one of the operands too without danger.
 * ----------
 */
static void
sub_var(NumericVar *var1, NumericVar *var2, NumericVar *result)
{
	/* ----------
	 * Decide on the signs of the two variables what to do
	 * ----------
	 */
	if (var1->sign == NUMERIC_POS)
	{
		if (var2->sign == NUMERIC_NEG)
		{
			/* ----------
			 * var1 is positive, var2 is negative
			 * result = +(ABS(var1) + ABS(var2))
			 * ----------
			 */
			add_abs(var1, var2, result);
			result->sign = NUMERIC_POS;
		}
		else
		{
			/* ----------
			 * Both are positive
			 * Must compare absolute values
			 * ----------
			 */
			switch (cmp_abs(var1, var2))
			{
				case 0: /* ----------
												 * ABS(var1) == ABS(var2)
												 * result = ZERO
												 * ----------
												 */
					zero_var(result);
					result->rscale = MAX(var1->rscale, var2->rscale);
					result->dscale = MAX(var1->dscale, var2->dscale);
					break;

				case 1: /* ----------
												 * ABS(var1) > ABS(var2)
												 * result = +(ABS(var1) - ABS(var2))
												 * ----------
												 */
					sub_abs(var1, var2, result);
					result->sign = NUMERIC_POS;
					break;

				case -1:		/* ----------
								 * ABS(var1) < ABS(var2)
								 * result = -(ABS(var2) - ABS(var1))
								 * ----------
								 */
					sub_abs(var2, var1, result);
					result->sign = NUMERIC_NEG;
					break;
			}
		}
	}
	else
	{
		if (var2->sign == NUMERIC_NEG)
		{
			/* ----------
			 * Both are negative
			 * Must compare absolute values
			 * ----------
			 */
			switch (cmp_abs(var1, var2))
			{
				case 0: /* ----------
												 * ABS(var1) == ABS(var2)
												 * result = ZERO
												 * ----------
												 */
					zero_var(result);
					result->rscale = MAX(var1->rscale, var2->rscale);
					result->dscale = MAX(var1->dscale, var2->dscale);
					break;

				case 1: /* ----------
												 * ABS(var1) > ABS(var2)
												 * result = -(ABS(var1) - ABS(var2))
												 * ----------
												 */
					sub_abs(var1, var2, result);
					result->sign = NUMERIC_NEG;
					break;

				case -1:		/* ----------
								 * ABS(var1) < ABS(var2)
								 * result = +(ABS(var2) - ABS(var1))
								 * ----------
								 */
					sub_abs(var2, var1, result);
					result->sign = NUMERIC_POS;
					break;
			}
		}
		else
		{
			/* ----------
			 * var1 is negative, var2 is positive
			 * result = -(ABS(var1) + ABS(var2))
			 * ----------
			 */
			add_abs(var1, var2, result);
			result->sign = NUMERIC_NEG;
		}
	}
}


/* ----------
 * mul_var() -
 *
 *	Multiplication on variable level. Product of var1 * var2 is stored
 *	in result.
 * ----------
 */
static void
mul_var(NumericVar *var1, NumericVar *var2, NumericVar *result)
{
	NumericDigit *res_buf;
	NumericDigit *res_digits;
	int			res_ndigits;
	int			res_weight;
	int			res_sign;
	int			i,
				ri,
				i1,
				i2;
	long		sum = 0;

	res_weight = var1->weight + var2->weight + 2;
	res_ndigits = var1->ndigits + var2->ndigits + 1;
	if (var1->sign == var2->sign)
		res_sign = NUMERIC_POS;
	else
		res_sign = NUMERIC_NEG;

	res_buf = digitbuf_alloc(res_ndigits);
	res_digits = res_buf;
	memset(res_digits, 0, res_ndigits);

	ri = res_ndigits;
	for (i1 = var1->ndigits - 1; i1 >= 0; i1--)
	{
		sum = 0;
		i = --ri;

		for (i2 = var2->ndigits - 1; i2 >= 0; i2--)
		{
			sum = sum + res_digits[i] + var1->digits[i1] * var2->digits[i2];
			res_digits[i--] = sum % 10;
			sum /= 10;
		}
		res_digits[i] = sum;
	}

	i = res_weight + global_rscale + 2;
	if (i >= 0 && i < res_ndigits)
	{
		sum = (res_digits[i] > 4) ? 1 : 0;
		res_ndigits = i;
		i--;
		while (sum)
		{
			sum += res_digits[i];
			res_digits[i--] = sum % 10;
			sum /= 10;
		}
	}

	while (res_ndigits > 0 && *res_digits == 0)
	{
		res_digits++;
		res_weight--;
		res_ndigits--;
	}
	while (res_ndigits > 0 && res_digits[res_ndigits - 1] == 0)
		res_ndigits--;

	if (res_ndigits == 0)
	{
		res_sign = NUMERIC_POS;
		res_weight = 0;
	}

	digitbuf_free(result->buf);
	result->buf = res_buf;
	result->digits = res_digits;
	result->ndigits = res_ndigits;
	result->weight = res_weight;
	result->rscale = global_rscale;
	result->sign = res_sign;
}


/* ----------
 * div_var() -
 *
 *	Division on variable level.
 * ----------
 */
static void
div_var(NumericVar *var1, NumericVar *var2, NumericVar *result)
{
	NumericDigit *res_digits;
	int			res_ndigits;
	int			res_sign;
	int			res_weight;
	NumericVar	dividend;
	NumericVar	divisor[10];
	int			ndigits_tmp;
	int			weight_tmp;
	int			rscale_tmp;
	int			ri;
	int			i;
	long		guess;
	long		first_have;
	long		first_div;
	int			first_nextdigit;
	int			stat = 0;

	/* ----------
	 * First of all division by zero check
	 * ----------
	 */
	ndigits_tmp = var2->ndigits + 1;
	if (ndigits_tmp == 1)
		elog(ERROR, "division by zero on numeric");

	/* ----------
	 * Determine the result sign, weight and number of digits to calculate
	 * ----------
	 */
	if (var1->sign == var2->sign)
		res_sign = NUMERIC_POS;
	else
		res_sign = NUMERIC_NEG;
	res_weight = var1->weight - var2->weight + 1;
	res_ndigits = global_rscale + res_weight;
	if (res_ndigits <= 0)
		res_ndigits = 1;

	/* ----------
	 * Now result zero check
	 * ----------
	 */
	if (var1->ndigits == 0)
	{
		zero_var(result);
		result->rscale = global_rscale;
		return;
	}

	/* ----------
	 * Initialize local variables
	 * ----------
	 */
	init_var(&dividend);
	for (i = 1; i < 10; i++)
		init_var(&divisor[i]);

	/* ----------
	 * Make a copy of the divisor which has one leading zero digit
	 * ----------
	 */
	divisor[1].ndigits = ndigits_tmp;
	divisor[1].rscale = var2->ndigits;
	divisor[1].sign = NUMERIC_POS;
	divisor[1].buf = digitbuf_alloc(ndigits_tmp);
	divisor[1].digits = divisor[1].buf;
	divisor[1].digits[0] = 0;
	memcpy(&(divisor[1].digits[1]), var2->digits, ndigits_tmp - 1);

	/* ----------
	 * Make a copy of the dividend
	 * ----------
	 */
	dividend.ndigits = var1->ndigits;
	dividend.weight = 0;
	dividend.rscale = var1->ndigits;
	dividend.sign = NUMERIC_POS;
	dividend.buf = digitbuf_alloc(var1->ndigits);
	dividend.digits = dividend.buf;
	memcpy(dividend.digits, var1->digits, var1->ndigits);

	/* ----------
	 * Setup the result
	 * ----------
	 */
	digitbuf_free(result->buf);
	result->buf = digitbuf_alloc(res_ndigits + 2);
	res_digits = result->buf;
	result->digits = res_digits;
	result->ndigits = res_ndigits;
	result->weight = res_weight;
	result->rscale = global_rscale;
	result->sign = res_sign;
	res_digits[0] = 0;

	first_div = divisor[1].digits[1] * 10;
	if (ndigits_tmp > 2)
		first_div += divisor[1].digits[2];

	first_have = 0;
	first_nextdigit = 0;

	weight_tmp = 1;
	rscale_tmp = divisor[1].rscale;

	for (ri = 0; ri <= res_ndigits; ri++)
	{
		first_have = first_have * 10;
		if (first_nextdigit >= 0 && first_nextdigit < dividend.ndigits)
			first_have += dividend.digits[first_nextdigit];
		first_nextdigit++;

		guess = (first_have * 10) / first_div + 1;
		if (guess > 9)
			guess = 9;

		while (guess > 0)
		{
			if (divisor[guess].buf == NULL)
			{
				int			i;
				long		sum = 0;

				memcpy(&divisor[guess], &divisor[1], sizeof(NumericVar));
				divisor[guess].buf = digitbuf_alloc(divisor[guess].ndigits);
				divisor[guess].digits = divisor[guess].buf;
				for (i = divisor[1].ndigits - 1; i >= 0; i--)
				{
					sum += divisor[1].digits[i] * guess;
					divisor[guess].digits[i] = sum % 10;
					sum /= 10;
				}
			}

			divisor[guess].weight = weight_tmp;
			divisor[guess].rscale = rscale_tmp;

			stat = cmp_abs(&dividend, &divisor[guess]);
			if (stat >= 0)
				break;

			guess--;
		}

		res_digits[ri + 1] = guess;
		if (stat == 0)
		{
			ri++;
			break;
		}

		weight_tmp--;
		rscale_tmp++;

		if (guess == 0)
			continue;

		sub_abs(&dividend, &divisor[guess], &dividend);

		first_nextdigit = dividend.weight - weight_tmp;
		first_have = 0;
		if (first_nextdigit >= 0 && first_nextdigit < dividend.ndigits)
			first_have = dividend.digits[first_nextdigit];
		first_nextdigit++;
	}

	result->ndigits = ri + 1;
	if (ri == res_ndigits + 1)
	{
		int		carry = (res_digits[ri] > 4) ? 1 : 0;

		result->ndigits = ri;
		res_digits[ri] = 0;

		while (carry && ri > 0)
		{
			carry += res_digits[--ri];
			res_digits[ri] = carry % 10;
			carry /= 10;
		}
	}

	while (result->ndigits > 0 && *(result->digits) == 0)
	{
		(result->digits)++;
		(result->weight)--;
		(result->ndigits)--;
	}
	while (result->ndigits > 0 && result->digits[result->ndigits - 1] == 0)
		(result->ndigits)--;
	if (result->ndigits == 0)
		result->sign = NUMERIC_POS;

	/*
	 * Tidy up
	 *
	 */
	digitbuf_free(dividend.buf);
	for (i = 1; i < 10; i++)
		digitbuf_free(divisor[i].buf);
}


/* ----------
 * mod_var() -
 *
 *	Calculate the modulo of two numerics at variable level
 * ----------
 */
static void
mod_var(NumericVar *var1, NumericVar *var2, NumericVar *result)
{
	NumericVar	tmp;
	int			save_global_rscale;

	init_var(&tmp);

	/* ----------
	 * We do it by fiddling around with global_rscale and truncating
	 * the result of the division.
	 * ----------
	 */
	save_global_rscale = global_rscale;
	global_rscale = var2->rscale + 2;

	div_var(var1, var2, &tmp);
	tmp.rscale = var2->rscale;
	tmp.ndigits = MAX(0, MIN(tmp.ndigits, tmp.weight + tmp.rscale + 1));

	global_rscale = var2->rscale;
	mul_var(var2, &tmp, &tmp);

	sub_var(var1, &tmp, result);

	global_rscale = save_global_rscale;
	free_var(&tmp);
}


/* ----------
 * ceil_var() -
 *
 *	Return the smallest integer greater than or equal to the argument
 *	on variable level
 * ----------
 */
static void
ceil_var(NumericVar *var, NumericVar *result)
{
	NumericVar	tmp;

	init_var(&tmp);
	set_var_from_var(var, &tmp);

	tmp.rscale = 0;
	tmp.ndigits = MIN(tmp.ndigits, MAX(0, tmp.weight + 1));
	if (tmp.sign == NUMERIC_POS && cmp_var(var, &tmp) != 0)
		add_var(&tmp, &const_one, &tmp);

	set_var_from_var(&tmp, result);
	free_var(&tmp);
}


/* ----------
 * floor_var() -
 *
 *	Return the largest integer equal to or less than the argument
 *	on variable level
 * ----------
 */
static void
floor_var(NumericVar *var, NumericVar *result)
{
	NumericVar	tmp;

	init_var(&tmp);
	set_var_from_var(var, &tmp);

	tmp.rscale = 0;
	tmp.ndigits = MIN(tmp.ndigits, MAX(0, tmp.weight + 1));
	if (tmp.sign == NUMERIC_NEG && cmp_var(var, &tmp) != 0)
		sub_var(&tmp, &const_one, &tmp);

	set_var_from_var(&tmp, result);
	free_var(&tmp);
}


/* ----------
 * sqrt_var() -
 *
 *	Compute the square root of x using Newtons algorithm
 * ----------
 */
static void
sqrt_var(NumericVar *arg, NumericVar *result)
{
	NumericVar	tmp_arg;
	NumericVar	tmp_val;
	NumericVar	last_val;
	int			res_rscale;
	int			save_global_rscale;
	int			stat;

	save_global_rscale = global_rscale;
	global_rscale += 8;
	res_rscale = global_rscale;

	stat = cmp_var(arg, &const_zero);
	if (stat == 0)
	{
		set_var_from_var(&const_zero, result);
		result->rscale = res_rscale;
		result->sign = NUMERIC_POS;
		return;
	}

	if (stat < 0)
		elog(ERROR, "math error on numeric - cannot compute SQRT of negative value");

	init_var(&tmp_arg);
	init_var(&tmp_val);
	init_var(&last_val);

	set_var_from_var(arg, &tmp_arg);
	set_var_from_var(result, &last_val);

	/* ----------
	 * Initialize the result to the first guess
	 * ----------
	 */
	digitbuf_free(result->buf);
	result->buf = digitbuf_alloc(1);
	result->digits = result->buf;
	result->digits[0] = tmp_arg.digits[0] / 2;
	if (result->digits[0] == 0)
		result->digits[0] = 1;
	result->ndigits = 1;
	result->weight = tmp_arg.weight / 2;
	result->rscale = res_rscale;
	result->sign = NUMERIC_POS;

	for (;;)
	{
		div_var(&tmp_arg, result, &tmp_val);

		add_var(result, &tmp_val, result);
		div_var(result, &const_two, result);

		if (cmp_var(&last_val, result) == 0)
			break;
		set_var_from_var(result, &last_val);
	}

	free_var(&last_val);
	free_var(&tmp_val);
	free_var(&tmp_arg);

	global_rscale = save_global_rscale;
	div_var(result, &const_one, result);
}


/* ----------
 * exp_var() -
 *
 *	Raise e to the power of x
 * ----------
 */
static void
exp_var(NumericVar *arg, NumericVar *result)
{
	NumericVar	x;
	NumericVar	xpow;
	NumericVar	ifac;
	NumericVar	elem;
	NumericVar	ni;
	int			d;
	int			i;
	int			ndiv2 = 0;
	bool		xneg = FALSE;
	int			save_global_rscale;

	init_var(&x);
	init_var(&xpow);
	init_var(&ifac);
	init_var(&elem);
	init_var(&ni);

	set_var_from_var(arg, &x);

	if (x.sign == NUMERIC_NEG)
	{
		xneg = TRUE;
		x.sign = NUMERIC_POS;
	}

	save_global_rscale = global_rscale;
	global_rscale = 0;
	for (i = x.weight, d = 0; i >= 0; i--, d++)
	{
		global_rscale *= 10;
		if (d < x.ndigits)
			global_rscale += x.digits[d];
		if (global_rscale >= 1000)
			elog(ERROR, "argument for EXP() too big");
	}

	global_rscale = global_rscale / 2 + save_global_rscale + 8;

	while (cmp_var(&x, &const_one) > 0)
	{
		ndiv2++;
		global_rscale++;
		div_var(&x, &const_two, &x);
	}

	add_var(&const_one, &x, result);
	set_var_from_var(&x, &xpow);
	set_var_from_var(&const_one, &ifac);
	set_var_from_var(&const_one, &ni);

	for (i = 2; TRUE; i++)
	{
		add_var(&ni, &const_one, &ni);
		mul_var(&xpow, &x, &xpow);
		mul_var(&ifac, &ni, &ifac);
		div_var(&xpow, &ifac, &elem);

		if (elem.ndigits == 0)
			break;

		add_var(result, &elem, result);
	}

	while (ndiv2-- > 0)
		mul_var(result, result, result);

	global_rscale = save_global_rscale;
	if (xneg)
		div_var(&const_one, result, result);
	else
		div_var(result, &const_one, result);

	result->sign = NUMERIC_POS;

	free_var(&x);
	free_var(&xpow);
	free_var(&ifac);
	free_var(&elem);
	free_var(&ni);
}


/* ----------
 * ln_var() -
 *
 *	Compute the natural log of x
 * ----------
 */
static void
ln_var(NumericVar *arg, NumericVar *result)
{
	NumericVar	x;
	NumericVar	xx;
	NumericVar	ni;
	NumericVar	elem;
	NumericVar	fact;
	int			i;
	int			save_global_rscale;

	if (cmp_var(arg, &const_zero) <= 0)
		elog(ERROR, "math error on numeric - cannot compute LN of value <= zero");

	save_global_rscale = global_rscale;
	global_rscale += 8;

	init_var(&x);
	init_var(&xx);
	init_var(&ni);
	init_var(&elem);
	init_var(&fact);

	set_var_from_var(&const_two, &fact);
	set_var_from_var(arg, &x);

	while (cmp_var(&x, &const_two) >= 0)
	{
		sqrt_var(&x, &x);
		mul_var(&fact, &const_two, &fact);
	}
	set_var_from_str("0.5", &elem);
	while (cmp_var(&x, &elem) <= 0)
	{
		sqrt_var(&x, &x);
		mul_var(&fact, &const_two, &fact);
	}

	sub_var(&x, &const_one, result);
	add_var(&x, &const_one, &elem);
	div_var(result, &elem, result);
	set_var_from_var(result, &xx);
	mul_var(result, result, &x);

	set_var_from_var(&const_one, &ni);

	for (i = 2; TRUE; i++)
	{
		add_var(&ni, &const_two, &ni);
		mul_var(&xx, &x, &xx);
		div_var(&xx, &ni, &elem);

		if (cmp_var(&elem, &const_zero) == 0)
			break;

		add_var(result, &elem, result);
	}

	global_rscale = save_global_rscale;
	mul_var(result, &fact, result);

	free_var(&x);
	free_var(&xx);
	free_var(&ni);
	free_var(&elem);
	free_var(&fact);
}


/* ----------
 * log_var() -
 *
 *	Compute the logarithm of x in a given base
 * ----------
 */
static void
log_var(NumericVar *base, NumericVar *num, NumericVar *result)
{
	NumericVar	ln_base;
	NumericVar	ln_num;

	global_rscale += 8;

	init_var(&ln_base);
	init_var(&ln_num);

	ln_var(base, &ln_base);
	ln_var(num, &ln_num);

	global_rscale -= 8;

	div_var(&ln_num, &ln_base, result);

	free_var(&ln_num);
	free_var(&ln_base);
}


/* ----------
 * power_var() -
 *
 *	Raise base to the power of exp
 * ----------
 */
static void
power_var(NumericVar *base, NumericVar *exp, NumericVar *result)
{
	NumericVar	ln_base;
	NumericVar	ln_num;
	int			save_global_rscale;

	save_global_rscale = global_rscale;
	global_rscale += global_rscale / 3 + 8;

	init_var(&ln_base);
	init_var(&ln_num);

	ln_var(base, &ln_base);
	mul_var(&ln_base, exp, &ln_num);

	global_rscale = save_global_rscale;

	exp_var(&ln_num, result);

	free_var(&ln_num);
	free_var(&ln_base);

}


/* ----------------------------------------------------------------------
 *
 * Following are the lowest level functions that operate unsigned
 * on the variable level
 *
 * ----------------------------------------------------------------------
 */


/* ----------
 * cmp_abs() -
 *
 *	Compare the absolute values of var1 and var2
 *	Returns:	-1 for ABS(var1) < ABS(var2)
 *				0  for ABS(var1) == ABS(var2)
 *				1  for ABS(var1) > ABS(var2)
 * ----------
 */
static int
cmp_abs(NumericVar *var1, NumericVar *var2)
{
	int			i1 = 0;
	int			i2 = 0;
	int			w1 = var1->weight;
	int			w2 = var2->weight;
	int			stat;

	while (w1 > w2 && i1 < var1->ndigits)
	{
		if (var1->digits[i1++] != 0)
			return 1;
		w1--;
	}
	while (w2 > w1 && i2 < var2->ndigits)
	{
		if (var2->digits[i2++] != 0)
			return -1;
		w2--;
	}

	if (w1 == w2)
	{
		while (i1 < var1->ndigits && i2 < var2->ndigits)
		{
			stat = var1->digits[i1++] - var2->digits[i2++];
			if (stat)
			{
				if (stat > 0)
					return 1;
				return -1;
			}
		}
	}

	while (i1 < var1->ndigits)
	{
		if (var1->digits[i1++] != 0)
			return 1;
	}
	while (i2 < var2->ndigits)
	{
		if (var2->digits[i2++] != 0)
			return -1;
	}

	return 0;
}


/* ----------
 * add_abs() -
 *
 *	Add the absolute values of two variables into result.
 *	result might point to one of the operands without danger.
 * ----------
 */
static void
add_abs(NumericVar *var1, NumericVar *var2, NumericVar *result)
{
	NumericDigit *res_buf;
	NumericDigit *res_digits;
	int			res_ndigits;
	int			res_weight;
	int			res_rscale;
	int			res_dscale;
	int			i,
				i1,
				i2;
	int			carry = 0;

	res_weight = MAX(var1->weight, var2->weight) + 1;
	res_rscale = MAX(var1->rscale, var2->rscale);
	res_dscale = MAX(var1->dscale, var2->dscale);
	res_ndigits = res_rscale + res_weight + 1;
	if (res_ndigits <= 0)
		res_ndigits = 1;

	res_buf = digitbuf_alloc(res_ndigits);
	res_digits = res_buf;

	i1 = res_rscale + var1->weight + 1;
	i2 = res_rscale + var2->weight + 1;
	for (i = res_ndigits - 1; i >= 0; i--)
	{
		i1--;
		i2--;
		if (i1 >= 0 && i1 < var1->ndigits)
			carry += var1->digits[i1];
		if (i2 >= 0 && i2 < var2->ndigits)
			carry += var2->digits[i2];

		res_digits[i] = carry % 10;
		carry /= 10;
	}

	while (res_ndigits > 0 && *res_digits == 0)
	{
		res_digits++;
		res_weight--;
		res_ndigits--;
	}
	while (res_ndigits > 0 && res_digits[res_ndigits - 1] == 0)
		res_ndigits--;

	if (res_ndigits == 0)
		res_weight = 0;

	digitbuf_free(result->buf);
	result->ndigits = res_ndigits;
	result->buf = res_buf;
	result->digits = res_digits;
	result->weight = res_weight;
	result->rscale = res_rscale;
	result->dscale = res_dscale;
}


/* ----------
 * sub_abs() -
 *
 *	Subtract the absolute value of var2 from the absolute value of var1
 *	and store in result. result might point to one of the operands
 *	without danger.
 *
 *	ABS(var1) MUST BE GREATER OR EQUAL ABS(var2) !!!
 * ----------
 */
static void
sub_abs(NumericVar *var1, NumericVar *var2, NumericVar *result)
{
	NumericDigit *res_buf;
	NumericDigit *res_digits;
	int			res_ndigits;
	int			res_weight;
	int			res_rscale;
	int			res_dscale;
	int			i,
				i1,
				i2;
	int			borrow = 0;

	res_weight = var1->weight;
	res_rscale = MAX(var1->rscale, var2->rscale);
	res_dscale = MAX(var1->dscale, var2->dscale);
	res_ndigits = res_rscale + res_weight + 1;
	if (res_ndigits <= 0)
		res_ndigits = 1;

	res_buf = digitbuf_alloc(res_ndigits);
	res_digits = res_buf;

	i1 = res_rscale + var1->weight + 1;
	i2 = res_rscale + var2->weight + 1;
	for (i = res_ndigits - 1; i >= 0; i--)
	{
		i1--;
		i2--;
		if (i1 >= 0 && i1 < var1->ndigits)
			borrow += var1->digits[i1];
		if (i2 >= 0 && i2 < var2->ndigits)
			borrow -= var2->digits[i2];

		if (borrow < 0)
		{
			res_digits[i] = borrow + 10;
			borrow = -1;
		}
		else
		{
			res_digits[i] = borrow;
			borrow = 0;
		}
	}

	while (res_ndigits > 0 && *res_digits == 0)
	{
		res_digits++;
		res_weight--;
		res_ndigits--;
	}
	while (res_ndigits > 0 && res_digits[res_ndigits - 1] == 0)
		res_ndigits--;

	if (res_ndigits == 0)
		res_weight = 0;

	digitbuf_free(result->buf);
	result->ndigits = res_ndigits;
	result->buf = res_buf;
	result->digits = res_digits;
	result->weight = res_weight;
	result->rscale = res_rscale;
	result->dscale = res_dscale;
}
