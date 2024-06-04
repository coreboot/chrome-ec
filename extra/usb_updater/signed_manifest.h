/* Copyright 2024 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef __CROS_EC_SIGNED_MANIFEST_H
#define __CROS_EC_SIGNED_MANIFEST_H


#include "compile_time_macros.h"
#include "stdint.h"

/*
 * This is the signed manifest header for Opentitan images. See also
 * third_party/lowriscv/opentitan/sw/device/silicon_creator/lib/manifest.h
 */
struct SignedManifest {
	uint32_t signature[96];
	uint32_t constraint_selector_bits;
	uint32_t constraint_device_id[8];
	uint32_t constraint_manuf_state_creator;
	uint32_t constraint_manuf_state_owner;
	uint32_t constraint_life_cycle_state;
	uint32_t modulus[96];
	uint32_t address_translation;
	uint32_t identifier;
	uint16_t manifest_version_major;
	uint16_t manifest_version_minor;
	uint32_t signed_region_end;
	uint32_t length;
	uint32_t version_major;
	uint32_t version_minor;
	uint32_t security_version;
	uint32_t timestamp_low;
	uint32_t timestamp_high;
	uint32_t binding_value[8];
	uint32_t max_key_version;
	uint32_t code_start;
	uint32_t code_end;
	uint32_t entry_point;
	uint32_t extensions[30];
};

BUILD_ASSERT(sizeof(struct SignedManifest) == 1024);
/* Verify a few of the field offsets we use most often */
BUILD_ASSERT(offsetof(struct SignedManifest, identifier) == 820);
BUILD_ASSERT(offsetof(struct SignedManifest, length) == 832);
BUILD_ASSERT(offsetof(struct SignedManifest, version_major) == 836);
BUILD_ASSERT(offsetof(struct SignedManifest, version_minor) == 840);

/* Identifier that marks the header as ROM_EXT "OTRE" */
#define ID_ROM_EXT 0x4552544F
/* Identifier that marks the header as owner firmware "OTB0" */
#define ID_OWNER_FW 0x3042544F

#endif /* __CROS_EC_SIGNED_MANIFEST_H */
