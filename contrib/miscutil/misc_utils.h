#ifndef MISC_UTILS_H
#define MISC_UTILS_H

int			query_limit(int limit);
int			backend_pid(void);
int			unlisten(char *relname);
int			max(int x, int y);
int			min(int x, int y);
int			assert_enable(int val);

#ifdef ASSERT_CHECKING_TEST
int			assert_test(int val);
#endif

int			active_listeners(text *relname);
#endif

/*
 * Local Variables:
 *  tab-width: 4
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 */
