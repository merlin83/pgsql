/*
 * rmgr.c
 *
 * Resource managers definition
 *
 * $PostgreSQL: pgsql/src/backend/access/transam/rmgr.c,v 1.16 2004/08/29 21:08:47 tgl Exp $
 */
#include "postgres.h"

#include "access/clog.h"
#include "access/gist.h"
#include "access/hash.h"
#include "access/heapam.h"
#include "access/nbtree.h"
#include "access/rtree.h"
#include "access/xact.h"
#include "access/xlog_internal.h"
#include "commands/dbcommands.h"
#include "commands/sequence.h"
#include "commands/tablespace.h"
#include "storage/smgr.h"


const RmgrData RmgrTable[RM_MAX_ID + 1] = {
	{"XLOG", xlog_redo, xlog_undo, xlog_desc, NULL, NULL},
	{"Transaction", xact_redo, xact_undo, xact_desc, NULL, NULL},
	{"Storage", smgr_redo, smgr_undo, smgr_desc, NULL, NULL},
	{"CLOG", clog_redo, clog_undo, clog_desc, NULL, NULL},
	{"Database", dbase_redo, dbase_undo, dbase_desc, NULL, NULL},
	{"Tablespace", tblspc_redo, tblspc_undo, tblspc_desc, NULL, NULL},
	{"Reserved 6", NULL, NULL, NULL, NULL, NULL},
	{"Reserved 7", NULL, NULL, NULL, NULL, NULL},
	{"Reserved 8", NULL, NULL, NULL, NULL, NULL},
	{"Reserved 9", NULL, NULL, NULL, NULL, NULL},
	{"Heap", heap_redo, heap_undo, heap_desc, NULL, NULL},
	{"Btree", btree_redo, btree_undo, btree_desc,
	btree_xlog_startup, btree_xlog_cleanup},
	{"Hash", hash_redo, hash_undo, hash_desc, NULL, NULL},
	{"Rtree", rtree_redo, rtree_undo, rtree_desc, NULL, NULL},
	{"Gist", gist_redo, gist_undo, gist_desc, NULL, NULL},
	{"Sequence", seq_redo, seq_undo, seq_desc, NULL, NULL}
};
