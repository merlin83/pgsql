/*-------------------------------------------------------------------------
 *
 * sequence.h--
 *	  prototypes for sequence.c.
 *
 *
 *-------------------------------------------------------------------------
 */
#ifndef SEQUENCE_H
#define SEQUENCE_H

/*
 * Columns of a sequnece relation
 */

#define SEQ_COL_NAME			1
#define SEQ_COL_LASTVAL			2
#define SEQ_COL_INCBY			3
#define SEQ_COL_MAXVALUE		4
#define SEQ_COL_MINVALUE		5
#define SEQ_COL_CACHE			6
#define SEQ_COL_CYCLE			7
#define SEQ_COL_CALLED			8

#define SEQ_COL_FIRSTCOL		SEQ_COL_NAME
#define SEQ_COL_LASTCOL			SEQ_COL_CALLED

extern void DefineSequence(CreateSeqStmt *stmt);
extern int4 nextval(struct varlena * seqname);
extern int4 currval(struct varlena * seqname);
extern void CloseSequences(void);

#endif							/* SEQUENCE_H */
