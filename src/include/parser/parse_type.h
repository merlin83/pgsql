/*-------------------------------------------------------------------------
 *
 * parse_type.h
 *
 *
 *
 * Portions Copyright (c) 1996-2005, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $PostgreSQL: pgsql/src/include/parser/parse_type.h,v 1.29 2004/12/31 22:03:38 pgsql Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef PARSE_TYPE_H
#define PARSE_TYPE_H

#include "access/htup.h"
#include "parser/parse_node.h"


typedef HeapTuple Type;

extern Oid	LookupTypeName(const TypeName *typename);
extern char *TypeNameToString(const TypeName *typename);
extern Oid	typenameTypeId(const TypeName *typename);
extern Type typenameType(const TypeName *typename);

extern bool typeidIsValid(Oid id);
extern Type typeidType(Oid id);

extern Oid	typeTypeId(Type tp);
extern int16 typeLen(Type t);
extern bool typeByVal(Type t);
extern char typeTypType(Type t);
extern char *typeTypeName(Type t);
extern Oid	typeTypeRelid(Type typ);
extern Datum stringTypeDatum(Type tp, char *string, int32 atttypmod);

extern Oid	typeidTypeRelid(Oid type_id);

extern void parseTypeString(const char *str, Oid *type_id, int32 *typmod);

#define ISCOMPLEX(typeid) (typeidTypeRelid(typeid) != InvalidOid)

#endif   /* PARSE_TYPE_H */
