/*-------------------------------------------------------------------------
 * 
 * sysfunc.c--
 *    process system functions and return a string result
 * 
 * Notes:
 * 1) I return a string result because most of the functions cannot return any
 *    normal type anyway (e.g. SYS_DATE, SYS_TIME, etc...), and the few that
 *    might (SYS_UID or whatever) can just return it as a string - no problem.
 *    This keeps the function flexible enough to be of good use.
 * 
 * Written by Chad Robinson, chadr@brttech.com
 * Last modified: 04/27/1996
 * -------------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 * Can't get much more obvious than this.  Might need to replace localtime()
 * on older systems...
 */
char *Sysfunc_system_date(void)
{
	time_t	cur_time_secs;
	struct	tm *cur_time_expanded;
	static	char buf[12]; /* Just for safety, y'understand... */
	
	time(&cur_time_secs);
	cur_time_expanded = localtime(&cur_time_secs);
#if defined(EUROPEAN_DATES) || defined(EUROPEAN_STYLE)
	sprintf(buf, "%02.2d-%02.2d-%04.4d", cur_time_expanded->tm_mday,
			cur_time_expanded->tm_mon+1, cur_time_expanded->tm_year+1900);
#else
	sprintf(buf, "%02.2d-%02.2d-%04.4d", cur_time_expanded->tm_mon+1,
			cur_time_expanded->tm_mday, cur_time_expanded->tm_year+1900);
#endif
	return &buf[0];
}

char *SystemFunctionHandler(char *funct)
{
	if (!strcmp(funct, "SYS_DATE"))
		return Sysfunc_system_date();
	return "*unknown function*";
}

#ifdef SYSFUNC_TEST
/*
 * Chad's rule of coding #4 - never delete a test function, even a stupid
 * one - you always need it 10 minutes after you delete it.
 */
void main(void)
{
	printf("Current system date: %s\n", SystemFunctionHandler("SYS_DATE"));
	return;
}
#endif
