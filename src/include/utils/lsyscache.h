/*-------------------------------------------------------------------------
 *
 * lsyscache.h
 *	  Convenience routines for common queries in the system catalog cache.
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: lsyscache.h,v 1.22 2000-01-26 05:58:38 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef LSYSCACHE_H
#define LSYSCACHE_H

#include "access/htup.h"

extern bool op_class(Oid opid, Oid opclass, Oid amopid);
extern char *get_attname(Oid relid, AttrNumber attnum);
extern AttrNumber get_attnum(Oid relid, char *attname);
extern Oid	get_atttype(Oid relid, AttrNumber attnum);
extern bool get_attisset(Oid relid, char *attname);
extern int32 get_atttypmod(Oid relid, AttrNumber attnum);
extern double get_attdisbursion(Oid relid, AttrNumber attnum,
								double min_estimate);
extern RegProcedure get_opcode(Oid opid);
extern char *get_opname(Oid opid);
extern bool op_mergejoinable(Oid opid, Oid ltype, Oid rtype,
				 Oid *leftOp, Oid *rightOp);
extern Oid	op_hashjoinable(Oid opid, Oid ltype, Oid rtype);
extern Oid	get_commutator(Oid opid);
extern HeapTuple get_operator_tuple(Oid opno);
extern Oid	get_negator(Oid opid);
extern RegProcedure get_oprrest(Oid opid);
extern RegProcedure get_oprjoin(Oid opid);
extern Oid	get_func_rettype(Oid funcid);
extern int	get_relnatts(Oid relid);
extern char *get_rel_name(Oid relid);
extern struct varlena *get_relstub(Oid relid, int no, bool *islast);
extern Oid	get_ruleid(char *rulename);
extern Oid	get_eventrelid(Oid ruleid);
extern int16 get_typlen(Oid typid);
extern bool get_typbyval(Oid typid);
extern Datum get_typdefault(Oid typid);

#endif	 /* LSYSCACHE_H */
