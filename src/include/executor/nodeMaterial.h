/*-------------------------------------------------------------------------
 *
 * nodeMaterial.h--
 *
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: nodeMaterial.h,v 1.5 1997-11-26 01:12:53 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef NODEMATERIAL_H
#define NODEMATERIAL_H

#include "executor/tuptable.h"
#include "nodes/execnodes.h"
#include "nodes/plannodes.h"

extern TupleTableSlot *ExecMaterial(Material *node);
extern bool ExecInitMaterial(Material *node, EState *estate, Plan *parent);
extern int	ExecCountSlotsMaterial(Material *node);
extern void ExecEndMaterial(Material *node);
extern List ExecMaterialMarkPos(Material *node);
extern void ExecMaterialRestrPos(Material *node);

#endif							/* NODEMATERIAL_H */
