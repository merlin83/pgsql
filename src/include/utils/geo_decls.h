/*-------------------------------------------------------------------------
 *
 * geo_decls.h - Declarations for various 2D constructs.
 *
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: geo_decls.h,v 1.30 2000-07-29 18:46:05 tgl Exp $
 *
 * NOTE
 *	  These routines do *not* use the float types from adt/.
 *
 *	  XXX These routines were not written by a numerical analyst.
 *
 *	  XXX I have made some attempt to flesh out the operators
 *		and data types. There are still some more to do. - tgl 97/04/19
 *
 *-------------------------------------------------------------------------
 */
#ifndef GEO_DECLS_H
#define GEO_DECLS_H

#include "access/attnum.h"
#include "fmgr.h"

/*--------------------------------------------------------------------
 * Useful floating point utilities and constants.
 *-------------------------------------------------------------------*/


#define EPSILON					1.0E-06

#ifdef EPSILON
#define FPzero(A)				(fabs(A) <= EPSILON)
#define FPeq(A,B)				(fabs((A) - (B)) <= EPSILON)
#define FPlt(A,B)				((B) - (A) > EPSILON)
#define FPle(A,B)				((A) - (B) <= EPSILON)
#define FPgt(A,B)				((A) - (B) > EPSILON)
#define FPge(A,B)				((B) - (A) <= EPSILON)
#else
#define FPzero(A)				(A == 0)
#define FPnzero(A)				(A != 0)
#define FPeq(A,B)				(A == B)
#define FPne(A,B)				(A != B)
#define FPlt(A,B)				(A < B)
#define FPle(A,B)				(A <= B)
#define FPgt(A,B)				(A > B)
#define FPge(A,B)				(A >= B)
#endif

#define HYPOT(A, B)				sqrt((A) * (A) + (B) * (B))

/*---------------------------------------------------------------------
 * Point - (x,y)
 *-------------------------------------------------------------------*/
typedef struct
{
	double		x,
				y;
} Point;


/*---------------------------------------------------------------------
 * LSEG - A straight line, specified by endpoints.
 *-------------------------------------------------------------------*/
typedef struct
{
	Point		p[2];

	double		m;				/* precomputed to save time, not in tuple */
} LSEG;


/*---------------------------------------------------------------------
 * PATH - Specified by vertex points.
 *-------------------------------------------------------------------*/
typedef struct
{
	int32		size;			/* XXX varlena */
	int32		npts;
	int32		closed;			/* is this a closed polygon? */
	int32		dummy;			/* padding to make it double align */
	Point		p[1];			/* variable length array of POINTs */
} PATH;


/*---------------------------------------------------------------------
 * LINE - Specified by its general equation (Ax+By+C=0).
 *		If there is a y-intercept, it is C, which
 *		 incidentally gives a freebie point on the line
 *		 (if B=0, then C is the x-intercept).
 *		Slope m is precalculated to save time; if
 *		 the line is not vertical, m == A.
 *-------------------------------------------------------------------*/
typedef struct
{
	double		A,
				B,
				C;

	double		m;
} LINE;


/*---------------------------------------------------------------------
 * BOX	- Specified by two corner points, which are
 *		 sorted to save calculation time later.
 *-------------------------------------------------------------------*/
typedef struct
{
	Point		high,
				low;			/* corner POINTs */
} BOX;

/*---------------------------------------------------------------------
 * POLYGON - Specified by an array of doubles defining the points,
 *		keeping the number of points and the bounding box for
 *		speed purposes.
 *-------------------------------------------------------------------*/
typedef struct
{
	int32		size;			/* XXX varlena */
	int32		npts;
	BOX			boundbox;
	Point		p[1];			/* variable length array of POINTs */
} POLYGON;

/*---------------------------------------------------------------------
 * CIRCLE - Specified by a center point and radius.
 *-------------------------------------------------------------------*/
typedef struct
{
	Point		center;
	double		radius;
} CIRCLE;

/*
 * fmgr interface macros
 *
 * Path and Polygon are toastable varlena types, the others are just
 * fixed-size pass-by-reference types.
 */

#define DatumGetPointP(X)    ((Point *) DatumGetPointer(X))
#define PointPGetDatum(X)    PointerGetDatum(X)
#define PG_GETARG_POINT_P(n) DatumGetPointP(PG_GETARG_DATUM(n))
#define PG_RETURN_POINT_P(x) return PointPGetDatum(x)

#define DatumGetLsegP(X)    ((LSEG *) DatumGetPointer(X))
#define LsegPGetDatum(X)    PointerGetDatum(X)
#define PG_GETARG_LSEG_P(n) DatumGetLsegP(PG_GETARG_DATUM(n))
#define PG_RETURN_LSEG_P(x) return LsegPGetDatum(x)

#define DatumGetPathP(X)         ((PATH *) PG_DETOAST_DATUM(X))
#define DatumGetPathPCopy(X)     ((PATH *) PG_DETOAST_DATUM_COPY(X))
#define PathPGetDatum(X)         PointerGetDatum(X)
#define PG_GETARG_PATH_P(n)      DatumGetPathP(PG_GETARG_DATUM(n))
#define PG_GETARG_PATH_P_COPY(n) DatumGetPathPCopy(PG_GETARG_DATUM(n))
#define PG_RETURN_PATH_P(x)      return PathPGetDatum(x)

#define DatumGetLineP(X)    ((LINE *) DatumGetPointer(X))
#define LinePGetDatum(X)    PointerGetDatum(X)
#define PG_GETARG_LINE_P(n) DatumGetLineP(PG_GETARG_DATUM(n))
#define PG_RETURN_LINE_P(x) return LinePGetDatum(x)

#define DatumGetBoxP(X)    ((BOX *) DatumGetPointer(X))
#define BoxPGetDatum(X)    PointerGetDatum(X)
#define PG_GETARG_BOX_P(n) DatumGetBoxP(PG_GETARG_DATUM(n))
#define PG_RETURN_BOX_P(x) return BoxPGetDatum(x)

#define DatumGetPolygonP(X)         ((POLYGON *) PG_DETOAST_DATUM(X))
#define DatumGetPolygonPCopy(X)     ((POLYGON *) PG_DETOAST_DATUM_COPY(X))
#define PolygonPGetDatum(X)         PointerGetDatum(X)
#define PG_GETARG_POLYGON_P(n)      DatumGetPolygonP(PG_GETARG_DATUM(n))
#define PG_GETARG_POLYGON_P_COPY(n) DatumGetPolygonPCopy(PG_GETARG_DATUM(n))
#define PG_RETURN_POLYGON_P(x)      return PolygonPGetDatum(x)

#define DatumGetCircleP(X)    ((CIRCLE *) DatumGetPointer(X))
#define CirclePGetDatum(X)    PointerGetDatum(X)
#define PG_GETARG_CIRCLE_P(n) DatumGetCircleP(PG_GETARG_DATUM(n))
#define PG_RETURN_CIRCLE_P(x) return CirclePGetDatum(x)


/*
 * in geo_ops.h
 */

/* public point routines */
extern Point *point_in(char *str);
extern char *point_out(Point *pt);
extern bool point_left(Point *pt1, Point *pt2);
extern bool point_right(Point *pt1, Point *pt2);
extern bool point_above(Point *pt1, Point *pt2);
extern bool point_below(Point *pt1, Point *pt2);
extern bool point_vert(Point *pt1, Point *pt2);
extern bool point_horiz(Point *pt1, Point *pt2);
extern bool point_eq(Point *pt1, Point *pt2);
extern bool point_ne(Point *pt1, Point *pt2);
extern int32 pointdist(Point *p1, Point *p2);
extern double *point_distance(Point *pt1, Point *pt2);
extern double *point_slope(Point *pt1, Point *pt2);

/* private routines */
extern double point_dt(Point *pt1, Point *pt2);
extern double point_sl(Point *pt1, Point *pt2);

extern Point *point(float8 *x, float8 *y);
extern Point *point_add(Point *p1, Point *p2);
extern Point *point_sub(Point *p1, Point *p2);
extern Point *point_mul(Point *p1, Point *p2);
extern Point *point_div(Point *p1, Point *p2);

/* public lseg routines */
extern LSEG *lseg_in(char *str);
extern char *lseg_out(LSEG *ls);
extern bool lseg_intersect(LSEG *l1, LSEG *l2);
extern bool lseg_parallel(LSEG *l1, LSEG *l2);
extern bool lseg_perp(LSEG *l1, LSEG *l2);
extern bool lseg_vertical(LSEG *lseg);
extern bool lseg_horizontal(LSEG *lseg);
extern bool lseg_eq(LSEG *l1, LSEG *l2);
extern bool lseg_ne(LSEG *l1, LSEG *l2);
extern bool lseg_lt(LSEG *l1, LSEG *l2);
extern bool lseg_le(LSEG *l1, LSEG *l2);
extern bool lseg_gt(LSEG *l1, LSEG *l2);
extern bool lseg_ge(LSEG *l1, LSEG *l2);
extern LSEG *lseg_construct(Point *pt1, Point *pt2);
extern double *lseg_length(LSEG *lseg);
extern double *lseg_distance(LSEG *l1, LSEG *l2);
extern Point *lseg_center(LSEG *lseg);
extern Point *lseg_interpt(LSEG *l1, LSEG *l2);
extern double *dist_pl(Point *pt, LINE *line);
extern double *dist_ps(Point *pt, LSEG *lseg);
extern Datum dist_ppath(PG_FUNCTION_ARGS);
extern double *dist_pb(Point *pt, BOX *box);
extern double *dist_sl(LSEG *lseg, LINE *line);
extern double *dist_sb(LSEG *lseg, BOX *box);
extern double *dist_lb(LINE *line, BOX *box);
extern Point *close_lseg(LSEG *l1, LSEG *l2);
extern Point *close_pl(Point *pt, LINE *line);
extern Point *close_ps(Point *pt, LSEG *lseg);
extern Point *close_pb(Point *pt, BOX *box);
extern Point *close_sl(LSEG *lseg, LINE *line);
extern Point *close_sb(LSEG *lseg, BOX *box);
extern Point *close_ls(LINE *line, LSEG *lseg);
extern Point *close_lb(LINE *line, BOX *box);
extern bool on_pl(Point *pt, LINE *line);
extern bool on_ps(Point *pt, LSEG *lseg);
extern bool on_pb(Point *pt, BOX *box);
extern Datum on_ppath(PG_FUNCTION_ARGS);
extern bool on_sl(LSEG *lseg, LINE *line);
extern bool on_sb(LSEG *lseg, BOX *box);
extern bool inter_sl(LSEG *lseg, LINE *line);
extern bool inter_sb(LSEG *lseg, BOX *box);
extern bool inter_lb(LINE *line, BOX *box);

/* private lseg routines */

/* public line routines */
extern LINE *line_in(char *str);
extern char *line_out(LINE *line);
extern Point *line_interpt(LINE *l1, LINE *l2);
extern double *line_distance(LINE *l1, LINE *l2);
extern LINE *line_construct_pp(Point *pt1, Point *pt2);
extern bool line_intersect(LINE *l1, LINE *l2);
extern bool line_parallel(LINE *l1, LINE *l2);
extern bool line_perp(LINE *l1, LINE *l2);
extern bool line_vertical(LINE *line);
extern bool line_horizontal(LINE *line);
extern bool line_eq(LINE *l1, LINE *l2);

/* private line routines */

/* public box routines */
extern BOX *box_in(char *str);
extern char *box_out(BOX *box);
extern bool box_same(BOX *box1, BOX *box2);
extern bool box_overlap(BOX *box1, BOX *box2);
extern bool box_overleft(BOX *box1, BOX *box2);
extern bool box_left(BOX *box1, BOX *box2);
extern bool box_right(BOX *box1, BOX *box2);
extern bool box_overright(BOX *box1, BOX *box2);
extern bool box_contained(BOX *box1, BOX *box2);
extern bool box_contain(BOX *box1, BOX *box2);
extern bool box_below(BOX *box1, BOX *box2);
extern bool box_above(BOX *box1, BOX *box2);
extern bool box_lt(BOX *box1, BOX *box2);
extern bool box_gt(BOX *box1, BOX *box2);
extern bool box_eq(BOX *box1, BOX *box2);
extern bool box_le(BOX *box1, BOX *box2);
extern bool box_ge(BOX *box1, BOX *box2);
extern Point *box_center(BOX *box);
extern double *box_area(BOX *box);
extern double *box_width(BOX *box);
extern double *box_height(BOX *box);
extern double *box_distance(BOX *box1, BOX *box2);
extern Point *box_center(BOX *box);
extern BOX *box_intersect(BOX *box1, BOX *box2);
extern LSEG *box_diagonal(BOX *box);
extern BOX *box(Point *p1, Point *p2);
extern BOX *box_add(BOX *box, Point *p);
extern BOX *box_sub(BOX *box, Point *p);
extern BOX *box_mul(BOX *box, Point *p);
extern BOX *box_div(BOX *box, Point *p);

/* private routines */
extern double box_dt(BOX *box1, BOX *box2);

/* public path routines */
extern Datum path_in(PG_FUNCTION_ARGS);
extern Datum path_out(PG_FUNCTION_ARGS);
extern Datum path_n_lt(PG_FUNCTION_ARGS);
extern Datum path_n_gt(PG_FUNCTION_ARGS);
extern Datum path_n_eq(PG_FUNCTION_ARGS);
extern Datum path_n_le(PG_FUNCTION_ARGS);
extern Datum path_n_ge(PG_FUNCTION_ARGS);
extern Datum path_inter(PG_FUNCTION_ARGS);
extern Datum path_distance(PG_FUNCTION_ARGS);
extern Datum path_length(PG_FUNCTION_ARGS);

extern Datum path_isclosed(PG_FUNCTION_ARGS);
extern Datum path_isopen(PG_FUNCTION_ARGS);
extern Datum path_npoints(PG_FUNCTION_ARGS);

extern Datum path_close(PG_FUNCTION_ARGS);
extern Datum path_open(PG_FUNCTION_ARGS);
extern Datum path_add(PG_FUNCTION_ARGS);
extern Datum path_add_pt(PG_FUNCTION_ARGS);
extern Datum path_sub_pt(PG_FUNCTION_ARGS);
extern Datum path_mul_pt(PG_FUNCTION_ARGS);
extern Datum path_div_pt(PG_FUNCTION_ARGS);

extern Datum path_center(PG_FUNCTION_ARGS);
extern Datum path_poly(PG_FUNCTION_ARGS);

/* public polygon routines */
extern Datum poly_in(PG_FUNCTION_ARGS);
extern Datum poly_out(PG_FUNCTION_ARGS);
extern Datum poly_left(PG_FUNCTION_ARGS);
extern Datum poly_overleft(PG_FUNCTION_ARGS);
extern Datum poly_right(PG_FUNCTION_ARGS);
extern Datum poly_overright(PG_FUNCTION_ARGS);
extern Datum poly_same(PG_FUNCTION_ARGS);
extern Datum poly_overlap(PG_FUNCTION_ARGS);
extern Datum poly_contain(PG_FUNCTION_ARGS);
extern Datum poly_contained(PG_FUNCTION_ARGS);
extern Datum poly_contain_pt(PG_FUNCTION_ARGS);
extern Datum pt_contained_poly(PG_FUNCTION_ARGS);

extern Datum poly_distance(PG_FUNCTION_ARGS);
extern Datum poly_npoints(PG_FUNCTION_ARGS);
extern Datum poly_center(PG_FUNCTION_ARGS);
extern Datum poly_box(PG_FUNCTION_ARGS);
extern Datum poly_path(PG_FUNCTION_ARGS);
extern Datum box_poly(PG_FUNCTION_ARGS);

/* public circle routines */
extern CIRCLE *circle_in(char *str);
extern char *circle_out(CIRCLE *circle);
extern bool circle_same(CIRCLE *circle1, CIRCLE *circle2);
extern bool circle_overlap(CIRCLE *circle1, CIRCLE *circle2);
extern bool circle_overleft(CIRCLE *circle1, CIRCLE *circle2);
extern bool circle_left(CIRCLE *circle1, CIRCLE *circle2);
extern bool circle_right(CIRCLE *circle1, CIRCLE *circle2);
extern bool circle_overright(CIRCLE *circle1, CIRCLE *circle2);
extern bool circle_contained(CIRCLE *circle1, CIRCLE *circle2);
extern bool circle_contain(CIRCLE *circle1, CIRCLE *circle2);
extern bool circle_below(CIRCLE *circle1, CIRCLE *circle2);
extern bool circle_above(CIRCLE *circle1, CIRCLE *circle2);

extern bool circle_eq(CIRCLE *circle1, CIRCLE *circle2);
extern bool circle_ne(CIRCLE *circle1, CIRCLE *circle2);
extern bool circle_lt(CIRCLE *circle1, CIRCLE *circle2);
extern bool circle_gt(CIRCLE *circle1, CIRCLE *circle2);
extern bool circle_le(CIRCLE *circle1, CIRCLE *circle2);
extern bool circle_ge(CIRCLE *circle1, CIRCLE *circle2);
extern bool circle_contain_pt(CIRCLE *circle, Point *point);
extern bool pt_contained_circle(Point *point, CIRCLE *circle);
extern CIRCLE *circle_add_pt(CIRCLE *circle, Point *point);
extern CIRCLE *circle_sub_pt(CIRCLE *circle, Point *point);
extern CIRCLE *circle_mul_pt(CIRCLE *circle, Point *point);
extern CIRCLE *circle_div_pt(CIRCLE *circle, Point *point);
extern double *circle_diameter(CIRCLE *circle);
extern double *circle_radius(CIRCLE *circle);
extern double *circle_distance(CIRCLE *circle1, CIRCLE *circle2);
extern double *dist_pc(Point *point, CIRCLE *circle);
extern Datum dist_cpoly(PG_FUNCTION_ARGS);
extern Point *circle_center(CIRCLE *circle);
extern CIRCLE *circle(Point *center, float8 *radius);
extern CIRCLE *box_circle(BOX *box);
extern BOX *circle_box(CIRCLE *circle);
extern Datum poly_circle(PG_FUNCTION_ARGS);
extern Datum circle_poly(PG_FUNCTION_ARGS);

/* private routines */
extern double *circle_area(CIRCLE *circle);
extern double circle_dt(CIRCLE *circle1, CIRCLE *circle2);

/* support routines for the rtree access method (rtproc.c) */
extern BOX *rt_box_union(BOX *a, BOX *b);
extern BOX *rt_box_inter(BOX *a, BOX *b);
extern void rt_box_size(BOX *a, float *size);
extern void rt_bigbox_size(BOX *a, float *size);
extern Datum rt_poly_size(PG_FUNCTION_ARGS);
extern Datum rt_poly_union(PG_FUNCTION_ARGS);
extern Datum rt_poly_inter(PG_FUNCTION_ARGS);

/* geo_selfuncs.c */
extern Datum areasel(PG_FUNCTION_ARGS);
extern Datum areajoinsel(PG_FUNCTION_ARGS);
extern Datum positionsel(PG_FUNCTION_ARGS);
extern Datum positionjoinsel(PG_FUNCTION_ARGS);
extern Datum contsel(PG_FUNCTION_ARGS);
extern Datum contjoinsel(PG_FUNCTION_ARGS);

#endif	 /* GEO_DECLS_H */
