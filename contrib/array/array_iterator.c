/*
 * array_iterator.c --
 *
 * This file defines a new class of operators which take an
 * array and a scalar value, iterate a scalar operator over the
 * elements of the array and the value and compute a result as
 * the logical OR or AND of the iteration results.
 *
 * Copyright (c) 1997, Massimo Dal Zotto <dz@cs.unitn.it>
 */

#include <ctype.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>

#include "postgres.h"
#include "miscadmin.h"
#include "access/xact.h"
#include "backend/fmgr.h"
#include "catalog/pg_type.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/memutils.h"
#include "utils/syscache.h"

#include "array_iterator.h"

static int32
array_iterator(Oid elemtype, Oid proc, int and, ArrayType *array, Datum value)
{
    HeapTuple typ_tuple;
    TypeTupleForm typ_struct;
    bool typbyval;
    int typlen;
    func_ptr proc_fn;
    int pronargs;
    int nitems, i, result;
    int ndim, *dim;
    char *p;

    /* Sanity checks */
    if ((array == (ArrayType *) NULL)
	|| (ARR_IS_LO(array) == true)) {
	/* elog(NOTICE, "array_iterator: array is null"); */
        return (0);
    }
    ndim = ARR_NDIM(array);
    dim = ARR_DIMS(array);
    nitems = getNitems(ndim, dim);
    if (nitems == 0) {
	/* elog(NOTICE, "array_iterator: nitems = 0"); */
        return (0);
    }

    /* Lookup element type information */
    typ_tuple = SearchSysCacheTuple(TYPOID, ObjectIdGetDatum(elemtype),0,0,0);
    if (!HeapTupleIsValid(typ_tuple)) {
        elog(WARN,"array_iterator: cache lookup failed for type %d", elemtype);
        return 0;
    }
    typ_struct = (TypeTupleForm) GETSTRUCT(typ_tuple);
    typlen   = typ_struct->typlen;
    typbyval = typ_struct->typbyval;

    /* Lookup the function entry point */
    proc_fn = (func_ptr) NULL;
    fmgr_info(proc, &proc_fn, &pronargs);
    if ((proc_fn == NULL) || (pronargs != 2)) {
	elog(WARN, "array_iterator: fmgr_info lookup failed for oid %d", proc);
        return (0);
    }

    /* Scan the array and apply the operator to each element */
    result = 0;
    p = ARR_DATA_PTR(array);
    for (i = 0; i < nitems; i++) {
        if (typbyval) {
            switch(typlen) {
	      case 1:
		result = (int) (*proc_fn)(*p, value);
		break;
	    case 2:
		result = (int) (*proc_fn)(* (int16 *) p, value);
		break;
	    case 3:
	    case 4:
		result = (int) (*proc_fn)(* (int32 *) p, value);
		break;
            }
            p += typlen;
        } else {
	    result = (int) (*proc_fn)(p, value);
            if (typlen > 0) {
		p += typlen;
	    } else {
                p += INTALIGN(* (int32 *) p);
	    }
        }
	if (result) {
	    if (!and) {
		return (1);
	    }
	} else {
	    if (and) {
		return (0);
	    }
	}
    }

    if (and && result) {
	return (1);
    } else {
	return (0);
    }
}

/*
 * Iterator functions for type _text
 */

int32
array_texteq(ArrayType *array, char* value)
{
    return array_iterator((Oid) 25,	/* text */
			  (Oid) 67,	/* texteq */
			  0,		/* logical or */
			  array, (Datum)value);
}

int32
array_all_texteq(ArrayType *array, char* value)
{
    return array_iterator((Oid) 25,	/* text */
			  (Oid) 67,	/* texteq */
			  1,		/* logical and */
			  array, (Datum)value);
}

int32
array_textregexeq(ArrayType *array, char* value)
{
    return array_iterator((Oid) 25,	/* text */
			  (Oid) 1254,	/* textregexeq */
			  0,		/* logical or */
			  array, (Datum)value);
}

int32
array_all_textregexeq(ArrayType *array, char* value)
{
    return array_iterator((Oid) 25,	/* text */
			  (Oid) 1254,	/* textregexeq */
			  1,		/* logical and */
			  array, (Datum)value);
}

/*
 * Iterator functions for type _char16. Note that the regexp
 * operators take the second argument of type text.
 */

int32
array_char16eq(ArrayType *array, char* value)
{
    return array_iterator((Oid) 20,	/* char16 */
			  (Oid) 1275,	/* char16eq */
			  0,		/* logical or */
			  array, (Datum)value);
}

int32
array_all_char16eq(ArrayType *array, char* value)
{
    return array_iterator((Oid) 20,	/* char16 */
			  (Oid) 1275,	/* char16eq */
			  1,		/* logical and */
			  array, (Datum)value);
}

int32
array_char16regexeq(ArrayType *array, char* value)
{
    return array_iterator((Oid) 20,	/* char16 */
			  (Oid) 1288,	/* char16regexeq */
			  0,		/* logical or */
			  array, (Datum)value);
}

int32
array_all_char16regexeq(ArrayType *array, char* value)
{
    return array_iterator((Oid) 20,	/* char16 */
			  (Oid) 1288,	/* char16regexeq */
			  1,		/* logical and */
			  array, (Datum)value);
}

/*
 * Iterator functions for type _int4
 */

int32
array_int4eq(ArrayType *array, int4 value)
{
    return array_iterator((Oid) 23,	/* int4 */
			  (Oid) 65,	/* int4eq */
			  0,		/* logical or */
			  array, (Datum)value);
}

int32
array_all_int4eq(ArrayType *array, int4 value)
{
    return array_iterator((Oid) 23,	/* int4 */
			  (Oid) 65,	/* int4eq */
			  1,		/* logical and */
			  array, (Datum)value);
}

int32
array_int4ne(ArrayType *array, int4 value)
{
    return array_iterator((Oid) 23,	/* int4 */
			  (Oid) 144,	/* int4ne */
			  0,		/* logical or */
			  array, (Datum)value);
}

int32
array_all_int4ne(ArrayType *array, int4 value)
{
    return array_iterator((Oid) 23,	/* int4 */
			  (Oid) 144,	/* int4ne */
			  1,		/* logical and */
			  array, (Datum)value);
}

int32
array_int4gt(ArrayType *array, int4 value)
{
    return array_iterator((Oid) 23,	/* int4 */
			  (Oid) 147,	/* int4gt */
			  0,		/* logical or */
			  array, (Datum)value);
}

int32
array_all_int4gt(ArrayType *array, int4 value)
{
    return array_iterator((Oid) 23,	/* int4 */
			  (Oid) 147,	/* int4gt */
			  1,		/* logical and */
			  array, (Datum)value);
}

int32
array_int4ge(ArrayType *array, int4 value)
{
    return array_iterator((Oid) 23,	/* int4 */
			  (Oid) 150,	/* int4ge */
			  0,		/* logical or */
			  array, (Datum)value);
}

int32
array_all_int4ge(ArrayType *array, int4 value)
{
    return array_iterator((Oid) 23,	/* int4 */
			  (Oid) 150,	/* int4ge */
			  1,		/* logical and */
			  array, (Datum)value);
}

int32
array_int4lt(ArrayType *array, int4 value)
{
    return array_iterator((Oid) 23,	/* int4 */
			  (Oid) 66,	/* int4lt */
			  0,		/* logical or */
			  array, (Datum)value);
}

int32
array_all_int4lt(ArrayType *array, int4 value)
{
    return array_iterator((Oid) 23,	/* int4 */
			  (Oid) 66,	/* int4lt */
			  1,		/* logical and */
			  array, (Datum)value);
}

int32
array_int4le(ArrayType *array, int4 value)
{
    return array_iterator((Oid) 23,	/* int4 */
			  (Oid) 149,	/* int4le */
			  0,		/* logical or */
			  array, (Datum)value);
}

int32
array_all_int4le(ArrayType *array, int4 value)
{
    return array_iterator((Oid) 23,	/* int4 */
			  (Oid) 149,	/* int4le */
			  1,		/* logical and */
			  array, (Datum)value);
}

/* end of file */
