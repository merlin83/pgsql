/*
 * guc.h
 *
 * External declarations pertaining to backend/utils/misc/guc.c and
 * backend/utils/misc/guc-file.l
 *
 * $Id: guc.h,v 1.20 2002-07-30 16:20:03 momjian Exp $
 */
#ifndef GUC_H
#define GUC_H

#include "nodes/pg_list.h"
#include "utils/array.h"


/*
 * Certain options can only be set at certain times. The rules are
 * like this:
 *
 * POSTMASTER options can only be set when the postmaster starts,
 * either from the configuration file or the command line.
 *
 * SIGHUP options can only be set at postmaster startup or by changing
 * the configuration file and sending the HUP signal to the postmaster
 * or a backend process. (Notice that the signal receipt will not be
 * evaluated immediately. The postmaster and the backend block at a
 * certain point in their main loop. It's safer to wait than to read a
 * file asynchronously.)
 *
 * BACKEND options can only be set at postmaster startup, from the
 * configuration file, or with the PGOPTIONS variable from the client
 * when the connection is initiated.  Furthermore, an already-started
 * backend will ignore changes to such an option in the configuration
 * file.  The idea is that these options are fixed for a given backend
 * once it's started, but they can vary across backends.
 *
 * SUSET options can be set at postmaster startup, with the SIGHUP
 * mechanism, or from SQL if you're a superuser. These options cannot
 * be set using the PGOPTIONS mechanism, because there is not check as
 * to who does this.
 *
 * USERSET options can be set by anyone any time.
 */
typedef enum
{
	PGC_POSTMASTER,
	PGC_SIGHUP,
	PGC_BACKEND,
	PGC_SUSET,
	PGC_USERSET
} GucContext;

/*
 * The following type records the source of the current setting.  A
 * new setting can only take effect if the previous setting had the
 * same or lower level.  (E.g, changing the config file doesn't
 * override the postmaster command line.)  Tracking the source allows us
 * to process sources in any convenient order without affecting results.
 * Sources <= PGC_S_OVERRIDE will set the default used by RESET, as well
 * as the current value.
 */
typedef enum
{
	PGC_S_DEFAULT = 0,			/* wired-in default */
	PGC_S_ENV_VAR = 1,			/* postmaster environment variable */
	PGC_S_FILE = 2,				/* postgresql.conf */
	PGC_S_ARGV = 3,				/* postmaster command line */
	PGC_S_DATABASE = 4,			/* per-database setting */
	PGC_S_USER = 5,				/* per-user setting */
	PGC_S_CLIENT = 6,			/* from client (PGOPTIONS) */
	PGC_S_OVERRIDE = 7,			/* special case to forcibly set default */
	PGC_S_SESSION = 8			/* SET command */
} GucSource;

extern void SetConfigOption(const char *name, const char *value,
				GucContext context, GucSource source);
extern const char *GetConfigOption(const char *name);
extern const char *GetConfigOptionResetString(const char *name);
extern void ProcessConfigFile(GucContext context);
extern void InitializeGUCOptions(void);
extern void ResetAllOptions(void);
extern void AtEOXact_GUC(bool isCommit);
extern void ParseLongOption(const char *string, char **name, char **value);
extern bool set_config_option(const char *name, const char *value,
							  GucContext context, GucSource source,
							  bool isLocal, bool DoIt);
extern void ShowGUCConfigOption(const char *name);
extern void ShowAllGUCConfig(void);
extern char *GetConfigOptionByName(const char *name, const char **varname);
extern char *GetConfigOptionByNum(int varnum, const char **varname, bool *noshow);
extern int GetNumConfigOptions(void);

extern void SetPGVariable(const char *name, List *args, bool is_local);
extern void GetPGVariable(const char *name);
extern void ResetPGVariable(const char *name);

extern char *flatten_set_variable_args(const char *name, List *args);

extern void ProcessGUCArray(ArrayType *array, GucSource source);
extern ArrayType *GUCArrayAdd(ArrayType *array, const char *name, const char *value);
extern ArrayType *GUCArrayDelete(ArrayType *array, const char *name);

extern bool Debug_print_query;
extern bool Debug_print_plan;
extern bool Debug_print_parse;
extern bool Debug_print_rewritten;
extern bool Debug_pretty_print;

extern bool Show_parser_stats;
extern bool Show_planner_stats;
extern bool Show_executor_stats;
extern bool Show_query_stats;
extern bool Show_btree_build_stats;

extern bool Explain_pretty_print;

extern bool SQL_inheritance;
extern bool Australian_timezones;

#endif   /* GUC_H */
