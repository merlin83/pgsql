/* 
 * This file contains stuff needed to be as compatible to Informix as possible.
 */

#include <decimal.h>
#include <datetime.h>

#define SQLNOTFOUND 100

#ifndef Date
#define Date long
#endif /* ! Date */

extern int rdatestr (Date, char *);
extern void rtoday (Date *);
extern int rjulmdy (Date, short *);
extern int rdefmtdate (Date *, char *, char *);
extern int rfmtdate (Date, char *, char *);
extern int rmdyjul (short *, Date *);
extern int rstrdate (char *, Date *);
extern int rdayofweek(Date);

extern int rfmtlong(long, char *, char *);
extern int rgetmsg(int, char *, int);
extern int risnull(int, char *);
extern int rsetnull(int, char *);
extern int rtypalign(int, int);
extern int rtypmsize(int, int);
extern void rupshift(char *);


