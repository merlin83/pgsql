/* dynamic SQL support routines
 *
 * $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/interfaces/ecpg/lib/Attic/descriptor.c,v 1.18 2001-11-05 17:46:37 momjian Exp $
 */

#include "postgres_fe.h"

#include "ecpgtype.h"
#include "ecpglib.h"
#include "ecpgerrno.h"
#include "extern.h"
#include "sql3types.h"

struct descriptor *all_descriptors = NULL;

/* old internal convenience function that might go away later */
static PGresult
		   *
ECPGresultByDescriptor(int line, const char *name)
{
	PGresult  **resultpp = ECPGdescriptor_lvalue(line, name);

	if (resultpp)
		return *resultpp;
	return NULL;
}

static unsigned int
ECPGDynamicType_DDT(Oid type)
{
	switch (type)
	{
		case 1082:
			return SQL3_DDT_DATE;		/* date */
		case 1083:
			return SQL3_DDT_TIME;		/* time */
		case 1184:
			return SQL3_DDT_TIMESTAMP_WITH_TIME_ZONE;	/* datetime */
		case 1296:
			return SQL3_DDT_TIMESTAMP_WITH_TIME_ZONE;	/* timestamp */
		default:
			return SQL3_DDT_ILLEGAL;
	}
}

bool
ECPGget_desc_header(int lineno, char *desc_name, int *count)
{
	PGresult   *ECPGresult = ECPGresultByDescriptor(lineno, desc_name);

	if (!ECPGresult)
		return false;

	*count = PQnfields(ECPGresult);
	ECPGlog("ECPGget_desc_header: found %d attributes.\n", *count);
	return true;
}

static bool
get_int_item(int lineno, void *var, enum ECPGttype vartype, int value)
{
	switch (vartype)
	{
		case ECPGt_short:
			*(short *) var = (short) value;
			break;
		case ECPGt_int:
			*(int *) var = (int) value;
			break;
		case ECPGt_long:
			*(long *) var = (long) value;
			break;
		case ECPGt_unsigned_short:
			*(unsigned short *) var = (unsigned short) value;
			break;
		case ECPGt_unsigned_int:
			*(unsigned int *) var = (unsigned int) value;
			break;
		case ECPGt_unsigned_long:
			*(unsigned long *) var = (unsigned long) value;
			break;
#ifdef HAVE_LONG_LONG_INT_64
		case ECPGt_long_long:
			*(long long int *) var = (long long int) value;
			break;
		case ECPGt_unsigned_long_long:
			*(unsigned long long int *) var = (unsigned long long int) value;
			break;
#endif   /* HAVE_LONG_LONG_INT_64 */
		case ECPGt_float:
			*(float *) var = (float) value;
			break;
		case ECPGt_double:
			*(double *) var = (double) value;
			break;
		default:
			ECPGraise(lineno, ECPG_VAR_NOT_NUMERIC, NULL);
			return (false);
	}

	return (true);
}

static bool
get_char_item(int lineno, void *var, enum ECPGttype vartype, char *value, int varcharsize)
{
	switch (vartype)
	{
		case ECPGt_char:
		case ECPGt_unsigned_char:
			strncpy((char *) var, value, varcharsize);
			break;
		case ECPGt_varchar:
			{
				struct ECPGgeneric_varchar *variable =
				(struct ECPGgeneric_varchar *) var;

				if (varcharsize == 0)
					strncpy(variable->arr, value, strlen(value));
				else
					strncpy(variable->arr, value, varcharsize);

				variable->len = strlen(value);
				if (varcharsize > 0 && variable->len > varcharsize)
					variable->len = varcharsize;
			}
			break;
		default:
			ECPGraise(lineno, ECPG_VAR_NOT_CHAR, NULL);
			return (false);
	}

	return (true);
}

bool
ECPGget_desc(int lineno, char *desc_name, int index,...)
{
	va_list		args;
	PGresult   *ECPGresult = ECPGresultByDescriptor(lineno, desc_name);
	enum ECPGdtype type;
	bool		Indicator_seen = false,
				Data_seen = false;
	int			ntuples,
				act_tuple;

	va_start(args, index);
	if (!ECPGresult)
		return (false);

	ntuples = PQntuples(ECPGresult);
	if (ntuples < 1)
	{
		ECPGraise(lineno, ECPG_NOT_FOUND, NULL);
		return (false);
	}

	if (index < 1 || index > PQnfields(ECPGresult))
	{
		ECPGraise(lineno, ECPG_INVALID_DESCRIPTOR_INDEX, NULL);
		return (false);
	}

	ECPGlog("ECPGget_desc: reading items for tuple %d\n", index);
	--index;

	type = va_arg(args, enum ECPGdtype);

	while (type != ECPGd_EODT)
	{
		char		type_str[20];
		long		varcharsize;
		long		offset;
		long		arrsize;
		enum ECPGttype vartype;
		void	   *var;

		vartype = va_arg(args, enum ECPGttype);
		var = va_arg(args, void *);
		varcharsize = va_arg(args, long);
		arrsize = va_arg(args, long);
		offset = va_arg(args, long);

		switch (type)
		{
			case (ECPGd_indicator):

				/*
				 * this is like ECPGexecute missing : allocate arrays,
				 * perhaps this should go into a common function !!
				 */
				if (ntuples > arrsize)
				{
					ECPGlog("ECPGget_desc line %d: Incorrect number of matches: %d don't fit into array of %d\n",
							lineno, ntuples, arrsize);
					ECPGraise(lineno, ECPG_TOO_MANY_MATCHES, NULL);
					return false;
				}
				Indicator_seen = true;
				for (act_tuple = 0; act_tuple < ntuples; act_tuple++)
				{
					if (!get_int_item(lineno, var, vartype, -PQgetisnull(ECPGresult, act_tuple, index)))
						return (false);
					var = (char *) var + offset;
					ECPGlog("ECPGget_desc: INDICATOR[%d] = %d\n", act_tuple, -PQgetisnull(ECPGresult, act_tuple, index));
				}
				break;

			case ECPGd_name:
				if (!get_char_item(lineno, var, vartype, PQfname(ECPGresult, index), varcharsize))
					return (false);

				ECPGlog("ECPGget_desc: NAME = %s\n", PQfname(ECPGresult, index));
				break;

			case ECPGd_nullable:
				if (!get_int_item(lineno, var, vartype, 1))
					return (false);

				break;

			case ECPGd_key_member:
				if (!get_int_item(lineno, var, vartype, 0))
					return (false);

				break;

			case ECPGd_scale:
				if (!get_int_item(lineno, var, vartype, (PQfmod(ECPGresult, index) - VARHDRSZ) & 0xffff))
					return (false);

				ECPGlog("ECPGget_desc: SCALE = %d\n", (PQfmod(ECPGresult, index) - VARHDRSZ) & 0xffff);
				break;

			case ECPGd_precision:
				if (!get_int_item(lineno, var, vartype, PQfmod(ECPGresult, index) >> 16))
					return (false);

				ECPGlog("ECPGget_desc: PRECISION = %d\n", PQfmod(ECPGresult, index) >> 16);
				break;

			case ECPGd_ret_length:
			case ECPGd_ret_octet:

				/*
				 * this is like ECPGexecute missing : allocate arrays,
				 * perhaps this should go into a common function !!
				 */
				if (ntuples > arrsize)
				{
					ECPGlog("ECPGget_desc line %d: Incorrect number of matches: %d don't fit into array of %d\n",
							lineno, ntuples, arrsize);
					ECPGraise(lineno, ECPG_TOO_MANY_MATCHES, NULL);
					return false;
				}
				for (act_tuple = 0; act_tuple < ntuples; act_tuple++)
				{
					if (!get_int_item(lineno, var, vartype, PQgetlength(ECPGresult, act_tuple, index)))
						return (false);
					var = (char *) var + offset;
					ECPGlog("ECPGget_desc: RETURNED[%d] = %d\n", act_tuple, PQgetlength(ECPGresult, act_tuple, index));
				}
				break;

			case ECPGd_octet:
				if (!get_int_item(lineno, var, vartype, PQfsize(ECPGresult, index)))
					return (false);

				ECPGlog("ECPGget_desc: OCTET_LENGTH = %d\n", PQfsize(ECPGresult, index));
				break;

			case ECPGd_length:
				if (!get_int_item(lineno, var, vartype, PQfmod(ECPGresult, index) - VARHDRSZ))
					return (false);

				ECPGlog("ECPGget_desc: LENGTH = %d\n", PQfmod(ECPGresult, index) - VARHDRSZ);
				break;

			case ECPGd_type:
				if (!get_int_item(lineno, var, vartype, ECPGDynamicType(PQftype(ECPGresult, index))))
					return (false);

				ECPGlog("ECPGget_desc: TYPE = %d\n", ECPGDynamicType(PQftype(ECPGresult, index)));
				break;

			case ECPGd_di_code:
				if (!get_int_item(lineno, var, vartype, ECPGDynamicType_DDT(PQftype(ECPGresult, index))))
					return (false);

				ECPGlog("ECPGget_desc: TYPE = %d\n", ECPGDynamicType_DDT(PQftype(ECPGresult, index)));
				break;
			case ECPGd_data:

				/*
				 * this is like ECPGexecute missing : allocate arrays,
				 * perhaps this should go into a common function !!
				 */
				if (ntuples > arrsize)
				{
					ECPGlog("ECPGget_desc line %d: Incorrect number of matches: %d don't fit into array of %d\n",
							lineno, ntuples, arrsize);
					ECPGraise(lineno, ECPG_TOO_MANY_MATCHES, NULL);
					return false;
				}
				Data_seen = true;
				for (act_tuple = 0; act_tuple < ntuples; act_tuple++)
				{
					if (PQgetisnull(ECPGresult, act_tuple, index))
						continue;		/* do not touch data on null value */
					if (!get_data(ECPGresult, act_tuple, index, lineno,
								  vartype, ECPGt_NO_INDICATOR, var, NULL,
								  varcharsize, offset, false))
						return (false);
				}
				break;

			case ECPGd_cardinality:
				if (!get_int_item(lineno, var, vartype, PQntuples(ECPGresult)))
					return (false);

				ECPGlog("ECPGget_desc: CARDINALITY = %d\n", PQntuples(ECPGresult));
				break;

			default:
				snprintf(type_str, sizeof(type_str), "%d", type);
				ECPGraise(lineno, ECPG_UNKNOWN_DESCRIPTOR_ITEM, type_str);
				return (false);
		}

		type = va_arg(args, enum ECPGdtype);
	}

	if (Data_seen && !Indicator_seen)
	{
		for (act_tuple = 0; act_tuple < ntuples; act_tuple++)
		{
			if (PQgetisnull(ECPGresult, act_tuple, index))
			{
				ECPGraise(lineno, ECPG_MISSING_INDICATOR, NULL);
				return (false);
			}
		}
	}

	return (true);
}

bool
ECPGdeallocate_desc(int line, const char *name)
{
	struct descriptor *i;
	struct descriptor **lastptr = &all_descriptors;

	for (i = all_descriptors; i; lastptr = &i->next, i = i->next)
	{
		if (!strcmp(name, i->name))
		{
			*lastptr = i->next;
			free(i->name);
			PQclear(i->result);
			free(i);
			return true;
		}
	}
	ECPGraise(line, ECPG_UNKNOWN_DESCRIPTOR, name);
	return false;
}

bool
ECPGallocate_desc(int line, const char *name)
{
	struct descriptor *new = (struct descriptor *) ecpg_alloc(sizeof(struct descriptor), line);

	new->next = all_descriptors;
	new->name = ecpg_alloc(strlen(name) + 1, line);
	new->result = PQmakeEmptyPGresult(NULL, 0);
	strcpy(new->name, name);
	all_descriptors = new;
	return true;
}

PGresult  **
ECPGdescriptor_lvalue(int line, const char *descriptor)
{
	struct descriptor *i;

	for (i = all_descriptors; i != NULL; i = i->next)
	{
		if (!strcmp(descriptor, i->name))
			return &i->result;
	}

	ECPGraise(line, ECPG_UNKNOWN_DESCRIPTOR, (char *) descriptor);
	return NULL;
}
