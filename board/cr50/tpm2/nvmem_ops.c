/* Copyright 2018 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "dcrypto.h"

#include "Global.h"
#include "NV_fp.h"
#include "util.h"

static void fill_rand_or_clear(void *buf, size_t size)
{
	if (!fips_rand_bytes(buf, size))
		memset(buf, 0, size);
}

void nvmem_wipe_cache(void)
{
	/*
	 * Inclusive list of NV indices not to be wiped out when invalidating
	 * the cache.
	 */
	const uint16_t allowlist_range[] = { 0x1007, 0x100b };

	NvSelectivelyInvalidateCache(allowlist_range);

	/*
	 * Wipe some confidential persistent data, same as TPM2_Clear().
	 */
	memset(&gp.ownerAuth, 0, sizeof(gp.ownerAuth));
	memset(&gp.endorsementAuth, 0, sizeof(gp.endorsementAuth));
	memset(&gp.lockoutAuth, 0, sizeof(gp.lockoutAuth));

	gp.SPSeed.t.size = PRIMARY_SEED_SIZE;
	fill_rand_or_clear(gp.SPSeed.t.buffer, sizeof(gp.SPSeed.t.buffer));
	gp.PPSeed.t.size = PRIMARY_SEED_SIZE;
	fill_rand_or_clear(gp.PPSeed.t.buffer, sizeof(gp.PPSeed.t.buffer));
	gp.phProof.t.size = PROOF_SIZE;
	fill_rand_or_clear(gp.phProof.t.buffer, sizeof(gp.phProof.t.buffer));
	gp.shProof.t.size = PROOF_SIZE;
	fill_rand_or_clear(gp.shProof.t.buffer, sizeof(gp.shProof.t.buffer));
	gp.ehProof.t.size = PROOF_SIZE;
	fill_rand_or_clear(gp.ehProof.t.buffer, sizeof(gp.ehProof.t.buffer));
}
