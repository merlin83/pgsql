/*-------------------------------------------------------------------------
 *
 * keys.h
 *	  prototypes for keys.c.
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: keys.h,v 1.15 1999-05-25 22:43:09 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef KEYS_H
#define KEYS_H

#include "nodes/nodes.h"
#include "nodes/relation.h"

extern bool match_indexkey_operand(int indexkey, Var *operand, RelOptInfo *rel);
extern Var *extract_join_key(JoinKey *jk, int outer_or_inner);
extern bool pathkeys_match(List *keys1, List *keys2, int *better_key);
extern List *collect_index_pathkeys(int *index_keys, List *tlist);

#endif	 /* KEYS_H */
