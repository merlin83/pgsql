/*-------------------------------------------------------------------------
 *
 * crypt.c
 *	Look into pg_shadow and check the encrypted password with
 *	the one passed in from the frontend.
 *
 * Modification History
 *
 * Dec 17, 1997 - Todd A. Brandys
 *	Orignal Version Completed.
 *
 * $Id: crypt.c,v 1.37 2001-08-17 15:40:07 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */

#include <errno.h>
#include <unistd.h>

#include "postgres.h"
#include "libpq/crypt.h"
#include "libpq/libpq.h"
#include "miscadmin.h"
#include "storage/fd.h"
#include "utils/nabstime.h"

#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif

char	  **pwd_cache = NULL;
int			pwd_cache_count = 0;

/*
 * crypt_getpwdfilename --- get name of password file
 *
 * Note that result string is palloc'd, and should be freed by the caller.
 */
char *
crypt_getpwdfilename(void)
{
	int			bufsize;
	char	   *pfnam;

	bufsize = strlen(DataDir) + 8 + strlen(CRYPT_PWD_FILE) + 1;
	pfnam = (char *) palloc(bufsize);
	snprintf(pfnam, bufsize, "%s/global/%s", DataDir, CRYPT_PWD_FILE);

	return pfnam;
}

/*
 * crypt_getpwdreloadfilename --- get name of password-reload-needed flag file
 *
 * Note that result string is palloc'd, and should be freed by the caller.
 */
char *
crypt_getpwdreloadfilename(void)
{
	char	   *pwdfilename;
	int			bufsize;
	char	   *rpfnam;

	pwdfilename = crypt_getpwdfilename();
	bufsize = strlen(pwdfilename) + strlen(CRYPT_PWD_RELOAD_SUFX) + 1;
	rpfnam = (char *) palloc(bufsize);
	snprintf(rpfnam, bufsize, "%s%s", pwdfilename, CRYPT_PWD_RELOAD_SUFX);
	pfree(pwdfilename);

	return rpfnam;
}

/*-------------------------------------------------------------------------*/

static FILE *
crypt_openpwdfile(void)
{
	char	   *filename;
	FILE	   *pwdfile;

	filename = crypt_getpwdfilename();
	pwdfile = AllocateFile(filename, "r");

	if (pwdfile == NULL && errno != ENOENT)
		elog(DEBUG, "could not open %s: %s", filename, strerror(errno));

	pfree(filename);

	return pwdfile;
}

/*-------------------------------------------------------------------------*/

static int
compar_user(const void *user_a, const void *user_b)
{

	int			min,
				value;
	char	   *login_a;
	char	   *login_b;

	login_a = *((char **) user_a);
	login_b = *((char **) user_b);

	/*
	 * We only really want to compare the user logins which are first.	We
	 * look for the first SEPSTR char getting the number of chars there
	 * are before it. We only need to compare to the min count from the
	 * two strings.
	 */
	min = strcspn(login_a, CRYPT_PWD_FILE_SEPSTR);
	value = strcspn(login_b, CRYPT_PWD_FILE_SEPSTR);
	if (value < min)
		min = value;

	/*
	 * We add one to min so that the separator character is included in
	 * the comparison.	Why?  I believe this will prevent logins that are
	 * proper prefixes of other logins from being 'masked out'.  Being
	 * conservative!
	 */
	return strncmp(login_a, login_b, min + 1);
}

/*-------------------------------------------------------------------------*/

static void
crypt_loadpwdfile(void)
{
	char	   *filename;
	int			result;
	FILE	   *pwd_file;
	char		buffer[256];

	filename = crypt_getpwdreloadfilename();
	result = unlink(filename);
	pfree(filename);

	/*
	 * We want to delete the flag file before reading the contents of the
	 * pg_pwd file.  If result == 0 then the unlink of the reload file was
	 * successful. This means that a backend performed a COPY of the
	 * pg_shadow file to pg_pwd.  Therefore we must now do a reload.
	 */
	if (!pwd_cache || result == 0)
	{
		if (pwd_cache)
		{						/* free the old data only if this is a
								 * reload */
			while (pwd_cache_count--)
				free((void *) pwd_cache[pwd_cache_count]);
			free((void *) pwd_cache);
			pwd_cache = NULL;
			pwd_cache_count = 0;
		}

		if (!(pwd_file = crypt_openpwdfile()))
			return;

		/*
		 * Here is where we load the data from pg_pwd.
		 */
		while (fgets(buffer, 256, pwd_file) != NULL)
		{

			/*
			 * We must remove the return char at the end of the string, as
			 * this will affect the correct parsing of the password entry.
			 */
			if (buffer[(result = strlen(buffer) - 1)] == '\n')
				buffer[result] = '\0';

			pwd_cache = (char **) realloc((void *) pwd_cache, sizeof(char *) * (pwd_cache_count + 1));
			pwd_cache[pwd_cache_count++] = strdup(buffer);
		}
		FreeFile(pwd_file);

		/*
		 * Now sort the entries in the cache for faster searching later.
		 */
		qsort((void *) pwd_cache, pwd_cache_count, sizeof(char *), compar_user);
	}
}

/*-------------------------------------------------------------------------*/

static void
crypt_parsepwdentry(char *buffer, char **pwd, char **valdate)
{

	char	   *parse = buffer;
	int			count,
				i;

	/*
	 * skip to the password field
	 */
	for (i = 0; i < 6; i++)
		parse += (strcspn(parse, CRYPT_PWD_FILE_SEPSTR) + 1);

	/*
	 * store a copy of user password to return
	 */
	count = strcspn(parse, CRYPT_PWD_FILE_SEPSTR);
	*pwd = (char *) palloc(count + 1);
	strncpy(*pwd, parse, count);
	(*pwd)[count] = '\0';
	parse += (count + 1);

	/*
	 * store a copy of date login becomes invalid
	 */
	count = strcspn(parse, CRYPT_PWD_FILE_SEPSTR);
	*valdate = (char *) palloc(count + 1);
	strncpy(*valdate, parse, count);
	(*valdate)[count] = '\0';
	parse += (count + 1);
}

/*-------------------------------------------------------------------------*/

static int
crypt_getloginfo(const char *user, char **passwd, char **valuntil)
{
	char	   *pwd,
			   *valdate;
	void	   *fakeout;

	*passwd = NULL;
	*valuntil = NULL;
	crypt_loadpwdfile();

	if (pwd_cache)
	{
		char	  **pwd_entry;
		char		user_search[NAMEDATALEN + 2];

		snprintf(user_search, NAMEDATALEN + 2, "%s\t", user);
		fakeout = (void *) &user_search;
		if ((pwd_entry = (char **) bsearch((void *) &fakeout, (void *) pwd_cache, pwd_cache_count, sizeof(char *), compar_user)))
		{
			crypt_parsepwdentry(*pwd_entry, &pwd, &valdate);
			*passwd = pwd;
			*valuntil = valdate;
			return STATUS_OK;
		}

		return STATUS_OK;
	}

	return STATUS_ERROR;
}

/*-------------------------------------------------------------------------*/

int
md5_crypt_verify(const Port *port, const char *user, const char *pgpass)
{

	char	   *passwd,
			   *valuntil,
			   *crypt_pwd;
	int			retval = STATUS_ERROR;
	AbsoluteTime vuntil,
				current;

	if (crypt_getloginfo(user, &passwd, &valuntil) == STATUS_ERROR)
		return STATUS_ERROR;

	if (passwd == NULL || *passwd == '\0')
	{
		if (passwd)
			pfree((void *) passwd);
		if (valuntil)
			pfree((void *) valuntil);
		return STATUS_ERROR;
	}

	/* If they encrypt their password, force MD5 */
	if (isMD5(passwd) && port->auth_method != uaMD5)
	{
		snprintf(PQerrormsg, PQERRORMSG_LENGTH,
			"Password is stored MD5 encrypted.  "
			"Only pg_hba.conf's MD5 protocol can be used for this user.\n");
		fputs(PQerrormsg, stderr);
		pqdebug("%s", PQerrormsg);
		return STATUS_ERROR;
	}

	/*
	 * Compare with the encrypted or plain password depending on the
	 * authentication method being used for this connection.
	 */
	switch (port->auth_method)
	{
		case uaMD5:
			crypt_pwd = palloc(MD5_PASSWD_LEN+1);
			if (isMD5(passwd))
			{
				if (!EncryptMD5(passwd + strlen("md5"),
								(char *)port->md5Salt,
								sizeof(port->md5Salt), crypt_pwd))
				{
					pfree(crypt_pwd);
					return STATUS_ERROR;
				}
			}
			else
			{
				char *crypt_pwd2 = palloc(MD5_PASSWD_LEN+1);

				if (!EncryptMD5(passwd, port->user, strlen(port->user),
								crypt_pwd2))
				{
					pfree(crypt_pwd);
					pfree(crypt_pwd2);
					return STATUS_ERROR;
				}
				if (!EncryptMD5(crypt_pwd2 + strlen("md5"), port->md5Salt,
								sizeof(port->md5Salt), crypt_pwd))
				{
					pfree(crypt_pwd);
					pfree(crypt_pwd2);
					return STATUS_ERROR;
				}
				pfree(crypt_pwd2);
			}
			break;
		case uaCrypt:
		{
			char salt[3];
			StrNCpy(salt, port->cryptSalt,3);
			crypt_pwd = crypt(passwd, salt);
			break;
		}
		default:
			crypt_pwd = passwd;
			break;
	}

	if (!strcmp(pgpass, crypt_pwd))
	{
		/*
		 * check here to be sure we are not past valuntil
		 */
		if (!valuntil || strcmp(valuntil, "\\N") == 0)
			vuntil = INVALID_ABSTIME;
		else
			vuntil = DatumGetAbsoluteTime(DirectFunctionCall1(nabstimein,
											 CStringGetDatum(valuntil)));
		current = GetCurrentAbsoluteTime();
		if (vuntil != INVALID_ABSTIME && vuntil < current)
			retval = STATUS_ERROR;
		else
			retval = STATUS_OK;
	}

	pfree(passwd);
	if (valuntil)
		pfree(valuntil);
	if (port->auth_method == uaMD5)
		pfree(crypt_pwd);

	return retval;
}
