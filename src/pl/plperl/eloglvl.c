#include "postgres.h"

#include "utils/elog.h"

/*
 * This kludge is necessary because of the conflicting
 * definitions of 'DEBUG' between postgres and perl.
 * we'll live.
 */

#include "eloglvl.h"

int
elog_DEBUG(void)
{
	return DEBUG1;
}

int
elog_LOG(void)
{
	return LOG;
}

int
elog_INFO(void)
{
	return INFO;
}

int
elog_NOTICE(void)
{
	return NOTICE;
}

int
elog_WARNING(void)
{
	return WARNING;
}

int
elog_ERROR(void)
{
	return ERROR;
}

