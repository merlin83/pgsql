/*-------------------------------------------------------------------------
 *
 * float.h
 *	  definitions for ANSI floating point
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: float.h,v 1.4 1999-02-13 23:17:37 momjian Exp $
 *
 * NOTES
 *	  These come straight out of ANSI X3.159-1989 (p.18) and
 *	  would be unnecessary if SunOS 4 were ANSI-compliant.
 *
 *	  This is only a partial listing because I'm lazy to type
 *	  the whole thing in.
 *
 *-------------------------------------------------------------------------
 */
#ifndef FLOAT_H
#define FLOAT_H

#define FLT_DIG 6
#define FLT_MIN ((float) 1.17549435e-38)
#define FLT_MAX ((float) 3.40282347e+38)
#define DBL_DIG 15
#define DBL_MIN 2.2250738585072014e-308
#define DBL_MAX 1.7976931348623157e+308

#endif	 /* FLOAT_H */
