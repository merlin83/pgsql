/*-------------------------------------------------------------------------
 *
 * execdefs.h
 *
 *
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: execdefs.h,v 1.6 2000-01-26 05:58:05 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef EXECDEFS_H
#define EXECDEFS_H

/* ----------------
 *		ExecutePlan() tuplecount definitions
 * ----------------
 */
#define ALL_TUPLES				0		/* return all tuples */
#define ONE_TUPLE				1		/* return only one tuple */

/* ----------------
 *		constants used by ExecMain
 * ----------------
 */
#define EXEC_RUN						3
#define EXEC_FOR						4
#define EXEC_BACK						5
#define EXEC_RETONE						6
#define EXEC_RESULT						7

/* ----------------
 *		Merge Join states
 * ----------------
 */
#define EXEC_MJ_INITIALIZE				1
#define EXEC_MJ_JOINMARK				2
#define EXEC_MJ_JOINTEST				3
#define EXEC_MJ_JOINTUPLES				4
#define EXEC_MJ_NEXTOUTER				5
#define EXEC_MJ_TESTOUTER				6
#define EXEC_MJ_NEXTINNER				7
#define EXEC_MJ_SKIPINNER				8
#define EXEC_MJ_SKIPOUTER				9
#define EXEC_MJ_FILLINNER			   10
#define EXEC_MJ_FILLOUTER			   11

#endif	 /* EXECDEFS_H */
