/*-------------------------------------------------------------------------
 *
 * geqo_mutation.h--
 *    prototypes for mutation functions in optimizer/geqo
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: geqo_mutation.h,v 1.1 1997-02-19 12:58:49 scrappy Exp $
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

#ifndef	GEQO_MUTATION_H
#define	GEQO_MUTATION_H


extern void geqo_mutation (Gene *tour, int num_gene);

#endif  /* GEQO_MUTATION_H */
