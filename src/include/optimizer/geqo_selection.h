/*-------------------------------------------------------------------------
 *
 * geqo_selection.h--
 *    prototypes for selection routines in optimizer/geqo
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: geqo_selection.h,v 1.1 1997-02-19 12:59:07 scrappy Exp $
 *
 *-------------------------------------------------------------------------
 */

/* contributed by:
   =*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
   *  Martin Utesch              * Institute of Automatic Control      *
   =                             = University of Mining and Technology =
   *  utesch@aut.tu-freiberg.de  * Freiberg, Germany                   *
   =*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
 */


#ifndef	GEQO_SELECTION_H
#define	GEQO_SELECTION_H


extern void geqo_selection (Chromosome *momma, Chromosome *daddy, Pool *pool, double bias);

#endif  /* GEQO_SELECTION_H */
