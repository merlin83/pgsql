/*-------------------------------------------------------------------------
 *
 * geqo_gene.h--
 *	  genome representation in optimizer/geqo
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: geqo_gene.h,v 1.2 1997-09-07 04:58:59 momjian Exp $
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


#ifndef GEQO_GENE_H
#define GEQO_GENE_H


/* we presume that int instead of Relid
   is o.k. for Gene; so don't change it! */
typedef
int				Gene;

typedef struct Chromosome
{
	Gene		   *string;
	Cost			worth;
}				Chromosome;

typedef struct Pool
{
	Chromosome	   *data;
	int				size;
	int				string_length;
}				Pool;

#endif							/* GEQO_GENE_H */
