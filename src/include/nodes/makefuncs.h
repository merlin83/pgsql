/*-------------------------------------------------------------------------
 *
 * makefuncs.h--
 *	  prototypes for the creator functions (for primitive nodes)
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: makefuncs.h,v 1.9 1998-02-10 16:04:24 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef MAKEFUNC_H
#define MAKEFUNC_H

#include <nodes/primnodes.h>
#include <utils/fcache.h>

extern Oper * makeOper(Oid opno,
		 Oid opid,
		 Oid opresulttype,
		 int opsize,
		 FunctionCachePtr op_fcache);

extern Var * makeVar(Index varno,
		AttrNumber varattno,
		Oid vartype,
		int16 vartypmod,
		Index varlevelsup,
		Index varnoold,
		AttrNumber varoattno);

extern Resdom * makeResdom(AttrNumber resno,
		   Oid restype,
		   int16 restypmod,
		   char *resname,
		   Index reskey,
		   Oid reskeyop,
		   int resjunk);

extern Const * makeConst(Oid consttype,
		  Size constlen,
		  Datum constvalue,
		  bool constisnull,
		  bool constbyval,
		  bool constisset,
		  bool constiscast);

#endif							/* MAKEFUNC_H */
