/*-------------------------------------------------------------------------
 *
 * parse_coerce.h
 *
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: parse_coerce.h,v 1.1 1998-05-09 23:31:34 thomas Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef PARSE_COERCE_H
#define PARSE_COERCE_H

typedef enum CATEGORY {
	INVALID_TYPE,
	UNKNOWN_TYPE,
	BOOLEAN_TYPE,
	STRING_TYPE,
	NUMERIC_TYPE,
	DATETIME_TYPE,
	TIMESPAN_TYPE,
	GEOMETRIC_TYPE,
	USER_TYPE,
	MIXED_TYPE
} CATEGORY;


#define IS_BUILTIN_TYPE(t) \
		  (((t) == BOOLOID) \
		|| ((t) == BPCHAROID) \
		|| ((t) == VARCHAROID) \
		|| ((t) == TEXTOID) \
		|| ((t) == CASHOID) \
		|| ((t) == INT4OID) \
		|| ((t) == DATETIMEOID) \
		|| ((t) == FLOAT8OID) \
		|| ((t) == ABSTIMEOID) \
		|| ((t) == TIMESTAMPOID) \
		|| ((t) == RELTIMEOID))


/* IS_BINARY_COMPATIBLE()
 * Check for types with the same underlying binary representation.
 * This allows us to cheat and directly exchange values without
 *  going through the trouble of calling a conversion function.
 */
#define IS_BINARY_COMPATIBLE(a,b) \
		  (((a) == BPCHAROID && (b) == TEXTOID) \
		|| ((a) == BPCHAROID && (b) == VARCHAROID) \
		|| ((a) == VARCHAROID && (b) == TEXTOID) \
		|| ((a) == VARCHAROID && (b) == BPCHAROID) \
		|| ((a) == TEXTOID && (b) == BPCHAROID) \
		|| ((a) == TEXTOID && (b) == VARCHAROID) \
		|| ((a) == CASHOID && (b) == INT4OID) \
		|| ((a) == INT4OID && (b) == CASHOID) \
		|| ((a) == DATETIMEOID && (b) == FLOAT8OID) \
		|| ((a) == FLOAT8OID && (b) == DATETIMEOID) \
		|| ((a) == ABSTIMEOID && (b) == TIMESTAMPOID) \
		|| ((a) == TIMESTAMPOID && (b) == ABSTIMEOID) \
		|| ((a) == ABSTIMEOID && (b) == INT4OID) \
		|| ((a) == INT4OID && (b) == ABSTIMEOID) \
		|| ((a) == RELTIMEOID && (b) == INT4OID) \
		|| ((a) == INT4OID && (b) == RELTIMEOID))

/* IS_HIGHER_TYPE()
 * These types are the most general in each of the type categories.
 */
#define IS_HIGHER_TYPE(t) \
		  (((t) == TEXTOID) \
		|| ((t) == FLOAT8OID) \
		|| ((t) == TIMESPANOID) \
		|| ((t) == DATETIMEOID) \
		|| ((t) == POLYGONOID))

/* IS_HIGHEST_TYPE()
 * These types are the most general in each of the type categories.
 * Since timespan and datetime overload so many functions, let's
 *  give datetime the preference.
 * Since text is a generic string type let's leave it out too.
 */
#define IS_HIGHEST_TYPE(t) \
		  (((t) == FLOAT8OID) \
		|| ((t) == DATETIMEOID) \
		|| ((t) == TIMESPANOID))


extern bool IsPreferredType(CATEGORY category, Oid type);
extern Oid PreferredType(CATEGORY category, Oid type);
extern CATEGORY TypeCategory(Oid type);

extern bool can_coerce_type(int nargs, Oid *input_typeids, Oid *func_typeids);
extern Node *coerce_type(ParseState *pstate, Node *node, Oid inputTypeId, Oid targetTypeId);

#endif							/* PARSE_COERCE_H */
