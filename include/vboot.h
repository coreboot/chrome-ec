/* Copyright 2017 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef __CROS_EC_INCLUDE_VBOOT_H
#define __CROS_EC_INCLUDE_VBOOT_H

#include "common.h"
#include "vb21_struct.h"
#include "rsa.h"
#include "sha256.h"

#define CR50_COMM_VERSION               0x00
#define CR50_COMM_PREAMBLE              0xec
#define MIN_LENGTH_PREAMBLE             4
#define CR50_COMM_MAGIC_CHAR0           'E'
#define CR50_COMM_MAGIC_CHAR1           'C'
#define CR50_COMM_MAGIC_WORD            ((CR50_COMM_MAGIC_CHAR1 << 8) | \
					 CR50_COMM_MAGIC_CHAR0)

/*
 * EC-Cr50 data stream looks like as follows:
 *
 *   [preamble][header][payload]
 *
 * preamble: CR50_COMM_PREAMBLE (at least MIN_LENGTH_PREAMBLE times)
 * header: struct cr50_comm_packet
 * payload: data[]
 */
struct cr50_comm_packet {
	/* Header */
	uint16_t magic;	/* CR50_COMM_MAGIC_WORD */
	uint8_t version;/* Struct version. 4MSB=Major. 4LSB=Minor. */
	uint8_t crc;	/* checksum computed from all bytes after crc */
	uint16_t cmd;	/* CR50_COMM_CMD_* if EC sends */
			/* CR50_COMM_RESPONSE(X) if CR50 sends. */
	uint8_t size;	/* Size of 'data[]' member. */
	uint8_t data[];	/* Payload */
} __packed;

#define CR50_COMM_MAX_DATA_SIZE         32
#define CR50_COMM_MAX_PACKET_SIZE       (sizeof(struct cr50_comm_packet) + \
					 CR50_COMM_MAX_DATA_SIZE)

/* EC-CR50 commands (2 bytes) for cr50_comm_packet.cmd */
#define CR50_COMM_CMD_SET_BOOT_MODE     0x0001
#define CR50_COMM_CMD_VERIFY_HASH       0x0002

/* EC-CR50 response codes (2 bytes) for cr50_comm_packet.cmd */
#define CR50_COMM_RESPONSE(X)           ((CR50_COMM_PREAMBLE << 8) | \
					 ((X) & 0xff))
#define CR50_COMM_SUCCESS               CR50_COMM_RESPONSE(0x00)
#define CR50_COMM_ERROR_UNKNOWN         CR50_COMM_RESPONSE(0x01)
#define CR50_COMM_ERROR_MAGIC           CR50_COMM_RESPONSE(0x02)
#define CR50_COMM_ERROR_CRC             CR50_COMM_RESPONSE(0x03)
#define CR50_COMM_ERROR_SIZE            CR50_COMM_RESPONSE(0x04)
#define CR50_COMM_ERROR_TIMEOUT         CR50_COMM_RESPONSE(0x05)
#define CR50_COMM_ERROR_UNDEFINED_CMD   CR50_COMM_RESPONSE(0x06)
#define CR50_COMM_ERROR_BAD_PAYLOAD     CR50_COMM_RESPONSE(0x07)
#define CR50_COMM_ERROR_STRUCT_VERSION  CR50_COMM_RESPONSE(0x08)
#define CR50_COMM_ERROR_NVMEM           CR50_COMM_RESPONSE(0x09)

/*
 * BIT(0) : NO_BOOT flag
 * BIT(1) : RECOVERY flag
 */
enum ec_efs_boot_mode {
	EC_EFS_BOOT_MODE_NORMAL           = 0x00,
	EC_EFS_BOOT_MODE_NO_BOOT          = 0x01,

	/* boot_mode is uint8_t */
	EC_EFS_BOOT_MODE_LIMIT            = 255,
};

/****************************************************************************
 * This is quoted from 2secdata_struct.h in the directory,
 * src/platform/vboot_reference/firmware/2lib/include/.
 ****************************************************************************/

/* Kernel secure storage space */
#define VB2_SECDATA_KERNEL_STRUCT_VERSION_MIN  0x10
#define VB2_SECDATA_KERNEL_UID          0x4752574c  /* 'LWRG' */
struct vb2_secdata_kernel {
	uint8_t struct_version;		/* top-half:major. bottom-half:minor. */
	uint8_t struct_size;		/* Whole structure size */
	uint8_t crc8;			/* CRC for everything below */
	uint8_t reserved0;

	uint32_t kernel_versions;	/* Kernel versions */
	uint8_t ec_hash[SHA256_DIGEST_SIZE];
} __packed;

/**
 * Validate key contents.
 *
 * @param key
 * @return EC_SUCCESS or EC_ERROR_*
 */
int vb21_is_packed_key_valid(const struct vb21_packed_key *key);

/**
 * Validate signature contents.
 *
 * @param sig Signature to be validated.
 * @param key Key to be used for validating <sig>.
 * @return EC_SUCCESS or EC_ERROR_*
 */
int vb21_is_signature_valid(const struct vb21_signature *sig,
			    const struct vb21_packed_key *key);

/**
 * Check data region is filled with ones
 *
 * @param data  Data to be validated.
 * @param start Offset where validation starts.
 * @param end   Offset where validation ends. data[end] won't be checked.
 * @return EC_SUCCESS or EC_ERROR_*
 */
int vboot_is_padding_valid(const uint8_t *data, uint32_t start, uint32_t end);

/**
 * Verify data by RSA signature
 *
 * @param data Data to be verified.
 * @param len  Number of bytes in <data>.
 * @param key  Key to be used for verification.
 * @param sig  Signature of <data>
 * @return EC_SUCCESS or EC_ERROR_*
 */
int vboot_verify(const uint8_t *data, int len,
		 const struct rsa_public_key *key, const uint8_t *sig);

/**
 * Entry point of EC EFS
 */
void vboot_main(void);

/**
 * Get if vboot requires PD comm to be enabled or not
 *
 * @return 1: need PD communication. 0: PD communication is not needed.
 */
int vboot_need_pd_comm(void);

#endif  /* __CROS_EC_INCLUDE_VBOOT_H */
