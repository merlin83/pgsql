/*
 * psql - the PostgreSQL interactive terminal
 *
 * Copyright 2000 by PostgreSQL Global Development Group
 *
 * $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/bin/psql/variables.h,v 1.9 2001-10-25 05:49:54 momjian Exp $
 */

/*
 * This implements a sort of variable repository. One could also think of it
 * as cheap version of an associative array. In each one of these
 * datastructures you can store name/value pairs.
 */

#ifndef VARIABLES_H
#define VARIABLES_H

#define VALID_VARIABLE_CHARS "abcdefghijklmnopqrstuvwxyz"\
							 "ABCDEFGHIJKLMNOPQRSTUVWXYZ" "0123456789_"

struct _variable
{
	char	   *name;
	char	   *value;
	struct _variable *next;
};

typedef struct _variable *VariableSpace;


VariableSpace CreateVariableSpace(void);
const char *GetVariable(VariableSpace space, const char *name);
bool		GetVariableBool(VariableSpace space, const char *name);
bool		SetVariable(VariableSpace space, const char *name, const char *value);
bool		SetVariableBool(VariableSpace space, const char *name);
bool		DeleteVariable(VariableSpace space, const char *name);
void		DestroyVariableSpace(VariableSpace space);

#endif	 /* VARIABLES_H */
