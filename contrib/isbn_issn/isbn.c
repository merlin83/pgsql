/*
 *	PostgreSQL type definitions for ISBNs.
 *
 *	$Id: isbn.c,v 1.3 2000-05-29 05:44:26 tgl Exp $
 */

#include <stdio.h>

#include <postgres.h>
#include <utils/palloc.h>

/*
 *	This is the internal storage format for ISBNs.
 *	NB: This is an intentional type pun with builtin type `char16'.
 */

typedef struct isbn
{
	char		num[13];
	char		pad[3];
}			isbn;

/*
 *	Various forward declarations:
 */

isbn	   *isbn_in(char *str);
char	   *isbn_out(isbn * addr);

bool		isbn_lt(isbn * a1, isbn * a2);
bool		isbn_le(isbn * a1, isbn * a2);
bool		isbn_eq(isbn * a1, isbn * a2);
bool		isbn_ge(isbn * a1, isbn * a2);
bool		isbn_gt(isbn * a1, isbn * a2);

bool		isbn_ne(isbn * a1, isbn * a2);

int4		isbn_cmp(isbn * a1, isbn * a2);

int4		isbn_sum(char *str);

/*
 *	ISBN reader.
 */

isbn *
isbn_in(char *str)
{
	isbn	   *result;

	if (strlen(str) != 13)
	{
		elog(ERROR, "isbn_in: invalid ISBN \"%s\"", str);
		return (NULL);
	}
	if (isbn_sum(str) != 0)
	{
		elog(ERROR, "isbn_in: purported ISBN \"%s\" failed checksum",
			 str);
		return (NULL);
	}

	result = (isbn *) palloc(sizeof(isbn));

	strncpy(result->num, str, 13);
	memset(result->pad, ' ', 3);
	return (result);
}

/*
 * The ISBN checksum is defined as follows:
 *
 * Number the digits from 1 to 9 (call this N).
 * Compute the sum, S, of N * D_N.
 * The check digit, C, is the value which satisfies the equation
 *	S + 10*C === 0 (mod 11)
 * The value 10 for C is written as `X'.
 *
 * For our purposes, we want the complete sum including the check
 * digit; if this is zero, then the checksum passed.  We also check
 * the syntactic validity if the provided string, and return 12
 * if any errors are found.
 */
int4
isbn_sum(char *str)
{
	int4		sum = 0,
				dashes = 0,
				val;
	int			i;

	for (i = 0; str[i] && i < 13; i++)
	{
		switch (str[i])
		{
			case '-':
				if (++dashes > 3)
					return 12;
				continue;

			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				val = str[i] - '0';
				break;

			case 'X':
			case 'x':
				val = 10;
				break;

			default:
				return 12;
		}

		sum += val * (i + 1 - dashes);
	}
	return (sum % 11);
}

/*
 *	ISBN output function.
 */

char *
isbn_out(isbn * num)
{
	char	   *result;

	if (num == NULL)
		return (NULL);

	result = (char *) palloc(14);

	result[0] = '\0';
	strncat(result, num->num, 13);
	return (result);
}

/*
 *	Boolean tests for magnitude.
 */

bool
isbn_lt(isbn * a1, isbn * a2)
{
	return (strncmp(a1->num, a2->num, 13) < 0);
};

bool
isbn_le(isbn * a1, isbn * a2)
{
	return (strncmp(a1->num, a2->num, 13) <= 0);
};

bool
isbn_eq(isbn * a1, isbn * a2)
{
	return (strncmp(a1->num, a2->num, 13) == 0);
};

bool
isbn_ge(isbn * a1, isbn * a2)
{
	return (strncmp(a1->num, a2->num, 13) >= 0);
};

bool
isbn_gt(isbn * a1, isbn * a2)
{
	return (strncmp(a1->num, a2->num, 13) > 0);
};

bool
isbn_ne(isbn * a1, isbn * a2)
{
	return (strncmp(a1->num, a2->num, 13) != 0);
};

/*
 *	Comparison function for sorting:
 */

int4
isbn_cmp(isbn * a1, isbn * a2)
{
	return (strncmp(a1->num, a2->num, 13));
}

/*
 *	eof
 */
