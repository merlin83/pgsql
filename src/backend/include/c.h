/*-------------------------------------------------------------------------
 *
 * c.h--
 *    Fundamental C definitions.  This is included by every .c file in
 *    postgres.
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: c.h,v 1.1.1.1 1996-07-09 06:21:28 scrappy Exp $
 *
 *-------------------------------------------------------------------------
 */
/*
 *   TABLE OF CONTENTS
 *
 *	When adding stuff to this file, please try and put stuff
 *	into the relevant section, or add new sections as appropriate.
 *
 *    section	description
 *    -------	------------------------------------------------
 *      1)      bool, true, false, TRUE, FALSE
 *	2)	__STDC__, non-ansi C definitions:
 *		Pointer typedef, NULL
 *		cpp magic macros
 *		type prefixes: const, signed, volatile, inline
 *	3)	standard system types
 *      4)      datum type
 *	5)	IsValid macros for system types
 *	6)	offsetof, lengthof, endof 
 *	7)	exception handling definitions, Assert, Trap, etc macros
 *	8)	Min, Max, Abs macros
 *	9)	externs
 *      10)      Berkeley-specific defs
 *	11)	system-specific hacks
 *
 *   NOTES
 *
 *	This file is MACHINE AND COMPILER dependent!!!  (For now.)
 *
 * ----------------------------------------------------------------
 */
#ifndef	C_H
#define C_H

/* ----------------------------------------------------------------
 *		Section 1:  bool, true, false, TRUE, FALSE
 * ----------------------------------------------------------------
 */
/*
 * bool --
 *	Boolean value, either true or false.
 *
 */
#define false	((char) 0)
#define true	((char) 1)
typedef char	bool;
typedef bool	*BoolPtr;

#ifndef TRUE
#define TRUE	1
#endif /* TRUE */

#ifndef FALSE
#define FALSE	0
#endif /* FALSE */

/* ----------------------------------------------------------------
 *		Section 2: __STDC__, non-ansi C definitions:
 *
 *		cpp magic macros
 *		Pointer typedef, NULL
 *		type prefixes: const, signed, volatile, inline
 * ----------------------------------------------------------------
 */

#ifdef	__STDC__ /* ANSI C */

/*
 * Pointer --
 *	Variable holding address of any memory resident object.
 */

/*
 *	XXX Pointer arithmetic is done with this, so it can't be void *
 *	under "true" ANSI compilers.
 */
typedef char *Pointer;

#ifndef	NULL
/*
 * NULL --
 *	Null pointer.
 */
#define NULL	((void *) 0)
#endif	/* !defined(NULL) */

#define	HAVE_ANSI_CPP	/* all ANSI C compilers must have this! */
#if defined(NEED_STD_HDRS)
#undef NEED_STD_HDRS	/* all ANSI systems must have stddef/stdlib */
#endif /* NEED_STD_HDRS */

#else	/* !defined(__STDC__) */ /* NOT ANSI C */

/*
 * Pointer --
 *	Variable containing address of any memory resident object.
 */
typedef char	*Pointer;

#ifndef	NULL
/*
 * NULL --
 *	Null pointer.
 */
#define NULL	0
#endif	/* !defined(NULL) */

/*
 * const --
 *	Type modifier.  Identifies read only variables.
 *
 * Example:
 *	extern const Version	RomVersion;
 */
#define const		/* const */

/*
 * signed --
 *	Type modifier.  Identifies signed integral types.
 */
#define signed		/* signed */

/*
 * volatile --
 *	Type modifier.  Identifies variables which may change in ways not
 *	noticeable by the compiler, e.g. via asynchronous interrupts.
 *
 * Example:
 *	extern volatile unsigned int	NumberOfInterrupts;
 */
#define volatile	/* volatile */

#endif	/* !defined(__STDC__) */ /* NOT ANSI C */

/*
 * CppAsString --
 *	Convert the argument to a string, using the C preprocessor.
 * CppConcat --
 *	Concatenate two arguments together, using the C preprocessor.
 */
#if defined(HAVE_ANSI_CPP)

#define CppAsString(identifier)	#identifier
#define CppConcat(x, y)		x##y
#define CppConcat0(x, y)	x##y
#define CppConcat1(x, y)	x##y
#define CppConcat2(x, y)	x##y
#define CppConcat3(x, y)	x##y
#define CppConcat4(x, y)	x##y

#else /* !HAVE_ANSI_CPP */

#define CppAsString(identifier)	"identifier"

/*
 * CppIdentity -- On Reiser based cpp's this is used to concatenate
 *	two tokens.  That is
 *		CppIdentity(A)B	==> AB
 *	We renamed it to _private_CppIdentity because it should not
 *	be referenced outside this file.  On other cpp's it
 *	produces  A  B.
 */
#define _priv_CppIdentity(x)x
#define CppConcat(x, y)		_priv_CppIdentity(x)y
#define CppConcat0(x, y)	_priv_CppIdentity(x)y
#define CppConcat1(x, y)	_priv_CppIdentity(x)y
#define CppConcat2(x, y)	_priv_CppIdentity(x)y
#define CppConcat3(x, y)	_priv_CppIdentity(x)y
#define CppConcat4(x, y)	_priv_CppIdentity(x)y

#endif /* !HAVE_ANSI_CPP */

#ifndef __GNUC__	/* GNU cc */
# define inline
#endif

#if defined(NEED_STD_HDRS)
/*
 * You're doomed.  We've removed almost all of our own C library 
 * extern declarations because they conflict on the different
 * systems.  You'll have to write your own stdlib.h.
 */
#include "stdlib.h"
#else /* NEED_STD_HDRS */
#include <stddef.h>
#include <stdlib.h>
#endif /* NEED_STD_HDRS */

/* ----------------------------------------------------------------
 *		Section 3:  standard system types
 * ----------------------------------------------------------------
 */

/*
 * intN --
 *	Signed integer, AT LEAST N BITS IN SIZE,
 *	used for numerical computations.
 */
typedef signed char	int8;		/* >= 8 bits */
typedef signed short	int16;		/* >= 16 bits */
typedef signed int	int32;		/* >= 32 bits */

/*
 * uintN --
 *	Unsigned integer, AT LEAST N BITS IN SIZE,
 *	used for numerical computations.
 */
typedef unsigned char	uint8;		/* >= 8 bits */
typedef unsigned short	uint16;		/* >= 16 bits */
typedef unsigned int	uint32;		/* >= 32 bits */

/*
 * floatN --
 *	Floating point number, AT LEAST N BITS IN SIZE,
 *	used for numerical computations.
 *
 *	Since sizeof(floatN) may be > sizeof(char *), always pass
 *	floatN by reference.
 */
typedef float		float32data;
typedef double		float64data;
typedef float		*float32;
typedef double		*float64;

/*
 * boolN --
 *	Boolean value, AT LEAST N BITS IN SIZE.
 */
typedef uint8		bool8;		/* >= 8 bits */
typedef uint16		bool16;		/* >= 16 bits */
typedef uint32		bool32;		/* >= 32 bits */

/*
 * bitsN --
 *	Unit of bitwise operation, AT LEAST N BITS IN SIZE.
 */
typedef uint8		bits8;		/* >= 8 bits */
typedef uint16		bits16;		/* >= 16 bits */
typedef uint32		bits32;		/* >= 32 bits */

/*
 * wordN --
 *	Unit of storage, AT LEAST N BITS IN SIZE,
 *	used to fetch/store data.
 */
typedef uint8		word8;		/* >= 8 bits */
typedef uint16		word16;		/* >= 16 bits */
typedef uint32		word32;		/* >= 32 bits */

/*
 * Size --
 *	Size of any memory resident object, as returned by sizeof.
 */
typedef unsigned int	Size;

/*
 * Index --
 *	Index into any memory resident array.
 *
 * Note:
 *	Indices are non negative.
 */
typedef unsigned int	Index;

#define MAXDIM 6
typedef struct {
	int indx[MAXDIM];
} IntArray;

/*
 * Offset --
 *	Offset into any memory resident array.
 *
 * Note:
 *	This differs from an Index in that an Index is always
 *	non negative, whereas Offset may be negative.
 */
typedef signed int	Offset;

/* ----------------------------------------------------------------
 *	 	Section 4:  datum type + support macros
 * ----------------------------------------------------------------
 */
/*
 * datum.h --
 *	POSTGRES abstract data type datum representation definitions.
 *
 * Note:
 *
 * Port Notes:
 *  Postgres makes the following assumption about machines:
 *
 *  sizeof(Datum) == sizeof(long) >= sizeof(void *) >= 4
 *
 *  Postgres also assumes that
 *
 *  sizeof(char) == 1
 *
 *  and that 
 *
 *  sizeof(short) == 2
 *
 *  If your machine meets these requirements, Datums should also be checked
 *  to see if the positioning is correct.
 *
 *	This file is MACHINE AND COMPILER dependent!!!
 */

typedef unsigned long Datum;	/* XXX sizeof(long) >= sizeof(void *) */
typedef Datum *       DatumPtr;

#define GET_1_BYTE(datum)   (((Datum) (datum)) & 0x000000ff)
#define GET_2_BYTES(datum)  (((Datum) (datum)) & 0x0000ffff)
#define GET_4_BYTES(datum)  (((Datum) (datum)) & 0xffffffff)
#define SET_1_BYTE(value)   (((Datum) (value)) & 0x000000ff)
#define SET_2_BYTES(value)  (((Datum) (value)) & 0x0000ffff)
#define SET_4_BYTES(value)  (((Datum) (value)) & 0xffffffff)

/*
 * DatumGetChar --
 *	Returns character value of a datum.
 */

#define DatumGetChar(X) ((char) GET_1_BYTE(X))

/*
 * CharGetDatum --
 *	Returns datum representation for a character.
 */

#define CharGetDatum(X) ((Datum) SET_1_BYTE(X))

/*
 * Int8GetDatum --
 *	Returns datum representation for an 8-bit integer.
 */

#define Int8GetDatum(X) ((Datum) SET_1_BYTE(X))

/*
 * DatumGetUInt8 --
 *	Returns 8-bit unsigned integer value of a datum.
 */

#define DatumGetUInt8(X) ((uint8) GET_1_BYTE(X))

/*
 * UInt8GetDatum --
 *	Returns datum representation for an 8-bit unsigned integer.
 */

#define UInt8GetDatum(X) ((Datum) SET_1_BYTE(X))

/*
 * DatumGetInt16 --
 *	Returns 16-bit integer value of a datum.
 */

#define DatumGetInt16(X) ((int16) GET_2_BYTES(X))

/*
 * Int16GetDatum --
 *	Returns datum representation for a 16-bit integer.
 */

#define Int16GetDatum(X) ((Datum) SET_2_BYTES(X))

/*
 * DatumGetUInt16 --
 *	Returns 16-bit unsigned integer value of a datum.
 */

#define DatumGetUInt16(X) ((uint16) GET_2_BYTES(X))

/*
 * UInt16GetDatum --
 *	Returns datum representation for a 16-bit unsigned integer.
 */

#define UInt16GetDatum(X) ((Datum) SET_2_BYTES(X))

/*
 * DatumGetInt32 --
 *	Returns 32-bit integer value of a datum.
 */

#define DatumGetInt32(X) ((int32) GET_4_BYTES(X))

/*
 * Int32GetDatum --
 *	Returns datum representation for a 32-bit integer.
 */

#define Int32GetDatum(X) ((Datum) SET_4_BYTES(X))

/*
 * DatumGetUInt32 --
 *	Returns 32-bit unsigned integer value of a datum.
 */

#define DatumGetUInt32(X) ((uint32) GET_4_BYTES(X))

/*
 * UInt32GetDatum --
 *	Returns datum representation for a 32-bit unsigned integer.
 */

#define UInt32GetDatum(X) ((Datum) SET_4_BYTES(X))

/*
 * DatumGetObjectId --
 *	Returns object identifier value of a datum.
 */

#define DatumGetObjectId(X) ((Oid) GET_4_BYTES(X))

/*
 * ObjectIdGetDatum --
 *	Returns datum representation for an object identifier.
 */

#define ObjectIdGetDatum(X) ((Datum) SET_4_BYTES(X))

/*
 * DatumGetPointer --
 *	Returns pointer value of a datum.
 */

#define DatumGetPointer(X) ((Pointer) X)

/*
 * PointerGetDatum --
 *	Returns datum representation for a pointer.
 */

#define PointerGetDatum(X) ((Datum) X)

/*
 * DatumGetName --
 *	Returns name value of a datum.
 */

#define DatumGetName(X) ((Name) DatumGetPointer((Datum) X))

/*
 * NameGetDatum --
 *	Returns datum representation for a name.
 */

#define NameGetDatum(X) PointerGetDatum((Pointer) X)


/*
 * DatumGetFloat32 --
 *	Returns 32-bit floating point value of a datum.
 *	This is really a pointer, of course.
 */

#define DatumGetFloat32(X) ((float32) DatumGetPointer((Datum) X))

/*
 * Float32GetDatum --
 *	Returns datum representation for a 32-bit floating point number.
 *	This is really a pointer, of course.
 */

#define Float32GetDatum(X) PointerGetDatum((Pointer) X)

/*
 * DatumGetFloat64 --
 *	Returns 64-bit floating point value of a datum.
 *	This is really a pointer, of course.
 */

#define DatumGetFloat64(X) ((float64) DatumGetPointer(X))

/*
 * Float64GetDatum --
 *	Returns datum representation for a 64-bit floating point number.
 *	This is really a pointer, of course.
 */

#define Float64GetDatum(X) PointerGetDatum((Pointer) X)

/* ----------------------------------------------------------------
 *		Section 5:  IsValid macros for system types
 * ----------------------------------------------------------------
 */
/*
 * BoolIsValid --
 *	True iff bool is valid.
 */
#define	BoolIsValid(boolean)	((boolean) == false || (boolean) == true)

/*
 * PointerIsValid --
 *	True iff pointer is valid.
 */
#define PointerIsValid(pointer)	(bool)((void*)(pointer) != NULL)

/*
 * PointerIsInBounds --
 *	True iff pointer is within given bounds.
 *
 * Note:
 *	Assumes the bounded interval to be [min,max),
 *	i.e. closed on the left and open on the right.
 */
#define PointerIsInBounds(pointer, min, max) \
	((min) <= (pointer) && (pointer) < (max))

/*
 * PointerIsAligned --
 *	True iff pointer is properly aligned to point to the given type.
 */
#define PointerIsAligned(pointer, type)	\
	(((long)(pointer) % (sizeof (type))) == 0)

/* ----------------------------------------------------------------
 *		Section 6:  offsetof, lengthof, endof
 * ----------------------------------------------------------------
 */
/*
 * offsetof --
 *	Offset of a structure/union field within that structure/union.
 *
 *	XXX This is supposed to be part of stddef.h, but isn't on
 *	some systems (like SunOS 4).
 */
#ifndef offsetof
#define offsetof(type, field)	((long) &((type *)0)->field)
#endif /* offsetof */

/*
 * lengthof --
 *	Number of elements in an array.
 */
#define lengthof(array)	(sizeof (array) / sizeof ((array)[0]))

/*
 * endof --
 *	Address of the element one past the last in an array.
 */
#define endof(array)	(&array[lengthof(array)])

/* ----------------------------------------------------------------
 *		Section 7:  exception handling definitions
 *			    Assert, Trap, etc macros
 * ----------------------------------------------------------------
 */
/*
 * Exception Handling definitions
 */

typedef char *ExcMessage;
typedef struct Exception {
	ExcMessage	message;
} Exception;

/*
 * NO_ASSERT_CHECKING, if defined, turns off all the assertions.
 * - plai  9/5/90
 *
 * It should _NOT_ be undef'ed in releases or in benchmark copies
 * 
 * #undef NO_ASSERT_CHECKING
 */

/*
 * Trap --
 *	Generates an exception if the given condition is true.
 *
 */
#define Trap(condition, exception) \
	{ if (condition) \
		ExceptionalCondition(CppAsString(condition), &(exception), \
			(char*)NULL, __FILE__, __LINE__); }

/*    
 *  TrapMacro is the same as Trap but it's intended for use in macros:
 *
 *	#define foo(x) (AssertM(x != 0) && bar(x))
 *
 *  Isn't CPP fun?
 */
#define TrapMacro(condition, exception) \
    ((bool) ((! condition) || \
	     (ExceptionalCondition(CppAsString(condition), \
				  &(exception), \
				  (char*) NULL, __FILE__, __LINE__))))
    
#ifdef NO_ASSERT_CHECKING
#define Assert(condition)
#define AssertMacro(condition)	true
#define AssertArg(condition)
#define AssertState(condition)
#else
#define Assert(condition) \
	Trap(!(condition), FailedAssertion)

#define AssertMacro(condition) \
	TrapMacro(!(condition), FailedAssertion)

#define AssertArg(condition) \
	Trap(!(condition), BadArg)

#define AssertState(condition) \
	Trap(!(condition), BadState)

#endif   /* NO_ASSERT_CHECKING */

/*
 * LogTrap --
 *	Generates an exception with a message if the given condition is true.
 *
 */
#define LogTrap(condition, exception, printArgs) \
	{ if (condition) \
		ExceptionalCondition(CppAsString(condition), &(exception), \
			form printArgs, __FILE__, __LINE__); }

/*    
 *  LogTrapMacro is the same as LogTrap but it's intended for use in macros:
 *
 *	#define foo(x) (LogAssertMacro(x != 0, "yow!") && bar(x))
 */
#define LogTrapMacro(condition, exception, printArgs) \
    ((bool) ((! condition) || \
	     (ExceptionalCondition(CppAsString(condition), \
				   &(exception), \
				   form printArgs, __FILE__, __LINE__))))
    
#ifdef NO_ASSERT_CHECKING
#define LogAssert(condition, printArgs)
#define LogAssertMacro(condition, printArgs) true
#define LogAssertArg(condition, printArgs)
#define LogAssertState(condition, printArgs)
#else
#define LogAssert(condition, printArgs) \
	LogTrap(!(condition), FailedAssertion, printArgs)

#define LogAssertMacro(condition, printArgs) \
	LogTrapMacro(!(condition), FailedAssertion, printArgs)

#define LogAssertArg(condition, printArgs) \
	LogTrap(!(condition), BadArg, printArgs)

#define LogAssertState(condition, printArgs) \
	LogTrap(!(condition), BadState, printArgs)

#endif   /* NO_ASSERT_CHECKING */

/* ----------------------------------------------------------------
 *		Section 8:  Min, Max, Abs macros
 * ----------------------------------------------------------------
 */
/*
 * Max --
 *	Return the maximum of two numbers.
 */
#define Max(x, y)	((x) > (y) ? (x) : (y))

/*
 * Min --
 *	Return the minimum of two numbers.
 */
#define Min(x, y)	((x) < (y) ? (x) : (y))

/*
 * Abs --
 *	Return the absolute value of the argument.
 */
#define Abs(x)		((x) >= 0 ? (x) : -(x))

/* ----------------------------------------------------------------
 *		Section 9: externs
 * ----------------------------------------------------------------
 */

extern Exception	FailedAssertion;
extern Exception	BadArg;
extern Exception	BadState;

/* in utils/error/assert.c */
extern int ExceptionalCondition(char *conditionName,
				Exception *exceptionP, char *details,
				char *fileName, int lineNumber);


/* ----------------
 *	form is used by assert and the exception handling stuff
 * ----------------
 */
extern char *form(char *fmt, ...);



/* ----------------------------------------------------------------
 *		Section 10: berkeley-specific configuration
 *
 * this section contains settings which are only relevant to the UC Berkeley
 * sites.  Other sites can ignore this
 * ----------------------------------------------------------------
 */

/* ----------------
 *	storage managers
 *
 *	These are experimental and are not supported in the code that
 *	we distribute to other sites.
 * ----------------
 */
#ifdef SEQUOIA
#define MAIN_MEMORY
#endif



/* ----------------------------------------------------------------
 *		Section 11: system-specific hacks
 *
 *	This should be limited to things that absolutely have to be
 *	included in every source file.  The changes should be factored
 *	into a separate file so that changes to one port don't require
 *	changes to c.h (and everyone recompiling their whole system).
 * ----------------------------------------------------------------
 */

#if defined(PORTNAME_hpux) 
#include "port/hpux/fixade.h"		/* for 8.07 unaligned access fixup */
#endif /* PORTNAME_hpux */

#if defined(PORTNAME_sparc)
#define	memmove(d, s, l)	bcopy(s, d, l)
#endif

/* These are for things that are one way on Unix and another on NT */
#ifndef WIN32
#define	NULL_DEV	"/dev/null"
#define COPY_CMD	"cp"
#define SEP_CHAR	'/'
#else
#define NULL_DEV	"NUL"
#define COPY_CMD	"copy"
#define SEP_CHAR 	'\\'
#endif /* WIN32 */

#if defined(WIN32)
#include "port/win32/nt.h"
#include "port/win32/machine.h"
#endif /* WIN32 */

/* ----------------
 *	end of c.h
 * ----------------
 */
#endif	/* C_H */
