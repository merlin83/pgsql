#ifndef _ECPG_ERROR_H
#define _ECPG_ERROR_H

#include <errno.h>

/* This is a list of all error codes the embedded SQL program can return */
#define	ECPG_NO_ERROR		0
#define ECPG_NOT_FOUND		100

/* system error codes returned by ecpglib get the correct number,
 * but are made negative
 */
#define ECPG_OUT_OF_MEMORY	-ENOMEM

/* first we have a set of ecpg messages, they start at 200 */
#define ECPG_UNSUPPORTED	-200
#define ECPG_TOO_MANY_ARGUMENTS	-201
#define ECPG_TOO_FEW_ARGUMENTS	-202
#define ECPG_TOO_MANY_MATCHES	-203
#define ECPG_INT_FORMAT		-204
#define ECPG_UINT_FORMAT	-205
#define ECPG_FLOAT_FORMAT	-206
#define ECPG_CONVERT_BOOL	-207
#define ECPG_EMPTY		-208
#define ECPG_NO_CONN		-209

/* finally the backend error messages, they start at 300 */
#define ECPG_PGSQL		-300
#define ECPG_TRANS		-301
#define ECPG_CONNECT		-302

#endif /* !_ECPG_ERROR_H */
