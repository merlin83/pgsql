/*-------------------------------------------------------------------------
 *
 * itemid.h
 *	  Standard POSTGRES buffer page item identifier definitions.
 *
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: itemid.h,v 1.11 2000-08-07 20:15:50 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef ITEMID_H
#define ITEMID_H


/*
 * An item pointer (also called line pointer) on a buffer page
 */
typedef struct ItemIdData
{								/* line pointers */
	unsigned	lp_off:15,		/* offset to start of tuple */
				lp_flags:2,		/* flags for tuple */
				lp_len:15;		/* length of tuple */
} ItemIdData;

typedef ItemIdData *ItemId;

/*
 * lp_flags contains these flags:
 */
#define LP_USED			0x01	/* this line pointer is being used */
/* currently, there is one unused flag bit ... */


/*
 * Item offsets, lengths, and flags are represented by these types when
 * they're not actually stored in an ItemIdData.
 */
typedef uint16 ItemOffset;
typedef uint16 ItemLength;

typedef bits16 ItemIdFlags;


/* ----------------
 *		support macros
 * ----------------
 */

/*
 *		ItemIdGetLength
 */
#define ItemIdGetLength(itemId) \
   ((itemId)->lp_len)

/*
 *		ItemIdGetOffset
 */
#define ItemIdGetOffset(itemId) \
   ((itemId)->lp_off)

/*
 *		ItemIdGetFlags
 */
#define ItemIdGetFlags(itemId) \
   ((itemId)->lp_flags)

/*
 * ItemIdIsValid
 *		True iff disk item identifier is valid.
 */
#define ItemIdIsValid(itemId)	PointerIsValid(itemId)

/*
 * ItemIdIsUsed
 *		True iff disk item identifier is in use.
 *
 * Note:
 *		Assumes disk item identifier is valid.
 */
#define ItemIdIsUsed(itemId) \
( \
	AssertMacro(ItemIdIsValid(itemId)), \
	(bool) (((itemId)->lp_flags & LP_USED) != 0) \
)

#endif	 /* ITEMID_H */
