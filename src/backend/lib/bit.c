/*-------------------------------------------------------------------------
 *
 * bit.c
 *	  Standard bit array code.
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/lib/Attic/bit.c,v 1.6 1999-02-13 23:15:32 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */

/*
 * utils/memutils.h contains declarations of the functions in this file
 */
#include <postgres.h>

#include <utils/bit.h>
#include <utils/memutils.h>

void
BitArraySetBit(BitArray bitArray, BitIndex bitIndex)
{
	bitArray[bitIndex / BitsPerByte]
	|= (1 << (BitsPerByte - (bitIndex % BitsPerByte) - 1));
	return;
}

void
BitArrayClearBit(BitArray bitArray, BitIndex bitIndex)
{
	bitArray[bitIndex / BitsPerByte]
	&= ~(1 << (BitsPerByte - (bitIndex % BitsPerByte) - 1));
	return;
}

bool
BitArrayBitIsSet(BitArray bitArray, BitIndex bitIndex)
{
	return ((bool) (((bitArray[bitIndex / BitsPerByte] &
					  (1 << (BitsPerByte - (bitIndex % BitsPerByte)
							 - 1)
					   )
					  ) != 0) ? 1 : 0));
}
