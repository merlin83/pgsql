/*-------------------------------------------------------------------------
 *
 * UUID generation functions using the OSSP UUID library
 *
 * Copyright (c) 2007 PostgreSQL Global Development Group
 *
 * $PostgreSQL: pgsql/contrib/uuid-ossp/uuid-ossp.c,v 1.2 2007/05/15 19:47:51 adunstan Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include "utils/uuid.h"

#include <uuid.h>


/* better both be 16 */
#if (UUID_LEN != UUID_LEN_BIN)
#error UUID length mismatch
#endif


PG_MODULE_MAGIC;


Datum uuid_nil(PG_FUNCTION_ARGS);
Datum uuid_ns_dns(PG_FUNCTION_ARGS);
Datum uuid_ns_url(PG_FUNCTION_ARGS);
Datum uuid_ns_oid(PG_FUNCTION_ARGS);
Datum uuid_ns_x500(PG_FUNCTION_ARGS);

Datum uuid_generate_v1(PG_FUNCTION_ARGS);
Datum uuid_generate_v1mc(PG_FUNCTION_ARGS);
Datum uuid_generate_v3(PG_FUNCTION_ARGS);
Datum uuid_generate_v4(PG_FUNCTION_ARGS);
Datum uuid_generate_v5(PG_FUNCTION_ARGS);


PG_FUNCTION_INFO_V1(uuid_nil);
PG_FUNCTION_INFO_V1(uuid_ns_dns);
PG_FUNCTION_INFO_V1(uuid_ns_url);
PG_FUNCTION_INFO_V1(uuid_ns_oid);
PG_FUNCTION_INFO_V1(uuid_ns_x500);

PG_FUNCTION_INFO_V1(uuid_generate_v1);
PG_FUNCTION_INFO_V1(uuid_generate_v1mc);
PG_FUNCTION_INFO_V1(uuid_generate_v3);
PG_FUNCTION_INFO_V1(uuid_generate_v4);
PG_FUNCTION_INFO_V1(uuid_generate_v5);


static char *
uuid_to_string(const uuid_t *uuid)
{
	char   *buf = palloc(UUID_LEN_STR + 1);
	void   *ptr = buf;
	size_t	len = UUID_LEN_STR + 1;

	uuid_export(uuid, UUID_FMT_STR, &ptr, &len);

	return buf;
}


static void
string_to_uuid(const char *str, uuid_t *uuid)
{
	uuid_import(uuid, UUID_FMT_STR, str, UUID_LEN_STR + 1);
}


static Datum
special_uuid_value(const char *name)
{
	uuid_t *uuid;
	char   *str;

	uuid_create(&uuid);
	uuid_load(uuid, name);
	str = uuid_to_string(uuid);
	uuid_destroy(uuid);

	return DirectFunctionCall1(uuid_in, CStringGetDatum(str));
}


Datum
uuid_nil(PG_FUNCTION_ARGS)
{
	return special_uuid_value("nil");
}


Datum
uuid_ns_dns(PG_FUNCTION_ARGS)
{
	return special_uuid_value("ns:DNS");
}


Datum
uuid_ns_url(PG_FUNCTION_ARGS)
{
	return special_uuid_value("ns:URL");
}


Datum
uuid_ns_oid(PG_FUNCTION_ARGS)
{
	return special_uuid_value("ns:OID");
}


Datum
uuid_ns_x500(PG_FUNCTION_ARGS)
{
	return special_uuid_value("ns:X500");
}


static Datum
uuid_generate_internal(int mode, const uuid_t *ns, const char *name)
{
	uuid_t *uuid;
	char   *str;

	uuid_create(&uuid);
	uuid_make(uuid, mode, ns, name);
	str = uuid_to_string(uuid);
	uuid_destroy(uuid);

	return DirectFunctionCall1(uuid_in, CStringGetDatum(str));
}


Datum
uuid_generate_v1(PG_FUNCTION_ARGS)
{
	return uuid_generate_internal(UUID_MAKE_V1, NULL, NULL);
}


Datum
uuid_generate_v1mc(PG_FUNCTION_ARGS)
{
	return uuid_generate_internal(UUID_MAKE_V1 | UUID_MAKE_MC, NULL, NULL);
}


static Datum
uuid_generate_v35_internal(int mode, pg_uuid_t *ns, text *name)
{
	uuid_t	   *ns_uuid;
	Datum		result;

	uuid_create(&ns_uuid);
	string_to_uuid(DatumGetCString(DirectFunctionCall1(uuid_out, UUIDPGetDatum(ns))),
				   ns_uuid);

	result = uuid_generate_internal(mode,
									ns_uuid,
									DatumGetCString(DirectFunctionCall1(textout, PointerGetDatum(name))));

	uuid_destroy(ns_uuid);

	return result;
}


Datum
uuid_generate_v3(PG_FUNCTION_ARGS)
{
	pg_uuid_t  *ns = PG_GETARG_UUID_P(0);
	text	   *name = PG_GETARG_TEXT_P(1);

	return uuid_generate_v35_internal(UUID_MAKE_V3, ns, name);
}


Datum
uuid_generate_v4(PG_FUNCTION_ARGS)
{
	return uuid_generate_internal(UUID_MAKE_V4, NULL, NULL);
}


Datum
uuid_generate_v5(PG_FUNCTION_ARGS)
{
	pg_uuid_t  *ns = PG_GETARG_UUID_P(0);
	text	   *name = PG_GETARG_TEXT_P(1);

	return uuid_generate_v35_internal(UUID_MAKE_V5, ns, name);
}
