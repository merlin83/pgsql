/*
 * variable.h
 *		Routines for handling specialized SET variables.
 *
 * Portions Copyright (c) 1996-2005, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $PostgreSQL: pgsql/src/include/commands/variable.h,v 1.27 2005/10/15 02:49:44 momjian Exp $
 */
#ifndef VARIABLE_H
#define VARIABLE_H

#include "utils/guc.h"


extern const char *assign_datestyle(const char *value,
				 bool doit, GucSource source);
extern const char *assign_timezone(const char *value,
				bool doit, GucSource source);
extern const char *show_timezone(void);
extern const char *assign_XactIsoLevel(const char *value,
					bool doit, GucSource source);
extern const char *show_XactIsoLevel(void);
extern bool assign_random_seed(double value,
				   bool doit, GucSource source);
extern const char *show_random_seed(void);
extern const char *assign_client_encoding(const char *value,
					   bool doit, GucSource source);
extern const char *assign_role(const char *value,
			bool doit, GucSource source);
extern const char *show_role(void);
extern const char *assign_session_authorization(const char *value,
							 bool doit, GucSource source);
extern const char *show_session_authorization(void);

#endif   /* VARIABLE_H */
