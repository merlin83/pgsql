/*
 * variable.h
 *		Routines for handling specialized SET variables.
 *
 * $Id: variable.h,v 1.19 2002-09-04 20:31:42 momjian Exp $
 *
 */
#ifndef VARIABLE_H
#define VARIABLE_H

extern const char *assign_datestyle(const char *value,
				 bool doit, bool interactive);
extern const char *show_datestyle(void);
extern const char *assign_timezone(const char *value,
				bool doit, bool interactive);
extern const char *show_timezone(void);
extern const char *assign_XactIsoLevel(const char *value,
					bool doit, bool interactive);
extern const char *show_XactIsoLevel(void);
extern bool assign_random_seed(double value,
				   bool doit, bool interactive);
extern const char *show_random_seed(void);
extern const char *assign_client_encoding(const char *value,
					   bool doit, bool interactive);
extern const char *assign_server_encoding(const char *value,
					   bool doit, bool interactive);
extern const char *show_server_encoding(void);
extern const char *assign_session_authorization(const char *value,
							 bool doit, bool interactive);
extern const char *show_session_authorization(void);

#endif   /* VARIABLE_H */
