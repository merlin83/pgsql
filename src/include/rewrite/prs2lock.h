/*-------------------------------------------------------------------------
 *
 * prs2lock.h--
 *    data structures for POSTGRES Rule System II (rewrite rules only)
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: prs2lock.h,v 1.2 1996-10-19 04:25:53 scrappy Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef PRS2LOCK_H
#define PRS2LOCK_H

#include "nodes/nodes.h"

/*
 * RewriteRule -
 *    holds a info for a rewrite rule
 *
 */
typedef struct RewriteRule {
    Oid			ruleId;
    CmdType		event;
    AttrNumber		attrno;
    Node		*qual;
    List		*actions;
    bool		isInstead;
} RewriteRule;

/*
 * RuleLock -
 *    all rules that apply to a particular relation. Even though we only
 *    have the rewrite rule system left and these are not really "locks",
 *    the name is kept for historical reasons.
 */
typedef struct RuleLock {
    int			numLocks;
    RewriteRule		**rules;
} RuleLock;

#endif	/* REWRITE_H */
