#include <postgres.h>

#ifdef __cplusplus
extern		"C"
{
#endif

	void		ECPGdebug(int, FILE *);
	bool		ECPGstatus(int, const char *);
	bool		ECPGsetconn(int, const char *);
	bool		ECPGconnect(int, const char *, const char *, const char *, const char *);
	bool		ECPGdo(int, const char *, char *,...);
	bool		ECPGtrans(int, const char *, const char *);
	bool		ECPGdisconnect(int, const char *);
	bool		ECPGprepare(int, char *, char *);
	bool		ECPGdeallocate(int, char *);
	char		*ECPGprepared_statement(char *);
		
	void		ECPGlog(const char *format,...);

#ifdef LIBPQ_FE_H
	bool		ECPGsetdb(PGconn *);

#endif

/* Here are some methods used by the lib. */
/* Returns a pointer to a string containing a simple type name. */
	const char *ECPGtype_name(enum ECPGttype);

/* A generic varchar type. */
	struct ECPGgeneric_varchar
	{
		int			len;
		char		arr[1];
	};

/* print an error message */
	void		sqlprint(void);

	struct cursor
	{
		const char *name;
		char	   *command;
		struct cursor *next;
	};

	extern int	no_auto_trans;

/* define this for simplicity as well as compatibility */

#define		  SQLCODE	 sqlca.sqlcode

#ifdef __cplusplus
}

#endif

#include <ecpgerrno.h>
