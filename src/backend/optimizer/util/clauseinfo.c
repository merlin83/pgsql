/*-------------------------------------------------------------------------
 *
 * clauseinfo.c--
 *    ClauseInfo node manipulation routines.
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *    $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/optimizer/util/Attic/clauseinfo.c,v 1.2 1996-07-22 23:30:57 scrappy Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "nodes/relation.h"
#include "nodes/nodeFuncs.h"

#include "optimizer/internal.h"
#include "optimizer/clauses.h"
#include "optimizer/clauseinfo.h"

/*    
 * valid-or-clause--
 *    
 * Returns t iff the clauseinfo node contains a 'normal' 'or' clause.
 *    
 */
bool
valid_or_clause(CInfo *clauseinfo)
{
     if (clauseinfo != NULL && 
	 !single_node((Node*)clauseinfo->clause) && 
	 !clauseinfo->notclause &&
	 or_clause((Node*)clauseinfo->clause)) 
       return(true);
     else
       return(false);
}

/*    
 * get-actual-clauses--
 *    
 * Returns a list containing the clauses from 'clauseinfo-list'.
 *    
 */
List *
get_actual_clauses(List *clauseinfo_list)
{
    List *temp = NIL;
    List *result = NIL;
    CInfo *clause = (CInfo *)NULL;

    foreach(temp,clauseinfo_list) {
	clause = (CInfo *)lfirst(temp);
	result = lappend(result,clause->clause);
    }
    return(result);
}

/*    
 * XXX NOTE:
 *  	The following routines must return their contents in the same order
 *    	(e.g., the first clause's info should be first, and so on) or else
 *    	get_index_sel() won't work.
 *    
 */

/*    
 * get_relattvals--
 *    For each member of  a list of clauseinfo nodes to be used with an
 *    index, create a vectori-long specifying:
 *    		the attnos,
 *    		the values of the clause constants, and 
 *    		flags indicating the type and location of the constant within
 *    			each clause.
 *    Each clause is of the form (op var some_type_of_constant), thus the
 *    flag indicating whether the constant is on the left or right should
 *    always be *SELEC-CONSTANT-RIGHT*.
 *    
 * 'clauseinfo-list' is a list of clauseinfo nodes
 *    
 * Returns a list of vectori-longs.
 *    
 */
void
get_relattvals(List *clauseinfo_list,
	       List **attnos,
	       List **values,
	       List **flags)
{
    List *result1 = NIL;
    List *result2 = NIL;
    List *result3 = NIL;
    CInfo *temp = (CInfo *)NULL;
    List *i = NIL;

    foreach (i,clauseinfo_list) {
	int dummy;
	AttrNumber attno;
	Datum constval;
	int flag;

	temp = (CInfo *)lfirst(i);
	get_relattval((Node*)temp->clause, &dummy, &attno, &constval, &flag);
	result1 = lappendi(result1, attno);
	result2 = lappendi(result2, constval);
	result3 = lappendi(result3, flag);
    }

    *attnos = result1;
    *values = result2;
    *flags = result3;
    return;
}

/*    
 * get_joinvars --
 *    Given a list of join clauseinfo nodes to be used with the index
 *    of an inner join relation, return three lists consisting of:
 *    		the attributes corresponding to the inner join relation
 *    		the value of the inner var clause (always "")
 *    		whether the attribute appears on the left or right side of
 *    			the operator.
 *    
 * 'relid' is the inner join relation
 * 'clauseinfo-list' is a list of qualification clauses to be used with
 * 	'rel'
 *    
 */
void
get_joinvars(Oid relid,
	     List *clauseinfo_list,
	     List **attnos,
	     List **values,
	     List **flags)
{
    List *result1 = NIL;
    List *result2 = NIL;
    List *result3 = NIL;
    List *temp;
     
    foreach(temp, clauseinfo_list) {
	CInfo *clauseinfo = lfirst(temp);
	Expr *clause = clauseinfo->clause;

	if( IsA (get_leftop(clause),Var) &&
	   (relid == (get_leftop(clause))->varno)) {
	    result1 = lappendi(result1, (int4)(get_leftop(clause))->varattno);
	    result2 = lappend(result2, "");
	    result3 = lappendi(result3, _SELEC_CONSTANT_RIGHT_);
	} else {
	    result1 = lappendi(result1, (int4)(get_rightop(clause))->varattno);
	    result2 = lappend(result2, "");
	    result3 = lappendi(result3, _SELEC_CONSTANT_LEFT_);
	}
    }
    *attnos = result1;
    *values = result2;
    *flags = result3;
    return;
}

/*    
 * get_opnos--
 *    Create and return a list containing the clause operators of each member
 *    of a list of clauseinfo nodes to be used with an index.
 *    
 */
List *
get_opnos(List *clauseinfo_list)
{
    CInfo *temp = (CInfo *)NULL;
    List *result = NIL;
    List *i = NIL;

    foreach(i,clauseinfo_list) {
	temp = (CInfo *)lfirst(i);
	result =
	    lappendi(result,
		     (((Oper*)temp->clause->oper)->opno));
    }
    return(result);
}


