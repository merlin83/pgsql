/*-------------------------------------------------------------------------
 *
 * read.c
 *	  routines to convert a string (legal ascii representation of node) back
 *	  to nodes
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/nodes/read.c,v 1.19 2000-01-14 00:53:21 tgl Exp $
 *
 * HISTORY
 *	  AUTHOR			DATE			MAJOR EVENT
 *	  Andrew Yu			Nov 2, 1994		file creation
 *
 *-------------------------------------------------------------------------
 */
#include <ctype.h>

#include "postgres.h"

#include "nodes/pg_list.h"
#include "nodes/readfuncs.h"

/*
 * stringToNode -
 *	  returns a Node with a given legal ASCII representation
 */
void *
stringToNode(char *str)
{
	void	   *retval;

	lsptok(str, NULL);			/* set the string used in lsptok */
	retval = nodeRead(true);	/* start reading */

	return retval;
}

/*****************************************************************************
 *
 * the lisp token parser
 *
 *****************************************************************************/

/*
 * lsptok --- retrieve next "token" from a string.
 *
 * Works kinda like strtok, except it never modifies the source string.
 * (Instead of storing nulls into the string, the length of the token
 * is returned to the caller.)
 * Also, the rules about what is a token are hard-wired rather than being
 * configured by passing a set of terminating characters.
 *
 * The string is initially set by passing a non-NULL "string" value,
 * and subsequent calls with string==NULL read the previously given value.
 * (Pass length==NULL to set the string without reading its first token.)
 *
 * The rules for tokens are:
 *  * Whitespace (space, tab, newline) always separates tokens.
 *  * The characters '(', ')', '{', '}' form individual tokens even
 *    without any whitespace around them.
 *  * Otherwise, a token is all the characters up to the next whitespace
 *    or occurrence of one of the four special characters.
 *  * A backslash '\' can be used to quote whitespace or one of the four
 *    special characters, so that it is treated as a plain token character.
 *    Backslashes themselves must also be backslashed for consistency.
 *    Any other character can be, but need not be, backslashed as well.
 *  * If the resulting token is '<>' (with no backslash), it is returned
 *    as a non-NULL pointer to the token but with length == 0.  Note that
 *    there is no other way to get a zero-length token.
 *
 * Returns a pointer to the start of the next token, and the length of the
 * token (including any embedded backslashes!) in *length.  If there are
 * no more tokens, NULL and 0 are returned.
 *
 * NOTE: this routine doesn't remove backslashes; the caller must do so
 * if necessary (see "debackslash").
 *
 * NOTE: prior to release 7.0, this routine also had a special case to treat
 * a token starting with '"' as extending to the next '"'.  This code was
 * broken, however, since it would fail to cope with a string containing an
 * embedded '"'.  I have therefore removed this special case, and instead
 * introduced rules for using backslashes to quote characters.  Higher-level
 * code should add backslashes to a string constant to ensure it is treated
 * as a single token.
 */
char *
lsptok(char *string, int *length)
{
	static char *saved_str = NULL;
	char	   *local_str;		/* working pointer to string */
	char	   *ret_str;		/* start of token to return */

	if (string != NULL)
	{
		saved_str = string;
		if (length == NULL)
			return NULL;
	}

	local_str = saved_str;

	while (*local_str == ' ' || *local_str == '\n' || *local_str == '\t')
		local_str++;

	if (*local_str == '\0')
	{
		*length = 0;
		saved_str = local_str;
		return NULL;			/* no more tokens */
	}

	/*
	 * Now pointing at start of next token.
	 */
	ret_str = local_str;

	if (*local_str == '(' || *local_str == ')' ||
		*local_str == '{' || *local_str == '}')
	{
		/* special 1-character token */
		local_str++;
	}
	else
	{
		/* Normal token, possibly containing backslashes */
		while (*local_str != '\0' &&
			   *local_str != ' ' && *local_str != '\n' &&
			   *local_str != '\t' &&
			   *local_str != '(' && *local_str != ')' &&
			   *local_str != '{' && *local_str != '}')
		{
			if (*local_str == '\\' && local_str[1] != '\0')
				local_str += 2;
			else
				local_str++;
		}
	}

	*length = local_str - ret_str;

	/* Recognize special case for "empty" token */
	if (*length == 2 && ret_str[0] == '<' && ret_str[1] == '>')
		*length = 0;

	saved_str = local_str;

	return ret_str;
}

/*
 * debackslash -
 *	  create a palloc'd string holding the given token.
 *	  any protective backslashes in the token are removed.
 */
char *
debackslash(char *token, int length)
{
	char   *result = palloc(length+1);
	char   *ptr = result;

	while (length > 0)
	{
		if (*token == '\\' && length > 1)
			token++, length--;
		*ptr++ = *token++;
		length--;
	}
	*ptr = '\0';
	return result;
}

#define RIGHT_PAREN (1000000 + 1)
#define LEFT_PAREN	(1000000 + 2)
#define PLAN_SYM	(1000000 + 3)
#define AT_SYMBOL	(1000000 + 4)
#define ATOM_TOKEN	(1000000 + 5)

/*
 * nodeTokenType -
 *	  returns the type of the node token contained in token.
 *	  It returns one of the following valid NodeTags:
 *		T_Integer, T_Float, T_String
 *	  and some of its own:
 *		RIGHT_PAREN, LEFT_PAREN, PLAN_SYM, AT_SYMBOL, ATOM_TOKEN
 *
 *	  Assumption: the ascii representation is legal
 */
static NodeTag
nodeTokenType(char *token, int length)
{
	NodeTag		retval;

	/*
	 * Check if the token is a number (decimal or integer, positive or
	 * negative)
	 */
	if (isdigit(*token) ||
		(length >= 2 && *token == '-' && isdigit(token[1])))
	{
		/*
		 * skip the optional '-' (i.e. negative number)
		 */
		if (*token == '-')
			token++, length--;

		/*
		 * See if there is a decimal point
		 */
		while (length > 0 && *token != '.')
			token++, length--;

		/*
		 * if there isn't, token's an int, otherwise it's a float.
		 */
		retval = (*token != '.') ? T_Integer : T_Float;
	}
	/*
	 * these three cases do not need length checks, since lsptok()
	 * will always treat them as single-byte tokens
	 */
	else if (*token == '(')
		retval = LEFT_PAREN;
	else if (*token == ')')
		retval = RIGHT_PAREN;
	else if (*token == '{')
		retval = PLAN_SYM;
	else if (*token == '@' && length == 1)
		retval = AT_SYMBOL;
	else if (*token == '\"' && length > 1 && token[length-1] == '\"')
		retval = T_String;
	else
		retval = ATOM_TOKEN;
	return retval;
}

/*
 * nodeRead -
 *	  Slightly higher-level reader.
 *
 * This routine applies some semantic knowledge on top of the purely
 * lexical tokenizer lsptok().  It can read
 *	* Value token nodes (integers, floats, or strings);
 *  * Plan nodes (via parsePlanString() from readfuncs.c);
 *  * Lists of the above.
 *
 * Secrets:  He assumes that lsptok already has the string (see above).
 * Any callers should set read_car_only to true.
 */
void *
nodeRead(bool read_car_only)
{
	char	   *token;
	int			tok_len;
	NodeTag		type;
	Node	   *this_value,
			   *return_value;
	bool		make_dotted_pair_cell = false;

	token = lsptok(NULL, &tok_len);

	if (token == NULL)
		return NULL;

	type = nodeTokenType(token, tok_len);

	switch (type)
	{
		case PLAN_SYM:
			this_value = parsePlanString();
			token = lsptok(NULL, &tok_len);
			if (token[0] != '}')
				elog(ERROR, "nodeRead: did not find '}' at end of plan node");
			if (!read_car_only)
				make_dotted_pair_cell = true;
			else
				make_dotted_pair_cell = false;
			break;
		case LEFT_PAREN:
			if (!read_car_only)
			{
				List	   *l = makeNode(List);

				lfirst(l) = nodeRead(false);
				lnext(l) = nodeRead(false);
				this_value = (Node *) l;
			}
			else
				this_value = nodeRead(false);
			break;
		case RIGHT_PAREN:
			this_value = NULL;
			break;
		case AT_SYMBOL:
			this_value = NULL;
			break;
		case ATOM_TOKEN:
			if (tok_len == 0)
			{
				/* must be "<>" */
				this_value = NULL;
				/*
				 * It might be NULL but it is an atom!
				 */
				if (read_car_only)
					make_dotted_pair_cell = false;
				else
					make_dotted_pair_cell = true;
			}
			else
			{
				/* !attention! result is not a Node.  Use with caution. */
				this_value = (Node *) debackslash(token, tok_len);
				make_dotted_pair_cell = true;
			}
			break;
		case T_Float:
			/* we know that the token terminates on a char atof will stop at */
			this_value = (Node *) makeFloat(atof(token));
			make_dotted_pair_cell = true;
			break;
		case T_Integer:
			/* we know that the token terminates on a char atoi will stop at */
			this_value = (Node *) makeInteger(atoi(token));
			make_dotted_pair_cell = true;
			break;
		case T_String:
			this_value = (Node *) makeString(debackslash(token+1, tok_len-2));
			make_dotted_pair_cell = true;
			break;
		default:
			elog(ERROR, "nodeRead: Bad type %d", type);
			this_value = NULL;	/* keep compiler happy */
			break;
	}
	if (make_dotted_pair_cell)
	{
		List	   *l = makeNode(List);

		lfirst(l) = this_value;

		if (!read_car_only)
			lnext(l) = nodeRead(false);
		else
			lnext(l) = NULL;
		return_value = (Node *) l;
	}
	else
		return_value = this_value;
	return return_value;
}
