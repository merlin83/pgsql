/*
 * mhash.c
 *		Wrapper for mhash library.
 * 
 * Copyright (c) 2000 Marko Kreen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: mhash.c,v 1.2 2001-02-10 02:31:26 tgl Exp $
 */

#include "postgres.h"

#include "pgcrypto.h"

#include <mhash.h>

static uint
pg_mhash_len(pg_digest *hash);
static uint8 *
pg_mhash_digest(pg_digest *hash, uint8 *src,
		uint len, uint8 *buf);

static uint
pg_mhash_len(pg_digest *h) {
	return mhash_get_block_size(h->misc.code);
}

static uint8 *
pg_mhash_digest(pg_digest *h, uint8 *src, uint len, uint8 *dst)
{
	uint8 *res;
	
	MHASH mh = mhash_init(h->misc.code);
	mhash(mh, src, len);
	res = mhash_end(mh);
	
	memcpy(dst, res, mhash_get_block_size(h->misc.code));
	mhash_free(res);
	
	return dst;
}

pg_digest *
pg_find_digest(pg_digest *h, char *name)
{
	size_t hnum, i, b;
	char *mname;
	
	hnum = mhash_count();
	for (i = 0; i <= hnum; i++) {
		mname = mhash_get_hash_name(i);
		if (mname == NULL)
			continue;
		b = strcasecmp(name, mname);
		free(mname);
		if (!b) {
			h->name = mhash_get_hash_name(i);
			h->length = pg_mhash_len;
			h->digest = pg_mhash_digest;
			h->misc.code = i;
			return h;
		}
	}
	return NULL;
}

