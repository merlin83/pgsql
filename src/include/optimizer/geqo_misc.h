/*-------------------------------------------------------------------------
 *
 * geqo_misc.h--
 *	  prototypes for printout routines in optimizer/geqo
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: geqo_misc.h,v 1.3 1997-09-08 02:37:46 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */

/* contributed by:
   =*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
   *  Martin Utesch				 * Institute of Automatic Control	   *
   =							 = University of Mining and Technology =
   *  utesch@aut.tu-freiberg.de  * Freiberg, Germany				   *
   =*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
 */

#ifndef GEQO_MISC_H
#define GEQO_MISC_H

#include <stdio.h>

extern void print_pool(FILE * fp, Pool * pool, int start, int stop);
extern void print_gen(FILE * fp, Pool * pool, int generation);
extern void print_edge_table(FILE * fp, Edge * edge_table, int num_gene);

extern void geqo_print_rel(Query * root, Rel * rel);
extern void geqo_print_path(Query * root, Path * path, int indent);
extern void geqo_print_joinclauses(Query * root, List * clauses);

#endif							/* GEQO_MISC_H */
