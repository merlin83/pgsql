/*-------------------------------------------------------------------------
 *
 * typecmds.h
 *	  prototypes for typecmds.c.
 *
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: typecmds.h,v 1.2 2002-12-10 16:12:53 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef TYPECMDS_H
#define TYPECMDS_H

#include "nodes/parsenodes.h"

#define DEFAULT_TYPDELIM		','

extern void DefineType(List *names, List *parameters);
extern void RemoveType(List *names, DropBehavior behavior);
extern void RemoveTypeById(Oid typeOid);
extern void DefineDomain(CreateDomainStmt *stmt);
extern void RemoveDomain(List *names, DropBehavior behavior);
extern Oid	DefineCompositeType(const RangeVar *typevar, List *coldeflist);

extern void AlterDomainDefault(List *names, Node *defaultRaw);
extern void AlterDomainNotNull(List *names, bool notNull);
extern void AlterDomainAddConstraint(List *names, Node *constr);
extern void AlterDomainDropConstraint(List *names, const char *constrName,
									  DropBehavior behavior);

#endif   /* TYPECMDS_H */
