// Copyright 2019 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// THIS CODE IS GENERATED - DO NOT MODIFY!

#ifndef TRUNKS_TPM_GENERATED_H_
#define TRUNKS_TPM_GENERATED_H_

#include <string>

#include <base/check.h>
#include <base/functional/callback_forward.h>

#include "trunks/trunks_export.h"

namespace trunks {

class AuthorizationDelegate;
class CommandTransceiver;

#if !defined(SHA1_DIGEST_SIZE)
#define SHA1_DIGEST_SIZE 20
#endif
#if !defined(SHA1_BLOCK_SIZE)
#define SHA1_BLOCK_SIZE 64
#endif
#if !defined(SHA1_DER_SIZE)
#define SHA1_DER_SIZE 15
#endif
#if !defined(SHA1_DER)
#define SHA1_DER                                                            \
  {                                                                         \
    0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2B, 0x0E, 0x03, 0x02, 0x1A, 0x05, \
        0x00, 0x04, 0x14                                                    \
  }
#endif
#if !defined(SHA256_DIGEST_SIZE)
#define SHA256_DIGEST_SIZE 32
#endif
#if !defined(SHA256_BLOCK_SIZE)
#define SHA256_BLOCK_SIZE 64
#endif
#if !defined(SHA256_DER_SIZE)
#define SHA256_DER_SIZE 19
#endif
#if !defined(SHA256_DER)
#define SHA256_DER                                                          \
  {                                                                         \
    0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, \
        0x04, 0x02, 0x01, 0x05, 0x00, 0x04, 0x20                            \
  }
#endif
#if !defined(SHA384_DIGEST_SIZE)
#define SHA384_DIGEST_SIZE 48
#endif
#if !defined(SHA384_BLOCK_SIZE)
#define SHA384_BLOCK_SIZE 128
#endif
#if !defined(SHA384_DER_SIZE)
#define SHA384_DER_SIZE 19
#endif
#if !defined(SHA384_DER)
#define SHA384_DER                                                          \
  {                                                                         \
    0x30, 0x41, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, \
        0x04, 0x02, 0x02, 0x05, 0x00, 0x04, 0x30                            \
  }
#endif
#if !defined(SHA512_DIGEST_SIZE)
#define SHA512_DIGEST_SIZE 64
#endif
#if !defined(SHA512_BLOCK_SIZE)
#define SHA512_BLOCK_SIZE 128
#endif
#if !defined(SHA512_DER_SIZE)
#define SHA512_DER_SIZE 19
#endif
#if !defined(SHA512_DER)
#define SHA512_DER                                                          \
  {                                                                         \
    0x30, 0x51, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, \
        0x04, 0x02, 0x03, 0x05, 0x00, 0x04, 0x40                            \
  }
#endif
#if !defined(SM3_256_DIGEST_SIZE)
#define SM3_256_DIGEST_SIZE 32
#endif
#if !defined(SM3_256_BLOCK_SIZE)
#define SM3_256_BLOCK_SIZE 64
#endif
#if !defined(SM3_256_DER_SIZE)
#define SM3_256_DER_SIZE 18
#endif
#if !defined(SM3_256_DER)
#define SM3_256_DER                                                         \
  {                                                                         \
    0x30, 0x30, 0x30, 0x0c, 0x06, 0x08, 0x2a, 0x81, 0x1c, 0x81, 0x45, 0x01, \
        0x83, 0x11, 0x05, 0x00, 0x04, 0x20                                  \
  }
#endif
#if !defined(MAX_SESSION_NUMBER)
#define MAX_SESSION_NUMBER 3
#endif
#if !defined(YES)
#define YES 1
#endif
#if !defined(NO)
#define NO 0
#endif
#if !defined(TRUE)
#define TRUE 1
#endif
#if !defined(FALSE)
#define FALSE 0
#endif
#if !defined(SET)
#define SET 1
#endif
#if !defined(CLEAR)
#define CLEAR 0
#endif
#if !defined(BIG_ENDIAN_TPM)
#define BIG_ENDIAN_TPM NO
#endif
#if !defined(LITTLE_ENDIAN_TPM)
#define LITTLE_ENDIAN_TPM YES
#endif
#if !defined(NO_AUTO_ALIGN)
#define NO_AUTO_ALIGN NO
#endif
#if !defined(RSA_KEY_SIZES_BITS)
#define RSA_KEY_SIZES_BITS \
  { 1024, 2048 }
#endif
#if !defined(MAX_RSA_KEY_BITS)
#define MAX_RSA_KEY_BITS 2048
#endif
#if !defined(MAX_RSA_KEY_BYTES)
#define MAX_RSA_KEY_BYTES ((MAX_RSA_KEY_BITS + 7) / 8)
#endif
#if !defined(ECC_CURVES)
#define ECC_CURVES                                      \
  {                                                     \
    trunks::TPM_ECC_NIST_P256, trunks::TPM_ECC_BN_P256, \
        trunks::TPM_ECC_SM2_P256                        \
  }
#endif
#if !defined(ECC_KEY_SIZES_BITS)
#define ECC_KEY_SIZES_BITS \
  { 256 }
#endif
#if !defined(MAX_ECC_KEY_BITS)
#define MAX_ECC_KEY_BITS 256
#endif
#if !defined(MAX_ECC_KEY_BYTES)
#define MAX_ECC_KEY_BYTES ((MAX_ECC_KEY_BITS + 7) / 8)
#endif
#if !defined(AES_KEY_SIZES_BITS)
#define AES_KEY_SIZES_BITS \
  { 128 }
#endif
#if !defined(MAX_AES_KEY_BITS)
#define MAX_AES_KEY_BITS 128
#endif
#if !defined(MAX_AES_BLOCK_SIZE_BYTES)
#define MAX_AES_BLOCK_SIZE_BYTES 16
#endif
#if !defined(MAX_AES_KEY_BYTES)
#define MAX_AES_KEY_BYTES ((MAX_AES_KEY_BITS + 7) / 8)
#endif
#if !defined(SM4_KEY_SIZES_BITS)
#define SM4_KEY_SIZES_BITS \
  { 128 }
#endif
#if !defined(MAX_SM4_KEY_BITS)
#define MAX_SM4_KEY_BITS 128
#endif
#if !defined(MAX_SM4_BLOCK_SIZE_BYTES)
#define MAX_SM4_BLOCK_SIZE_BYTES 16
#endif
#if !defined(MAX_SM4_KEY_BYTES)
#define MAX_SM4_KEY_BYTES ((MAX_SM4_KEY_BITS + 7) / 8)
#endif
#if !defined(MAX_SYM_KEY_BITS)
#define MAX_SYM_KEY_BITS MAX_AES_KEY_BITS
#endif
#if !defined(MAX_SYM_KEY_BYTES)
#define MAX_SYM_KEY_BYTES MAX_AES_KEY_BYTES
#endif
#if !defined(MAX_SYM_BLOCK_SIZE)
#define MAX_SYM_BLOCK_SIZE MAX_AES_BLOCK_SIZE_BYTES
#endif
#if !defined(FIELD_UPGRADE_IMPLEMENTED)
#define FIELD_UPGRADE_IMPLEMENTED NO
#endif
#if !defined(BSIZE)
#define BSIZE trunks::UINT16
#endif
#if !defined(BUFFER_ALIGNMENT)
#define BUFFER_ALIGNMENT 4
#endif
#if !defined(IMPLEMENTATION_PCR)
#define IMPLEMENTATION_PCR 24
#endif
#if !defined(PLATFORM_PCR)
#define PLATFORM_PCR 24
#endif
#if !defined(DRTM_PCR)
#define DRTM_PCR 17
#endif
#if !defined(HCRTM_PCR)
#define HCRTM_PCR 0
#endif
#if !defined(NUM_LOCALITIES)
#define NUM_LOCALITIES 5
#endif
#if !defined(MAX_HANDLE_NUM)
#define MAX_HANDLE_NUM 3
#endif
#if !defined(MAX_ACTIVE_SESSIONS)
#define MAX_ACTIVE_SESSIONS 64
#endif
#if !defined(CONTEXT_SLOT)
#define CONTEXT_SLOT trunks::UINT16
#endif
#if !defined(CONTEXT_COUNTER)
#define CONTEXT_COUNTER trunks::UINT64
#endif
#if !defined(MAX_LOADED_SESSIONS)
#define MAX_LOADED_SESSIONS 3
#endif
#if !defined(MAX_SESSION_NUM)
#define MAX_SESSION_NUM 3
#endif
#if !defined(MAX_LOADED_OBJECTS)
#define MAX_LOADED_OBJECTS 3
#endif
#if !defined(MIN_EVICT_OBJECTS)
#define MIN_EVICT_OBJECTS 2
#endif
#if !defined(PCR_SELECT_MIN)
#define PCR_SELECT_MIN ((PLATFORM_PCR + 7) / 8)
#endif
#if !defined(PCR_SELECT_MAX)
#define PCR_SELECT_MAX ((IMPLEMENTATION_PCR + 7) / 8)
#endif
#if !defined(NUM_POLICY_PCR_GROUP)
#define NUM_POLICY_PCR_GROUP 1
#endif
#if !defined(NUM_AUTHVALUE_PCR_GROUP)
#define NUM_AUTHVALUE_PCR_GROUP 1
#endif
#if !defined(MAX_CONTEXT_SIZE)
#define MAX_CONTEXT_SIZE 4000
#endif
#if !defined(MAX_DIGEST_BUFFER)
#define MAX_DIGEST_BUFFER 1024
#endif
#if !defined(MAX_NV_INDEX_SIZE)
#define MAX_NV_INDEX_SIZE 2048
#endif
#if !defined(MAX_NV_BUFFER_SIZE)
#define MAX_NV_BUFFER_SIZE 1024
#endif
#if !defined(MAX_CAP_BUFFER)
#define MAX_CAP_BUFFER 1024
#endif
#if !defined(NV_MEMORY_SIZE)
#define NV_MEMORY_SIZE 16384
#endif
#if !defined(NUM_STATIC_PCR)
#define NUM_STATIC_PCR 16
#endif
#if !defined(MAX_ALG_LIST_SIZE)
#define MAX_ALG_LIST_SIZE 64
#endif
#if !defined(TIMER_PRESCALE)
#define TIMER_PRESCALE 100000
#endif
#if !defined(PRIMARY_SEED_SIZE)
#define PRIMARY_SEED_SIZE 32
#endif
#if !defined(CONTEXT_ENCRYPT_ALG)
#define CONTEXT_ENCRYPT_ALG trunks::TPM_ALG_AES
#endif
#if !defined(CONTEXT_ENCRYPT_KEY_BITS)
#define CONTEXT_ENCRYPT_KEY_BITS MAX_SYM_KEY_BITS
#endif
#if !defined(CONTEXT_ENCRYPT_KEY_BYTES)
#define CONTEXT_ENCRYPT_KEY_BYTES ((CONTEXT_ENCRYPT_KEY_BITS + 7) / 8)
#endif
#if !defined(CONTEXT_INTEGRITY_HASH_ALG)
#define CONTEXT_INTEGRITY_HASH_ALG trunks::TPM_ALG_SHA256
#endif
#if !defined(CONTEXT_INTEGRITY_HASH_SIZE)
#define CONTEXT_INTEGRITY_HASH_SIZE SHA256_DIGEST_SIZE
#endif
#if !defined(PROOF_SIZE)
#define PROOF_SIZE CONTEXT_INTEGRITY_HASH_SIZE
#endif
#if !defined(NV_CLOCK_UPDATE_INTERVAL)
#define NV_CLOCK_UPDATE_INTERVAL 12
#endif
#if !defined(NUM_POLICY_PCR)
#define NUM_POLICY_PCR 1
#endif
#if !defined(MAX_COMMAND_SIZE)
#define MAX_COMMAND_SIZE 4096
#endif
#if !defined(MAX_RESPONSE_SIZE)
#define MAX_RESPONSE_SIZE 4096
#endif
#if !defined(ORDERLY_BITS)
#define ORDERLY_BITS 8
#endif
#if !defined(MAX_ORDERLY_COUNT)
#define MAX_ORDERLY_COUNT ((1 << ORDERLY_BITS) - 1)
#endif
#if !defined(ALG_ID_FIRST)
#define ALG_ID_FIRST trunks::TPM_ALG_FIRST
#endif
#if !defined(ALG_ID_LAST)
#define ALG_ID_LAST trunks::TPM_ALG_LAST
#endif
#if !defined(MAX_SYM_DATA)
#define MAX_SYM_DATA 128
#endif
#if !defined(MAX_RNG_ENTROPY_SIZE)
#define MAX_RNG_ENTROPY_SIZE 64
#endif
#if !defined(RAM_INDEX_SPACE)
#define RAM_INDEX_SPACE 512
#endif
#if !defined(RSA_DEFAULT_PUBLIC_EXPONENT)
#define RSA_DEFAULT_PUBLIC_EXPONENT 0x00010001
#endif
#if !defined(ENABLE_PCR_NO_INCREMENT)
#define ENABLE_PCR_NO_INCREMENT YES
#endif
#if !defined(CRT_FORMAT_RSA)
#define CRT_FORMAT_RSA YES
#endif
#if !defined(PRIVATE_VENDOR_SPECIFIC_BYTES)
#define PRIVATE_VENDOR_SPECIFIC_BYTES \
  ((MAX_RSA_KEY_BYTES / 2) * (3 + CRT_FORMAT_RSA * 2))
#endif
#if !defined(MAX_CAP_DATA)
#define MAX_CAP_DATA \
  (MAX_CAP_BUFFER - sizeof(trunks::TPM_CAP) - sizeof(trunks::UINT32))
#endif
#if !defined(MAX_CAP_ALGS)
#define MAX_CAP_ALGS (trunks::TPM_ALG_LAST - trunks::TPM_ALG_FIRST + 1)
#endif
#if !defined(MAX_CAP_HANDLES)
#define MAX_CAP_HANDLES (MAX_CAP_DATA / sizeof(trunks::TPM_HANDLE))
#endif
#if !defined(MAX_CAP_CC)
#define MAX_CAP_CC ((trunks::TPM_CC_LAST - trunks::TPM_CC_FIRST) + 1)
#endif
#if !defined(MAX_CAP_CCE)
#define MAX_CAP_CCE ((trunks::TPM_CCE_LAST - trunks::TPM_CCE_FIRST) + 1)
#endif
#if !defined(MAX_CAP_CC_ALL)
#define MAX_CAP_CC_ALL (MAX_CAP_CC + MAX_CAP_CCE)
#endif
#if !defined(MAX_TPM_PROPERTIES)
#define MAX_TPM_PROPERTIES (MAX_CAP_DATA / sizeof(trunks::TPMS_TAGGED_PROPERTY))
#endif
#if !defined(MAX_PCR_PROPERTIES)
#define MAX_PCR_PROPERTIES \
  (MAX_CAP_DATA / sizeof(trunks::TPMS_TAGGED_PCR_SELECT))
#endif
#if !defined(MAX_ECC_CURVES)
#define MAX_ECC_CURVES (MAX_CAP_DATA / sizeof(trunks::TPM_ECC_CURVE))
#endif
#if !defined(HASH_COUNT)
#define HASH_COUNT 5
#endif

typedef uint8_t UINT8;
typedef uint8_t BYTE;
typedef int8_t INT8;
typedef int BOOL;
typedef uint16_t UINT16;
typedef int16_t INT16;
typedef uint32_t UINT32;
typedef int32_t INT32;
typedef uint64_t UINT64;
typedef int64_t INT64;
typedef UINT32 TPM_ALGORITHM_ID;
typedef UINT32 TPM_MODIFIER_INDICATOR;
typedef UINT32 TPM_AUTHORIZATION_SIZE;
typedef UINT32 TPM_PARAMETER_SIZE;
typedef UINT16 TPM_KEY_SIZE;
typedef UINT16 TPM_KEY_BITS;
typedef UINT32 TPM_HANDLE;
struct TPM2B_DIGEST;
typedef TPM2B_DIGEST TPM2B_NONCE;
typedef TPM2B_DIGEST TPM2B_AUTH;
typedef TPM2B_DIGEST TPM2B_OPERAND;
struct TPMS_SCHEME_SIGHASH;
typedef TPMS_SCHEME_SIGHASH TPMS_SCHEME_HMAC;
typedef TPMS_SCHEME_SIGHASH TPMS_SCHEME_RSASSA;
typedef TPMS_SCHEME_SIGHASH TPMS_SCHEME_RSAPSS;
typedef TPMS_SCHEME_SIGHASH TPMS_SCHEME_ECDSA;
typedef TPMS_SCHEME_SIGHASH TPMS_SCHEME_SM2;
typedef TPMS_SCHEME_SIGHASH TPMS_SCHEME_ECSCHNORR;
typedef BYTE TPMI_YES_NO;
typedef TPM_HANDLE TPMI_DH_OBJECT;
typedef TPM_HANDLE TPMI_DH_PERSISTENT;
typedef TPM_HANDLE TPMI_DH_ENTITY;
typedef TPM_HANDLE TPMI_DH_PCR;
typedef TPM_HANDLE TPMI_SH_AUTH_SESSION;
typedef TPM_HANDLE TPMI_SH_HMAC;
typedef TPM_HANDLE TPMI_SH_POLICY;
typedef TPM_HANDLE TPMI_DH_CONTEXT;
typedef TPM_HANDLE TPMI_RH_HIERARCHY;
typedef TPM_HANDLE TPMI_RH_ENABLES;
typedef TPM_HANDLE TPMI_RH_HIERARCHY_AUTH;
typedef TPM_HANDLE TPMI_RH_PLATFORM;
typedef TPM_HANDLE TPMI_RH_OWNER;
typedef TPM_HANDLE TPMI_RH_ENDORSEMENT;
typedef TPM_HANDLE TPMI_RH_PROVISION;
typedef TPM_HANDLE TPMI_RH_CLEAR;
typedef TPM_HANDLE TPMI_RH_NV_AUTH;
typedef TPM_HANDLE TPMI_RH_LOCKOUT;
typedef TPM_HANDLE TPMI_RH_NV_INDEX;
typedef UINT16 TPM_ALG_ID;
typedef TPM_ALG_ID TPMI_ALG_HASH;
typedef TPM_ALG_ID TPMI_ALG_ASYM;
typedef TPM_ALG_ID TPMI_ALG_SYM;
typedef TPM_ALG_ID TPMI_ALG_SYM_OBJECT;
typedef TPM_ALG_ID TPMI_ALG_SYM_MODE;
typedef TPM_ALG_ID TPMI_ALG_KDF;
typedef TPM_ALG_ID TPMI_ALG_SIG_SCHEME;
typedef TPM_ALG_ID TPMI_ECC_KEY_EXCHANGE;
typedef UINT16 TPM_ST;
typedef TPM_ST TPMI_ST_COMMAND_TAG;
typedef TPM_ST TPMI_ST_ATTEST;
typedef TPM_KEY_BITS TPMI_AES_KEY_BITS;
typedef TPM_KEY_BITS TPMI_SM4_KEY_BITS;
typedef TPM_ALG_ID TPMI_ALG_KEYEDHASH_SCHEME;
typedef TPM_ALG_ID TPMI_ALG_ASYM_SCHEME;
typedef TPM_ALG_ID TPMI_ALG_RSA_SCHEME;
typedef TPM_ALG_ID TPMI_ALG_RSA_DECRYPT;
typedef TPM_KEY_BITS TPMI_RSA_KEY_BITS;
typedef TPM_ALG_ID TPMI_ALG_ECC_SCHEME;
typedef UINT16 TPM_ECC_CURVE;
typedef TPM_ECC_CURVE TPMI_ECC_CURVE;
typedef TPM_ALG_ID TPMI_ALG_PUBLIC;
typedef UINT32 TPMA_ALGORITHM;
typedef UINT32 TPMA_OBJECT;
typedef UINT8 TPMA_SESSION;
typedef UINT8 TPMA_LOCALITY;
typedef UINT32 TPMA_PERMANENT;
typedef UINT32 TPMA_STARTUP_CLEAR;
typedef UINT32 TPMA_MEMORY;
typedef UINT32 TPM_CC;
typedef TPM_CC TPMA_CC;
typedef UINT32 TPM_NV_INDEX;
typedef UINT32 TPMA_NV;
typedef UINT32 TPM_SPEC;
typedef UINT32 TPM_GENERATED;
typedef UINT32 TPM_RC;
typedef INT8 TPM_CLOCK_ADJUST;
typedef UINT16 TPM_EO;
typedef UINT16 TPM_SU;
typedef UINT8 TPM_SE;
typedef UINT32 TPM_CAP;
typedef UINT32 TPM_PT;
typedef UINT32 TPM_PT_PCR;
typedef UINT32 TPM_PS;
typedef UINT8 TPM_HT;
typedef UINT32 TPM_RH;
typedef TPM_HANDLE TPM_HC;

constexpr TPM_SPEC TPM_SPEC_FAMILY = 0x322E3000;
constexpr TPM_SPEC TPM_SPEC_LEVEL = 00;
constexpr TPM_SPEC TPM_SPEC_VERSION = 99;
constexpr TPM_SPEC TPM_SPEC_YEAR = 2013;
constexpr TPM_SPEC TPM_SPEC_DAY_OF_YEAR = 304;
constexpr TPM_GENERATED TPM_GENERATED_VALUE = 0xff544347;
constexpr TPM_ALG_ID TPM_ALG_ERROR = 0x0000;
constexpr TPM_ALG_ID TPM_ALG_FIRST = 0x0001;
constexpr TPM_ALG_ID TPM_ALG_RSA = 0x0001;
constexpr TPM_ALG_ID TPM_ALG_SHA = 0x0004;
constexpr TPM_ALG_ID TPM_ALG_SHA1 = 0x0004;
constexpr TPM_ALG_ID TPM_ALG_HMAC = 0x0005;
constexpr TPM_ALG_ID TPM_ALG_AES = 0x0006;
constexpr TPM_ALG_ID TPM_ALG_MGF1 = 0x0007;
constexpr TPM_ALG_ID TPM_ALG_KEYEDHASH = 0x0008;
constexpr TPM_ALG_ID TPM_ALG_XOR = 0x000A;
constexpr TPM_ALG_ID TPM_ALG_SHA256 = 0x000B;
constexpr TPM_ALG_ID TPM_ALG_SHA384 = 0x000C;
constexpr TPM_ALG_ID TPM_ALG_SHA512 = 0x000D;
constexpr TPM_ALG_ID TPM_ALG_NULL = 0x0010;
constexpr TPM_ALG_ID TPM_ALG_SM3_256 = 0x0012;
constexpr TPM_ALG_ID TPM_ALG_SM4 = 0x0013;
constexpr TPM_ALG_ID TPM_ALG_RSASSA = 0x0014;
constexpr TPM_ALG_ID TPM_ALG_RSAES = 0x0015;
constexpr TPM_ALG_ID TPM_ALG_RSAPSS = 0x0016;
constexpr TPM_ALG_ID TPM_ALG_OAEP = 0x0017;
constexpr TPM_ALG_ID TPM_ALG_ECDSA = 0x0018;
constexpr TPM_ALG_ID TPM_ALG_ECDH = 0x0019;
constexpr TPM_ALG_ID TPM_ALG_ECDAA = 0x001A;
constexpr TPM_ALG_ID TPM_ALG_SM2 = 0x001B;
constexpr TPM_ALG_ID TPM_ALG_ECSCHNORR = 0x001C;
constexpr TPM_ALG_ID TPM_ALG_ECMQV = 0x001D;
constexpr TPM_ALG_ID TPM_ALG_KDF1_SP800_56a = 0x0020;
constexpr TPM_ALG_ID TPM_ALG_KDF2 = 0x0021;
constexpr TPM_ALG_ID TPM_ALG_KDF1_SP800_108 = 0x0022;
constexpr TPM_ALG_ID TPM_ALG_ECC = 0x0023;
constexpr TPM_ALG_ID TPM_ALG_SYMCIPHER = 0x0025;
constexpr TPM_ALG_ID TPM_ALG_CTR = 0x0040;
constexpr TPM_ALG_ID TPM_ALG_OFB = 0x0041;
constexpr TPM_ALG_ID TPM_ALG_CBC = 0x0042;
constexpr TPM_ALG_ID TPM_ALG_CFB = 0x0043;
constexpr TPM_ALG_ID TPM_ALG_ECB = 0x0044;
constexpr TPM_ALG_ID TPM_ALG_LAST = 0x0044;
constexpr TPM_ECC_CURVE TPM_ECC_NONE = 0x0000;
constexpr TPM_ECC_CURVE TPM_ECC_NIST_P192 = 0x0001;
constexpr TPM_ECC_CURVE TPM_ECC_NIST_P224 = 0x0002;
constexpr TPM_ECC_CURVE TPM_ECC_NIST_P256 = 0x0003;
constexpr TPM_ECC_CURVE TPM_ECC_NIST_P384 = 0x0004;
constexpr TPM_ECC_CURVE TPM_ECC_NIST_P521 = 0x0005;
constexpr TPM_ECC_CURVE TPM_ECC_BN_P256 = 0x0010;
constexpr TPM_ECC_CURVE TPM_ECC_BN_P638 = 0x0011;
constexpr TPM_ECC_CURVE TPM_ECC_SM2_P256 = 0x0020;
constexpr TPM_CC TPM_CC_FIRST = 0x0000011F;
constexpr TPM_CC TPM_CC_PP_FIRST = 0x0000011F;
constexpr TPM_CC TPM_CC_NV_UndefineSpaceSpecial = 0x0000011F;
constexpr TPM_CC TPM_CC_EvictControl = 0x00000120;
constexpr TPM_CC TPM_CC_HierarchyControl = 0x00000121;
constexpr TPM_CC TPM_CC_NV_UndefineSpace = 0x00000122;
constexpr TPM_CC TPM_CC_ChangeEPS = 0x00000124;
constexpr TPM_CC TPM_CC_ChangePPS = 0x00000125;
constexpr TPM_CC TPM_CC_Clear = 0x00000126;
constexpr TPM_CC TPM_CC_ClearControl = 0x00000127;
constexpr TPM_CC TPM_CC_ClockSet = 0x00000128;
constexpr TPM_CC TPM_CC_HierarchyChangeAuth = 0x00000129;
constexpr TPM_CC TPM_CC_NV_DefineSpace = 0x0000012A;
constexpr TPM_CC TPM_CC_PCR_Allocate = 0x0000012B;
constexpr TPM_CC TPM_CC_PCR_SetAuthPolicy = 0x0000012C;
constexpr TPM_CC TPM_CC_PP_Commands = 0x0000012D;
constexpr TPM_CC TPM_CC_SetPrimaryPolicy = 0x0000012E;
constexpr TPM_CC TPM_CC_FieldUpgradeStart = 0x0000012F;
constexpr TPM_CC TPM_CC_ClockRateAdjust = 0x00000130;
constexpr TPM_CC TPM_CC_CreatePrimary = 0x00000131;
constexpr TPM_CC TPM_CC_NV_GlobalWriteLock = 0x00000132;
constexpr TPM_CC TPM_CC_PP_LAST = 0x00000132;
constexpr TPM_CC TPM_CC_GetCommandAuditDigest = 0x00000133;
constexpr TPM_CC TPM_CC_NV_Increment = 0x00000134;
constexpr TPM_CC TPM_CC_NV_SetBits = 0x00000135;
constexpr TPM_CC TPM_CC_NV_Extend = 0x00000136;
constexpr TPM_CC TPM_CC_NV_Write = 0x00000137;
constexpr TPM_CC TPM_CC_NV_WriteLock = 0x00000138;
constexpr TPM_CC TPM_CC_DictionaryAttackLockReset = 0x00000139;
constexpr TPM_CC TPM_CC_DictionaryAttackParameters = 0x0000013A;
constexpr TPM_CC TPM_CC_NV_ChangeAuth = 0x0000013B;
constexpr TPM_CC TPM_CC_PCR_Event = 0x0000013C;
constexpr TPM_CC TPM_CC_PCR_Reset = 0x0000013D;
constexpr TPM_CC TPM_CC_SequenceComplete = 0x0000013E;
constexpr TPM_CC TPM_CC_SetAlgorithmSet = 0x0000013F;
constexpr TPM_CC TPM_CC_SetCommandCodeAuditStatus = 0x00000140;
constexpr TPM_CC TPM_CC_FieldUpgradeData = 0x00000141;
constexpr TPM_CC TPM_CC_IncrementalSelfTest = 0x00000142;
constexpr TPM_CC TPM_CC_SelfTest = 0x00000143;
constexpr TPM_CC TPM_CC_Startup = 0x00000144;
constexpr TPM_CC TPM_CC_Shutdown = 0x00000145;
constexpr TPM_CC TPM_CC_StirRandom = 0x00000146;
constexpr TPM_CC TPM_CC_ActivateCredential = 0x00000147;
constexpr TPM_CC TPM_CC_Certify = 0x00000148;
constexpr TPM_CC TPM_CC_PolicyNV = 0x00000149;
constexpr TPM_CC TPM_CC_CertifyCreation = 0x0000014A;
constexpr TPM_CC TPM_CC_Duplicate = 0x0000014B;
constexpr TPM_CC TPM_CC_GetTime = 0x0000014C;
constexpr TPM_CC TPM_CC_GetSessionAuditDigest = 0x0000014D;
constexpr TPM_CC TPM_CC_NV_Read = 0x0000014E;
constexpr TPM_CC TPM_CC_NV_ReadLock = 0x0000014F;
constexpr TPM_CC TPM_CC_ObjectChangeAuth = 0x00000150;
constexpr TPM_CC TPM_CC_PolicySecret = 0x00000151;
constexpr TPM_CC TPM_CC_Rewrap = 0x00000152;
constexpr TPM_CC TPM_CC_Create = 0x00000153;
constexpr TPM_CC TPM_CC_ECDH_ZGen = 0x00000154;
constexpr TPM_CC TPM_CC_HMAC = 0x00000155;
constexpr TPM_CC TPM_CC_Import = 0x00000156;
constexpr TPM_CC TPM_CC_Load = 0x00000157;
constexpr TPM_CC TPM_CC_Quote = 0x00000158;
constexpr TPM_CC TPM_CC_RSA_Decrypt = 0x00000159;
constexpr TPM_CC TPM_CC_HMAC_Start = 0x0000015B;
constexpr TPM_CC TPM_CC_SequenceUpdate = 0x0000015C;
constexpr TPM_CC TPM_CC_Sign = 0x0000015D;
constexpr TPM_CC TPM_CC_Unseal = 0x0000015E;
constexpr TPM_CC TPM_CC_PolicySigned = 0x00000160;
constexpr TPM_CC TPM_CC_ContextLoad = 0x00000161;
constexpr TPM_CC TPM_CC_ContextSave = 0x00000162;
constexpr TPM_CC TPM_CC_ECDH_KeyGen = 0x00000163;
constexpr TPM_CC TPM_CC_EncryptDecrypt = 0x00000164;
constexpr TPM_CC TPM_CC_FlushContext = 0x00000165;
constexpr TPM_CC TPM_CC_LoadExternal = 0x00000167;
constexpr TPM_CC TPM_CC_MakeCredential = 0x00000168;
constexpr TPM_CC TPM_CC_NV_ReadPublic = 0x00000169;
constexpr TPM_CC TPM_CC_PolicyAuthorize = 0x0000016A;
constexpr TPM_CC TPM_CC_PolicyAuthValue = 0x0000016B;
constexpr TPM_CC TPM_CC_PolicyCommandCode = 0x0000016C;
constexpr TPM_CC TPM_CC_PolicyCounterTimer = 0x0000016D;
constexpr TPM_CC TPM_CC_PolicyCpHash = 0x0000016E;
constexpr TPM_CC TPM_CC_PolicyLocality = 0x0000016F;
constexpr TPM_CC TPM_CC_PolicyNameHash = 0x00000170;
constexpr TPM_CC TPM_CC_PolicyOR = 0x00000171;
constexpr TPM_CC TPM_CC_PolicyTicket = 0x00000172;
constexpr TPM_CC TPM_CC_ReadPublic = 0x00000173;
constexpr TPM_CC TPM_CC_RSA_Encrypt = 0x00000174;
constexpr TPM_CC TPM_CC_StartAuthSession = 0x00000176;
constexpr TPM_CC TPM_CC_VerifySignature = 0x00000177;
constexpr TPM_CC TPM_CC_ECC_Parameters = 0x00000178;
constexpr TPM_CC TPM_CC_FirmwareRead = 0x00000179;
constexpr TPM_CC TPM_CC_GetCapability = 0x0000017A;
constexpr TPM_CC TPM_CC_GetRandom = 0x0000017B;
constexpr TPM_CC TPM_CC_GetTestResult = 0x0000017C;
constexpr TPM_CC TPM_CC_Hash = 0x0000017D;
constexpr TPM_CC TPM_CC_PCR_Read = 0x0000017E;
constexpr TPM_CC TPM_CC_PolicyPCR = 0x0000017F;
constexpr TPM_CC TPM_CC_PolicyRestart = 0x00000180;
constexpr TPM_CC TPM_CC_ReadClock = 0x00000181;
constexpr TPM_CC TPM_CC_PCR_Extend = 0x00000182;
constexpr TPM_CC TPM_CC_PCR_SetAuthValue = 0x00000183;
constexpr TPM_CC TPM_CC_NV_Certify = 0x00000184;
constexpr TPM_CC TPM_CC_EventSequenceComplete = 0x00000185;
constexpr TPM_CC TPM_CC_HashSequenceStart = 0x00000186;
constexpr TPM_CC TPM_CC_PolicyPhysicalPresence = 0x00000187;
constexpr TPM_CC TPM_CC_PolicyDuplicationSelect = 0x00000188;
constexpr TPM_CC TPM_CC_PolicyGetDigest = 0x00000189;
constexpr TPM_CC TPM_CC_TestParms = 0x0000018A;
constexpr TPM_CC TPM_CC_Commit = 0x0000018B;
constexpr TPM_CC TPM_CC_PolicyPassword = 0x0000018C;
constexpr TPM_CC TPM_CC_ZGen_2Phase = 0x0000018D;
constexpr TPM_CC TPM_CC_EC_Ephemeral = 0x0000018E;
constexpr TPM_CC TPM_CC_PolicyNvWritten = 0x0000018F;
constexpr TPM_CC TPM_CC_LAST = 0x0000018F;
constexpr TPM_CC TPM_CCE_FIRST = 0x20008001;
constexpr TPM_CC TPM_CCE_PolicyFidoSigned = 0x20008001;
constexpr TPM_CC TPM_CCE_LAST = 0x20008001;
constexpr TPM_RC TPM_RC_SUCCESS = 0x000;
constexpr TPM_RC TPM_RC_BAD_TAG = 0x01E;
constexpr TPM_RC RC_VER1 = 0x100;
constexpr TPM_RC TPM_RC_INITIALIZE = RC_VER1 + 0x000;
constexpr TPM_RC TPM_RC_FAILURE = RC_VER1 + 0x001;
constexpr TPM_RC TPM_RC_SEQUENCE = RC_VER1 + 0x003;
constexpr TPM_RC TPM_RC_PRIVATE = RC_VER1 + 0x00B;
constexpr TPM_RC TPM_RC_HMAC = RC_VER1 + 0x019;
constexpr TPM_RC TPM_RC_DISABLED = RC_VER1 + 0x020;
constexpr TPM_RC TPM_RC_EXCLUSIVE = RC_VER1 + 0x021;
constexpr TPM_RC TPM_RC_AUTH_TYPE = RC_VER1 + 0x024;
constexpr TPM_RC TPM_RC_AUTH_MISSING = RC_VER1 + 0x025;
constexpr TPM_RC TPM_RC_POLICY = RC_VER1 + 0x026;
constexpr TPM_RC TPM_RC_PCR = RC_VER1 + 0x027;
constexpr TPM_RC TPM_RC_PCR_CHANGED = RC_VER1 + 0x028;
constexpr TPM_RC TPM_RC_UPGRADE = RC_VER1 + 0x02D;
constexpr TPM_RC TPM_RC_TOO_MANY_CONTEXTS = RC_VER1 + 0x02E;
constexpr TPM_RC TPM_RC_AUTH_UNAVAILABLE = RC_VER1 + 0x02F;
constexpr TPM_RC TPM_RC_REBOOT = RC_VER1 + 0x030;
constexpr TPM_RC TPM_RC_UNBALANCED = RC_VER1 + 0x031;
constexpr TPM_RC TPM_RC_COMMAND_SIZE = RC_VER1 + 0x042;
constexpr TPM_RC TPM_RC_COMMAND_CODE = RC_VER1 + 0x043;
constexpr TPM_RC TPM_RC_AUTHSIZE = RC_VER1 + 0x044;
constexpr TPM_RC TPM_RC_AUTH_CONTEXT = RC_VER1 + 0x045;
constexpr TPM_RC TPM_RC_NV_RANGE = RC_VER1 + 0x046;
constexpr TPM_RC TPM_RC_NV_SIZE = RC_VER1 + 0x047;
constexpr TPM_RC TPM_RC_NV_LOCKED = RC_VER1 + 0x048;
constexpr TPM_RC TPM_RC_NV_AUTHORIZATION = RC_VER1 + 0x049;
constexpr TPM_RC TPM_RC_NV_UNINITIALIZED = RC_VER1 + 0x04A;
constexpr TPM_RC TPM_RC_NV_SPACE = RC_VER1 + 0x04B;
constexpr TPM_RC TPM_RC_NV_DEFINED = RC_VER1 + 0x04C;
constexpr TPM_RC TPM_RC_BAD_CONTEXT = RC_VER1 + 0x050;
constexpr TPM_RC TPM_RC_CPHASH = RC_VER1 + 0x051;
constexpr TPM_RC TPM_RC_PARENT = RC_VER1 + 0x052;
constexpr TPM_RC TPM_RC_NEEDS_TEST = RC_VER1 + 0x053;
constexpr TPM_RC TPM_RC_NO_RESULT = RC_VER1 + 0x054;
constexpr TPM_RC TPM_RC_SENSITIVE = RC_VER1 + 0x055;
constexpr TPM_RC RC_MAX_FM0 = RC_VER1 + 0x07F;
constexpr TPM_RC RC_FMT1 = 0x080;
constexpr TPM_RC TPM_RC_ASYMMETRIC = RC_FMT1 + 0x001;
constexpr TPM_RC TPM_RC_ATTRIBUTES = RC_FMT1 + 0x002;
constexpr TPM_RC TPM_RC_HASH = RC_FMT1 + 0x003;
constexpr TPM_RC TPM_RC_VALUE = RC_FMT1 + 0x004;
constexpr TPM_RC TPM_RC_HIERARCHY = RC_FMT1 + 0x005;
constexpr TPM_RC TPM_RC_KEY_SIZE = RC_FMT1 + 0x007;
constexpr TPM_RC TPM_RC_MGF = RC_FMT1 + 0x008;
constexpr TPM_RC TPM_RC_MODE = RC_FMT1 + 0x009;
constexpr TPM_RC TPM_RC_TYPE = RC_FMT1 + 0x00A;
constexpr TPM_RC TPM_RC_HANDLE = RC_FMT1 + 0x00B;
constexpr TPM_RC TPM_RC_KDF = RC_FMT1 + 0x00C;
constexpr TPM_RC TPM_RC_RANGE = RC_FMT1 + 0x00D;
constexpr TPM_RC TPM_RC_AUTH_FAIL = RC_FMT1 + 0x00E;
constexpr TPM_RC TPM_RC_NONCE = RC_FMT1 + 0x00F;
constexpr TPM_RC TPM_RC_PP = RC_FMT1 + 0x010;
constexpr TPM_RC TPM_RC_SCHEME = RC_FMT1 + 0x012;
constexpr TPM_RC TPM_RC_SIZE = RC_FMT1 + 0x015;
constexpr TPM_RC TPM_RC_SYMMETRIC = RC_FMT1 + 0x016;
constexpr TPM_RC TPM_RC_TAG = RC_FMT1 + 0x017;
constexpr TPM_RC TPM_RC_SELECTOR = RC_FMT1 + 0x018;
constexpr TPM_RC TPM_RC_INSUFFICIENT = RC_FMT1 + 0x01A;
constexpr TPM_RC TPM_RC_SIGNATURE = RC_FMT1 + 0x01B;
constexpr TPM_RC TPM_RC_KEY = RC_FMT1 + 0x01C;
constexpr TPM_RC TPM_RC_POLICY_FAIL = RC_FMT1 + 0x01D;
constexpr TPM_RC TPM_RC_INTEGRITY = RC_FMT1 + 0x01F;
constexpr TPM_RC TPM_RC_TICKET = RC_FMT1 + 0x020;
constexpr TPM_RC TPM_RC_RESERVED_BITS = RC_FMT1 + 0x021;
constexpr TPM_RC TPM_RC_BAD_AUTH = RC_FMT1 + 0x022;
constexpr TPM_RC TPM_RC_EXPIRED = RC_FMT1 + 0x023;
constexpr TPM_RC TPM_RC_POLICY_CC = RC_FMT1 + 0x024;
constexpr TPM_RC TPM_RC_BINDING = RC_FMT1 + 0x025;
constexpr TPM_RC TPM_RC_CURVE = RC_FMT1 + 0x026;
constexpr TPM_RC TPM_RC_ECC_POINT = RC_FMT1 + 0x027;
constexpr TPM_RC RC_WARN = 0x900;
constexpr TPM_RC TPM_RC_CONTEXT_GAP = RC_WARN + 0x001;
constexpr TPM_RC TPM_RC_OBJECT_MEMORY = RC_WARN + 0x002;
constexpr TPM_RC TPM_RC_SESSION_MEMORY = RC_WARN + 0x003;
constexpr TPM_RC TPM_RC_MEMORY = RC_WARN + 0x004;
constexpr TPM_RC TPM_RC_SESSION_HANDLES = RC_WARN + 0x005;
constexpr TPM_RC TPM_RC_OBJECT_HANDLES = RC_WARN + 0x006;
constexpr TPM_RC TPM_RC_LOCALITY = RC_WARN + 0x007;
constexpr TPM_RC TPM_RC_YIELDED = RC_WARN + 0x008;
constexpr TPM_RC TPM_RC_CANCELED = RC_WARN + 0x009;
constexpr TPM_RC TPM_RC_TESTING = RC_WARN + 0x00A;
constexpr TPM_RC TPM_RC_REFERENCE_H0 = RC_WARN + 0x010;
constexpr TPM_RC TPM_RC_REFERENCE_H1 = RC_WARN + 0x011;
constexpr TPM_RC TPM_RC_REFERENCE_H2 = RC_WARN + 0x012;
constexpr TPM_RC TPM_RC_REFERENCE_H3 = RC_WARN + 0x013;
constexpr TPM_RC TPM_RC_REFERENCE_H4 = RC_WARN + 0x014;
constexpr TPM_RC TPM_RC_REFERENCE_H5 = RC_WARN + 0x015;
constexpr TPM_RC TPM_RC_REFERENCE_H6 = RC_WARN + 0x016;
constexpr TPM_RC TPM_RC_REFERENCE_S0 = RC_WARN + 0x018;
constexpr TPM_RC TPM_RC_REFERENCE_S1 = RC_WARN + 0x019;
constexpr TPM_RC TPM_RC_REFERENCE_S2 = RC_WARN + 0x01A;
constexpr TPM_RC TPM_RC_REFERENCE_S3 = RC_WARN + 0x01B;
constexpr TPM_RC TPM_RC_REFERENCE_S4 = RC_WARN + 0x01C;
constexpr TPM_RC TPM_RC_REFERENCE_S5 = RC_WARN + 0x01D;
constexpr TPM_RC TPM_RC_REFERENCE_S6 = RC_WARN + 0x01E;
constexpr TPM_RC TPM_RC_NV_RATE = RC_WARN + 0x020;
constexpr TPM_RC TPM_RC_LOCKOUT = RC_WARN + 0x021;
constexpr TPM_RC TPM_RC_RETRY = RC_WARN + 0x022;
constexpr TPM_RC TPM_RC_NV_UNAVAILABLE = RC_WARN + 0x023;
constexpr TPM_RC TPM_RC_NOT_USED = RC_WARN + 0x7F;
constexpr TPM_RC TPM_RC_H = 0x000;
constexpr TPM_RC TPM_RC_P = 0x040;
constexpr TPM_RC TPM_RC_S = 0x800;
constexpr TPM_RC TPM_RC_1 = 0x100;
constexpr TPM_RC TPM_RC_2 = 0x200;
constexpr TPM_RC TPM_RC_3 = 0x300;
constexpr TPM_RC TPM_RC_4 = 0x400;
constexpr TPM_RC TPM_RC_5 = 0x500;
constexpr TPM_RC TPM_RC_6 = 0x600;
constexpr TPM_RC TPM_RC_7 = 0x700;
constexpr TPM_RC TPM_RC_8 = 0x800;
constexpr TPM_RC TPM_RC_9 = 0x900;
constexpr TPM_RC TPM_RC_A = 0xA00;
constexpr TPM_RC TPM_RC_B = 0xB00;
constexpr TPM_RC TPM_RC_C = 0xC00;
constexpr TPM_RC TPM_RC_D = 0xD00;
constexpr TPM_RC TPM_RC_E = 0xE00;
constexpr TPM_RC TPM_RC_F = 0xF00;
constexpr TPM_RC TPM_RC_N_MASK = 0xF00;
constexpr TPM_CLOCK_ADJUST TPM_CLOCK_COARSE_SLOWER = -3;
constexpr TPM_CLOCK_ADJUST TPM_CLOCK_MEDIUM_SLOWER = -2;
constexpr TPM_CLOCK_ADJUST TPM_CLOCK_FINE_SLOWER = -1;
constexpr TPM_CLOCK_ADJUST TPM_CLOCK_NO_CHANGE = 0;
constexpr TPM_CLOCK_ADJUST TPM_CLOCK_FINE_FASTER = 1;
constexpr TPM_CLOCK_ADJUST TPM_CLOCK_MEDIUM_FASTER = 2;
constexpr TPM_CLOCK_ADJUST TPM_CLOCK_COARSE_FASTER = 3;
constexpr TPM_EO TPM_EO_EQ = 0x0000;
constexpr TPM_EO TPM_EO_NEQ = 0x0001;
constexpr TPM_EO TPM_EO_SIGNED_GT = 0x0002;
constexpr TPM_EO TPM_EO_UNSIGNED_GT = 0x0003;
constexpr TPM_EO TPM_EO_SIGNED_LT = 0x0004;
constexpr TPM_EO TPM_EO_UNSIGNED_LT = 0x0005;
constexpr TPM_EO TPM_EO_SIGNED_GE = 0x0006;
constexpr TPM_EO TPM_EO_UNSIGNED_GE = 0x0007;
constexpr TPM_EO TPM_EO_SIGNED_LE = 0x0008;
constexpr TPM_EO TPM_EO_UNSIGNED_LE = 0x0009;
constexpr TPM_EO TPM_EO_BITSET = 0x000A;
constexpr TPM_EO TPM_EO_BITCLEAR = 0x000B;
constexpr TPM_ST TPM_ST_RSP_COMMAND = 0x00C4;
constexpr TPM_ST TPM_ST_NULL = 0X8000;
constexpr TPM_ST TPM_ST_NO_SESSIONS = 0x8001;
constexpr TPM_ST TPM_ST_SESSIONS = 0x8002;
constexpr TPM_ST TPM_ST_ATTEST_NV = 0x8014;
constexpr TPM_ST TPM_ST_ATTEST_COMMAND_AUDIT = 0x8015;
constexpr TPM_ST TPM_ST_ATTEST_SESSION_AUDIT = 0x8016;
constexpr TPM_ST TPM_ST_ATTEST_CERTIFY = 0x8017;
constexpr TPM_ST TPM_ST_ATTEST_QUOTE = 0x8018;
constexpr TPM_ST TPM_ST_ATTEST_TIME = 0x8019;
constexpr TPM_ST TPM_ST_ATTEST_CREATION = 0x801A;
constexpr TPM_ST TPM_ST_CREATION = 0x8021;
constexpr TPM_ST TPM_ST_VERIFIED = 0x8022;
constexpr TPM_ST TPM_ST_AUTH_SECRET = 0x8023;
constexpr TPM_ST TPM_ST_HASHCHECK = 0x8024;
constexpr TPM_ST TPM_ST_AUTH_SIGNED = 0x8025;
constexpr TPM_ST TPM_ST_FU_MANIFEST = 0x8029;
constexpr TPM_SU TPM_SU_CLEAR = 0x0000;
constexpr TPM_SU TPM_SU_STATE = 0x0001;
constexpr TPM_SE TPM_SE_HMAC = 0x00;
constexpr TPM_SE TPM_SE_POLICY = 0x01;
constexpr TPM_SE TPM_SE_TRIAL = 0x03;
constexpr TPM_CAP TPM_CAP_FIRST = 0x00000000;
constexpr TPM_CAP TPM_CAP_ALGS = 0x00000000;
constexpr TPM_CAP TPM_CAP_HANDLES = 0x00000001;
constexpr TPM_CAP TPM_CAP_COMMANDS = 0x00000002;
constexpr TPM_CAP TPM_CAP_PP_COMMANDS = 0x00000003;
constexpr TPM_CAP TPM_CAP_AUDIT_COMMANDS = 0x00000004;
constexpr TPM_CAP TPM_CAP_PCRS = 0x00000005;
constexpr TPM_CAP TPM_CAP_TPM_PROPERTIES = 0x00000006;
constexpr TPM_CAP TPM_CAP_PCR_PROPERTIES = 0x00000007;
constexpr TPM_CAP TPM_CAP_ECC_CURVES = 0x00000008;
constexpr TPM_CAP TPM_CAP_LAST = 0x00000008;
constexpr TPM_CAP TPM_CAP_VENDOR_PROPERTY = 0x00000100;
constexpr TPM_PT TPM_PT_NONE = 0x00000000;
constexpr TPM_PT PT_GROUP = 0x00000100;
constexpr TPM_PT PT_FIXED = PT_GROUP * 1;
constexpr TPM_PT TPM_PT_FAMILY_INDICATOR = PT_FIXED + 0;
constexpr TPM_PT TPM_PT_LEVEL = PT_FIXED + 1;
constexpr TPM_PT TPM_PT_REVISION = PT_FIXED + 2;
constexpr TPM_PT TPM_PT_DAY_OF_YEAR = PT_FIXED + 3;
constexpr TPM_PT TPM_PT_YEAR = PT_FIXED + 4;
constexpr TPM_PT TPM_PT_MANUFACTURER = PT_FIXED + 5;
constexpr TPM_PT TPM_PT_VENDOR_STRING_1 = PT_FIXED + 6;
constexpr TPM_PT TPM_PT_VENDOR_STRING_2 = PT_FIXED + 7;
constexpr TPM_PT TPM_PT_VENDOR_STRING_3 = PT_FIXED + 8;
constexpr TPM_PT TPM_PT_VENDOR_STRING_4 = PT_FIXED + 9;
constexpr TPM_PT TPM_PT_VENDOR_TPM_TYPE = PT_FIXED + 10;
constexpr TPM_PT TPM_PT_FIRMWARE_VERSION_1 = PT_FIXED + 11;
constexpr TPM_PT TPM_PT_FIRMWARE_VERSION_2 = PT_FIXED + 12;
constexpr TPM_PT TPM_PT_INPUT_BUFFER = PT_FIXED + 13;
constexpr TPM_PT TPM_PT_HR_TRANSIENT_MIN = PT_FIXED + 14;
constexpr TPM_PT TPM_PT_HR_PERSISTENT_MIN = PT_FIXED + 15;
constexpr TPM_PT TPM_PT_HR_LOADED_MIN = PT_FIXED + 16;
constexpr TPM_PT TPM_PT_ACTIVE_SESSIONS_MAX = PT_FIXED + 17;
constexpr TPM_PT TPM_PT_PCR_COUNT = PT_FIXED + 18;
constexpr TPM_PT TPM_PT_PCR_SELECT_MIN = PT_FIXED + 19;
constexpr TPM_PT TPM_PT_CONTEXT_GAP_MAX = PT_FIXED + 20;
constexpr TPM_PT TPM_PT_NV_COUNTERS_MAX = PT_FIXED + 22;
constexpr TPM_PT TPM_PT_NV_INDEX_MAX = PT_FIXED + 23;
constexpr TPM_PT TPM_PT_MEMORY = PT_FIXED + 24;
constexpr TPM_PT TPM_PT_CLOCK_UPDATE = PT_FIXED + 25;
constexpr TPM_PT TPM_PT_CONTEXT_HASH = PT_FIXED + 26;
constexpr TPM_PT TPM_PT_CONTEXT_SYM = PT_FIXED + 27;
constexpr TPM_PT TPM_PT_CONTEXT_SYM_SIZE = PT_FIXED + 28;
constexpr TPM_PT TPM_PT_ORDERLY_COUNT = PT_FIXED + 29;
constexpr TPM_PT TPM_PT_MAX_COMMAND_SIZE = PT_FIXED + 30;
constexpr TPM_PT TPM_PT_MAX_RESPONSE_SIZE = PT_FIXED + 31;
constexpr TPM_PT TPM_PT_MAX_DIGEST = PT_FIXED + 32;
constexpr TPM_PT TPM_PT_MAX_OBJECT_CONTEXT = PT_FIXED + 33;
constexpr TPM_PT TPM_PT_MAX_SESSION_CONTEXT = PT_FIXED + 34;
constexpr TPM_PT TPM_PT_PS_FAMILY_INDICATOR = PT_FIXED + 35;
constexpr TPM_PT TPM_PT_PS_LEVEL = PT_FIXED + 36;
constexpr TPM_PT TPM_PT_PS_REVISION = PT_FIXED + 37;
constexpr TPM_PT TPM_PT_PS_DAY_OF_YEAR = PT_FIXED + 38;
constexpr TPM_PT TPM_PT_PS_YEAR = PT_FIXED + 39;
constexpr TPM_PT TPM_PT_SPLIT_MAX = PT_FIXED + 40;
constexpr TPM_PT TPM_PT_TOTAL_COMMANDS = PT_FIXED + 41;
constexpr TPM_PT TPM_PT_LIBRARY_COMMANDS = PT_FIXED + 42;
constexpr TPM_PT TPM_PT_VENDOR_COMMANDS = PT_FIXED + 43;
constexpr TPM_PT TPM_PT_NV_BUFFER_MAX = PT_FIXED + 44;
constexpr TPM_PT PT_VAR = PT_GROUP * 2;
constexpr TPM_PT TPM_PT_PERMANENT = PT_VAR + 0;
constexpr TPM_PT TPM_PT_STARTUP_CLEAR = PT_VAR + 1;
constexpr TPM_PT TPM_PT_HR_NV_INDEX = PT_VAR + 2;
constexpr TPM_PT TPM_PT_HR_LOADED = PT_VAR + 3;
constexpr TPM_PT TPM_PT_HR_LOADED_AVAIL = PT_VAR + 4;
constexpr TPM_PT TPM_PT_HR_ACTIVE = PT_VAR + 5;
constexpr TPM_PT TPM_PT_HR_ACTIVE_AVAIL = PT_VAR + 6;
constexpr TPM_PT TPM_PT_HR_TRANSIENT_AVAIL = PT_VAR + 7;
constexpr TPM_PT TPM_PT_HR_PERSISTENT = PT_VAR + 8;
constexpr TPM_PT TPM_PT_HR_PERSISTENT_AVAIL = PT_VAR + 9;
constexpr TPM_PT TPM_PT_NV_COUNTERS = PT_VAR + 10;
constexpr TPM_PT TPM_PT_NV_COUNTERS_AVAIL = PT_VAR + 11;
constexpr TPM_PT TPM_PT_ALGORITHM_SET = PT_VAR + 12;
constexpr TPM_PT TPM_PT_LOADED_CURVES = PT_VAR + 13;
constexpr TPM_PT TPM_PT_LOCKOUT_COUNTER = PT_VAR + 14;
constexpr TPM_PT TPM_PT_MAX_AUTH_FAIL = PT_VAR + 15;
constexpr TPM_PT TPM_PT_LOCKOUT_INTERVAL = PT_VAR + 16;
constexpr TPM_PT TPM_PT_LOCKOUT_RECOVERY = PT_VAR + 17;
constexpr TPM_PT TPM_PT_NV_WRITE_RECOVERY = PT_VAR + 18;
constexpr TPM_PT TPM_PT_AUDIT_COUNTER_0 = PT_VAR + 19;
constexpr TPM_PT TPM_PT_AUDIT_COUNTER_1 = PT_VAR + 20;
constexpr TPM_PT_PCR TPM_PT_PCR_FIRST = 0x00000000;
constexpr TPM_PT_PCR TPM_PT_PCR_SAVE = 0x00000000;
constexpr TPM_PT_PCR TPM_PT_PCR_EXTEND_L0 = 0x00000001;
constexpr TPM_PT_PCR TPM_PT_PCR_RESET_L0 = 0x00000002;
constexpr TPM_PT_PCR TPM_PT_PCR_EXTEND_L1 = 0x00000003;
constexpr TPM_PT_PCR TPM_PT_PCR_RESET_L1 = 0x00000004;
constexpr TPM_PT_PCR TPM_PT_PCR_EXTEND_L2 = 0x00000005;
constexpr TPM_PT_PCR TPM_PT_PCR_RESET_L2 = 0x00000006;
constexpr TPM_PT_PCR TPM_PT_PCR_EXTEND_L3 = 0x00000007;
constexpr TPM_PT_PCR TPM_PT_PCR_RESET_L3 = 0x00000008;
constexpr TPM_PT_PCR TPM_PT_PCR_EXTEND_L4 = 0x00000009;
constexpr TPM_PT_PCR TPM_PT_PCR_RESET_L4 = 0x0000000A;
constexpr TPM_PT_PCR TPM_PT_PCR_NO_INCREMENT = 0x00000011;
constexpr TPM_PT_PCR TPM_PT_PCR_DRTM_RESET = 0x00000012;
constexpr TPM_PT_PCR TPM_PT_PCR_POLICY = 0x00000013;
constexpr TPM_PT_PCR TPM_PT_PCR_AUTH = 0x00000014;
constexpr TPM_PT_PCR TPM_PT_PCR_LAST = 0x00000014;
constexpr TPM_PS TPM_PS_MAIN = 0x00000000;
constexpr TPM_PS TPM_PS_PC = 0x00000001;
constexpr TPM_PS TPM_PS_PDA = 0x00000002;
constexpr TPM_PS TPM_PS_CELL_PHONE = 0x00000003;
constexpr TPM_PS TPM_PS_SERVER = 0x00000004;
constexpr TPM_PS TPM_PS_PERIPHERAL = 0x00000005;
constexpr TPM_PS TPM_PS_TSS = 0x00000006;
constexpr TPM_PS TPM_PS_STORAGE = 0x00000007;
constexpr TPM_PS TPM_PS_AUTHENTICATION = 0x00000008;
constexpr TPM_PS TPM_PS_EMBEDDED = 0x00000009;
constexpr TPM_PS TPM_PS_HARDCOPY = 0x0000000A;
constexpr TPM_PS TPM_PS_INFRASTRUCTURE = 0x0000000B;
constexpr TPM_PS TPM_PS_VIRTUALIZATION = 0x0000000C;
constexpr TPM_PS TPM_PS_TNC = 0x0000000D;
constexpr TPM_PS TPM_PS_MULTI_TENANT = 0x0000000E;
constexpr TPM_PS TPM_PS_TC = 0x0000000F;
constexpr TPM_HT TPM_HT_PCR = 0x00;
constexpr TPM_HT TPM_HT_NV_INDEX = 0x01;
constexpr TPM_HT TPM_HT_HMAC_SESSION = 0x02;
constexpr TPM_HT TPM_HT_LOADED_SESSION = 0x02;
constexpr TPM_HT TPM_HT_POLICY_SESSION = 0x03;
constexpr TPM_HT TPM_HT_ACTIVE_SESSION = 0x03;
constexpr TPM_HT TPM_HT_PERMANENT = 0x40;
constexpr TPM_HT TPM_HT_TRANSIENT = 0x80;
constexpr TPM_HT TPM_HT_PERSISTENT = 0x81;
constexpr TPM_RH TPM_RH_FIRST = 0x40000000;
constexpr TPM_RH TPM_RH_SRK = 0x40000000;
constexpr TPM_RH TPM_RH_OWNER = 0x40000001;
constexpr TPM_RH TPM_RH_REVOKE = 0x40000002;
constexpr TPM_RH TPM_RH_TRANSPORT = 0x40000003;
constexpr TPM_RH TPM_RH_OPERATOR = 0x40000004;
constexpr TPM_RH TPM_RH_ADMIN = 0x40000005;
constexpr TPM_RH TPM_RH_EK = 0x40000006;
constexpr TPM_RH TPM_RH_NULL = 0x40000007;
constexpr TPM_RH TPM_RH_UNASSIGNED = 0x40000008;
constexpr TPM_RH TPM_RS_PW = 0x40000009;
constexpr TPM_RH TPM_RH_LOCKOUT = 0x4000000A;
constexpr TPM_RH TPM_RH_ENDORSEMENT = 0x4000000B;
constexpr TPM_RH TPM_RH_PLATFORM = 0x4000000C;
constexpr TPM_RH TPM_RH_PLATFORM_NV = 0x4000000D;
constexpr TPM_RH TPM_RH_LAST = 0x4000000D;
constexpr TPM_HC HR_HANDLE_MASK = 0x00FFFFFF;
constexpr TPM_HC HR_RANGE_MASK = 0xFF000000;
constexpr TPM_HC HR_SHIFT = 24;
constexpr TPM_HC HR_PCR = (TPM_HT_PCR << HR_SHIFT);
constexpr TPM_HC HR_HMAC_SESSION = (TPM_HT_HMAC_SESSION << HR_SHIFT);
constexpr TPM_HC HR_POLICY_SESSION = (TPM_HT_POLICY_SESSION << HR_SHIFT);
constexpr TPM_HC HR_TRANSIENT = (TPM_HT_TRANSIENT << HR_SHIFT);
constexpr TPM_HC HR_PERSISTENT = (TPM_HT_PERSISTENT << HR_SHIFT);
constexpr TPM_HC HR_NV_INDEX = (TPM_HT_NV_INDEX << HR_SHIFT);
constexpr TPM_HC HR_PERMANENT = (TPM_HT_PERMANENT << HR_SHIFT);
constexpr TPM_HC PCR_FIRST = (HR_PCR + 0);
constexpr TPM_HC PCR_LAST = (PCR_FIRST + IMPLEMENTATION_PCR - 1);
constexpr TPM_HC HMAC_SESSION_FIRST = (HR_HMAC_SESSION + 0);
constexpr TPM_HC HMAC_SESSION_LAST =
    (HMAC_SESSION_FIRST + MAX_ACTIVE_SESSIONS - 1);
constexpr TPM_HC LOADED_SESSION_LAST = HMAC_SESSION_LAST;
constexpr TPM_HC POLICY_SESSION_FIRST = (HR_POLICY_SESSION + 0);
constexpr TPM_HC POLICY_SESSION_LAST =
    (POLICY_SESSION_FIRST + MAX_ACTIVE_SESSIONS - 1);
constexpr TPM_HC TRANSIENT_FIRST = (HR_TRANSIENT + 0);
constexpr TPM_HC ACTIVE_SESSION_FIRST = POLICY_SESSION_FIRST;
constexpr TPM_HC ACTIVE_SESSION_LAST = POLICY_SESSION_LAST;
constexpr TPM_HC TRANSIENT_LAST = (TRANSIENT_FIRST + MAX_LOADED_OBJECTS - 1);
constexpr TPM_HC PERSISTENT_FIRST = (HR_PERSISTENT + 0);
constexpr TPM_HC PERSISTENT_LAST = (PERSISTENT_FIRST + 0x00FFFFFF);
constexpr TPM_HC PLATFORM_PERSISTENT = (PERSISTENT_FIRST + 0x00800000);
constexpr TPM_HC NV_INDEX_FIRST = (HR_NV_INDEX + 0);
constexpr TPM_HC NV_INDEX_LAST = (NV_INDEX_FIRST + 0x00FFFFFF);
constexpr TPM_HC PERMANENT_FIRST = TPM_RH_FIRST;
constexpr TPM_HC PERMANENT_LAST = TPM_RH_LAST;

struct TPMS_ALGORITHM_DESCRIPTION {
  TPM_ALG_ID alg = {};
  TPMA_ALGORITHM attributes = {};
};

union TPMU_HA {
  BYTE sha1[SHA1_DIGEST_SIZE];
  BYTE sha256[SHA256_DIGEST_SIZE];
  BYTE sm3_256[SM3_256_DIGEST_SIZE];
  BYTE sha384[SHA384_DIGEST_SIZE];
  BYTE sha512[SHA512_DIGEST_SIZE];
};

struct TPMT_HA {
  TPMI_ALG_HASH hash_alg = {};
  TPMU_HA digest = {};
};

struct TPM2B_DIGEST {
  UINT16 size = {};
  BYTE buffer[sizeof(TPMU_HA)] = {};
};

struct TPM2B_DATA {
  UINT16 size = {};
  BYTE buffer[sizeof(TPMT_HA)] = {};
};

struct TPM2B_EVENT {
  UINT16 size = {};
  BYTE buffer[1024] = {};
};

struct TPM2B_MAX_BUFFER {
  UINT16 size = {};
  BYTE buffer[MAX_DIGEST_BUFFER] = {};
};

struct TPM2B_MAX_NV_BUFFER {
  UINT16 size = {};
  BYTE buffer[MAX_NV_BUFFER_SIZE] = {};
};

struct TPM2B_TIMEOUT {
  UINT16 size = {};
  BYTE buffer[sizeof(UINT64)] = {};
};

struct TPM2B_IV {
  UINT16 size = {};
  BYTE buffer[MAX_SYM_BLOCK_SIZE] = {};
};

union TPMU_NAME {
  TPMT_HA digest;
  TPM_HANDLE handle;
};

struct TPM2B_NAME {
  UINT16 size = {};
  BYTE name[sizeof(TPMU_NAME)] = {};
};

struct TPMS_PCR_SELECT {
  UINT8 sizeof_select = {};
  BYTE pcr_select[PCR_SELECT_MAX] = {};
};

struct TPMS_PCR_SELECTION {
  TPMI_ALG_HASH hash = {};
  UINT8 sizeof_select = {};
  BYTE pcr_select[PCR_SELECT_MAX] = {};
};

struct TPMT_TK_CREATION {
  TPM_ST tag = {};
  TPMI_RH_HIERARCHY hierarchy = {};
  TPM2B_DIGEST digest = {};
};

struct TPMT_TK_VERIFIED {
  TPM_ST tag = {};
  TPMI_RH_HIERARCHY hierarchy = {};
  TPM2B_DIGEST digest = {};
};

struct TPMT_TK_AUTH {
  TPM_ST tag = {};
  TPMI_RH_HIERARCHY hierarchy = {};
  TPM2B_DIGEST digest = {};
};

struct TPMT_TK_HASHCHECK {
  TPM_ST tag = {};
  TPMI_RH_HIERARCHY hierarchy = {};
  TPM2B_DIGEST digest = {};
};

struct TPMS_ALG_PROPERTY {
  TPM_ALG_ID alg = {};
  TPMA_ALGORITHM alg_properties = {};
};

struct TPMS_TAGGED_PROPERTY {
  TPM_PT property = {};
  UINT32 value = {};
};

struct TPMS_TAGGED_PCR_SELECT {
  TPM_PT tag = {};
  UINT8 sizeof_select = {};
  BYTE pcr_select[PCR_SELECT_MAX] = {};
};

struct TPML_CC {
  UINT32 count = {};
  TPM_CC command_codes[MAX_CAP_CC_ALL] = {};
};

struct TPML_CCA {
  UINT32 count = {};
  TPMA_CC command_attributes[MAX_CAP_CC_ALL] = {};
};

struct TPML_ALG {
  UINT32 count = {};
  TPM_ALG_ID algorithms[MAX_ALG_LIST_SIZE] = {};
};

struct TPML_HANDLE {
  UINT32 count = {};
  TPM_HANDLE handle[MAX_CAP_HANDLES] = {};
};

struct TPML_DIGEST {
  UINT32 count = {};
  TPM2B_DIGEST digests[8] = {};
};

struct TPML_DIGEST_VALUES {
  UINT32 count = {};
  TPMT_HA digests[HASH_COUNT] = {};
};

struct TPM2B_DIGEST_VALUES {
  UINT16 size = {};
  BYTE buffer[sizeof(TPML_DIGEST_VALUES)] = {};
};

struct TPML_PCR_SELECTION {
  UINT32 count = {};
  TPMS_PCR_SELECTION pcr_selections[HASH_COUNT] = {};
};

struct TPML_ALG_PROPERTY {
  UINT32 count = {};
  TPMS_ALG_PROPERTY alg_properties[MAX_CAP_ALGS] = {};
};

struct TPML_TAGGED_TPM_PROPERTY {
  UINT32 count = {};
  TPMS_TAGGED_PROPERTY tpm_property[MAX_TPM_PROPERTIES] = {};
};

struct TPML_TAGGED_PCR_PROPERTY {
  UINT32 count = {};
  TPMS_TAGGED_PCR_SELECT pcr_property[MAX_PCR_PROPERTIES] = {};
};

struct TPML_ECC_CURVE {
  UINT32 count = {};
  TPM_ECC_CURVE ecc_curves[MAX_ECC_CURVES] = {};
};

union TPMU_CAPABILITIES {
  TPML_ALG_PROPERTY algorithms;
  TPML_HANDLE handles;
  TPML_CCA command;
  TPML_CC pp_commands;
  TPML_CC audit_commands;
  TPML_PCR_SELECTION assigned_pcr;
  TPML_TAGGED_TPM_PROPERTY tpm_properties;
  TPML_TAGGED_PCR_PROPERTY pcr_properties;
  TPML_ECC_CURVE ecc_curves;
};

struct TPMS_CAPABILITY_DATA {
  TPM_CAP capability = {};
  TPMU_CAPABILITIES data = {};
};

struct TPMS_CLOCK_INFO {
  UINT64 clock = {};
  UINT32 reset_count = {};
  UINT32 restart_count = {};
  TPMI_YES_NO safe = {};
};

struct TPMS_TIME_INFO {
  UINT64 time = {};
  TPMS_CLOCK_INFO clock_info = {};
};

struct TPMS_TIME_ATTEST_INFO {
  TPMS_TIME_INFO time = {};
  UINT64 firmware_version = {};
};

struct TPMS_CERTIFY_INFO {
  TPM2B_NAME name = {};
  TPM2B_NAME qualified_name = {};
};

struct TPMS_QUOTE_INFO {
  TPML_PCR_SELECTION pcr_select = {};
  TPM2B_DIGEST pcr_digest = {};
};

struct TPMS_COMMAND_AUDIT_INFO {
  UINT64 audit_counter = {};
  TPM_ALG_ID digest_alg = {};
  TPM2B_DIGEST audit_digest = {};
  TPM2B_DIGEST command_digest = {};
};

struct TPMS_SESSION_AUDIT_INFO {
  TPMI_YES_NO exclusive_session = {};
  TPM2B_DIGEST session_digest = {};
};

struct TPMS_CREATION_INFO {
  TPM2B_NAME object_name = {};
  TPM2B_DIGEST creation_hash = {};
};

struct TPMS_NV_CERTIFY_INFO {
  TPM2B_NAME index_name = {};
  UINT16 offset = {};
  TPM2B_MAX_NV_BUFFER nv_contents = {};
};

union TPMU_ATTEST {
  TPMS_CERTIFY_INFO certify;
  TPMS_CREATION_INFO creation;
  TPMS_QUOTE_INFO quote;
  TPMS_COMMAND_AUDIT_INFO command_audit;
  TPMS_SESSION_AUDIT_INFO session_audit;
  TPMS_TIME_ATTEST_INFO time;
  TPMS_NV_CERTIFY_INFO nv;
};

struct TPMS_ATTEST {
  TPM_GENERATED magic = {};
  TPMI_ST_ATTEST type = {};
  TPM2B_NAME qualified_signer = {};
  TPM2B_DATA extra_data = {};
  TPMS_CLOCK_INFO clock_info = {};
  UINT64 firmware_version = {};
  TPMU_ATTEST attested = {};
};

struct TPM2B_ATTEST {
  UINT16 size = {};
  BYTE attestation_data[sizeof(TPMS_ATTEST)] = {};
};

struct TPMS_AUTH_COMMAND {
  TPMI_SH_AUTH_SESSION session_handle = {};
  TPM2B_NONCE nonce = {};
  TPMA_SESSION session_attributes = {};
  TPM2B_AUTH hmac = {};
};

struct TPMS_AUTH_RESPONSE {
  TPM2B_NONCE nonce = {};
  TPMA_SESSION session_attributes = {};
  TPM2B_AUTH hmac = {};
};

union TPMU_SYM_KEY_BITS {
  TPMI_AES_KEY_BITS aes;
  TPMI_SM4_KEY_BITS sm4;
  TPM_KEY_BITS sym;
  TPMI_ALG_HASH xor_;
};

union TPMU_SYM_MODE {
  TPMI_ALG_SYM_MODE aes;
  TPMI_ALG_SYM_MODE sm4;
  TPMI_ALG_SYM_MODE sym;
};

union TPMU_SYM_DETAILS {};

struct TPMT_SYM_DEF {
  TPMI_ALG_SYM algorithm = {};
  TPMU_SYM_KEY_BITS key_bits = {};
  TPMU_SYM_MODE mode = {};
  TPMU_SYM_DETAILS details = {};
};

struct TPMT_SYM_DEF_OBJECT {
  TPMI_ALG_SYM_OBJECT algorithm = {};
  TPMU_SYM_KEY_BITS key_bits = {};
  TPMU_SYM_MODE mode = {};
  TPMU_SYM_DETAILS details = {};
};

struct TPM2B_SYM_KEY {
  UINT16 size = {};
  BYTE buffer[MAX_SYM_KEY_BYTES] = {};
};

struct TPMS_SYMCIPHER_PARMS {
  TPMT_SYM_DEF_OBJECT sym = {};
};

struct TPM2B_SENSITIVE_DATA {
  UINT16 size = {};
  BYTE buffer[MAX_SYM_DATA] = {};
};

struct TPMS_SENSITIVE_CREATE {
  TPM2B_AUTH user_auth = {};
  TPM2B_SENSITIVE_DATA data = {};
};

struct TPM2B_SENSITIVE_CREATE {
  UINT16 size = {};
  TPMS_SENSITIVE_CREATE sensitive = {};
};

struct TPMS_SCHEME_SIGHASH {
  TPMI_ALG_HASH hash_alg = {};
};

struct TPMS_SCHEME_XOR {
  TPMI_ALG_HASH hash_alg = {};
  TPMI_ALG_KDF kdf = {};
};

union TPMU_SCHEME_KEYEDHASH {
  TPMS_SCHEME_HMAC hmac;
  TPMS_SCHEME_XOR xor_;
};

struct TPMT_KEYEDHASH_SCHEME {
  TPMI_ALG_KEYEDHASH_SCHEME scheme = {};
  TPMU_SCHEME_KEYEDHASH details = {};
};

struct TPMS_SCHEME_ECDAA {
  TPMI_ALG_HASH hash_alg = {};
  UINT16 count = {};
};

union TPMU_SIG_SCHEME {
  TPMS_SCHEME_RSASSA rsassa;
  TPMS_SCHEME_RSAPSS rsapss;
  TPMS_SCHEME_ECDSA ecdsa;
  TPMS_SCHEME_SM2 sm2;
  TPMS_SCHEME_ECDAA ecdaa;
  TPMS_SCHEME_ECSCHNORR ec_schnorr;
  TPMS_SCHEME_HMAC hmac;
  TPMS_SCHEME_SIGHASH any;
};

struct TPMT_SIG_SCHEME {
  TPMI_ALG_SIG_SCHEME scheme = {};
  TPMU_SIG_SCHEME details = {};
};

struct TPMS_SCHEME_OAEP {
  TPMI_ALG_HASH hash_alg = {};
};

struct TPMS_SCHEME_ECDH {
  TPMI_ALG_HASH hash_alg = {};
};

struct TPMS_SCHEME_MGF1 {
  TPMI_ALG_HASH hash_alg = {};
};

struct TPMS_SCHEME_KDF1_SP800_56a {
  TPMI_ALG_HASH hash_alg = {};
};

struct TPMS_SCHEME_KDF2 {
  TPMI_ALG_HASH hash_alg = {};
};

struct TPMS_SCHEME_KDF1_SP800_108 {
  TPMI_ALG_HASH hash_alg = {};
};

union TPMU_KDF_SCHEME {
  TPMS_SCHEME_MGF1 mgf1;
  TPMS_SCHEME_KDF1_SP800_56a kdf1_sp800_56a;
  TPMS_SCHEME_KDF2 kdf2;
  TPMS_SCHEME_KDF1_SP800_108 kdf1_sp800_108;
};

struct TPMT_KDF_SCHEME {
  TPMI_ALG_KDF scheme = {};
  TPMU_KDF_SCHEME details = {};
};

union TPMU_ASYM_SCHEME {
  TPMS_SCHEME_RSASSA rsassa;
  TPMS_SCHEME_RSAPSS rsapss;
  TPMS_SCHEME_OAEP oaep;
  TPMS_SCHEME_ECDSA ecdsa;
  TPMS_SCHEME_SM2 sm2;
  TPMS_SCHEME_ECDAA ecdaa;
  TPMS_SCHEME_ECSCHNORR ec_schnorr;
  TPMS_SCHEME_ECDH ecdh;
  TPMS_SCHEME_SIGHASH any_sig;
};

struct TPMT_ASYM_SCHEME {
  TPMI_ALG_ASYM_SCHEME scheme = {};
  TPMU_ASYM_SCHEME details = {};
};

struct TPMT_RSA_SCHEME {
  TPMI_ALG_RSA_SCHEME scheme = {};
  TPMU_ASYM_SCHEME details = {};
};

struct TPMT_RSA_DECRYPT {
  TPMI_ALG_RSA_DECRYPT scheme = {};
  TPMU_ASYM_SCHEME details = {};
};

struct TPM2B_PUBLIC_KEY_RSA {
  UINT16 size = {};
  BYTE buffer[MAX_RSA_KEY_BYTES] = {};
};

struct TPM2B_PRIVATE_KEY_RSA {
  UINT16 size = {};
  BYTE buffer[MAX_RSA_KEY_BYTES / 2] = {};
};

struct TPM2B_ECC_PARAMETER {
  UINT16 size = {};
  BYTE buffer[MAX_ECC_KEY_BYTES] = {};
};

struct TPMS_ECC_POINT {
  TPM2B_ECC_PARAMETER x = {};
  TPM2B_ECC_PARAMETER y = {};
};

struct TPM2B_ECC_POINT {
  UINT16 size = {};
  TPMS_ECC_POINT point = {};
};

struct TPMT_ECC_SCHEME {
  TPMI_ALG_ECC_SCHEME scheme = {};
  TPMU_SIG_SCHEME details = {};
};

struct TPMS_ALGORITHM_DETAIL_ECC {
  TPM_ECC_CURVE curve_id = {};
  UINT16 key_size = {};
  TPMT_KDF_SCHEME kdf = {};
  TPMT_ECC_SCHEME sign = {};
  TPM2B_ECC_PARAMETER p = {};
  TPM2B_ECC_PARAMETER a = {};
  TPM2B_ECC_PARAMETER b = {};
  TPM2B_ECC_PARAMETER g_x = {};
  TPM2B_ECC_PARAMETER g_y = {};
  TPM2B_ECC_PARAMETER n = {};
  TPM2B_ECC_PARAMETER h = {};
};

struct TPMS_SIGNATURE_RSASSA {
  TPMI_ALG_HASH hash = {};
  TPM2B_PUBLIC_KEY_RSA sig = {};
};

struct TPMS_SIGNATURE_RSAPSS {
  TPMI_ALG_HASH hash = {};
  TPM2B_PUBLIC_KEY_RSA sig = {};
};

struct TPMS_SIGNATURE_ECDSA {
  TPMI_ALG_HASH hash = {};
  TPM2B_ECC_PARAMETER signature_r = {};
  TPM2B_ECC_PARAMETER signature_s = {};
};

union TPMU_SIGNATURE {
  TPMS_SIGNATURE_RSASSA rsassa;
  TPMS_SIGNATURE_RSAPSS rsapss;
  TPMS_SIGNATURE_ECDSA ecdsa;
  TPMS_SIGNATURE_ECDSA sm2;
  TPMS_SIGNATURE_ECDSA ecdaa;
  TPMS_SIGNATURE_ECDSA ecschnorr;
  TPMT_HA hmac;
  TPMS_SCHEME_SIGHASH any;
};

struct TPMT_SIGNATURE {
  TPMI_ALG_SIG_SCHEME sig_alg = {};
  TPMU_SIGNATURE signature = {};
};

union TPMU_ENCRYPTED_SECRET {
  BYTE ecc[sizeof(TPMS_ECC_POINT)];
  BYTE rsa[MAX_RSA_KEY_BYTES];
  BYTE symmetric[sizeof(TPM2B_DIGEST)];
  BYTE keyed_hash[sizeof(TPM2B_DIGEST)];
};

struct TPM2B_ENCRYPTED_SECRET {
  UINT16 size = {};
  BYTE secret[sizeof(TPMU_ENCRYPTED_SECRET)] = {};
};

struct TPMS_KEYEDHASH_PARMS {
  TPMT_KEYEDHASH_SCHEME scheme = {};
};

struct TPMS_ASYM_PARMS {
  TPMT_SYM_DEF_OBJECT symmetric = {};
  TPMT_ASYM_SCHEME scheme = {};
};

struct TPMS_RSA_PARMS {
  TPMT_SYM_DEF_OBJECT symmetric = {};
  TPMT_RSA_SCHEME scheme = {};
  TPMI_RSA_KEY_BITS key_bits = {};
  UINT32 exponent = {};
};

struct TPMS_ECC_PARMS {
  TPMT_SYM_DEF_OBJECT symmetric = {};
  TPMT_ECC_SCHEME scheme = {};
  TPMI_ECC_CURVE curve_id = {};
  TPMT_KDF_SCHEME kdf = {};
};

union TPMU_PUBLIC_PARMS {
  TPMS_KEYEDHASH_PARMS keyed_hash_detail;
  TPMS_SYMCIPHER_PARMS sym_detail;
  TPMS_RSA_PARMS rsa_detail;
  TPMS_ECC_PARMS ecc_detail;
  TPMS_ASYM_PARMS asym_detail;
};

struct TPMT_PUBLIC_PARMS {
  TPMI_ALG_PUBLIC type = {};
  TPMU_PUBLIC_PARMS parameters = {};
};

union TPMU_PUBLIC_ID {
  TPM2B_DIGEST keyed_hash;
  TPM2B_DIGEST sym;
  TPM2B_PUBLIC_KEY_RSA rsa;
  TPMS_ECC_POINT ecc;
};

struct TPMT_PUBLIC {
  TPMI_ALG_PUBLIC type = {};
  TPMI_ALG_HASH name_alg = {};
  TPMA_OBJECT object_attributes = {};
  TPM2B_DIGEST auth_policy = {};
  TPMU_PUBLIC_PARMS parameters = {};
  TPMU_PUBLIC_ID unique = {};
};

struct TPM2B_PUBLIC {
  UINT16 size = {};
  TPMT_PUBLIC public_area = {};
};

struct TPM2B_PRIVATE_VENDOR_SPECIFIC {
  UINT16 size = {};
  BYTE buffer[PRIVATE_VENDOR_SPECIFIC_BYTES] = {};
};

union TPMU_SENSITIVE_COMPOSITE {
  TPM2B_PRIVATE_KEY_RSA rsa;
  TPM2B_ECC_PARAMETER ecc;
  TPM2B_SENSITIVE_DATA bits;
  TPM2B_SYM_KEY sym;
  TPM2B_PRIVATE_VENDOR_SPECIFIC any;
};

struct TPMT_SENSITIVE {
  TPMI_ALG_PUBLIC sensitive_type = {};
  TPM2B_AUTH auth_value = {};
  TPM2B_DIGEST seed_value = {};
  TPMU_SENSITIVE_COMPOSITE sensitive = {};
};

struct TPM2B_SENSITIVE {
  UINT16 size = {};
  TPMT_SENSITIVE sensitive_area = {};
};

struct TPM2B_PRIVATE {
  UINT16 size = {};
  BYTE buffer[MAX_DIGEST_BUFFER] = {};
};

struct TPM2B_ID_OBJECT {
  UINT16 size = {};
  BYTE credential[MAX_DIGEST_BUFFER] = {};
};

struct TPMS_NV_PUBLIC {
  TPMI_RH_NV_INDEX nv_index = {};
  TPMI_ALG_HASH name_alg = {};
  TPMA_NV attributes = {};
  TPM2B_DIGEST auth_policy = {};
  UINT16 data_size = {};
};

struct TPM2B_NV_PUBLIC {
  UINT16 size = {};
  TPMS_NV_PUBLIC nv_public = {};
};

struct TPM2B_CONTEXT_SENSITIVE {
  UINT16 size = {};
  BYTE buffer[MAX_CONTEXT_SIZE] = {};
};

struct TPMS_CONTEXT_DATA {
  TPM2B_DIGEST integrity = {};
  TPM2B_CONTEXT_SENSITIVE encrypted = {};
};

struct TPM2B_CONTEXT_DATA {
  UINT16 size = {};
  BYTE buffer[sizeof(TPMS_CONTEXT_DATA)] = {};
};

struct TPMS_CONTEXT {
  UINT64 sequence = {};
  TPMI_DH_CONTEXT saved_handle = {};
  TPMI_RH_HIERARCHY hierarchy = {};
  TPM2B_CONTEXT_DATA context_blob = {};
};

struct TPMS_CREATION_DATA {
  TPML_PCR_SELECTION pcr_select = {};
  TPM2B_DIGEST pcr_digest = {};
  TPMA_LOCALITY locality = {};
  TPM_ALG_ID parent_name_alg = {};
  TPM2B_NAME parent_name = {};
  TPM2B_NAME parent_qualified_name = {};
  TPM2B_DATA outside_info = {};
};

struct TPM2B_CREATION_DATA {
  UINT16 size = {};
  TPMS_CREATION_DATA creation_data = {};
};

struct FIDO_DATA_RANGE {
  UINT16 offset = {};
  UINT16 size = {};
};

TRUNKS_EXPORT size_t GetNumberOfRequestHandles(TPM_CC command_code);
TRUNKS_EXPORT size_t GetNumberOfResponseHandles(TPM_CC command_code);

TRUNKS_EXPORT TPM_RC Serialize_uint8_t(const uint8_t& value,
                                       std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_uint8_t(std::string* buffer,
                                   uint8_t* value,
                                   std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_int8_t(const int8_t& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_int8_t(std::string* buffer,
                                  int8_t* value,
                                  std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_int(const int& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_int(std::string* buffer,
                               int* value,
                               std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_uint16_t(const uint16_t& value,
                                        std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_uint16_t(std::string* buffer,
                                    uint16_t* value,
                                    std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_int16_t(const int16_t& value,
                                       std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_int16_t(std::string* buffer,
                                   int16_t* value,
                                   std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_uint32_t(const uint32_t& value,
                                        std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_uint32_t(std::string* buffer,
                                    uint32_t* value,
                                    std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_int32_t(const int32_t& value,
                                       std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_int32_t(std::string* buffer,
                                   int32_t* value,
                                   std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_uint64_t(const uint64_t& value,
                                        std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_uint64_t(std::string* buffer,
                                    uint64_t* value,
                                    std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_int64_t(const int64_t& value,
                                       std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_int64_t(std::string* buffer,
                                   int64_t* value,
                                   std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_UINT8(const UINT8& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_UINT8(std::string* buffer,
                                 UINT8* value,
                                 std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_BYTE(const BYTE& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_BYTE(std::string* buffer,
                                BYTE* value,
                                std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_INT8(const INT8& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_INT8(std::string* buffer,
                                INT8* value,
                                std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_BOOL(const BOOL& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_BOOL(std::string* buffer,
                                BOOL* value,
                                std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_UINT16(const UINT16& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_UINT16(std::string* buffer,
                                  UINT16* value,
                                  std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_INT16(const INT16& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_INT16(std::string* buffer,
                                 INT16* value,
                                 std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_UINT32(const UINT32& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_UINT32(std::string* buffer,
                                  UINT32* value,
                                  std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_INT32(const INT32& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_INT32(std::string* buffer,
                                 INT32* value,
                                 std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_UINT64(const UINT64& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_UINT64(std::string* buffer,
                                  UINT64* value,
                                  std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_INT64(const INT64& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_INT64(std::string* buffer,
                                 INT64* value,
                                 std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM_ALGORITHM_ID(const TPM_ALGORITHM_ID& value,
                                                std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM_ALGORITHM_ID(std::string* buffer,
                                            TPM_ALGORITHM_ID* value,
                                            std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM_MODIFIER_INDICATOR(
    const TPM_MODIFIER_INDICATOR& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM_MODIFIER_INDICATOR(std::string* buffer,
                                                  TPM_MODIFIER_INDICATOR* value,
                                                  std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM_AUTHORIZATION_SIZE(
    const TPM_AUTHORIZATION_SIZE& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM_AUTHORIZATION_SIZE(std::string* buffer,
                                                  TPM_AUTHORIZATION_SIZE* value,
                                                  std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM_PARAMETER_SIZE(
    const TPM_PARAMETER_SIZE& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM_PARAMETER_SIZE(std::string* buffer,
                                              TPM_PARAMETER_SIZE* value,
                                              std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM_KEY_SIZE(const TPM_KEY_SIZE& value,
                                            std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM_KEY_SIZE(std::string* buffer,
                                        TPM_KEY_SIZE* value,
                                        std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM_KEY_BITS(const TPM_KEY_BITS& value,
                                            std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM_KEY_BITS(std::string* buffer,
                                        TPM_KEY_BITS* value,
                                        std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM_HANDLE(const TPM_HANDLE& value,
                                          std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM_HANDLE(std::string* buffer,
                                      TPM_HANDLE* value,
                                      std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM2B_NONCE(const TPM2B_NONCE& value,
                                           std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM2B_NONCE(std::string* buffer,
                                       TPM2B_NONCE* value,
                                       std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM2B_AUTH(const TPM2B_AUTH& value,
                                          std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM2B_AUTH(std::string* buffer,
                                      TPM2B_AUTH* value,
                                      std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM2B_OPERAND(const TPM2B_OPERAND& value,
                                             std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM2B_OPERAND(std::string* buffer,
                                         TPM2B_OPERAND* value,
                                         std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_SCHEME_HMAC(const TPMS_SCHEME_HMAC& value,
                                                std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_SCHEME_HMAC(std::string* buffer,
                                            TPMS_SCHEME_HMAC* value,
                                            std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_SCHEME_RSASSA(
    const TPMS_SCHEME_RSASSA& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_SCHEME_RSASSA(std::string* buffer,
                                              TPMS_SCHEME_RSASSA* value,
                                              std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_SCHEME_RSAPSS(
    const TPMS_SCHEME_RSAPSS& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_SCHEME_RSAPSS(std::string* buffer,
                                              TPMS_SCHEME_RSAPSS* value,
                                              std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_SCHEME_ECDSA(const TPMS_SCHEME_ECDSA& value,
                                                 std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_SCHEME_ECDSA(std::string* buffer,
                                             TPMS_SCHEME_ECDSA* value,
                                             std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_SCHEME_SM2(const TPMS_SCHEME_SM2& value,
                                               std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_SCHEME_SM2(std::string* buffer,
                                           TPMS_SCHEME_SM2* value,
                                           std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_SCHEME_ECSCHNORR(
    const TPMS_SCHEME_ECSCHNORR& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_SCHEME_ECSCHNORR(std::string* buffer,
                                                 TPMS_SCHEME_ECSCHNORR* value,
                                                 std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_YES_NO(const TPMI_YES_NO& value,
                                           std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_YES_NO(std::string* buffer,
                                       TPMI_YES_NO* value,
                                       std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_DH_OBJECT(const TPMI_DH_OBJECT& value,
                                              std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_DH_OBJECT(std::string* buffer,
                                          TPMI_DH_OBJECT* value,
                                          std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_DH_PERSISTENT(
    const TPMI_DH_PERSISTENT& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_DH_PERSISTENT(std::string* buffer,
                                              TPMI_DH_PERSISTENT* value,
                                              std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_DH_ENTITY(const TPMI_DH_ENTITY& value,
                                              std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_DH_ENTITY(std::string* buffer,
                                          TPMI_DH_ENTITY* value,
                                          std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_DH_PCR(const TPMI_DH_PCR& value,
                                           std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_DH_PCR(std::string* buffer,
                                       TPMI_DH_PCR* value,
                                       std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_SH_AUTH_SESSION(
    const TPMI_SH_AUTH_SESSION& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_SH_AUTH_SESSION(std::string* buffer,
                                                TPMI_SH_AUTH_SESSION* value,
                                                std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_SH_HMAC(const TPMI_SH_HMAC& value,
                                            std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_SH_HMAC(std::string* buffer,
                                        TPMI_SH_HMAC* value,
                                        std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_SH_POLICY(const TPMI_SH_POLICY& value,
                                              std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_SH_POLICY(std::string* buffer,
                                          TPMI_SH_POLICY* value,
                                          std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_DH_CONTEXT(const TPMI_DH_CONTEXT& value,
                                               std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_DH_CONTEXT(std::string* buffer,
                                           TPMI_DH_CONTEXT* value,
                                           std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_RH_HIERARCHY(const TPMI_RH_HIERARCHY& value,
                                                 std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_RH_HIERARCHY(std::string* buffer,
                                             TPMI_RH_HIERARCHY* value,
                                             std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_RH_ENABLES(const TPMI_RH_ENABLES& value,
                                               std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_RH_ENABLES(std::string* buffer,
                                           TPMI_RH_ENABLES* value,
                                           std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_RH_HIERARCHY_AUTH(
    const TPMI_RH_HIERARCHY_AUTH& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_RH_HIERARCHY_AUTH(std::string* buffer,
                                                  TPMI_RH_HIERARCHY_AUTH* value,
                                                  std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_RH_PLATFORM(const TPMI_RH_PLATFORM& value,
                                                std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_RH_PLATFORM(std::string* buffer,
                                            TPMI_RH_PLATFORM* value,
                                            std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_RH_OWNER(const TPMI_RH_OWNER& value,
                                             std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_RH_OWNER(std::string* buffer,
                                         TPMI_RH_OWNER* value,
                                         std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_RH_ENDORSEMENT(
    const TPMI_RH_ENDORSEMENT& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_RH_ENDORSEMENT(std::string* buffer,
                                               TPMI_RH_ENDORSEMENT* value,
                                               std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_RH_PROVISION(const TPMI_RH_PROVISION& value,
                                                 std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_RH_PROVISION(std::string* buffer,
                                             TPMI_RH_PROVISION* value,
                                             std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_RH_CLEAR(const TPMI_RH_CLEAR& value,
                                             std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_RH_CLEAR(std::string* buffer,
                                         TPMI_RH_CLEAR* value,
                                         std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_RH_NV_AUTH(const TPMI_RH_NV_AUTH& value,
                                               std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_RH_NV_AUTH(std::string* buffer,
                                           TPMI_RH_NV_AUTH* value,
                                           std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_RH_LOCKOUT(const TPMI_RH_LOCKOUT& value,
                                               std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_RH_LOCKOUT(std::string* buffer,
                                           TPMI_RH_LOCKOUT* value,
                                           std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_RH_NV_INDEX(const TPMI_RH_NV_INDEX& value,
                                                std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_RH_NV_INDEX(std::string* buffer,
                                            TPMI_RH_NV_INDEX* value,
                                            std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_ALG_HASH(const TPMI_ALG_HASH& value,
                                             std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_ALG_HASH(std::string* buffer,
                                         TPMI_ALG_HASH* value,
                                         std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_ALG_ASYM(const TPMI_ALG_ASYM& value,
                                             std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_ALG_ASYM(std::string* buffer,
                                         TPMI_ALG_ASYM* value,
                                         std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_ALG_SYM(const TPMI_ALG_SYM& value,
                                            std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_ALG_SYM(std::string* buffer,
                                        TPMI_ALG_SYM* value,
                                        std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_ALG_SYM_OBJECT(
    const TPMI_ALG_SYM_OBJECT& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_ALG_SYM_OBJECT(std::string* buffer,
                                               TPMI_ALG_SYM_OBJECT* value,
                                               std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_ALG_SYM_MODE(const TPMI_ALG_SYM_MODE& value,
                                                 std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_ALG_SYM_MODE(std::string* buffer,
                                             TPMI_ALG_SYM_MODE* value,
                                             std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_ALG_KDF(const TPMI_ALG_KDF& value,
                                            std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_ALG_KDF(std::string* buffer,
                                        TPMI_ALG_KDF* value,
                                        std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_ALG_SIG_SCHEME(
    const TPMI_ALG_SIG_SCHEME& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_ALG_SIG_SCHEME(std::string* buffer,
                                               TPMI_ALG_SIG_SCHEME* value,
                                               std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_ECC_KEY_EXCHANGE(
    const TPMI_ECC_KEY_EXCHANGE& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_ECC_KEY_EXCHANGE(std::string* buffer,
                                                 TPMI_ECC_KEY_EXCHANGE* value,
                                                 std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_ST_COMMAND_TAG(
    const TPMI_ST_COMMAND_TAG& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_ST_COMMAND_TAG(std::string* buffer,
                                               TPMI_ST_COMMAND_TAG* value,
                                               std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_ST_ATTEST(const TPMI_ST_ATTEST& value,
                                              std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_ST_ATTEST(std::string* buffer,
                                          TPMI_ST_ATTEST* value,
                                          std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_AES_KEY_BITS(const TPMI_AES_KEY_BITS& value,
                                                 std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_AES_KEY_BITS(std::string* buffer,
                                             TPMI_AES_KEY_BITS* value,
                                             std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_SM4_KEY_BITS(const TPMI_SM4_KEY_BITS& value,
                                                 std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_SM4_KEY_BITS(std::string* buffer,
                                             TPMI_SM4_KEY_BITS* value,
                                             std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_ALG_KEYEDHASH_SCHEME(
    const TPMI_ALG_KEYEDHASH_SCHEME& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC
Parse_TPMI_ALG_KEYEDHASH_SCHEME(std::string* buffer,
                                TPMI_ALG_KEYEDHASH_SCHEME* value,
                                std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_ALG_ASYM_SCHEME(
    const TPMI_ALG_ASYM_SCHEME& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_ALG_ASYM_SCHEME(std::string* buffer,
                                                TPMI_ALG_ASYM_SCHEME* value,
                                                std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_ALG_RSA_SCHEME(
    const TPMI_ALG_RSA_SCHEME& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_ALG_RSA_SCHEME(std::string* buffer,
                                               TPMI_ALG_RSA_SCHEME* value,
                                               std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_ALG_RSA_DECRYPT(
    const TPMI_ALG_RSA_DECRYPT& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_ALG_RSA_DECRYPT(std::string* buffer,
                                                TPMI_ALG_RSA_DECRYPT* value,
                                                std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_RSA_KEY_BITS(const TPMI_RSA_KEY_BITS& value,
                                                 std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_RSA_KEY_BITS(std::string* buffer,
                                             TPMI_RSA_KEY_BITS* value,
                                             std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_ALG_ECC_SCHEME(
    const TPMI_ALG_ECC_SCHEME& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_ALG_ECC_SCHEME(std::string* buffer,
                                               TPMI_ALG_ECC_SCHEME* value,
                                               std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_ECC_CURVE(const TPMI_ECC_CURVE& value,
                                              std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_ECC_CURVE(std::string* buffer,
                                          TPMI_ECC_CURVE* value,
                                          std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMI_ALG_PUBLIC(const TPMI_ALG_PUBLIC& value,
                                               std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMI_ALG_PUBLIC(std::string* buffer,
                                           TPMI_ALG_PUBLIC* value,
                                           std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMA_ALGORITHM(const TPMA_ALGORITHM& value,
                                              std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMA_ALGORITHM(std::string* buffer,
                                          TPMA_ALGORITHM* value,
                                          std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMA_OBJECT(const TPMA_OBJECT& value,
                                           std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMA_OBJECT(std::string* buffer,
                                       TPMA_OBJECT* value,
                                       std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMA_SESSION(const TPMA_SESSION& value,
                                            std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMA_SESSION(std::string* buffer,
                                        TPMA_SESSION* value,
                                        std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMA_LOCALITY(const TPMA_LOCALITY& value,
                                             std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMA_LOCALITY(std::string* buffer,
                                         TPMA_LOCALITY* value,
                                         std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMA_PERMANENT(const TPMA_PERMANENT& value,
                                              std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMA_PERMANENT(std::string* buffer,
                                          TPMA_PERMANENT* value,
                                          std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMA_STARTUP_CLEAR(
    const TPMA_STARTUP_CLEAR& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMA_STARTUP_CLEAR(std::string* buffer,
                                              TPMA_STARTUP_CLEAR* value,
                                              std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMA_MEMORY(const TPMA_MEMORY& value,
                                           std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMA_MEMORY(std::string* buffer,
                                       TPMA_MEMORY* value,
                                       std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMA_CC(const TPMA_CC& value,
                                       std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMA_CC(std::string* buffer,
                                   TPMA_CC* value,
                                   std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM_NV_INDEX(const TPM_NV_INDEX& value,
                                            std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM_NV_INDEX(std::string* buffer,
                                        TPM_NV_INDEX* value,
                                        std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMA_NV(const TPMA_NV& value,
                                       std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMA_NV(std::string* buffer,
                                   TPMA_NV* value,
                                   std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM_SPEC(const TPM_SPEC& value,
                                        std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM_SPEC(std::string* buffer,
                                    TPM_SPEC* value,
                                    std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM_GENERATED(const TPM_GENERATED& value,
                                             std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM_GENERATED(std::string* buffer,
                                         TPM_GENERATED* value,
                                         std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM_ALG_ID(const TPM_ALG_ID& value,
                                          std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM_ALG_ID(std::string* buffer,
                                      TPM_ALG_ID* value,
                                      std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM_ECC_CURVE(const TPM_ECC_CURVE& value,
                                             std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM_ECC_CURVE(std::string* buffer,
                                         TPM_ECC_CURVE* value,
                                         std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM_CC(const TPM_CC& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM_CC(std::string* buffer,
                                  TPM_CC* value,
                                  std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM_RC(const TPM_RC& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM_RC(std::string* buffer,
                                  TPM_RC* value,
                                  std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM_CLOCK_ADJUST(const TPM_CLOCK_ADJUST& value,
                                                std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM_CLOCK_ADJUST(std::string* buffer,
                                            TPM_CLOCK_ADJUST* value,
                                            std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM_EO(const TPM_EO& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM_EO(std::string* buffer,
                                  TPM_EO* value,
                                  std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM_ST(const TPM_ST& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM_ST(std::string* buffer,
                                  TPM_ST* value,
                                  std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM_SU(const TPM_SU& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM_SU(std::string* buffer,
                                  TPM_SU* value,
                                  std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM_SE(const TPM_SE& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM_SE(std::string* buffer,
                                  TPM_SE* value,
                                  std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM_CAP(const TPM_CAP& value,
                                       std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM_CAP(std::string* buffer,
                                   TPM_CAP* value,
                                   std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM_PT(const TPM_PT& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM_PT(std::string* buffer,
                                  TPM_PT* value,
                                  std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM_PT_PCR(const TPM_PT_PCR& value,
                                          std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM_PT_PCR(std::string* buffer,
                                      TPM_PT_PCR* value,
                                      std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM_PS(const TPM_PS& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM_PS(std::string* buffer,
                                  TPM_PS* value,
                                  std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM_HT(const TPM_HT& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM_HT(std::string* buffer,
                                  TPM_HT* value,
                                  std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM_RH(const TPM_RH& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM_RH(std::string* buffer,
                                  TPM_RH* value,
                                  std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM_HC(const TPM_HC& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM_HC(std::string* buffer,
                                  TPM_HC* value,
                                  std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_ALGORITHM_DESCRIPTION(
    const TPMS_ALGORITHM_DESCRIPTION& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC
Parse_TPMS_ALGORITHM_DESCRIPTION(std::string* buffer,
                                 TPMS_ALGORITHM_DESCRIPTION* value,
                                 std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMT_HA(const TPMT_HA& value,
                                       std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMT_HA(std::string* buffer,
                                   TPMT_HA* value,
                                   std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM2B_DIGEST(const TPM2B_DIGEST& value,
                                            std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM2B_DIGEST(std::string* buffer,
                                        TPM2B_DIGEST* value,
                                        std::string* value_bytes);

TRUNKS_EXPORT TPM2B_DIGEST Make_TPM2B_DIGEST(const std::string& bytes);
TRUNKS_EXPORT std::string StringFrom_TPM2B_DIGEST(const TPM2B_DIGEST& tpm2b);

TRUNKS_EXPORT TPM_RC Serialize_TPM2B_DATA(const TPM2B_DATA& value,
                                          std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM2B_DATA(std::string* buffer,
                                      TPM2B_DATA* value,
                                      std::string* value_bytes);

TRUNKS_EXPORT TPM2B_DATA Make_TPM2B_DATA(const std::string& bytes);
TRUNKS_EXPORT std::string StringFrom_TPM2B_DATA(const TPM2B_DATA& tpm2b);

TRUNKS_EXPORT TPM_RC Serialize_TPM2B_EVENT(const TPM2B_EVENT& value,
                                           std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM2B_EVENT(std::string* buffer,
                                       TPM2B_EVENT* value,
                                       std::string* value_bytes);

TRUNKS_EXPORT TPM2B_EVENT Make_TPM2B_EVENT(const std::string& bytes);
TRUNKS_EXPORT std::string StringFrom_TPM2B_EVENT(const TPM2B_EVENT& tpm2b);

TRUNKS_EXPORT TPM_RC Serialize_TPM2B_MAX_BUFFER(const TPM2B_MAX_BUFFER& value,
                                                std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM2B_MAX_BUFFER(std::string* buffer,
                                            TPM2B_MAX_BUFFER* value,
                                            std::string* value_bytes);

TRUNKS_EXPORT TPM2B_MAX_BUFFER Make_TPM2B_MAX_BUFFER(const std::string& bytes);
TRUNKS_EXPORT std::string StringFrom_TPM2B_MAX_BUFFER(
    const TPM2B_MAX_BUFFER& tpm2b);

TRUNKS_EXPORT TPM_RC Serialize_TPM2B_MAX_NV_BUFFER(
    const TPM2B_MAX_NV_BUFFER& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM2B_MAX_NV_BUFFER(std::string* buffer,
                                               TPM2B_MAX_NV_BUFFER* value,
                                               std::string* value_bytes);

TRUNKS_EXPORT TPM2B_MAX_NV_BUFFER
Make_TPM2B_MAX_NV_BUFFER(const std::string& bytes);
TRUNKS_EXPORT std::string StringFrom_TPM2B_MAX_NV_BUFFER(
    const TPM2B_MAX_NV_BUFFER& tpm2b);

TRUNKS_EXPORT TPM_RC Serialize_TPM2B_TIMEOUT(const TPM2B_TIMEOUT& value,
                                             std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM2B_TIMEOUT(std::string* buffer,
                                         TPM2B_TIMEOUT* value,
                                         std::string* value_bytes);

TRUNKS_EXPORT TPM2B_TIMEOUT Make_TPM2B_TIMEOUT(const std::string& bytes);
TRUNKS_EXPORT std::string StringFrom_TPM2B_TIMEOUT(const TPM2B_TIMEOUT& tpm2b);

TRUNKS_EXPORT TPM_RC Serialize_TPM2B_IV(const TPM2B_IV& value,
                                        std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM2B_IV(std::string* buffer,
                                    TPM2B_IV* value,
                                    std::string* value_bytes);

TRUNKS_EXPORT TPM2B_IV Make_TPM2B_IV(const std::string& bytes);
TRUNKS_EXPORT std::string StringFrom_TPM2B_IV(const TPM2B_IV& tpm2b);

TRUNKS_EXPORT TPM_RC Serialize_TPM2B_NAME(const TPM2B_NAME& value,
                                          std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM2B_NAME(std::string* buffer,
                                      TPM2B_NAME* value,
                                      std::string* value_bytes);

TRUNKS_EXPORT TPM2B_NAME Make_TPM2B_NAME(const std::string& bytes);
TRUNKS_EXPORT std::string StringFrom_TPM2B_NAME(const TPM2B_NAME& tpm2b);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_PCR_SELECT(const TPMS_PCR_SELECT& value,
                                               std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_PCR_SELECT(std::string* buffer,
                                           TPMS_PCR_SELECT* value,
                                           std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_PCR_SELECTION(
    const TPMS_PCR_SELECTION& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_PCR_SELECTION(std::string* buffer,
                                              TPMS_PCR_SELECTION* value,
                                              std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMT_TK_CREATION(const TPMT_TK_CREATION& value,
                                                std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMT_TK_CREATION(std::string* buffer,
                                            TPMT_TK_CREATION* value,
                                            std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMT_TK_VERIFIED(const TPMT_TK_VERIFIED& value,
                                                std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMT_TK_VERIFIED(std::string* buffer,
                                            TPMT_TK_VERIFIED* value,
                                            std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMT_TK_AUTH(const TPMT_TK_AUTH& value,
                                            std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMT_TK_AUTH(std::string* buffer,
                                        TPMT_TK_AUTH* value,
                                        std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMT_TK_HASHCHECK(const TPMT_TK_HASHCHECK& value,
                                                 std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMT_TK_HASHCHECK(std::string* buffer,
                                             TPMT_TK_HASHCHECK* value,
                                             std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_ALG_PROPERTY(const TPMS_ALG_PROPERTY& value,
                                                 std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_ALG_PROPERTY(std::string* buffer,
                                             TPMS_ALG_PROPERTY* value,
                                             std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_TAGGED_PROPERTY(
    const TPMS_TAGGED_PROPERTY& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_TAGGED_PROPERTY(std::string* buffer,
                                                TPMS_TAGGED_PROPERTY* value,
                                                std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_TAGGED_PCR_SELECT(
    const TPMS_TAGGED_PCR_SELECT& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_TAGGED_PCR_SELECT(std::string* buffer,
                                                  TPMS_TAGGED_PCR_SELECT* value,
                                                  std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPML_CC(const TPML_CC& value,
                                       std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPML_CC(std::string* buffer,
                                   TPML_CC* value,
                                   std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPML_CCA(const TPML_CCA& value,
                                        std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPML_CCA(std::string* buffer,
                                    TPML_CCA* value,
                                    std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPML_ALG(const TPML_ALG& value,
                                        std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPML_ALG(std::string* buffer,
                                    TPML_ALG* value,
                                    std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPML_HANDLE(const TPML_HANDLE& value,
                                           std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPML_HANDLE(std::string* buffer,
                                       TPML_HANDLE* value,
                                       std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPML_DIGEST(const TPML_DIGEST& value,
                                           std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPML_DIGEST(std::string* buffer,
                                       TPML_DIGEST* value,
                                       std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPML_DIGEST_VALUES(
    const TPML_DIGEST_VALUES& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPML_DIGEST_VALUES(std::string* buffer,
                                              TPML_DIGEST_VALUES* value,
                                              std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM2B_DIGEST_VALUES(
    const TPM2B_DIGEST_VALUES& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM2B_DIGEST_VALUES(std::string* buffer,
                                               TPM2B_DIGEST_VALUES* value,
                                               std::string* value_bytes);

TRUNKS_EXPORT TPM2B_DIGEST_VALUES
Make_TPM2B_DIGEST_VALUES(const std::string& bytes);
TRUNKS_EXPORT std::string StringFrom_TPM2B_DIGEST_VALUES(
    const TPM2B_DIGEST_VALUES& tpm2b);

TRUNKS_EXPORT TPM_RC Serialize_TPML_PCR_SELECTION(
    const TPML_PCR_SELECTION& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPML_PCR_SELECTION(std::string* buffer,
                                              TPML_PCR_SELECTION* value,
                                              std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPML_ALG_PROPERTY(const TPML_ALG_PROPERTY& value,
                                                 std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPML_ALG_PROPERTY(std::string* buffer,
                                             TPML_ALG_PROPERTY* value,
                                             std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPML_TAGGED_TPM_PROPERTY(
    const TPML_TAGGED_TPM_PROPERTY& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC
Parse_TPML_TAGGED_TPM_PROPERTY(std::string* buffer,
                               TPML_TAGGED_TPM_PROPERTY* value,
                               std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPML_TAGGED_PCR_PROPERTY(
    const TPML_TAGGED_PCR_PROPERTY& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC
Parse_TPML_TAGGED_PCR_PROPERTY(std::string* buffer,
                               TPML_TAGGED_PCR_PROPERTY* value,
                               std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPML_ECC_CURVE(const TPML_ECC_CURVE& value,
                                              std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPML_ECC_CURVE(std::string* buffer,
                                          TPML_ECC_CURVE* value,
                                          std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_CAPABILITY_DATA(
    const TPMS_CAPABILITY_DATA& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_CAPABILITY_DATA(std::string* buffer,
                                                TPMS_CAPABILITY_DATA* value,
                                                std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_CLOCK_INFO(const TPMS_CLOCK_INFO& value,
                                               std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_CLOCK_INFO(std::string* buffer,
                                           TPMS_CLOCK_INFO* value,
                                           std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_TIME_INFO(const TPMS_TIME_INFO& value,
                                              std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_TIME_INFO(std::string* buffer,
                                          TPMS_TIME_INFO* value,
                                          std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_TIME_ATTEST_INFO(
    const TPMS_TIME_ATTEST_INFO& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_TIME_ATTEST_INFO(std::string* buffer,
                                                 TPMS_TIME_ATTEST_INFO* value,
                                                 std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_CERTIFY_INFO(const TPMS_CERTIFY_INFO& value,
                                                 std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_CERTIFY_INFO(std::string* buffer,
                                             TPMS_CERTIFY_INFO* value,
                                             std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_QUOTE_INFO(const TPMS_QUOTE_INFO& value,
                                               std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_QUOTE_INFO(std::string* buffer,
                                           TPMS_QUOTE_INFO* value,
                                           std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_COMMAND_AUDIT_INFO(
    const TPMS_COMMAND_AUDIT_INFO& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC
Parse_TPMS_COMMAND_AUDIT_INFO(std::string* buffer,
                              TPMS_COMMAND_AUDIT_INFO* value,
                              std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_SESSION_AUDIT_INFO(
    const TPMS_SESSION_AUDIT_INFO& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC
Parse_TPMS_SESSION_AUDIT_INFO(std::string* buffer,
                              TPMS_SESSION_AUDIT_INFO* value,
                              std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_CREATION_INFO(
    const TPMS_CREATION_INFO& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_CREATION_INFO(std::string* buffer,
                                              TPMS_CREATION_INFO* value,
                                              std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_NV_CERTIFY_INFO(
    const TPMS_NV_CERTIFY_INFO& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_NV_CERTIFY_INFO(std::string* buffer,
                                                TPMS_NV_CERTIFY_INFO* value,
                                                std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_ATTEST(const TPMS_ATTEST& value,
                                           std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_ATTEST(std::string* buffer,
                                       TPMS_ATTEST* value,
                                       std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM2B_ATTEST(const TPM2B_ATTEST& value,
                                            std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM2B_ATTEST(std::string* buffer,
                                        TPM2B_ATTEST* value,
                                        std::string* value_bytes);

TRUNKS_EXPORT TPM2B_ATTEST Make_TPM2B_ATTEST(const std::string& bytes);
TRUNKS_EXPORT std::string StringFrom_TPM2B_ATTEST(const TPM2B_ATTEST& tpm2b);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_AUTH_COMMAND(const TPMS_AUTH_COMMAND& value,
                                                 std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_AUTH_COMMAND(std::string* buffer,
                                             TPMS_AUTH_COMMAND* value,
                                             std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_AUTH_RESPONSE(
    const TPMS_AUTH_RESPONSE& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_AUTH_RESPONSE(std::string* buffer,
                                              TPMS_AUTH_RESPONSE* value,
                                              std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMT_SYM_DEF(const TPMT_SYM_DEF& value,
                                            std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMT_SYM_DEF(std::string* buffer,
                                        TPMT_SYM_DEF* value,
                                        std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMT_SYM_DEF_OBJECT(
    const TPMT_SYM_DEF_OBJECT& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMT_SYM_DEF_OBJECT(std::string* buffer,
                                               TPMT_SYM_DEF_OBJECT* value,
                                               std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM2B_SYM_KEY(const TPM2B_SYM_KEY& value,
                                             std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM2B_SYM_KEY(std::string* buffer,
                                         TPM2B_SYM_KEY* value,
                                         std::string* value_bytes);

TRUNKS_EXPORT TPM2B_SYM_KEY Make_TPM2B_SYM_KEY(const std::string& bytes);
TRUNKS_EXPORT std::string StringFrom_TPM2B_SYM_KEY(const TPM2B_SYM_KEY& tpm2b);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_SYMCIPHER_PARMS(
    const TPMS_SYMCIPHER_PARMS& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_SYMCIPHER_PARMS(std::string* buffer,
                                                TPMS_SYMCIPHER_PARMS* value,
                                                std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM2B_SENSITIVE_DATA(
    const TPM2B_SENSITIVE_DATA& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM2B_SENSITIVE_DATA(std::string* buffer,
                                                TPM2B_SENSITIVE_DATA* value,
                                                std::string* value_bytes);

TRUNKS_EXPORT TPM2B_SENSITIVE_DATA
Make_TPM2B_SENSITIVE_DATA(const std::string& bytes);
TRUNKS_EXPORT std::string StringFrom_TPM2B_SENSITIVE_DATA(
    const TPM2B_SENSITIVE_DATA& tpm2b);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_SENSITIVE_CREATE(
    const TPMS_SENSITIVE_CREATE& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_SENSITIVE_CREATE(std::string* buffer,
                                                 TPMS_SENSITIVE_CREATE* value,
                                                 std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM2B_SENSITIVE_CREATE(
    const TPM2B_SENSITIVE_CREATE& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM2B_SENSITIVE_CREATE(std::string* buffer,
                                                  TPM2B_SENSITIVE_CREATE* value,
                                                  std::string* value_bytes);

TRUNKS_EXPORT TPM2B_SENSITIVE_CREATE
Make_TPM2B_SENSITIVE_CREATE(const TPMS_SENSITIVE_CREATE& inner);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_SCHEME_SIGHASH(
    const TPMS_SCHEME_SIGHASH& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_SCHEME_SIGHASH(std::string* buffer,
                                               TPMS_SCHEME_SIGHASH* value,
                                               std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_SCHEME_XOR(const TPMS_SCHEME_XOR& value,
                                               std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_SCHEME_XOR(std::string* buffer,
                                           TPMS_SCHEME_XOR* value,
                                           std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMT_KEYEDHASH_SCHEME(
    const TPMT_KEYEDHASH_SCHEME& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMT_KEYEDHASH_SCHEME(std::string* buffer,
                                                 TPMT_KEYEDHASH_SCHEME* value,
                                                 std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_SCHEME_ECDAA(const TPMS_SCHEME_ECDAA& value,
                                                 std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_SCHEME_ECDAA(std::string* buffer,
                                             TPMS_SCHEME_ECDAA* value,
                                             std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMT_SIG_SCHEME(const TPMT_SIG_SCHEME& value,
                                               std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMT_SIG_SCHEME(std::string* buffer,
                                           TPMT_SIG_SCHEME* value,
                                           std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_SCHEME_OAEP(const TPMS_SCHEME_OAEP& value,
                                                std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_SCHEME_OAEP(std::string* buffer,
                                            TPMS_SCHEME_OAEP* value,
                                            std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_SCHEME_ECDH(const TPMS_SCHEME_ECDH& value,
                                                std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_SCHEME_ECDH(std::string* buffer,
                                            TPMS_SCHEME_ECDH* value,
                                            std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_SCHEME_MGF1(const TPMS_SCHEME_MGF1& value,
                                                std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_SCHEME_MGF1(std::string* buffer,
                                            TPMS_SCHEME_MGF1* value,
                                            std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_SCHEME_KDF1_SP800_56a(
    const TPMS_SCHEME_KDF1_SP800_56a& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC
Parse_TPMS_SCHEME_KDF1_SP800_56a(std::string* buffer,
                                 TPMS_SCHEME_KDF1_SP800_56a* value,
                                 std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_SCHEME_KDF2(const TPMS_SCHEME_KDF2& value,
                                                std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_SCHEME_KDF2(std::string* buffer,
                                            TPMS_SCHEME_KDF2* value,
                                            std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_SCHEME_KDF1_SP800_108(
    const TPMS_SCHEME_KDF1_SP800_108& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC
Parse_TPMS_SCHEME_KDF1_SP800_108(std::string* buffer,
                                 TPMS_SCHEME_KDF1_SP800_108* value,
                                 std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMT_KDF_SCHEME(const TPMT_KDF_SCHEME& value,
                                               std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMT_KDF_SCHEME(std::string* buffer,
                                           TPMT_KDF_SCHEME* value,
                                           std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMT_ASYM_SCHEME(const TPMT_ASYM_SCHEME& value,
                                                std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMT_ASYM_SCHEME(std::string* buffer,
                                            TPMT_ASYM_SCHEME* value,
                                            std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMT_RSA_SCHEME(const TPMT_RSA_SCHEME& value,
                                               std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMT_RSA_SCHEME(std::string* buffer,
                                           TPMT_RSA_SCHEME* value,
                                           std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMT_RSA_DECRYPT(const TPMT_RSA_DECRYPT& value,
                                                std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMT_RSA_DECRYPT(std::string* buffer,
                                            TPMT_RSA_DECRYPT* value,
                                            std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM2B_PUBLIC_KEY_RSA(
    const TPM2B_PUBLIC_KEY_RSA& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM2B_PUBLIC_KEY_RSA(std::string* buffer,
                                                TPM2B_PUBLIC_KEY_RSA* value,
                                                std::string* value_bytes);

TRUNKS_EXPORT TPM2B_PUBLIC_KEY_RSA
Make_TPM2B_PUBLIC_KEY_RSA(const std::string& bytes);
TRUNKS_EXPORT std::string StringFrom_TPM2B_PUBLIC_KEY_RSA(
    const TPM2B_PUBLIC_KEY_RSA& tpm2b);

TRUNKS_EXPORT TPM_RC Serialize_TPM2B_PRIVATE_KEY_RSA(
    const TPM2B_PRIVATE_KEY_RSA& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM2B_PRIVATE_KEY_RSA(std::string* buffer,
                                                 TPM2B_PRIVATE_KEY_RSA* value,
                                                 std::string* value_bytes);

TRUNKS_EXPORT TPM2B_PRIVATE_KEY_RSA
Make_TPM2B_PRIVATE_KEY_RSA(const std::string& bytes);
TRUNKS_EXPORT std::string StringFrom_TPM2B_PRIVATE_KEY_RSA(
    const TPM2B_PRIVATE_KEY_RSA& tpm2b);

TRUNKS_EXPORT TPM_RC Serialize_TPM2B_ECC_PARAMETER(
    const TPM2B_ECC_PARAMETER& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM2B_ECC_PARAMETER(std::string* buffer,
                                               TPM2B_ECC_PARAMETER* value,
                                               std::string* value_bytes);

TRUNKS_EXPORT TPM2B_ECC_PARAMETER
Make_TPM2B_ECC_PARAMETER(const std::string& bytes);
TRUNKS_EXPORT std::string StringFrom_TPM2B_ECC_PARAMETER(
    const TPM2B_ECC_PARAMETER& tpm2b);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_ECC_POINT(const TPMS_ECC_POINT& value,
                                              std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_ECC_POINT(std::string* buffer,
                                          TPMS_ECC_POINT* value,
                                          std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM2B_ECC_POINT(const TPM2B_ECC_POINT& value,
                                               std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM2B_ECC_POINT(std::string* buffer,
                                           TPM2B_ECC_POINT* value,
                                           std::string* value_bytes);

TRUNKS_EXPORT TPM2B_ECC_POINT Make_TPM2B_ECC_POINT(const TPMS_ECC_POINT& inner);

TRUNKS_EXPORT TPM_RC Serialize_TPMT_ECC_SCHEME(const TPMT_ECC_SCHEME& value,
                                               std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMT_ECC_SCHEME(std::string* buffer,
                                           TPMT_ECC_SCHEME* value,
                                           std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_ALGORITHM_DETAIL_ECC(
    const TPMS_ALGORITHM_DETAIL_ECC& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC
Parse_TPMS_ALGORITHM_DETAIL_ECC(std::string* buffer,
                                TPMS_ALGORITHM_DETAIL_ECC* value,
                                std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_SIGNATURE_RSASSA(
    const TPMS_SIGNATURE_RSASSA& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_SIGNATURE_RSASSA(std::string* buffer,
                                                 TPMS_SIGNATURE_RSASSA* value,
                                                 std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_SIGNATURE_RSAPSS(
    const TPMS_SIGNATURE_RSAPSS& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_SIGNATURE_RSAPSS(std::string* buffer,
                                                 TPMS_SIGNATURE_RSAPSS* value,
                                                 std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_SIGNATURE_ECDSA(
    const TPMS_SIGNATURE_ECDSA& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_SIGNATURE_ECDSA(std::string* buffer,
                                                TPMS_SIGNATURE_ECDSA* value,
                                                std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMT_SIGNATURE(const TPMT_SIGNATURE& value,
                                              std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMT_SIGNATURE(std::string* buffer,
                                          TPMT_SIGNATURE* value,
                                          std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM2B_ENCRYPTED_SECRET(
    const TPM2B_ENCRYPTED_SECRET& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM2B_ENCRYPTED_SECRET(std::string* buffer,
                                                  TPM2B_ENCRYPTED_SECRET* value,
                                                  std::string* value_bytes);

TRUNKS_EXPORT TPM2B_ENCRYPTED_SECRET
Make_TPM2B_ENCRYPTED_SECRET(const std::string& bytes);
TRUNKS_EXPORT std::string StringFrom_TPM2B_ENCRYPTED_SECRET(
    const TPM2B_ENCRYPTED_SECRET& tpm2b);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_KEYEDHASH_PARMS(
    const TPMS_KEYEDHASH_PARMS& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_KEYEDHASH_PARMS(std::string* buffer,
                                                TPMS_KEYEDHASH_PARMS* value,
                                                std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_ASYM_PARMS(const TPMS_ASYM_PARMS& value,
                                               std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_ASYM_PARMS(std::string* buffer,
                                           TPMS_ASYM_PARMS* value,
                                           std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_RSA_PARMS(const TPMS_RSA_PARMS& value,
                                              std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_RSA_PARMS(std::string* buffer,
                                          TPMS_RSA_PARMS* value,
                                          std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_ECC_PARMS(const TPMS_ECC_PARMS& value,
                                              std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_ECC_PARMS(std::string* buffer,
                                          TPMS_ECC_PARMS* value,
                                          std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMT_PUBLIC_PARMS(const TPMT_PUBLIC_PARMS& value,
                                                 std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMT_PUBLIC_PARMS(std::string* buffer,
                                             TPMT_PUBLIC_PARMS* value,
                                             std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMT_PUBLIC(const TPMT_PUBLIC& value,
                                           std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMT_PUBLIC(std::string* buffer,
                                       TPMT_PUBLIC* value,
                                       std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM2B_PUBLIC(const TPM2B_PUBLIC& value,
                                            std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM2B_PUBLIC(std::string* buffer,
                                        TPM2B_PUBLIC* value,
                                        std::string* value_bytes);

TRUNKS_EXPORT TPM2B_PUBLIC Make_TPM2B_PUBLIC(const TPMT_PUBLIC& inner);

TRUNKS_EXPORT TPM_RC Serialize_TPM2B_PRIVATE_VENDOR_SPECIFIC(
    const TPM2B_PRIVATE_VENDOR_SPECIFIC& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC
Parse_TPM2B_PRIVATE_VENDOR_SPECIFIC(std::string* buffer,
                                    TPM2B_PRIVATE_VENDOR_SPECIFIC* value,
                                    std::string* value_bytes);

TRUNKS_EXPORT TPM2B_PRIVATE_VENDOR_SPECIFIC
Make_TPM2B_PRIVATE_VENDOR_SPECIFIC(const std::string& bytes);
TRUNKS_EXPORT std::string StringFrom_TPM2B_PRIVATE_VENDOR_SPECIFIC(
    const TPM2B_PRIVATE_VENDOR_SPECIFIC& tpm2b);

TRUNKS_EXPORT TPM_RC Serialize_TPMT_SENSITIVE(const TPMT_SENSITIVE& value,
                                              std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMT_SENSITIVE(std::string* buffer,
                                          TPMT_SENSITIVE* value,
                                          std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM2B_SENSITIVE(const TPM2B_SENSITIVE& value,
                                               std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM2B_SENSITIVE(std::string* buffer,
                                           TPM2B_SENSITIVE* value,
                                           std::string* value_bytes);

TRUNKS_EXPORT TPM2B_SENSITIVE Make_TPM2B_SENSITIVE(const TPMT_SENSITIVE& inner);

TRUNKS_EXPORT TPM_RC Serialize_TPM2B_PRIVATE(const TPM2B_PRIVATE& value,
                                             std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM2B_PRIVATE(std::string* buffer,
                                         TPM2B_PRIVATE* value,
                                         std::string* value_bytes);

TRUNKS_EXPORT TPM2B_PRIVATE Make_TPM2B_PRIVATE(const std::string& bytes);
TRUNKS_EXPORT std::string StringFrom_TPM2B_PRIVATE(const TPM2B_PRIVATE& tpm2b);

TRUNKS_EXPORT TPM_RC Serialize_TPM2B_ID_OBJECT(const TPM2B_ID_OBJECT& value,
                                               std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM2B_ID_OBJECT(std::string* buffer,
                                           TPM2B_ID_OBJECT* value,
                                           std::string* value_bytes);

TRUNKS_EXPORT TPM2B_ID_OBJECT Make_TPM2B_ID_OBJECT(const std::string& bytes);
TRUNKS_EXPORT std::string StringFrom_TPM2B_ID_OBJECT(
    const TPM2B_ID_OBJECT& tpm2b);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_NV_PUBLIC(const TPMS_NV_PUBLIC& value,
                                              std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_NV_PUBLIC(std::string* buffer,
                                          TPMS_NV_PUBLIC* value,
                                          std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM2B_NV_PUBLIC(const TPM2B_NV_PUBLIC& value,
                                               std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM2B_NV_PUBLIC(std::string* buffer,
                                           TPM2B_NV_PUBLIC* value,
                                           std::string* value_bytes);

TRUNKS_EXPORT TPM2B_NV_PUBLIC Make_TPM2B_NV_PUBLIC(const TPMS_NV_PUBLIC& inner);

TRUNKS_EXPORT TPM_RC Serialize_TPM2B_CONTEXT_SENSITIVE(
    const TPM2B_CONTEXT_SENSITIVE& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC
Parse_TPM2B_CONTEXT_SENSITIVE(std::string* buffer,
                              TPM2B_CONTEXT_SENSITIVE* value,
                              std::string* value_bytes);

TRUNKS_EXPORT TPM2B_CONTEXT_SENSITIVE
Make_TPM2B_CONTEXT_SENSITIVE(const std::string& bytes);
TRUNKS_EXPORT std::string StringFrom_TPM2B_CONTEXT_SENSITIVE(
    const TPM2B_CONTEXT_SENSITIVE& tpm2b);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_CONTEXT_DATA(const TPMS_CONTEXT_DATA& value,
                                                 std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_CONTEXT_DATA(std::string* buffer,
                                             TPMS_CONTEXT_DATA* value,
                                             std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM2B_CONTEXT_DATA(
    const TPM2B_CONTEXT_DATA& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM2B_CONTEXT_DATA(std::string* buffer,
                                              TPM2B_CONTEXT_DATA* value,
                                              std::string* value_bytes);

TRUNKS_EXPORT TPM2B_CONTEXT_DATA
Make_TPM2B_CONTEXT_DATA(const std::string& bytes);
TRUNKS_EXPORT std::string StringFrom_TPM2B_CONTEXT_DATA(
    const TPM2B_CONTEXT_DATA& tpm2b);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_CONTEXT(const TPMS_CONTEXT& value,
                                            std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_CONTEXT(std::string* buffer,
                                        TPMS_CONTEXT* value,
                                        std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMS_CREATION_DATA(
    const TPMS_CREATION_DATA& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMS_CREATION_DATA(std::string* buffer,
                                              TPMS_CREATION_DATA* value,
                                              std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPM2B_CREATION_DATA(
    const TPM2B_CREATION_DATA& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPM2B_CREATION_DATA(std::string* buffer,
                                               TPM2B_CREATION_DATA* value,
                                               std::string* value_bytes);

TRUNKS_EXPORT TPM2B_CREATION_DATA
Make_TPM2B_CREATION_DATA(const TPMS_CREATION_DATA& inner);

TRUNKS_EXPORT TPM_RC Serialize_TPMU_HA(const TPMU_HA& value,
                                       std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMU_HA(std::string* buffer,
                                   TPMU_HA* value,
                                   std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMU_NAME(const TPMU_NAME& value,
                                         std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMU_NAME(std::string* buffer,
                                     TPMU_NAME* value,
                                     std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMU_CAPABILITIES(const TPMU_CAPABILITIES& value,
                                                 std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMU_CAPABILITIES(std::string* buffer,
                                             TPMU_CAPABILITIES* value,
                                             std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMU_ATTEST(const TPMU_ATTEST& value,
                                           std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMU_ATTEST(std::string* buffer,
                                       TPMU_ATTEST* value,
                                       std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMU_SYM_KEY_BITS(const TPMU_SYM_KEY_BITS& value,
                                                 std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMU_SYM_KEY_BITS(std::string* buffer,
                                             TPMU_SYM_KEY_BITS* value,
                                             std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMU_SYM_MODE(const TPMU_SYM_MODE& value,
                                             std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMU_SYM_MODE(std::string* buffer,
                                         TPMU_SYM_MODE* value,
                                         std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMU_SCHEME_KEYEDHASH(
    const TPMU_SCHEME_KEYEDHASH& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMU_SCHEME_KEYEDHASH(std::string* buffer,
                                                 TPMU_SCHEME_KEYEDHASH* value,
                                                 std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMU_SIG_SCHEME(const TPMU_SIG_SCHEME& value,
                                               std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMU_SIG_SCHEME(std::string* buffer,
                                           TPMU_SIG_SCHEME* value,
                                           std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMU_KDF_SCHEME(const TPMU_KDF_SCHEME& value,
                                               std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMU_KDF_SCHEME(std::string* buffer,
                                           TPMU_KDF_SCHEME* value,
                                           std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMU_ASYM_SCHEME(const TPMU_ASYM_SCHEME& value,
                                                std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMU_ASYM_SCHEME(std::string* buffer,
                                            TPMU_ASYM_SCHEME* value,
                                            std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMU_SIGNATURE(const TPMU_SIGNATURE& value,
                                              std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMU_SIGNATURE(std::string* buffer,
                                          TPMU_SIGNATURE* value,
                                          std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMU_ENCRYPTED_SECRET(
    const TPMU_ENCRYPTED_SECRET& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMU_ENCRYPTED_SECRET(std::string* buffer,
                                                 TPMU_ENCRYPTED_SECRET* value,
                                                 std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMU_PUBLIC_ID(const TPMU_PUBLIC_ID& value,
                                              std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMU_PUBLIC_ID(std::string* buffer,
                                          TPMU_PUBLIC_ID* value,
                                          std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMU_PUBLIC_PARMS(const TPMU_PUBLIC_PARMS& value,
                                                 std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMU_PUBLIC_PARMS(std::string* buffer,
                                             TPMU_PUBLIC_PARMS* value,
                                             std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMU_SENSITIVE_COMPOSITE(
    const TPMU_SENSITIVE_COMPOSITE& value, std::string* buffer);

TRUNKS_EXPORT TPM_RC
Parse_TPMU_SENSITIVE_COMPOSITE(std::string* buffer,
                               TPMU_SENSITIVE_COMPOSITE* value,
                               std::string* value_bytes);

TRUNKS_EXPORT TPM_RC Serialize_TPMU_SYM_DETAILS(const TPMU_SYM_DETAILS& value,
                                                std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_TPMU_SYM_DETAILS(std::string* buffer,
                                            TPMU_SYM_DETAILS* value,
                                            std::string* value_bytes);

class TRUNKS_EXPORT Tpm {
 public:
  // Does not take ownership of |transceiver|.
  explicit Tpm(CommandTransceiver* transceiver) : transceiver_(transceiver) {}
  Tpm(const Tpm&) = delete;
  Tpm& operator=(const Tpm&) = delete;

  virtual ~Tpm() {}
  CommandTransceiver* get_transceiver() { return transceiver_; }

  typedef base::OnceCallback<void(TPM_RC response_code)> StartupResponse;
  static TPM_RC SerializeCommand_Startup(
      const TPM_SU& startup_type,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_Startup(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void Startup(const TPM_SU& startup_type,
                       AuthorizationDelegate* authorization_delegate,
                       StartupResponse callback);
  virtual TPM_RC StartupSync(const TPM_SU& startup_type,
                             AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)> ShutdownResponse;
  static TPM_RC SerializeCommand_Shutdown(
      const TPM_SU& shutdown_type,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_Shutdown(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void Shutdown(const TPM_SU& shutdown_type,
                        AuthorizationDelegate* authorization_delegate,
                        ShutdownResponse callback);
  virtual TPM_RC ShutdownSync(const TPM_SU& shutdown_type,
                              AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)> SelfTestResponse;
  static TPM_RC SerializeCommand_SelfTest(
      const TPMI_YES_NO& full_test,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_SelfTest(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void SelfTest(const TPMI_YES_NO& full_test,
                        AuthorizationDelegate* authorization_delegate,
                        SelfTestResponse callback);
  virtual TPM_RC SelfTestSync(const TPMI_YES_NO& full_test,
                              AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPML_ALG& to_do_list)>
      IncrementalSelfTestResponse;
  static TPM_RC SerializeCommand_IncrementalSelfTest(
      const TPML_ALG& to_test,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_IncrementalSelfTest(
      const std::string& response,
      TPML_ALG* to_do_list,
      AuthorizationDelegate* authorization_delegate);
  virtual void IncrementalSelfTest(
      const TPML_ALG& to_test,
      AuthorizationDelegate* authorization_delegate,
      IncrementalSelfTestResponse callback);
  virtual TPM_RC IncrementalSelfTestSync(
      const TPML_ALG& to_test,
      TPML_ALG* to_do_list,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM2B_MAX_BUFFER& out_data,
                                  const TPM_RC& test_result)>
      GetTestResultResponse;
  static TPM_RC SerializeCommand_GetTestResult(
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_GetTestResult(
      const std::string& response,
      TPM2B_MAX_BUFFER* out_data,
      TPM_RC* test_result,
      AuthorizationDelegate* authorization_delegate);
  virtual void GetTestResult(AuthorizationDelegate* authorization_delegate,
                             GetTestResultResponse callback);
  virtual TPM_RC GetTestResultSync(
      TPM2B_MAX_BUFFER* out_data,
      TPM_RC* test_result,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPMI_SH_AUTH_SESSION& session_handle,
                                  const TPM2B_NONCE& nonce_tpm)>
      StartAuthSessionResponse;
  static TPM_RC SerializeCommand_StartAuthSession(
      const TPMI_DH_OBJECT& tpm_key,
      const std::string& tpm_key_name,
      const TPMI_DH_ENTITY& bind,
      const std::string& bind_name,
      const TPM2B_NONCE& nonce_caller,
      const TPM2B_ENCRYPTED_SECRET& encrypted_salt,
      const TPM_SE& session_type,
      const TPMT_SYM_DEF& symmetric,
      const TPMI_ALG_HASH& auth_hash,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_StartAuthSession(
      const std::string& response,
      TPMI_SH_AUTH_SESSION* session_handle,
      TPM2B_NONCE* nonce_tpm,
      AuthorizationDelegate* authorization_delegate);
  virtual void StartAuthSession(const TPMI_DH_OBJECT& tpm_key,
                                const std::string& tpm_key_name,
                                const TPMI_DH_ENTITY& bind,
                                const std::string& bind_name,
                                const TPM2B_NONCE& nonce_caller,
                                const TPM2B_ENCRYPTED_SECRET& encrypted_salt,
                                const TPM_SE& session_type,
                                const TPMT_SYM_DEF& symmetric,
                                const TPMI_ALG_HASH& auth_hash,
                                AuthorizationDelegate* authorization_delegate,
                                StartAuthSessionResponse callback);
  virtual TPM_RC StartAuthSessionSync(
      const TPMI_DH_OBJECT& tpm_key,
      const std::string& tpm_key_name,
      const TPMI_DH_ENTITY& bind,
      const std::string& bind_name,
      const TPM2B_NONCE& nonce_caller,
      const TPM2B_ENCRYPTED_SECRET& encrypted_salt,
      const TPM_SE& session_type,
      const TPMT_SYM_DEF& symmetric,
      const TPMI_ALG_HASH& auth_hash,
      TPMI_SH_AUTH_SESSION* session_handle,
      TPM2B_NONCE* nonce_tpm,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)> PolicyRestartResponse;
  static TPM_RC SerializeCommand_PolicyRestart(
      const TPMI_SH_POLICY& session_handle,
      const std::string& session_handle_name,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_PolicyRestart(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void PolicyRestart(const TPMI_SH_POLICY& session_handle,
                             const std::string& session_handle_name,
                             AuthorizationDelegate* authorization_delegate,
                             PolicyRestartResponse callback);
  virtual TPM_RC PolicyRestartSync(
      const TPMI_SH_POLICY& session_handle,
      const std::string& session_handle_name,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM2B_PRIVATE& out_private,
                                  const TPM2B_PUBLIC& out_public,
                                  const TPM2B_CREATION_DATA& creation_data,
                                  const TPM2B_DIGEST& creation_hash,
                                  const TPMT_TK_CREATION& creation_ticket)>
      CreateResponse;
  static TPM_RC SerializeCommand_Create(
      const TPMI_DH_OBJECT& parent_handle,
      const std::string& parent_handle_name,
      const TPM2B_SENSITIVE_CREATE& in_sensitive,
      const TPM2B_PUBLIC& in_public,
      const TPM2B_DATA& outside_info,
      const TPML_PCR_SELECTION& creation_pcr,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_Create(
      const std::string& response,
      TPM2B_PRIVATE* out_private,
      TPM2B_PUBLIC* out_public,
      TPM2B_CREATION_DATA* creation_data,
      TPM2B_DIGEST* creation_hash,
      TPMT_TK_CREATION* creation_ticket,
      AuthorizationDelegate* authorization_delegate);
  virtual void Create(const TPMI_DH_OBJECT& parent_handle,
                      const std::string& parent_handle_name,
                      const TPM2B_SENSITIVE_CREATE& in_sensitive,
                      const TPM2B_PUBLIC& in_public,
                      const TPM2B_DATA& outside_info,
                      const TPML_PCR_SELECTION& creation_pcr,
                      AuthorizationDelegate* authorization_delegate,
                      CreateResponse callback);
  virtual TPM_RC CreateSync(const TPMI_DH_OBJECT& parent_handle,
                            const std::string& parent_handle_name,
                            const TPM2B_SENSITIVE_CREATE& in_sensitive,
                            const TPM2B_PUBLIC& in_public,
                            const TPM2B_DATA& outside_info,
                            const TPML_PCR_SELECTION& creation_pcr,
                            TPM2B_PRIVATE* out_private,
                            TPM2B_PUBLIC* out_public,
                            TPM2B_CREATION_DATA* creation_data,
                            TPM2B_DIGEST* creation_hash,
                            TPMT_TK_CREATION* creation_ticket,
                            AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM_HANDLE& object_handle,
                                  const TPM2B_NAME& name)>
      LoadResponse;
  static TPM_RC SerializeCommand_Load(
      const TPMI_DH_OBJECT& parent_handle,
      const std::string& parent_handle_name,
      const TPM2B_PRIVATE& in_private,
      const TPM2B_PUBLIC& in_public,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_Load(
      const std::string& response,
      TPM_HANDLE* object_handle,
      TPM2B_NAME* name,
      AuthorizationDelegate* authorization_delegate);
  virtual void Load(const TPMI_DH_OBJECT& parent_handle,
                    const std::string& parent_handle_name,
                    const TPM2B_PRIVATE& in_private,
                    const TPM2B_PUBLIC& in_public,
                    AuthorizationDelegate* authorization_delegate,
                    LoadResponse callback);
  virtual TPM_RC LoadSync(const TPMI_DH_OBJECT& parent_handle,
                          const std::string& parent_handle_name,
                          const TPM2B_PRIVATE& in_private,
                          const TPM2B_PUBLIC& in_public,
                          TPM_HANDLE* object_handle,
                          TPM2B_NAME* name,
                          AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM_HANDLE& object_handle,
                                  const TPM2B_NAME& name)>
      LoadExternalResponse;
  static TPM_RC SerializeCommand_LoadExternal(
      const TPM2B_SENSITIVE& in_private,
      const TPM2B_PUBLIC& in_public,
      const TPMI_RH_HIERARCHY& hierarchy,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_LoadExternal(
      const std::string& response,
      TPM_HANDLE* object_handle,
      TPM2B_NAME* name,
      AuthorizationDelegate* authorization_delegate);
  virtual void LoadExternal(const TPM2B_SENSITIVE& in_private,
                            const TPM2B_PUBLIC& in_public,
                            const TPMI_RH_HIERARCHY& hierarchy,
                            AuthorizationDelegate* authorization_delegate,
                            LoadExternalResponse callback);
  virtual TPM_RC LoadExternalSync(
      const TPM2B_SENSITIVE& in_private,
      const TPM2B_PUBLIC& in_public,
      const TPMI_RH_HIERARCHY& hierarchy,
      TPM_HANDLE* object_handle,
      TPM2B_NAME* name,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM2B_PUBLIC& out_public,
                                  const TPM2B_NAME& name,
                                  const TPM2B_NAME& qualified_name)>
      ReadPublicResponse;
  static TPM_RC SerializeCommand_ReadPublic(
      const TPMI_DH_OBJECT& object_handle,
      const std::string& object_handle_name,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_ReadPublic(
      const std::string& response,
      TPM2B_PUBLIC* out_public,
      TPM2B_NAME* name,
      TPM2B_NAME* qualified_name,
      AuthorizationDelegate* authorization_delegate);
  virtual void ReadPublic(const TPMI_DH_OBJECT& object_handle,
                          const std::string& object_handle_name,
                          AuthorizationDelegate* authorization_delegate,
                          ReadPublicResponse callback);
  virtual TPM_RC ReadPublicSync(const TPMI_DH_OBJECT& object_handle,
                                const std::string& object_handle_name,
                                TPM2B_PUBLIC* out_public,
                                TPM2B_NAME* name,
                                TPM2B_NAME* qualified_name,
                                AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM2B_DIGEST& cert_info)>
      ActivateCredentialResponse;
  static TPM_RC SerializeCommand_ActivateCredential(
      const TPMI_DH_OBJECT& activate_handle,
      const std::string& activate_handle_name,
      const TPMI_DH_OBJECT& key_handle,
      const std::string& key_handle_name,
      const TPM2B_ID_OBJECT& credential_blob,
      const TPM2B_ENCRYPTED_SECRET& secret,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_ActivateCredential(
      const std::string& response,
      TPM2B_DIGEST* cert_info,
      AuthorizationDelegate* authorization_delegate);
  virtual void ActivateCredential(const TPMI_DH_OBJECT& activate_handle,
                                  const std::string& activate_handle_name,
                                  const TPMI_DH_OBJECT& key_handle,
                                  const std::string& key_handle_name,
                                  const TPM2B_ID_OBJECT& credential_blob,
                                  const TPM2B_ENCRYPTED_SECRET& secret,
                                  AuthorizationDelegate* authorization_delegate,
                                  ActivateCredentialResponse callback);
  virtual TPM_RC ActivateCredentialSync(
      const TPMI_DH_OBJECT& activate_handle,
      const std::string& activate_handle_name,
      const TPMI_DH_OBJECT& key_handle,
      const std::string& key_handle_name,
      const TPM2B_ID_OBJECT& credential_blob,
      const TPM2B_ENCRYPTED_SECRET& secret,
      TPM2B_DIGEST* cert_info,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM2B_ID_OBJECT& credential_blob,
                                  const TPM2B_ENCRYPTED_SECRET& secret)>
      MakeCredentialResponse;
  static TPM_RC SerializeCommand_MakeCredential(
      const TPMI_DH_OBJECT& handle,
      const std::string& handle_name,
      const TPM2B_DIGEST& credential,
      const TPM2B_NAME& object_name,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_MakeCredential(
      const std::string& response,
      TPM2B_ID_OBJECT* credential_blob,
      TPM2B_ENCRYPTED_SECRET* secret,
      AuthorizationDelegate* authorization_delegate);
  virtual void MakeCredential(const TPMI_DH_OBJECT& handle,
                              const std::string& handle_name,
                              const TPM2B_DIGEST& credential,
                              const TPM2B_NAME& object_name,
                              AuthorizationDelegate* authorization_delegate,
                              MakeCredentialResponse callback);
  virtual TPM_RC MakeCredentialSync(
      const TPMI_DH_OBJECT& handle,
      const std::string& handle_name,
      const TPM2B_DIGEST& credential,
      const TPM2B_NAME& object_name,
      TPM2B_ID_OBJECT* credential_blob,
      TPM2B_ENCRYPTED_SECRET* secret,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM2B_SENSITIVE_DATA& out_data)>
      UnsealResponse;
  static TPM_RC SerializeCommand_Unseal(
      const TPMI_DH_OBJECT& item_handle,
      const std::string& item_handle_name,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_Unseal(
      const std::string& response,
      TPM2B_SENSITIVE_DATA* out_data,
      AuthorizationDelegate* authorization_delegate);
  virtual void Unseal(const TPMI_DH_OBJECT& item_handle,
                      const std::string& item_handle_name,
                      AuthorizationDelegate* authorization_delegate,
                      UnsealResponse callback);
  virtual TPM_RC UnsealSync(const TPMI_DH_OBJECT& item_handle,
                            const std::string& item_handle_name,
                            TPM2B_SENSITIVE_DATA* out_data,
                            AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM2B_PRIVATE& out_private)>
      ObjectChangeAuthResponse;
  static TPM_RC SerializeCommand_ObjectChangeAuth(
      const TPMI_DH_OBJECT& object_handle,
      const std::string& object_handle_name,
      const TPMI_DH_OBJECT& parent_handle,
      const std::string& parent_handle_name,
      const TPM2B_AUTH& new_auth,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_ObjectChangeAuth(
      const std::string& response,
      TPM2B_PRIVATE* out_private,
      AuthorizationDelegate* authorization_delegate);
  virtual void ObjectChangeAuth(const TPMI_DH_OBJECT& object_handle,
                                const std::string& object_handle_name,
                                const TPMI_DH_OBJECT& parent_handle,
                                const std::string& parent_handle_name,
                                const TPM2B_AUTH& new_auth,
                                AuthorizationDelegate* authorization_delegate,
                                ObjectChangeAuthResponse callback);
  virtual TPM_RC ObjectChangeAuthSync(
      const TPMI_DH_OBJECT& object_handle,
      const std::string& object_handle_name,
      const TPMI_DH_OBJECT& parent_handle,
      const std::string& parent_handle_name,
      const TPM2B_AUTH& new_auth,
      TPM2B_PRIVATE* out_private,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM2B_DATA& encryption_key_out,
                                  const TPM2B_PRIVATE& duplicate,
                                  const TPM2B_ENCRYPTED_SECRET& out_sym_seed)>
      DuplicateResponse;
  static TPM_RC SerializeCommand_Duplicate(
      const TPMI_DH_OBJECT& object_handle,
      const std::string& object_handle_name,
      const TPMI_DH_OBJECT& new_parent_handle,
      const std::string& new_parent_handle_name,
      const TPM2B_DATA& encryption_key_in,
      const TPMT_SYM_DEF_OBJECT& symmetric_alg,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_Duplicate(
      const std::string& response,
      TPM2B_DATA* encryption_key_out,
      TPM2B_PRIVATE* duplicate,
      TPM2B_ENCRYPTED_SECRET* out_sym_seed,
      AuthorizationDelegate* authorization_delegate);
  virtual void Duplicate(const TPMI_DH_OBJECT& object_handle,
                         const std::string& object_handle_name,
                         const TPMI_DH_OBJECT& new_parent_handle,
                         const std::string& new_parent_handle_name,
                         const TPM2B_DATA& encryption_key_in,
                         const TPMT_SYM_DEF_OBJECT& symmetric_alg,
                         AuthorizationDelegate* authorization_delegate,
                         DuplicateResponse callback);
  virtual TPM_RC DuplicateSync(const TPMI_DH_OBJECT& object_handle,
                               const std::string& object_handle_name,
                               const TPMI_DH_OBJECT& new_parent_handle,
                               const std::string& new_parent_handle_name,
                               const TPM2B_DATA& encryption_key_in,
                               const TPMT_SYM_DEF_OBJECT& symmetric_alg,
                               TPM2B_DATA* encryption_key_out,
                               TPM2B_PRIVATE* duplicate,
                               TPM2B_ENCRYPTED_SECRET* out_sym_seed,
                               AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM2B_PRIVATE& out_duplicate,
                                  const TPM2B_ENCRYPTED_SECRET& out_sym_seed)>
      RewrapResponse;
  static TPM_RC SerializeCommand_Rewrap(
      const TPMI_DH_OBJECT& old_parent,
      const std::string& old_parent_name,
      const TPMI_DH_OBJECT& new_parent,
      const std::string& new_parent_name,
      const TPM2B_PRIVATE& in_duplicate,
      const TPM2B_NAME& name,
      const TPM2B_ENCRYPTED_SECRET& in_sym_seed,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_Rewrap(
      const std::string& response,
      TPM2B_PRIVATE* out_duplicate,
      TPM2B_ENCRYPTED_SECRET* out_sym_seed,
      AuthorizationDelegate* authorization_delegate);
  virtual void Rewrap(const TPMI_DH_OBJECT& old_parent,
                      const std::string& old_parent_name,
                      const TPMI_DH_OBJECT& new_parent,
                      const std::string& new_parent_name,
                      const TPM2B_PRIVATE& in_duplicate,
                      const TPM2B_NAME& name,
                      const TPM2B_ENCRYPTED_SECRET& in_sym_seed,
                      AuthorizationDelegate* authorization_delegate,
                      RewrapResponse callback);
  virtual TPM_RC RewrapSync(const TPMI_DH_OBJECT& old_parent,
                            const std::string& old_parent_name,
                            const TPMI_DH_OBJECT& new_parent,
                            const std::string& new_parent_name,
                            const TPM2B_PRIVATE& in_duplicate,
                            const TPM2B_NAME& name,
                            const TPM2B_ENCRYPTED_SECRET& in_sym_seed,
                            TPM2B_PRIVATE* out_duplicate,
                            TPM2B_ENCRYPTED_SECRET* out_sym_seed,
                            AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM2B_PRIVATE& out_private)>
      ImportResponse;
  static TPM_RC SerializeCommand_Import(
      const TPMI_DH_OBJECT& parent_handle,
      const std::string& parent_handle_name,
      const TPM2B_DATA& encryption_key,
      const TPM2B_PUBLIC& object_public,
      const TPM2B_PRIVATE& duplicate,
      const TPM2B_ENCRYPTED_SECRET& in_sym_seed,
      const TPMT_SYM_DEF_OBJECT& symmetric_alg,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_Import(
      const std::string& response,
      TPM2B_PRIVATE* out_private,
      AuthorizationDelegate* authorization_delegate);
  virtual void Import(const TPMI_DH_OBJECT& parent_handle,
                      const std::string& parent_handle_name,
                      const TPM2B_DATA& encryption_key,
                      const TPM2B_PUBLIC& object_public,
                      const TPM2B_PRIVATE& duplicate,
                      const TPM2B_ENCRYPTED_SECRET& in_sym_seed,
                      const TPMT_SYM_DEF_OBJECT& symmetric_alg,
                      AuthorizationDelegate* authorization_delegate,
                      ImportResponse callback);
  virtual TPM_RC ImportSync(const TPMI_DH_OBJECT& parent_handle,
                            const std::string& parent_handle_name,
                            const TPM2B_DATA& encryption_key,
                            const TPM2B_PUBLIC& object_public,
                            const TPM2B_PRIVATE& duplicate,
                            const TPM2B_ENCRYPTED_SECRET& in_sym_seed,
                            const TPMT_SYM_DEF_OBJECT& symmetric_alg,
                            TPM2B_PRIVATE* out_private,
                            AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM2B_PUBLIC_KEY_RSA& out_data)>
      RSA_EncryptResponse;
  static TPM_RC SerializeCommand_RSA_Encrypt(
      const TPMI_DH_OBJECT& key_handle,
      const std::string& key_handle_name,
      const TPM2B_PUBLIC_KEY_RSA& message,
      const TPMT_RSA_DECRYPT& in_scheme,
      const TPM2B_DATA& label,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_RSA_Encrypt(
      const std::string& response,
      TPM2B_PUBLIC_KEY_RSA* out_data,
      AuthorizationDelegate* authorization_delegate);
  virtual void RSA_Encrypt(const TPMI_DH_OBJECT& key_handle,
                           const std::string& key_handle_name,
                           const TPM2B_PUBLIC_KEY_RSA& message,
                           const TPMT_RSA_DECRYPT& in_scheme,
                           const TPM2B_DATA& label,
                           AuthorizationDelegate* authorization_delegate,
                           RSA_EncryptResponse callback);
  virtual TPM_RC RSA_EncryptSync(const TPMI_DH_OBJECT& key_handle,
                                 const std::string& key_handle_name,
                                 const TPM2B_PUBLIC_KEY_RSA& message,
                                 const TPMT_RSA_DECRYPT& in_scheme,
                                 const TPM2B_DATA& label,
                                 TPM2B_PUBLIC_KEY_RSA* out_data,
                                 AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM2B_PUBLIC_KEY_RSA& message)>
      RSA_DecryptResponse;
  static TPM_RC SerializeCommand_RSA_Decrypt(
      const TPMI_DH_OBJECT& key_handle,
      const std::string& key_handle_name,
      const TPM2B_PUBLIC_KEY_RSA& cipher_text,
      const TPMT_RSA_DECRYPT& in_scheme,
      const TPM2B_DATA& label,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_RSA_Decrypt(
      const std::string& response,
      TPM2B_PUBLIC_KEY_RSA* message,
      AuthorizationDelegate* authorization_delegate);
  virtual void RSA_Decrypt(const TPMI_DH_OBJECT& key_handle,
                           const std::string& key_handle_name,
                           const TPM2B_PUBLIC_KEY_RSA& cipher_text,
                           const TPMT_RSA_DECRYPT& in_scheme,
                           const TPM2B_DATA& label,
                           AuthorizationDelegate* authorization_delegate,
                           RSA_DecryptResponse callback);
  virtual TPM_RC RSA_DecryptSync(const TPMI_DH_OBJECT& key_handle,
                                 const std::string& key_handle_name,
                                 const TPM2B_PUBLIC_KEY_RSA& cipher_text,
                                 const TPMT_RSA_DECRYPT& in_scheme,
                                 const TPM2B_DATA& label,
                                 TPM2B_PUBLIC_KEY_RSA* message,
                                 AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM2B_ECC_POINT& z_point,
                                  const TPM2B_ECC_POINT& pub_point)>
      ECDH_KeyGenResponse;
  static TPM_RC SerializeCommand_ECDH_KeyGen(
      const TPMI_DH_OBJECT& key_handle,
      const std::string& key_handle_name,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_ECDH_KeyGen(
      const std::string& response,
      TPM2B_ECC_POINT* z_point,
      TPM2B_ECC_POINT* pub_point,
      AuthorizationDelegate* authorization_delegate);
  virtual void ECDH_KeyGen(const TPMI_DH_OBJECT& key_handle,
                           const std::string& key_handle_name,
                           AuthorizationDelegate* authorization_delegate,
                           ECDH_KeyGenResponse callback);
  virtual TPM_RC ECDH_KeyGenSync(const TPMI_DH_OBJECT& key_handle,
                                 const std::string& key_handle_name,
                                 TPM2B_ECC_POINT* z_point,
                                 TPM2B_ECC_POINT* pub_point,
                                 AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM2B_ECC_POINT& out_point)>
      ECDH_ZGenResponse;
  static TPM_RC SerializeCommand_ECDH_ZGen(
      const TPMI_DH_OBJECT& key_handle,
      const std::string& key_handle_name,
      const TPM2B_ECC_POINT& in_point,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_ECDH_ZGen(
      const std::string& response,
      TPM2B_ECC_POINT* out_point,
      AuthorizationDelegate* authorization_delegate);
  virtual void ECDH_ZGen(const TPMI_DH_OBJECT& key_handle,
                         const std::string& key_handle_name,
                         const TPM2B_ECC_POINT& in_point,
                         AuthorizationDelegate* authorization_delegate,
                         ECDH_ZGenResponse callback);
  virtual TPM_RC ECDH_ZGenSync(const TPMI_DH_OBJECT& key_handle,
                               const std::string& key_handle_name,
                               const TPM2B_ECC_POINT& in_point,
                               TPM2B_ECC_POINT* out_point,
                               AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPMS_ALGORITHM_DETAIL_ECC& parameters)>
      ECC_ParametersResponse;
  static TPM_RC SerializeCommand_ECC_Parameters(
      const TPMI_ECC_CURVE& curve_id,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_ECC_Parameters(
      const std::string& response,
      TPMS_ALGORITHM_DETAIL_ECC* parameters,
      AuthorizationDelegate* authorization_delegate);
  virtual void ECC_Parameters(const TPMI_ECC_CURVE& curve_id,
                              AuthorizationDelegate* authorization_delegate,
                              ECC_ParametersResponse callback);
  virtual TPM_RC ECC_ParametersSync(
      const TPMI_ECC_CURVE& curve_id,
      TPMS_ALGORITHM_DETAIL_ECC* parameters,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM2B_ECC_POINT& out_z1,
                                  const TPM2B_ECC_POINT& out_z2)>
      ZGen_2PhaseResponse;
  static TPM_RC SerializeCommand_ZGen_2Phase(
      const TPMI_DH_OBJECT& key_a,
      const std::string& key_a_name,
      const TPM2B_ECC_POINT& in_qs_b,
      const TPM2B_ECC_POINT& in_qe_b,
      const TPMI_ECC_KEY_EXCHANGE& in_scheme,
      const UINT16& counter,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_ZGen_2Phase(
      const std::string& response,
      TPM2B_ECC_POINT* out_z1,
      TPM2B_ECC_POINT* out_z2,
      AuthorizationDelegate* authorization_delegate);
  virtual void ZGen_2Phase(const TPMI_DH_OBJECT& key_a,
                           const std::string& key_a_name,
                           const TPM2B_ECC_POINT& in_qs_b,
                           const TPM2B_ECC_POINT& in_qe_b,
                           const TPMI_ECC_KEY_EXCHANGE& in_scheme,
                           const UINT16& counter,
                           AuthorizationDelegate* authorization_delegate,
                           ZGen_2PhaseResponse callback);
  virtual TPM_RC ZGen_2PhaseSync(const TPMI_DH_OBJECT& key_a,
                                 const std::string& key_a_name,
                                 const TPM2B_ECC_POINT& in_qs_b,
                                 const TPM2B_ECC_POINT& in_qe_b,
                                 const TPMI_ECC_KEY_EXCHANGE& in_scheme,
                                 const UINT16& counter,
                                 TPM2B_ECC_POINT* out_z1,
                                 TPM2B_ECC_POINT* out_z2,
                                 AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM2B_MAX_BUFFER& out_data,
                                  const TPM2B_IV& iv_out)>
      EncryptDecryptResponse;
  static TPM_RC SerializeCommand_EncryptDecrypt(
      const TPMI_DH_OBJECT& key_handle,
      const std::string& key_handle_name,
      const TPMI_YES_NO& decrypt,
      const TPMI_ALG_SYM_MODE& mode,
      const TPM2B_IV& iv_in,
      const TPM2B_MAX_BUFFER& in_data,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_EncryptDecrypt(
      const std::string& response,
      TPM2B_MAX_BUFFER* out_data,
      TPM2B_IV* iv_out,
      AuthorizationDelegate* authorization_delegate);
  virtual void EncryptDecrypt(const TPMI_DH_OBJECT& key_handle,
                              const std::string& key_handle_name,
                              const TPMI_YES_NO& decrypt,
                              const TPMI_ALG_SYM_MODE& mode,
                              const TPM2B_IV& iv_in,
                              const TPM2B_MAX_BUFFER& in_data,
                              AuthorizationDelegate* authorization_delegate,
                              EncryptDecryptResponse callback);
  virtual TPM_RC EncryptDecryptSync(
      const TPMI_DH_OBJECT& key_handle,
      const std::string& key_handle_name,
      const TPMI_YES_NO& decrypt,
      const TPMI_ALG_SYM_MODE& mode,
      const TPM2B_IV& iv_in,
      const TPM2B_MAX_BUFFER& in_data,
      TPM2B_MAX_BUFFER* out_data,
      TPM2B_IV* iv_out,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM2B_DIGEST& out_hash,
                                  const TPMT_TK_HASHCHECK& validation)>
      HashResponse;
  static TPM_RC SerializeCommand_Hash(
      const TPM2B_MAX_BUFFER& data,
      const TPMI_ALG_HASH& hash_alg,
      const TPMI_RH_HIERARCHY& hierarchy,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_Hash(
      const std::string& response,
      TPM2B_DIGEST* out_hash,
      TPMT_TK_HASHCHECK* validation,
      AuthorizationDelegate* authorization_delegate);
  virtual void Hash(const TPM2B_MAX_BUFFER& data,
                    const TPMI_ALG_HASH& hash_alg,
                    const TPMI_RH_HIERARCHY& hierarchy,
                    AuthorizationDelegate* authorization_delegate,
                    HashResponse callback);
  virtual TPM_RC HashSync(const TPM2B_MAX_BUFFER& data,
                          const TPMI_ALG_HASH& hash_alg,
                          const TPMI_RH_HIERARCHY& hierarchy,
                          TPM2B_DIGEST* out_hash,
                          TPMT_TK_HASHCHECK* validation,
                          AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM2B_DIGEST& out_hmac)>
      HMACResponse;
  static TPM_RC SerializeCommand_HMAC(
      const TPMI_DH_OBJECT& handle,
      const std::string& handle_name,
      const TPM2B_MAX_BUFFER& buffer,
      const TPMI_ALG_HASH& hash_alg,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_HMAC(
      const std::string& response,
      TPM2B_DIGEST* out_hmac,
      AuthorizationDelegate* authorization_delegate);
  virtual void HMAC(const TPMI_DH_OBJECT& handle,
                    const std::string& handle_name,
                    const TPM2B_MAX_BUFFER& buffer,
                    const TPMI_ALG_HASH& hash_alg,
                    AuthorizationDelegate* authorization_delegate,
                    HMACResponse callback);
  virtual TPM_RC HMACSync(const TPMI_DH_OBJECT& handle,
                          const std::string& handle_name,
                          const TPM2B_MAX_BUFFER& buffer,
                          const TPMI_ALG_HASH& hash_alg,
                          TPM2B_DIGEST* out_hmac,
                          AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM2B_DIGEST& random_bytes)>
      GetRandomResponse;
  static TPM_RC SerializeCommand_GetRandom(
      const UINT16& bytes_requested,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_GetRandom(
      const std::string& response,
      TPM2B_DIGEST* random_bytes,
      AuthorizationDelegate* authorization_delegate);
  virtual void GetRandom(const UINT16& bytes_requested,
                         AuthorizationDelegate* authorization_delegate,
                         GetRandomResponse callback);
  virtual TPM_RC GetRandomSync(const UINT16& bytes_requested,
                               TPM2B_DIGEST* random_bytes,
                               AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)> StirRandomResponse;
  static TPM_RC SerializeCommand_StirRandom(
      const TPM2B_SENSITIVE_DATA& in_data,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_StirRandom(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void StirRandom(const TPM2B_SENSITIVE_DATA& in_data,
                          AuthorizationDelegate* authorization_delegate,
                          StirRandomResponse callback);
  virtual TPM_RC StirRandomSync(const TPM2B_SENSITIVE_DATA& in_data,
                                AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPMI_DH_OBJECT& sequence_handle)>
      HMAC_StartResponse;
  static TPM_RC SerializeCommand_HMAC_Start(
      const TPMI_DH_OBJECT& handle,
      const std::string& handle_name,
      const TPM2B_AUTH& auth,
      const TPMI_ALG_HASH& hash_alg,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_HMAC_Start(
      const std::string& response,
      TPMI_DH_OBJECT* sequence_handle,
      AuthorizationDelegate* authorization_delegate);
  virtual void HMAC_Start(const TPMI_DH_OBJECT& handle,
                          const std::string& handle_name,
                          const TPM2B_AUTH& auth,
                          const TPMI_ALG_HASH& hash_alg,
                          AuthorizationDelegate* authorization_delegate,
                          HMAC_StartResponse callback);
  virtual TPM_RC HMAC_StartSync(const TPMI_DH_OBJECT& handle,
                                const std::string& handle_name,
                                const TPM2B_AUTH& auth,
                                const TPMI_ALG_HASH& hash_alg,
                                TPMI_DH_OBJECT* sequence_handle,
                                AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPMI_DH_OBJECT& sequence_handle)>
      HashSequenceStartResponse;
  static TPM_RC SerializeCommand_HashSequenceStart(
      const TPM2B_AUTH& auth,
      const TPMI_ALG_HASH& hash_alg,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_HashSequenceStart(
      const std::string& response,
      TPMI_DH_OBJECT* sequence_handle,
      AuthorizationDelegate* authorization_delegate);
  virtual void HashSequenceStart(const TPM2B_AUTH& auth,
                                 const TPMI_ALG_HASH& hash_alg,
                                 AuthorizationDelegate* authorization_delegate,
                                 HashSequenceStartResponse callback);
  virtual TPM_RC HashSequenceStartSync(
      const TPM2B_AUTH& auth,
      const TPMI_ALG_HASH& hash_alg,
      TPMI_DH_OBJECT* sequence_handle,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)> SequenceUpdateResponse;
  static TPM_RC SerializeCommand_SequenceUpdate(
      const TPMI_DH_OBJECT& sequence_handle,
      const std::string& sequence_handle_name,
      const TPM2B_MAX_BUFFER& buffer,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_SequenceUpdate(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void SequenceUpdate(const TPMI_DH_OBJECT& sequence_handle,
                              const std::string& sequence_handle_name,
                              const TPM2B_MAX_BUFFER& buffer,
                              AuthorizationDelegate* authorization_delegate,
                              SequenceUpdateResponse callback);
  virtual TPM_RC SequenceUpdateSync(
      const TPMI_DH_OBJECT& sequence_handle,
      const std::string& sequence_handle_name,
      const TPM2B_MAX_BUFFER& buffer,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM2B_DIGEST& result,
                                  const TPMT_TK_HASHCHECK& validation)>
      SequenceCompleteResponse;
  static TPM_RC SerializeCommand_SequenceComplete(
      const TPMI_DH_OBJECT& sequence_handle,
      const std::string& sequence_handle_name,
      const TPM2B_MAX_BUFFER& buffer,
      const TPMI_RH_HIERARCHY& hierarchy,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_SequenceComplete(
      const std::string& response,
      TPM2B_DIGEST* result,
      TPMT_TK_HASHCHECK* validation,
      AuthorizationDelegate* authorization_delegate);
  virtual void SequenceComplete(const TPMI_DH_OBJECT& sequence_handle,
                                const std::string& sequence_handle_name,
                                const TPM2B_MAX_BUFFER& buffer,
                                const TPMI_RH_HIERARCHY& hierarchy,
                                AuthorizationDelegate* authorization_delegate,
                                SequenceCompleteResponse callback);
  virtual TPM_RC SequenceCompleteSync(
      const TPMI_DH_OBJECT& sequence_handle,
      const std::string& sequence_handle_name,
      const TPM2B_MAX_BUFFER& buffer,
      const TPMI_RH_HIERARCHY& hierarchy,
      TPM2B_DIGEST* result,
      TPMT_TK_HASHCHECK* validation,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPML_DIGEST_VALUES& results)>
      EventSequenceCompleteResponse;
  static TPM_RC SerializeCommand_EventSequenceComplete(
      const TPMI_DH_PCR& pcr_handle,
      const std::string& pcr_handle_name,
      const TPMI_DH_OBJECT& sequence_handle,
      const std::string& sequence_handle_name,
      const TPM2B_MAX_BUFFER& buffer,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_EventSequenceComplete(
      const std::string& response,
      TPML_DIGEST_VALUES* results,
      AuthorizationDelegate* authorization_delegate);
  virtual void EventSequenceComplete(
      const TPMI_DH_PCR& pcr_handle,
      const std::string& pcr_handle_name,
      const TPMI_DH_OBJECT& sequence_handle,
      const std::string& sequence_handle_name,
      const TPM2B_MAX_BUFFER& buffer,
      AuthorizationDelegate* authorization_delegate,
      EventSequenceCompleteResponse callback);
  virtual TPM_RC EventSequenceCompleteSync(
      const TPMI_DH_PCR& pcr_handle,
      const std::string& pcr_handle_name,
      const TPMI_DH_OBJECT& sequence_handle,
      const std::string& sequence_handle_name,
      const TPM2B_MAX_BUFFER& buffer,
      TPML_DIGEST_VALUES* results,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM2B_ATTEST& certify_info,
                                  const TPMT_SIGNATURE& signature)>
      CertifyResponse;
  static TPM_RC SerializeCommand_Certify(
      const TPMI_DH_OBJECT& object_handle,
      const std::string& object_handle_name,
      const TPMI_DH_OBJECT& sign_handle,
      const std::string& sign_handle_name,
      const TPM2B_DATA& qualifying_data,
      const TPMT_SIG_SCHEME& in_scheme,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_Certify(
      const std::string& response,
      TPM2B_ATTEST* certify_info,
      TPMT_SIGNATURE* signature,
      AuthorizationDelegate* authorization_delegate);
  virtual void Certify(const TPMI_DH_OBJECT& object_handle,
                       const std::string& object_handle_name,
                       const TPMI_DH_OBJECT& sign_handle,
                       const std::string& sign_handle_name,
                       const TPM2B_DATA& qualifying_data,
                       const TPMT_SIG_SCHEME& in_scheme,
                       AuthorizationDelegate* authorization_delegate,
                       CertifyResponse callback);
  virtual TPM_RC CertifySync(const TPMI_DH_OBJECT& object_handle,
                             const std::string& object_handle_name,
                             const TPMI_DH_OBJECT& sign_handle,
                             const std::string& sign_handle_name,
                             const TPM2B_DATA& qualifying_data,
                             const TPMT_SIG_SCHEME& in_scheme,
                             TPM2B_ATTEST* certify_info,
                             TPMT_SIGNATURE* signature,
                             AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM2B_ATTEST& certify_info,
                                  const TPMT_SIGNATURE& signature)>
      CertifyCreationResponse;
  static TPM_RC SerializeCommand_CertifyCreation(
      const TPMI_DH_OBJECT& sign_handle,
      const std::string& sign_handle_name,
      const TPMI_DH_OBJECT& object_handle,
      const std::string& object_handle_name,
      const TPM2B_DATA& qualifying_data,
      const TPM2B_DIGEST& creation_hash,
      const TPMT_SIG_SCHEME& in_scheme,
      const TPMT_TK_CREATION& creation_ticket,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_CertifyCreation(
      const std::string& response,
      TPM2B_ATTEST* certify_info,
      TPMT_SIGNATURE* signature,
      AuthorizationDelegate* authorization_delegate);
  virtual void CertifyCreation(const TPMI_DH_OBJECT& sign_handle,
                               const std::string& sign_handle_name,
                               const TPMI_DH_OBJECT& object_handle,
                               const std::string& object_handle_name,
                               const TPM2B_DATA& qualifying_data,
                               const TPM2B_DIGEST& creation_hash,
                               const TPMT_SIG_SCHEME& in_scheme,
                               const TPMT_TK_CREATION& creation_ticket,
                               AuthorizationDelegate* authorization_delegate,
                               CertifyCreationResponse callback);
  virtual TPM_RC CertifyCreationSync(
      const TPMI_DH_OBJECT& sign_handle,
      const std::string& sign_handle_name,
      const TPMI_DH_OBJECT& object_handle,
      const std::string& object_handle_name,
      const TPM2B_DATA& qualifying_data,
      const TPM2B_DIGEST& creation_hash,
      const TPMT_SIG_SCHEME& in_scheme,
      const TPMT_TK_CREATION& creation_ticket,
      TPM2B_ATTEST* certify_info,
      TPMT_SIGNATURE* signature,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM2B_ATTEST& quoted,
                                  const TPMT_SIGNATURE& signature)>
      QuoteResponse;
  static TPM_RC SerializeCommand_Quote(
      const TPMI_DH_OBJECT& sign_handle,
      const std::string& sign_handle_name,
      const TPM2B_DATA& qualifying_data,
      const TPMT_SIG_SCHEME& in_scheme,
      const TPML_PCR_SELECTION& pcrselect,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_Quote(
      const std::string& response,
      TPM2B_ATTEST* quoted,
      TPMT_SIGNATURE* signature,
      AuthorizationDelegate* authorization_delegate);
  virtual void Quote(const TPMI_DH_OBJECT& sign_handle,
                     const std::string& sign_handle_name,
                     const TPM2B_DATA& qualifying_data,
                     const TPMT_SIG_SCHEME& in_scheme,
                     const TPML_PCR_SELECTION& pcrselect,
                     AuthorizationDelegate* authorization_delegate,
                     QuoteResponse callback);
  virtual TPM_RC QuoteSync(const TPMI_DH_OBJECT& sign_handle,
                           const std::string& sign_handle_name,
                           const TPM2B_DATA& qualifying_data,
                           const TPMT_SIG_SCHEME& in_scheme,
                           const TPML_PCR_SELECTION& pcrselect,
                           TPM2B_ATTEST* quoted,
                           TPMT_SIGNATURE* signature,
                           AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM2B_ATTEST& audit_info,
                                  const TPMT_SIGNATURE& signature)>
      GetSessionAuditDigestResponse;
  static TPM_RC SerializeCommand_GetSessionAuditDigest(
      const TPMI_RH_ENDORSEMENT& privacy_admin_handle,
      const std::string& privacy_admin_handle_name,
      const TPMI_DH_OBJECT& sign_handle,
      const std::string& sign_handle_name,
      const TPMI_SH_HMAC& session_handle,
      const std::string& session_handle_name,
      const TPM2B_DATA& qualifying_data,
      const TPMT_SIG_SCHEME& in_scheme,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_GetSessionAuditDigest(
      const std::string& response,
      TPM2B_ATTEST* audit_info,
      TPMT_SIGNATURE* signature,
      AuthorizationDelegate* authorization_delegate);
  virtual void GetSessionAuditDigest(
      const TPMI_RH_ENDORSEMENT& privacy_admin_handle,
      const std::string& privacy_admin_handle_name,
      const TPMI_DH_OBJECT& sign_handle,
      const std::string& sign_handle_name,
      const TPMI_SH_HMAC& session_handle,
      const std::string& session_handle_name,
      const TPM2B_DATA& qualifying_data,
      const TPMT_SIG_SCHEME& in_scheme,
      AuthorizationDelegate* authorization_delegate,
      GetSessionAuditDigestResponse callback);
  virtual TPM_RC GetSessionAuditDigestSync(
      const TPMI_RH_ENDORSEMENT& privacy_admin_handle,
      const std::string& privacy_admin_handle_name,
      const TPMI_DH_OBJECT& sign_handle,
      const std::string& sign_handle_name,
      const TPMI_SH_HMAC& session_handle,
      const std::string& session_handle_name,
      const TPM2B_DATA& qualifying_data,
      const TPMT_SIG_SCHEME& in_scheme,
      TPM2B_ATTEST* audit_info,
      TPMT_SIGNATURE* signature,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM2B_ATTEST& audit_info,
                                  const TPMT_SIGNATURE& signature)>
      GetCommandAuditDigestResponse;
  static TPM_RC SerializeCommand_GetCommandAuditDigest(
      const TPMI_RH_ENDORSEMENT& privacy_handle,
      const std::string& privacy_handle_name,
      const TPMI_DH_OBJECT& sign_handle,
      const std::string& sign_handle_name,
      const TPM2B_DATA& qualifying_data,
      const TPMT_SIG_SCHEME& in_scheme,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_GetCommandAuditDigest(
      const std::string& response,
      TPM2B_ATTEST* audit_info,
      TPMT_SIGNATURE* signature,
      AuthorizationDelegate* authorization_delegate);
  virtual void GetCommandAuditDigest(
      const TPMI_RH_ENDORSEMENT& privacy_handle,
      const std::string& privacy_handle_name,
      const TPMI_DH_OBJECT& sign_handle,
      const std::string& sign_handle_name,
      const TPM2B_DATA& qualifying_data,
      const TPMT_SIG_SCHEME& in_scheme,
      AuthorizationDelegate* authorization_delegate,
      GetCommandAuditDigestResponse callback);
  virtual TPM_RC GetCommandAuditDigestSync(
      const TPMI_RH_ENDORSEMENT& privacy_handle,
      const std::string& privacy_handle_name,
      const TPMI_DH_OBJECT& sign_handle,
      const std::string& sign_handle_name,
      const TPM2B_DATA& qualifying_data,
      const TPMT_SIG_SCHEME& in_scheme,
      TPM2B_ATTEST* audit_info,
      TPMT_SIGNATURE* signature,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM2B_ATTEST& time_info,
                                  const TPMT_SIGNATURE& signature)>
      GetTimeResponse;
  static TPM_RC SerializeCommand_GetTime(
      const TPMI_RH_ENDORSEMENT& privacy_admin_handle,
      const std::string& privacy_admin_handle_name,
      const TPMI_DH_OBJECT& sign_handle,
      const std::string& sign_handle_name,
      const TPM2B_DATA& qualifying_data,
      const TPMT_SIG_SCHEME& in_scheme,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_GetTime(
      const std::string& response,
      TPM2B_ATTEST* time_info,
      TPMT_SIGNATURE* signature,
      AuthorizationDelegate* authorization_delegate);
  virtual void GetTime(const TPMI_RH_ENDORSEMENT& privacy_admin_handle,
                       const std::string& privacy_admin_handle_name,
                       const TPMI_DH_OBJECT& sign_handle,
                       const std::string& sign_handle_name,
                       const TPM2B_DATA& qualifying_data,
                       const TPMT_SIG_SCHEME& in_scheme,
                       AuthorizationDelegate* authorization_delegate,
                       GetTimeResponse callback);
  virtual TPM_RC GetTimeSync(const TPMI_RH_ENDORSEMENT& privacy_admin_handle,
                             const std::string& privacy_admin_handle_name,
                             const TPMI_DH_OBJECT& sign_handle,
                             const std::string& sign_handle_name,
                             const TPM2B_DATA& qualifying_data,
                             const TPMT_SIG_SCHEME& in_scheme,
                             TPM2B_ATTEST* time_info,
                             TPMT_SIGNATURE* signature,
                             AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const UINT32& param_size_out,
                                  const TPM2B_ECC_POINT& k,
                                  const TPM2B_ECC_POINT& l,
                                  const TPM2B_ECC_POINT& e,
                                  const UINT16& counter)>
      CommitResponse;
  static TPM_RC SerializeCommand_Commit(
      const TPMI_DH_OBJECT& sign_handle,
      const std::string& sign_handle_name,
      const UINT32& param_size,
      const TPM2B_ECC_POINT& p1,
      const TPM2B_SENSITIVE_DATA& s2,
      const TPM2B_ECC_PARAMETER& y2,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_Commit(
      const std::string& response,
      UINT32* param_size_out,
      TPM2B_ECC_POINT* k,
      TPM2B_ECC_POINT* l,
      TPM2B_ECC_POINT* e,
      UINT16* counter,
      AuthorizationDelegate* authorization_delegate);
  virtual void Commit(const TPMI_DH_OBJECT& sign_handle,
                      const std::string& sign_handle_name,
                      const UINT32& param_size,
                      const TPM2B_ECC_POINT& p1,
                      const TPM2B_SENSITIVE_DATA& s2,
                      const TPM2B_ECC_PARAMETER& y2,
                      AuthorizationDelegate* authorization_delegate,
                      CommitResponse callback);
  virtual TPM_RC CommitSync(const TPMI_DH_OBJECT& sign_handle,
                            const std::string& sign_handle_name,
                            const UINT32& param_size,
                            const TPM2B_ECC_POINT& p1,
                            const TPM2B_SENSITIVE_DATA& s2,
                            const TPM2B_ECC_PARAMETER& y2,
                            UINT32* param_size_out,
                            TPM2B_ECC_POINT* k,
                            TPM2B_ECC_POINT* l,
                            TPM2B_ECC_POINT* e,
                            UINT16* counter,
                            AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const UINT32& param_size_out,
                                  const TPM2B_ECC_POINT& q,
                                  const UINT16& counter)>
      EC_EphemeralResponse;
  static TPM_RC SerializeCommand_EC_Ephemeral(
      const UINT32& param_size,
      const TPMI_ECC_CURVE& curve_id,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_EC_Ephemeral(
      const std::string& response,
      UINT32* param_size_out,
      TPM2B_ECC_POINT* q,
      UINT16* counter,
      AuthorizationDelegate* authorization_delegate);
  virtual void EC_Ephemeral(const UINT32& param_size,
                            const TPMI_ECC_CURVE& curve_id,
                            AuthorizationDelegate* authorization_delegate,
                            EC_EphemeralResponse callback);
  virtual TPM_RC EC_EphemeralSync(
      const UINT32& param_size,
      const TPMI_ECC_CURVE& curve_id,
      UINT32* param_size_out,
      TPM2B_ECC_POINT* q,
      UINT16* counter,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPMT_TK_VERIFIED& validation)>
      VerifySignatureResponse;
  static TPM_RC SerializeCommand_VerifySignature(
      const TPMI_DH_OBJECT& key_handle,
      const std::string& key_handle_name,
      const TPM2B_DIGEST& digest,
      const TPMT_SIGNATURE& signature,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_VerifySignature(
      const std::string& response,
      TPMT_TK_VERIFIED* validation,
      AuthorizationDelegate* authorization_delegate);
  virtual void VerifySignature(const TPMI_DH_OBJECT& key_handle,
                               const std::string& key_handle_name,
                               const TPM2B_DIGEST& digest,
                               const TPMT_SIGNATURE& signature,
                               AuthorizationDelegate* authorization_delegate,
                               VerifySignatureResponse callback);
  virtual TPM_RC VerifySignatureSync(
      const TPMI_DH_OBJECT& key_handle,
      const std::string& key_handle_name,
      const TPM2B_DIGEST& digest,
      const TPMT_SIGNATURE& signature,
      TPMT_TK_VERIFIED* validation,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPMT_SIGNATURE& signature)>
      SignResponse;
  static TPM_RC SerializeCommand_Sign(
      const TPMI_DH_OBJECT& key_handle,
      const std::string& key_handle_name,
      const TPM2B_DIGEST& digest,
      const TPMT_SIG_SCHEME& in_scheme,
      const TPMT_TK_HASHCHECK& validation,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_Sign(
      const std::string& response,
      TPMT_SIGNATURE* signature,
      AuthorizationDelegate* authorization_delegate);
  virtual void Sign(const TPMI_DH_OBJECT& key_handle,
                    const std::string& key_handle_name,
                    const TPM2B_DIGEST& digest,
                    const TPMT_SIG_SCHEME& in_scheme,
                    const TPMT_TK_HASHCHECK& validation,
                    AuthorizationDelegate* authorization_delegate,
                    SignResponse callback);
  virtual TPM_RC SignSync(const TPMI_DH_OBJECT& key_handle,
                          const std::string& key_handle_name,
                          const TPM2B_DIGEST& digest,
                          const TPMT_SIG_SCHEME& in_scheme,
                          const TPMT_TK_HASHCHECK& validation,
                          TPMT_SIGNATURE* signature,
                          AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)>
      SetCommandCodeAuditStatusResponse;
  static TPM_RC SerializeCommand_SetCommandCodeAuditStatus(
      const TPMI_RH_PROVISION& auth,
      const std::string& auth_name,
      const TPMI_ALG_HASH& audit_alg,
      const TPML_CC& set_list,
      const TPML_CC& clear_list,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_SetCommandCodeAuditStatus(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void SetCommandCodeAuditStatus(
      const TPMI_RH_PROVISION& auth,
      const std::string& auth_name,
      const TPMI_ALG_HASH& audit_alg,
      const TPML_CC& set_list,
      const TPML_CC& clear_list,
      AuthorizationDelegate* authorization_delegate,
      SetCommandCodeAuditStatusResponse callback);
  virtual TPM_RC SetCommandCodeAuditStatusSync(
      const TPMI_RH_PROVISION& auth,
      const std::string& auth_name,
      const TPMI_ALG_HASH& audit_alg,
      const TPML_CC& set_list,
      const TPML_CC& clear_list,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)> PCR_ExtendResponse;
  static TPM_RC SerializeCommand_PCR_Extend(
      const TPMI_DH_PCR& pcr_handle,
      const std::string& pcr_handle_name,
      const TPML_DIGEST_VALUES& digests,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_PCR_Extend(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void PCR_Extend(const TPMI_DH_PCR& pcr_handle,
                          const std::string& pcr_handle_name,
                          const TPML_DIGEST_VALUES& digests,
                          AuthorizationDelegate* authorization_delegate,
                          PCR_ExtendResponse callback);
  virtual TPM_RC PCR_ExtendSync(const TPMI_DH_PCR& pcr_handle,
                                const std::string& pcr_handle_name,
                                const TPML_DIGEST_VALUES& digests,
                                AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPML_DIGEST_VALUES& digests)>
      PCR_EventResponse;
  static TPM_RC SerializeCommand_PCR_Event(
      const TPMI_DH_PCR& pcr_handle,
      const std::string& pcr_handle_name,
      const TPM2B_EVENT& event_data,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_PCR_Event(
      const std::string& response,
      TPML_DIGEST_VALUES* digests,
      AuthorizationDelegate* authorization_delegate);
  virtual void PCR_Event(const TPMI_DH_PCR& pcr_handle,
                         const std::string& pcr_handle_name,
                         const TPM2B_EVENT& event_data,
                         AuthorizationDelegate* authorization_delegate,
                         PCR_EventResponse callback);
  virtual TPM_RC PCR_EventSync(const TPMI_DH_PCR& pcr_handle,
                               const std::string& pcr_handle_name,
                               const TPM2B_EVENT& event_data,
                               TPML_DIGEST_VALUES* digests,
                               AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const UINT32& pcr_update_counter,
                                  const TPML_PCR_SELECTION& pcr_selection_out,
                                  const TPML_DIGEST& pcr_values)>
      PCR_ReadResponse;
  static TPM_RC SerializeCommand_PCR_Read(
      const TPML_PCR_SELECTION& pcr_selection_in,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_PCR_Read(
      const std::string& response,
      UINT32* pcr_update_counter,
      TPML_PCR_SELECTION* pcr_selection_out,
      TPML_DIGEST* pcr_values,
      AuthorizationDelegate* authorization_delegate);
  virtual void PCR_Read(const TPML_PCR_SELECTION& pcr_selection_in,
                        AuthorizationDelegate* authorization_delegate,
                        PCR_ReadResponse callback);
  virtual TPM_RC PCR_ReadSync(const TPML_PCR_SELECTION& pcr_selection_in,
                              UINT32* pcr_update_counter,
                              TPML_PCR_SELECTION* pcr_selection_out,
                              TPML_DIGEST* pcr_values,
                              AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPMI_YES_NO& allocation_success,
                                  const UINT32& max_pcr,
                                  const UINT32& size_needed,
                                  const UINT32& size_available)>
      PCR_AllocateResponse;
  static TPM_RC SerializeCommand_PCR_Allocate(
      const TPMI_RH_PLATFORM& auth_handle,
      const std::string& auth_handle_name,
      const TPML_PCR_SELECTION& pcr_allocation,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_PCR_Allocate(
      const std::string& response,
      TPMI_YES_NO* allocation_success,
      UINT32* max_pcr,
      UINT32* size_needed,
      UINT32* size_available,
      AuthorizationDelegate* authorization_delegate);
  virtual void PCR_Allocate(const TPMI_RH_PLATFORM& auth_handle,
                            const std::string& auth_handle_name,
                            const TPML_PCR_SELECTION& pcr_allocation,
                            AuthorizationDelegate* authorization_delegate,
                            PCR_AllocateResponse callback);
  virtual TPM_RC PCR_AllocateSync(
      const TPMI_RH_PLATFORM& auth_handle,
      const std::string& auth_handle_name,
      const TPML_PCR_SELECTION& pcr_allocation,
      TPMI_YES_NO* allocation_success,
      UINT32* max_pcr,
      UINT32* size_needed,
      UINT32* size_available,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)>
      PCR_SetAuthPolicyResponse;
  static TPM_RC SerializeCommand_PCR_SetAuthPolicy(
      const TPMI_RH_PLATFORM& auth_handle,
      const std::string& auth_handle_name,
      const TPMI_DH_PCR& pcr_num,
      const std::string& pcr_num_name,
      const TPM2B_DIGEST& auth_policy,
      const TPMI_ALG_HASH& policy_digest,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_PCR_SetAuthPolicy(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void PCR_SetAuthPolicy(const TPMI_RH_PLATFORM& auth_handle,
                                 const std::string& auth_handle_name,
                                 const TPMI_DH_PCR& pcr_num,
                                 const std::string& pcr_num_name,
                                 const TPM2B_DIGEST& auth_policy,
                                 const TPMI_ALG_HASH& policy_digest,
                                 AuthorizationDelegate* authorization_delegate,
                                 PCR_SetAuthPolicyResponse callback);
  virtual TPM_RC PCR_SetAuthPolicySync(
      const TPMI_RH_PLATFORM& auth_handle,
      const std::string& auth_handle_name,
      const TPMI_DH_PCR& pcr_num,
      const std::string& pcr_num_name,
      const TPM2B_DIGEST& auth_policy,
      const TPMI_ALG_HASH& policy_digest,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)>
      PCR_SetAuthValueResponse;
  static TPM_RC SerializeCommand_PCR_SetAuthValue(
      const TPMI_DH_PCR& pcr_handle,
      const std::string& pcr_handle_name,
      const TPM2B_DIGEST& auth,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_PCR_SetAuthValue(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void PCR_SetAuthValue(const TPMI_DH_PCR& pcr_handle,
                                const std::string& pcr_handle_name,
                                const TPM2B_DIGEST& auth,
                                AuthorizationDelegate* authorization_delegate,
                                PCR_SetAuthValueResponse callback);
  virtual TPM_RC PCR_SetAuthValueSync(
      const TPMI_DH_PCR& pcr_handle,
      const std::string& pcr_handle_name,
      const TPM2B_DIGEST& auth,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)> PCR_ResetResponse;
  static TPM_RC SerializeCommand_PCR_Reset(
      const TPMI_DH_PCR& pcr_handle,
      const std::string& pcr_handle_name,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_PCR_Reset(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void PCR_Reset(const TPMI_DH_PCR& pcr_handle,
                         const std::string& pcr_handle_name,
                         AuthorizationDelegate* authorization_delegate,
                         PCR_ResetResponse callback);
  virtual TPM_RC PCR_ResetSync(const TPMI_DH_PCR& pcr_handle,
                               const std::string& pcr_handle_name,
                               AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM2B_TIMEOUT& timeout,
                                  const TPMT_TK_AUTH& policy_ticket)>
      PolicySignedResponse;
  static TPM_RC SerializeCommand_PolicySigned(
      const TPMI_DH_OBJECT& auth_object,
      const std::string& auth_object_name,
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      const TPM2B_NONCE& nonce_tpm,
      const TPM2B_DIGEST& cp_hash_a,
      const TPM2B_NONCE& policy_ref,
      const INT32& expiration,
      const TPMT_SIGNATURE& auth,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_PolicySigned(
      const std::string& response,
      TPM2B_TIMEOUT* timeout,
      TPMT_TK_AUTH* policy_ticket,
      AuthorizationDelegate* authorization_delegate);
  virtual void PolicySigned(const TPMI_DH_OBJECT& auth_object,
                            const std::string& auth_object_name,
                            const TPMI_SH_POLICY& policy_session,
                            const std::string& policy_session_name,
                            const TPM2B_NONCE& nonce_tpm,
                            const TPM2B_DIGEST& cp_hash_a,
                            const TPM2B_NONCE& policy_ref,
                            const INT32& expiration,
                            const TPMT_SIGNATURE& auth,
                            AuthorizationDelegate* authorization_delegate,
                            PolicySignedResponse callback);
  virtual TPM_RC PolicySignedSync(
      const TPMI_DH_OBJECT& auth_object,
      const std::string& auth_object_name,
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      const TPM2B_NONCE& nonce_tpm,
      const TPM2B_DIGEST& cp_hash_a,
      const TPM2B_NONCE& policy_ref,
      const INT32& expiration,
      const TPMT_SIGNATURE& auth,
      TPM2B_TIMEOUT* timeout,
      TPMT_TK_AUTH* policy_ticket,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM2B_TIMEOUT& timeout,
                                  const TPMT_TK_AUTH& policy_ticket)>
      PolicySecretResponse;
  static TPM_RC SerializeCommand_PolicySecret(
      const TPMI_DH_ENTITY& auth_handle,
      const std::string& auth_handle_name,
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      const TPM2B_NONCE& nonce_tpm,
      const TPM2B_DIGEST& cp_hash_a,
      const TPM2B_NONCE& policy_ref,
      const INT32& expiration,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_PolicySecret(
      const std::string& response,
      TPM2B_TIMEOUT* timeout,
      TPMT_TK_AUTH* policy_ticket,
      AuthorizationDelegate* authorization_delegate);
  virtual void PolicySecret(const TPMI_DH_ENTITY& auth_handle,
                            const std::string& auth_handle_name,
                            const TPMI_SH_POLICY& policy_session,
                            const std::string& policy_session_name,
                            const TPM2B_NONCE& nonce_tpm,
                            const TPM2B_DIGEST& cp_hash_a,
                            const TPM2B_NONCE& policy_ref,
                            const INT32& expiration,
                            AuthorizationDelegate* authorization_delegate,
                            PolicySecretResponse callback);
  virtual TPM_RC PolicySecretSync(
      const TPMI_DH_ENTITY& auth_handle,
      const std::string& auth_handle_name,
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      const TPM2B_NONCE& nonce_tpm,
      const TPM2B_DIGEST& cp_hash_a,
      const TPM2B_NONCE& policy_ref,
      const INT32& expiration,
      TPM2B_TIMEOUT* timeout,
      TPMT_TK_AUTH* policy_ticket,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)> PolicyTicketResponse;
  static TPM_RC SerializeCommand_PolicyTicket(
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      const TPM2B_TIMEOUT& timeout,
      const TPM2B_DIGEST& cp_hash_a,
      const TPM2B_NONCE& policy_ref,
      const TPM2B_NAME& auth_name,
      const TPMT_TK_AUTH& ticket,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_PolicyTicket(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void PolicyTicket(const TPMI_SH_POLICY& policy_session,
                            const std::string& policy_session_name,
                            const TPM2B_TIMEOUT& timeout,
                            const TPM2B_DIGEST& cp_hash_a,
                            const TPM2B_NONCE& policy_ref,
                            const TPM2B_NAME& auth_name,
                            const TPMT_TK_AUTH& ticket,
                            AuthorizationDelegate* authorization_delegate,
                            PolicyTicketResponse callback);
  virtual TPM_RC PolicyTicketSync(
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      const TPM2B_TIMEOUT& timeout,
      const TPM2B_DIGEST& cp_hash_a,
      const TPM2B_NONCE& policy_ref,
      const TPM2B_NAME& auth_name,
      const TPMT_TK_AUTH& ticket,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)> PolicyORResponse;
  static TPM_RC SerializeCommand_PolicyOR(
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      const TPML_DIGEST& p_hash_list,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_PolicyOR(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void PolicyOR(const TPMI_SH_POLICY& policy_session,
                        const std::string& policy_session_name,
                        const TPML_DIGEST& p_hash_list,
                        AuthorizationDelegate* authorization_delegate,
                        PolicyORResponse callback);
  virtual TPM_RC PolicyORSync(const TPMI_SH_POLICY& policy_session,
                              const std::string& policy_session_name,
                              const TPML_DIGEST& p_hash_list,
                              AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)> PolicyPCRResponse;
  static TPM_RC SerializeCommand_PolicyPCR(
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      const TPM2B_DIGEST& pcr_digest,
      const TPML_PCR_SELECTION& pcrs,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_PolicyPCR(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void PolicyPCR(const TPMI_SH_POLICY& policy_session,
                         const std::string& policy_session_name,
                         const TPM2B_DIGEST& pcr_digest,
                         const TPML_PCR_SELECTION& pcrs,
                         AuthorizationDelegate* authorization_delegate,
                         PolicyPCRResponse callback);
  virtual TPM_RC PolicyPCRSync(const TPMI_SH_POLICY& policy_session,
                               const std::string& policy_session_name,
                               const TPM2B_DIGEST& pcr_digest,
                               const TPML_PCR_SELECTION& pcrs,
                               AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)> PolicyLocalityResponse;
  static TPM_RC SerializeCommand_PolicyLocality(
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      const TPMA_LOCALITY& locality,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_PolicyLocality(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void PolicyLocality(const TPMI_SH_POLICY& policy_session,
                              const std::string& policy_session_name,
                              const TPMA_LOCALITY& locality,
                              AuthorizationDelegate* authorization_delegate,
                              PolicyLocalityResponse callback);
  virtual TPM_RC PolicyLocalitySync(
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      const TPMA_LOCALITY& locality,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)> PolicyNVResponse;
  static TPM_RC SerializeCommand_PolicyNV(
      const TPMI_RH_NV_AUTH& auth_handle,
      const std::string& auth_handle_name,
      const TPMI_RH_NV_INDEX& nv_index,
      const std::string& nv_index_name,
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      const TPM2B_OPERAND& operand_b,
      const UINT16& offset,
      const TPM_EO& operation,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_PolicyNV(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void PolicyNV(const TPMI_RH_NV_AUTH& auth_handle,
                        const std::string& auth_handle_name,
                        const TPMI_RH_NV_INDEX& nv_index,
                        const std::string& nv_index_name,
                        const TPMI_SH_POLICY& policy_session,
                        const std::string& policy_session_name,
                        const TPM2B_OPERAND& operand_b,
                        const UINT16& offset,
                        const TPM_EO& operation,
                        AuthorizationDelegate* authorization_delegate,
                        PolicyNVResponse callback);
  virtual TPM_RC PolicyNVSync(const TPMI_RH_NV_AUTH& auth_handle,
                              const std::string& auth_handle_name,
                              const TPMI_RH_NV_INDEX& nv_index,
                              const std::string& nv_index_name,
                              const TPMI_SH_POLICY& policy_session,
                              const std::string& policy_session_name,
                              const TPM2B_OPERAND& operand_b,
                              const UINT16& offset,
                              const TPM_EO& operation,
                              AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)>
      PolicyCounterTimerResponse;
  static TPM_RC SerializeCommand_PolicyCounterTimer(
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      const TPM2B_OPERAND& operand_b,
      const UINT16& offset,
      const TPM_EO& operation,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_PolicyCounterTimer(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void PolicyCounterTimer(const TPMI_SH_POLICY& policy_session,
                                  const std::string& policy_session_name,
                                  const TPM2B_OPERAND& operand_b,
                                  const UINT16& offset,
                                  const TPM_EO& operation,
                                  AuthorizationDelegate* authorization_delegate,
                                  PolicyCounterTimerResponse callback);
  virtual TPM_RC PolicyCounterTimerSync(
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      const TPM2B_OPERAND& operand_b,
      const UINT16& offset,
      const TPM_EO& operation,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)>
      PolicyCommandCodeResponse;
  static TPM_RC SerializeCommand_PolicyCommandCode(
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      const TPM_CC& code,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_PolicyCommandCode(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void PolicyCommandCode(const TPMI_SH_POLICY& policy_session,
                                 const std::string& policy_session_name,
                                 const TPM_CC& code,
                                 AuthorizationDelegate* authorization_delegate,
                                 PolicyCommandCodeResponse callback);
  virtual TPM_RC PolicyCommandCodeSync(
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      const TPM_CC& code,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)>
      PolicyPhysicalPresenceResponse;
  static TPM_RC SerializeCommand_PolicyPhysicalPresence(
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_PolicyPhysicalPresence(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void PolicyPhysicalPresence(
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      AuthorizationDelegate* authorization_delegate,
      PolicyPhysicalPresenceResponse callback);
  virtual TPM_RC PolicyPhysicalPresenceSync(
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)> PolicyCpHashResponse;
  static TPM_RC SerializeCommand_PolicyCpHash(
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      const TPM2B_DIGEST& cp_hash_a,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_PolicyCpHash(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void PolicyCpHash(const TPMI_SH_POLICY& policy_session,
                            const std::string& policy_session_name,
                            const TPM2B_DIGEST& cp_hash_a,
                            AuthorizationDelegate* authorization_delegate,
                            PolicyCpHashResponse callback);
  virtual TPM_RC PolicyCpHashSync(
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      const TPM2B_DIGEST& cp_hash_a,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)> PolicyNameHashResponse;
  static TPM_RC SerializeCommand_PolicyNameHash(
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      const TPM2B_DIGEST& name_hash,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_PolicyNameHash(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void PolicyNameHash(const TPMI_SH_POLICY& policy_session,
                              const std::string& policy_session_name,
                              const TPM2B_DIGEST& name_hash,
                              AuthorizationDelegate* authorization_delegate,
                              PolicyNameHashResponse callback);
  virtual TPM_RC PolicyNameHashSync(
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      const TPM2B_DIGEST& name_hash,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)>
      PolicyDuplicationSelectResponse;
  static TPM_RC SerializeCommand_PolicyDuplicationSelect(
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      const TPM2B_NAME& object_name,
      const TPM2B_NAME& new_parent_name,
      const TPMI_YES_NO& include_object,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_PolicyDuplicationSelect(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void PolicyDuplicationSelect(
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      const TPM2B_NAME& object_name,
      const TPM2B_NAME& new_parent_name,
      const TPMI_YES_NO& include_object,
      AuthorizationDelegate* authorization_delegate,
      PolicyDuplicationSelectResponse callback);
  virtual TPM_RC PolicyDuplicationSelectSync(
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      const TPM2B_NAME& object_name,
      const TPM2B_NAME& new_parent_name,
      const TPMI_YES_NO& include_object,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)>
      PolicyAuthorizeResponse;
  static TPM_RC SerializeCommand_PolicyAuthorize(
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      const TPM2B_DIGEST& approved_policy,
      const TPM2B_NONCE& policy_ref,
      const TPM2B_NAME& key_sign,
      const TPMT_TK_VERIFIED& check_ticket,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_PolicyAuthorize(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void PolicyAuthorize(const TPMI_SH_POLICY& policy_session,
                               const std::string& policy_session_name,
                               const TPM2B_DIGEST& approved_policy,
                               const TPM2B_NONCE& policy_ref,
                               const TPM2B_NAME& key_sign,
                               const TPMT_TK_VERIFIED& check_ticket,
                               AuthorizationDelegate* authorization_delegate,
                               PolicyAuthorizeResponse callback);
  virtual TPM_RC PolicyAuthorizeSync(
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      const TPM2B_DIGEST& approved_policy,
      const TPM2B_NONCE& policy_ref,
      const TPM2B_NAME& key_sign,
      const TPMT_TK_VERIFIED& check_ticket,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)>
      PolicyAuthValueResponse;
  static TPM_RC SerializeCommand_PolicyAuthValue(
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_PolicyAuthValue(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void PolicyAuthValue(const TPMI_SH_POLICY& policy_session,
                               const std::string& policy_session_name,
                               AuthorizationDelegate* authorization_delegate,
                               PolicyAuthValueResponse callback);
  virtual TPM_RC PolicyAuthValueSync(
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)> PolicyPasswordResponse;
  static TPM_RC SerializeCommand_PolicyPassword(
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_PolicyPassword(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void PolicyPassword(const TPMI_SH_POLICY& policy_session,
                              const std::string& policy_session_name,
                              AuthorizationDelegate* authorization_delegate,
                              PolicyPasswordResponse callback);
  virtual TPM_RC PolicyPasswordSync(
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM2B_DIGEST& policy_digest)>
      PolicyGetDigestResponse;
  static TPM_RC SerializeCommand_PolicyGetDigest(
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_PolicyGetDigest(
      const std::string& response,
      TPM2B_DIGEST* policy_digest,
      AuthorizationDelegate* authorization_delegate);
  virtual void PolicyGetDigest(const TPMI_SH_POLICY& policy_session,
                               const std::string& policy_session_name,
                               AuthorizationDelegate* authorization_delegate,
                               PolicyGetDigestResponse callback);
  virtual TPM_RC PolicyGetDigestSync(
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      TPM2B_DIGEST* policy_digest,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)>
      PolicyNvWrittenResponse;
  static TPM_RC SerializeCommand_PolicyNvWritten(
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      const TPMI_YES_NO& written_set,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_PolicyNvWritten(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void PolicyNvWritten(const TPMI_SH_POLICY& policy_session,
                               const std::string& policy_session_name,
                               const TPMI_YES_NO& written_set,
                               AuthorizationDelegate* authorization_delegate,
                               PolicyNvWrittenResponse callback);
  virtual TPM_RC PolicyNvWrittenSync(
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      const TPMI_YES_NO& written_set,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM_HANDLE& object_handle,
                                  const TPM2B_PUBLIC& out_public,
                                  const TPM2B_CREATION_DATA& creation_data,
                                  const TPM2B_DIGEST& creation_hash,
                                  const TPMT_TK_CREATION& creation_ticket,
                                  const TPM2B_NAME& name)>
      CreatePrimaryResponse;
  static TPM_RC SerializeCommand_CreatePrimary(
      const TPMI_RH_HIERARCHY& primary_handle,
      const std::string& primary_handle_name,
      const TPM2B_SENSITIVE_CREATE& in_sensitive,
      const TPM2B_PUBLIC& in_public,
      const TPM2B_DATA& outside_info,
      const TPML_PCR_SELECTION& creation_pcr,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_CreatePrimary(
      const std::string& response,
      TPM_HANDLE* object_handle,
      TPM2B_PUBLIC* out_public,
      TPM2B_CREATION_DATA* creation_data,
      TPM2B_DIGEST* creation_hash,
      TPMT_TK_CREATION* creation_ticket,
      TPM2B_NAME* name,
      AuthorizationDelegate* authorization_delegate);
  virtual void CreatePrimary(const TPMI_RH_HIERARCHY& primary_handle,
                             const std::string& primary_handle_name,
                             const TPM2B_SENSITIVE_CREATE& in_sensitive,
                             const TPM2B_PUBLIC& in_public,
                             const TPM2B_DATA& outside_info,
                             const TPML_PCR_SELECTION& creation_pcr,
                             AuthorizationDelegate* authorization_delegate,
                             CreatePrimaryResponse callback);
  virtual TPM_RC CreatePrimarySync(
      const TPMI_RH_HIERARCHY& primary_handle,
      const std::string& primary_handle_name,
      const TPM2B_SENSITIVE_CREATE& in_sensitive,
      const TPM2B_PUBLIC& in_public,
      const TPM2B_DATA& outside_info,
      const TPML_PCR_SELECTION& creation_pcr,
      TPM_HANDLE* object_handle,
      TPM2B_PUBLIC* out_public,
      TPM2B_CREATION_DATA* creation_data,
      TPM2B_DIGEST* creation_hash,
      TPMT_TK_CREATION* creation_ticket,
      TPM2B_NAME* name,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)>
      HierarchyControlResponse;
  static TPM_RC SerializeCommand_HierarchyControl(
      const TPMI_RH_HIERARCHY& auth_handle,
      const std::string& auth_handle_name,
      const TPMI_RH_ENABLES& enable,
      const TPMI_YES_NO& state,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_HierarchyControl(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void HierarchyControl(const TPMI_RH_HIERARCHY& auth_handle,
                                const std::string& auth_handle_name,
                                const TPMI_RH_ENABLES& enable,
                                const TPMI_YES_NO& state,
                                AuthorizationDelegate* authorization_delegate,
                                HierarchyControlResponse callback);
  virtual TPM_RC HierarchyControlSync(
      const TPMI_RH_HIERARCHY& auth_handle,
      const std::string& auth_handle_name,
      const TPMI_RH_ENABLES& enable,
      const TPMI_YES_NO& state,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)>
      SetPrimaryPolicyResponse;
  static TPM_RC SerializeCommand_SetPrimaryPolicy(
      const TPMI_RH_HIERARCHY& auth_handle,
      const std::string& auth_handle_name,
      const TPM2B_DIGEST& auth_policy,
      const TPMI_ALG_HASH& hash_alg,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_SetPrimaryPolicy(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void SetPrimaryPolicy(const TPMI_RH_HIERARCHY& auth_handle,
                                const std::string& auth_handle_name,
                                const TPM2B_DIGEST& auth_policy,
                                const TPMI_ALG_HASH& hash_alg,
                                AuthorizationDelegate* authorization_delegate,
                                SetPrimaryPolicyResponse callback);
  virtual TPM_RC SetPrimaryPolicySync(
      const TPMI_RH_HIERARCHY& auth_handle,
      const std::string& auth_handle_name,
      const TPM2B_DIGEST& auth_policy,
      const TPMI_ALG_HASH& hash_alg,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)> ChangePPSResponse;
  static TPM_RC SerializeCommand_ChangePPS(
      const TPMI_RH_PLATFORM& auth_handle,
      const std::string& auth_handle_name,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_ChangePPS(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void ChangePPS(const TPMI_RH_PLATFORM& auth_handle,
                         const std::string& auth_handle_name,
                         AuthorizationDelegate* authorization_delegate,
                         ChangePPSResponse callback);
  virtual TPM_RC ChangePPSSync(const TPMI_RH_PLATFORM& auth_handle,
                               const std::string& auth_handle_name,
                               AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)> ChangeEPSResponse;
  static TPM_RC SerializeCommand_ChangeEPS(
      const TPMI_RH_PLATFORM& auth_handle,
      const std::string& auth_handle_name,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_ChangeEPS(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void ChangeEPS(const TPMI_RH_PLATFORM& auth_handle,
                         const std::string& auth_handle_name,
                         AuthorizationDelegate* authorization_delegate,
                         ChangeEPSResponse callback);
  virtual TPM_RC ChangeEPSSync(const TPMI_RH_PLATFORM& auth_handle,
                               const std::string& auth_handle_name,
                               AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)> ClearResponse;
  static TPM_RC SerializeCommand_Clear(
      const TPMI_RH_CLEAR& auth_handle,
      const std::string& auth_handle_name,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_Clear(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void Clear(const TPMI_RH_CLEAR& auth_handle,
                     const std::string& auth_handle_name,
                     AuthorizationDelegate* authorization_delegate,
                     ClearResponse callback);
  virtual TPM_RC ClearSync(const TPMI_RH_CLEAR& auth_handle,
                           const std::string& auth_handle_name,
                           AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)> ClearControlResponse;
  static TPM_RC SerializeCommand_ClearControl(
      const TPMI_RH_CLEAR& auth,
      const std::string& auth_name,
      const TPMI_YES_NO& disable,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_ClearControl(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void ClearControl(const TPMI_RH_CLEAR& auth,
                            const std::string& auth_name,
                            const TPMI_YES_NO& disable,
                            AuthorizationDelegate* authorization_delegate,
                            ClearControlResponse callback);
  virtual TPM_RC ClearControlSync(
      const TPMI_RH_CLEAR& auth,
      const std::string& auth_name,
      const TPMI_YES_NO& disable,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)>
      HierarchyChangeAuthResponse;
  static TPM_RC SerializeCommand_HierarchyChangeAuth(
      const TPMI_RH_HIERARCHY_AUTH& auth_handle,
      const std::string& auth_handle_name,
      const TPM2B_AUTH& new_auth,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_HierarchyChangeAuth(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void HierarchyChangeAuth(
      const TPMI_RH_HIERARCHY_AUTH& auth_handle,
      const std::string& auth_handle_name,
      const TPM2B_AUTH& new_auth,
      AuthorizationDelegate* authorization_delegate,
      HierarchyChangeAuthResponse callback);
  virtual TPM_RC HierarchyChangeAuthSync(
      const TPMI_RH_HIERARCHY_AUTH& auth_handle,
      const std::string& auth_handle_name,
      const TPM2B_AUTH& new_auth,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)>
      DictionaryAttackLockResetResponse;
  static TPM_RC SerializeCommand_DictionaryAttackLockReset(
      const TPMI_RH_LOCKOUT& lock_handle,
      const std::string& lock_handle_name,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_DictionaryAttackLockReset(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void DictionaryAttackLockReset(
      const TPMI_RH_LOCKOUT& lock_handle,
      const std::string& lock_handle_name,
      AuthorizationDelegate* authorization_delegate,
      DictionaryAttackLockResetResponse callback);
  virtual TPM_RC DictionaryAttackLockResetSync(
      const TPMI_RH_LOCKOUT& lock_handle,
      const std::string& lock_handle_name,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)>
      DictionaryAttackParametersResponse;
  static TPM_RC SerializeCommand_DictionaryAttackParameters(
      const TPMI_RH_LOCKOUT& lock_handle,
      const std::string& lock_handle_name,
      const UINT32& new_max_tries,
      const UINT32& new_recovery_time,
      const UINT32& lockout_recovery,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_DictionaryAttackParameters(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void DictionaryAttackParameters(
      const TPMI_RH_LOCKOUT& lock_handle,
      const std::string& lock_handle_name,
      const UINT32& new_max_tries,
      const UINT32& new_recovery_time,
      const UINT32& lockout_recovery,
      AuthorizationDelegate* authorization_delegate,
      DictionaryAttackParametersResponse callback);
  virtual TPM_RC DictionaryAttackParametersSync(
      const TPMI_RH_LOCKOUT& lock_handle,
      const std::string& lock_handle_name,
      const UINT32& new_max_tries,
      const UINT32& new_recovery_time,
      const UINT32& lockout_recovery,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)> PP_CommandsResponse;
  static TPM_RC SerializeCommand_PP_Commands(
      const TPMI_RH_PLATFORM& auth,
      const std::string& auth_name,
      const TPML_CC& set_list,
      const TPML_CC& clear_list,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_PP_Commands(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void PP_Commands(const TPMI_RH_PLATFORM& auth,
                           const std::string& auth_name,
                           const TPML_CC& set_list,
                           const TPML_CC& clear_list,
                           AuthorizationDelegate* authorization_delegate,
                           PP_CommandsResponse callback);
  virtual TPM_RC PP_CommandsSync(const TPMI_RH_PLATFORM& auth,
                                 const std::string& auth_name,
                                 const TPML_CC& set_list,
                                 const TPML_CC& clear_list,
                                 AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)>
      SetAlgorithmSetResponse;
  static TPM_RC SerializeCommand_SetAlgorithmSet(
      const TPMI_RH_PLATFORM& auth_handle,
      const std::string& auth_handle_name,
      const UINT32& algorithm_set,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_SetAlgorithmSet(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void SetAlgorithmSet(const TPMI_RH_PLATFORM& auth_handle,
                               const std::string& auth_handle_name,
                               const UINT32& algorithm_set,
                               AuthorizationDelegate* authorization_delegate,
                               SetAlgorithmSetResponse callback);
  virtual TPM_RC SetAlgorithmSetSync(
      const TPMI_RH_PLATFORM& auth_handle,
      const std::string& auth_handle_name,
      const UINT32& algorithm_set,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)>
      FieldUpgradeStartResponse;
  static TPM_RC SerializeCommand_FieldUpgradeStart(
      const TPMI_RH_PLATFORM& authorization,
      const std::string& authorization_name,
      const TPMI_DH_OBJECT& key_handle,
      const std::string& key_handle_name,
      const TPM2B_DIGEST& fu_digest,
      const TPMT_SIGNATURE& manifest_signature,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_FieldUpgradeStart(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void FieldUpgradeStart(const TPMI_RH_PLATFORM& authorization,
                                 const std::string& authorization_name,
                                 const TPMI_DH_OBJECT& key_handle,
                                 const std::string& key_handle_name,
                                 const TPM2B_DIGEST& fu_digest,
                                 const TPMT_SIGNATURE& manifest_signature,
                                 AuthorizationDelegate* authorization_delegate,
                                 FieldUpgradeStartResponse callback);
  virtual TPM_RC FieldUpgradeStartSync(
      const TPMI_RH_PLATFORM& authorization,
      const std::string& authorization_name,
      const TPMI_DH_OBJECT& key_handle,
      const std::string& key_handle_name,
      const TPM2B_DIGEST& fu_digest,
      const TPMT_SIGNATURE& manifest_signature,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPMT_HA& next_digest,
                                  const TPMT_HA& first_digest)>
      FieldUpgradeDataResponse;
  static TPM_RC SerializeCommand_FieldUpgradeData(
      const TPM2B_MAX_BUFFER& fu_data,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_FieldUpgradeData(
      const std::string& response,
      TPMT_HA* next_digest,
      TPMT_HA* first_digest,
      AuthorizationDelegate* authorization_delegate);
  virtual void FieldUpgradeData(const TPM2B_MAX_BUFFER& fu_data,
                                AuthorizationDelegate* authorization_delegate,
                                FieldUpgradeDataResponse callback);
  virtual TPM_RC FieldUpgradeDataSync(
      const TPM2B_MAX_BUFFER& fu_data,
      TPMT_HA* next_digest,
      TPMT_HA* first_digest,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM2B_MAX_BUFFER& fu_data)>
      FirmwareReadResponse;
  static TPM_RC SerializeCommand_FirmwareRead(
      const UINT32& sequence_number,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_FirmwareRead(
      const std::string& response,
      TPM2B_MAX_BUFFER* fu_data,
      AuthorizationDelegate* authorization_delegate);
  virtual void FirmwareRead(const UINT32& sequence_number,
                            AuthorizationDelegate* authorization_delegate,
                            FirmwareReadResponse callback);
  virtual TPM_RC FirmwareReadSync(
      const UINT32& sequence_number,
      TPM2B_MAX_BUFFER* fu_data,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPMS_CONTEXT& context)>
      ContextSaveResponse;
  static TPM_RC SerializeCommand_ContextSave(
      const TPMI_DH_CONTEXT& save_handle,
      const std::string& save_handle_name,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_ContextSave(
      const std::string& response,
      TPMS_CONTEXT* context,
      AuthorizationDelegate* authorization_delegate);
  virtual void ContextSave(const TPMI_DH_CONTEXT& save_handle,
                           const std::string& save_handle_name,
                           AuthorizationDelegate* authorization_delegate,
                           ContextSaveResponse callback);
  virtual TPM_RC ContextSaveSync(const TPMI_DH_CONTEXT& save_handle,
                                 const std::string& save_handle_name,
                                 TPMS_CONTEXT* context,
                                 AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPMI_DH_CONTEXT& loaded_handle)>
      ContextLoadResponse;
  static TPM_RC SerializeCommand_ContextLoad(
      const TPMS_CONTEXT& context,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_ContextLoad(
      const std::string& response,
      TPMI_DH_CONTEXT* loaded_handle,
      AuthorizationDelegate* authorization_delegate);
  virtual void ContextLoad(const TPMS_CONTEXT& context,
                           AuthorizationDelegate* authorization_delegate,
                           ContextLoadResponse callback);
  virtual TPM_RC ContextLoadSync(const TPMS_CONTEXT& context,
                                 TPMI_DH_CONTEXT* loaded_handle,
                                 AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)> FlushContextResponse;
  static TPM_RC SerializeCommand_FlushContext(
      const TPMI_DH_CONTEXT& flush_handle,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_FlushContext(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void FlushContext(const TPMI_DH_CONTEXT& flush_handle,
                            AuthorizationDelegate* authorization_delegate,
                            FlushContextResponse callback);
  virtual TPM_RC FlushContextSync(
      const TPMI_DH_CONTEXT& flush_handle,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)> EvictControlResponse;
  static TPM_RC SerializeCommand_EvictControl(
      const TPMI_RH_PROVISION& auth,
      const std::string& auth_name,
      const TPMI_DH_OBJECT& object_handle,
      const std::string& object_handle_name,
      const TPMI_DH_PERSISTENT& persistent_handle,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_EvictControl(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void EvictControl(const TPMI_RH_PROVISION& auth,
                            const std::string& auth_name,
                            const TPMI_DH_OBJECT& object_handle,
                            const std::string& object_handle_name,
                            const TPMI_DH_PERSISTENT& persistent_handle,
                            AuthorizationDelegate* authorization_delegate,
                            EvictControlResponse callback);
  virtual TPM_RC EvictControlSync(
      const TPMI_RH_PROVISION& auth,
      const std::string& auth_name,
      const TPMI_DH_OBJECT& object_handle,
      const std::string& object_handle_name,
      const TPMI_DH_PERSISTENT& persistent_handle,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPMS_TIME_INFO& current_time)>
      ReadClockResponse;
  static TPM_RC SerializeCommand_ReadClock(
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_ReadClock(
      const std::string& response,
      TPMS_TIME_INFO* current_time,
      AuthorizationDelegate* authorization_delegate);
  virtual void ReadClock(AuthorizationDelegate* authorization_delegate,
                         ReadClockResponse callback);
  virtual TPM_RC ReadClockSync(TPMS_TIME_INFO* current_time,
                               AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)> ClockSetResponse;
  static TPM_RC SerializeCommand_ClockSet(
      const TPMI_RH_PROVISION& auth,
      const std::string& auth_name,
      const UINT64& new_time,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_ClockSet(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void ClockSet(const TPMI_RH_PROVISION& auth,
                        const std::string& auth_name,
                        const UINT64& new_time,
                        AuthorizationDelegate* authorization_delegate,
                        ClockSetResponse callback);
  virtual TPM_RC ClockSetSync(const TPMI_RH_PROVISION& auth,
                              const std::string& auth_name,
                              const UINT64& new_time,
                              AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)>
      ClockRateAdjustResponse;
  static TPM_RC SerializeCommand_ClockRateAdjust(
      const TPMI_RH_PROVISION& auth,
      const std::string& auth_name,
      const TPM_CLOCK_ADJUST& rate_adjust,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_ClockRateAdjust(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void ClockRateAdjust(const TPMI_RH_PROVISION& auth,
                               const std::string& auth_name,
                               const TPM_CLOCK_ADJUST& rate_adjust,
                               AuthorizationDelegate* authorization_delegate,
                               ClockRateAdjustResponse callback);
  virtual TPM_RC ClockRateAdjustSync(
      const TPMI_RH_PROVISION& auth,
      const std::string& auth_name,
      const TPM_CLOCK_ADJUST& rate_adjust,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPMI_YES_NO& more_data,
                                  const TPMS_CAPABILITY_DATA& capability_data)>
      GetCapabilityResponse;
  static TPM_RC SerializeCommand_GetCapability(
      const TPM_CAP& capability,
      const UINT32& property,
      const UINT32& property_count,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_GetCapability(
      const std::string& response,
      TPMI_YES_NO* more_data,
      TPMS_CAPABILITY_DATA* capability_data,
      AuthorizationDelegate* authorization_delegate);
  virtual void GetCapability(const TPM_CAP& capability,
                             const UINT32& property,
                             const UINT32& property_count,
                             AuthorizationDelegate* authorization_delegate,
                             GetCapabilityResponse callback);
  virtual TPM_RC GetCapabilitySync(
      const TPM_CAP& capability,
      const UINT32& property,
      const UINT32& property_count,
      TPMI_YES_NO* more_data,
      TPMS_CAPABILITY_DATA* capability_data,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)> TestParmsResponse;
  static TPM_RC SerializeCommand_TestParms(
      const TPMT_PUBLIC_PARMS& parameters,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_TestParms(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void TestParms(const TPMT_PUBLIC_PARMS& parameters,
                         AuthorizationDelegate* authorization_delegate,
                         TestParmsResponse callback);
  virtual TPM_RC TestParmsSync(const TPMT_PUBLIC_PARMS& parameters,
                               AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)> NV_DefineSpaceResponse;
  static TPM_RC SerializeCommand_NV_DefineSpace(
      const TPMI_RH_PROVISION& auth_handle,
      const std::string& auth_handle_name,
      const TPM2B_AUTH& auth,
      const TPM2B_NV_PUBLIC& public_info,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_NV_DefineSpace(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void NV_DefineSpace(const TPMI_RH_PROVISION& auth_handle,
                              const std::string& auth_handle_name,
                              const TPM2B_AUTH& auth,
                              const TPM2B_NV_PUBLIC& public_info,
                              AuthorizationDelegate* authorization_delegate,
                              NV_DefineSpaceResponse callback);
  virtual TPM_RC NV_DefineSpaceSync(
      const TPMI_RH_PROVISION& auth_handle,
      const std::string& auth_handle_name,
      const TPM2B_AUTH& auth,
      const TPM2B_NV_PUBLIC& public_info,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)>
      NV_UndefineSpaceResponse;
  static TPM_RC SerializeCommand_NV_UndefineSpace(
      const TPMI_RH_PROVISION& auth_handle,
      const std::string& auth_handle_name,
      const TPMI_RH_NV_INDEX& nv_index,
      const std::string& nv_index_name,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_NV_UndefineSpace(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void NV_UndefineSpace(const TPMI_RH_PROVISION& auth_handle,
                                const std::string& auth_handle_name,
                                const TPMI_RH_NV_INDEX& nv_index,
                                const std::string& nv_index_name,
                                AuthorizationDelegate* authorization_delegate,
                                NV_UndefineSpaceResponse callback);
  virtual TPM_RC NV_UndefineSpaceSync(
      const TPMI_RH_PROVISION& auth_handle,
      const std::string& auth_handle_name,
      const TPMI_RH_NV_INDEX& nv_index,
      const std::string& nv_index_name,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)>
      NV_UndefineSpaceSpecialResponse;
  static TPM_RC SerializeCommand_NV_UndefineSpaceSpecial(
      const TPMI_RH_NV_INDEX& nv_index,
      const std::string& nv_index_name,
      const TPMI_RH_PLATFORM& platform,
      const std::string& platform_name,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_NV_UndefineSpaceSpecial(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void NV_UndefineSpaceSpecial(
      const TPMI_RH_NV_INDEX& nv_index,
      const std::string& nv_index_name,
      const TPMI_RH_PLATFORM& platform,
      const std::string& platform_name,
      AuthorizationDelegate* authorization_delegate,
      NV_UndefineSpaceSpecialResponse callback);
  virtual TPM_RC NV_UndefineSpaceSpecialSync(
      const TPMI_RH_NV_INDEX& nv_index,
      const std::string& nv_index_name,
      const TPMI_RH_PLATFORM& platform,
      const std::string& platform_name,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM2B_NV_PUBLIC& nv_public,
                                  const TPM2B_NAME& nv_name)>
      NV_ReadPublicResponse;
  static TPM_RC SerializeCommand_NV_ReadPublic(
      const TPMI_RH_NV_INDEX& nv_index,
      const std::string& nv_index_name,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_NV_ReadPublic(
      const std::string& response,
      TPM2B_NV_PUBLIC* nv_public,
      TPM2B_NAME* nv_name,
      AuthorizationDelegate* authorization_delegate);
  virtual void NV_ReadPublic(const TPMI_RH_NV_INDEX& nv_index,
                             const std::string& nv_index_name,
                             AuthorizationDelegate* authorization_delegate,
                             NV_ReadPublicResponse callback);
  virtual TPM_RC NV_ReadPublicSync(
      const TPMI_RH_NV_INDEX& nv_index,
      const std::string& nv_index_name,
      TPM2B_NV_PUBLIC* nv_public,
      TPM2B_NAME* nv_name,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)> NV_WriteResponse;
  static TPM_RC SerializeCommand_NV_Write(
      const TPMI_RH_NV_AUTH& auth_handle,
      const std::string& auth_handle_name,
      const TPMI_RH_NV_INDEX& nv_index,
      const std::string& nv_index_name,
      const TPM2B_MAX_NV_BUFFER& data,
      const UINT16& offset,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_NV_Write(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void NV_Write(const TPMI_RH_NV_AUTH& auth_handle,
                        const std::string& auth_handle_name,
                        const TPMI_RH_NV_INDEX& nv_index,
                        const std::string& nv_index_name,
                        const TPM2B_MAX_NV_BUFFER& data,
                        const UINT16& offset,
                        AuthorizationDelegate* authorization_delegate,
                        NV_WriteResponse callback);
  virtual TPM_RC NV_WriteSync(const TPMI_RH_NV_AUTH& auth_handle,
                              const std::string& auth_handle_name,
                              const TPMI_RH_NV_INDEX& nv_index,
                              const std::string& nv_index_name,
                              const TPM2B_MAX_NV_BUFFER& data,
                              const UINT16& offset,
                              AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)> NV_IncrementResponse;
  static TPM_RC SerializeCommand_NV_Increment(
      const TPMI_RH_NV_AUTH& auth_handle,
      const std::string& auth_handle_name,
      const TPMI_RH_NV_INDEX& nv_index,
      const std::string& nv_index_name,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_NV_Increment(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void NV_Increment(const TPMI_RH_NV_AUTH& auth_handle,
                            const std::string& auth_handle_name,
                            const TPMI_RH_NV_INDEX& nv_index,
                            const std::string& nv_index_name,
                            AuthorizationDelegate* authorization_delegate,
                            NV_IncrementResponse callback);
  virtual TPM_RC NV_IncrementSync(
      const TPMI_RH_NV_AUTH& auth_handle,
      const std::string& auth_handle_name,
      const TPMI_RH_NV_INDEX& nv_index,
      const std::string& nv_index_name,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)> NV_ExtendResponse;
  static TPM_RC SerializeCommand_NV_Extend(
      const TPMI_RH_NV_AUTH& auth_handle,
      const std::string& auth_handle_name,
      const TPMI_RH_NV_INDEX& nv_index,
      const std::string& nv_index_name,
      const TPM2B_MAX_NV_BUFFER& data,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_NV_Extend(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void NV_Extend(const TPMI_RH_NV_AUTH& auth_handle,
                         const std::string& auth_handle_name,
                         const TPMI_RH_NV_INDEX& nv_index,
                         const std::string& nv_index_name,
                         const TPM2B_MAX_NV_BUFFER& data,
                         AuthorizationDelegate* authorization_delegate,
                         NV_ExtendResponse callback);
  virtual TPM_RC NV_ExtendSync(const TPMI_RH_NV_AUTH& auth_handle,
                               const std::string& auth_handle_name,
                               const TPMI_RH_NV_INDEX& nv_index,
                               const std::string& nv_index_name,
                               const TPM2B_MAX_NV_BUFFER& data,
                               AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)> NV_SetBitsResponse;
  static TPM_RC SerializeCommand_NV_SetBits(
      const TPMI_RH_NV_AUTH& auth_handle,
      const std::string& auth_handle_name,
      const TPMI_RH_NV_INDEX& nv_index,
      const std::string& nv_index_name,
      const UINT64& bits,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_NV_SetBits(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void NV_SetBits(const TPMI_RH_NV_AUTH& auth_handle,
                          const std::string& auth_handle_name,
                          const TPMI_RH_NV_INDEX& nv_index,
                          const std::string& nv_index_name,
                          const UINT64& bits,
                          AuthorizationDelegate* authorization_delegate,
                          NV_SetBitsResponse callback);
  virtual TPM_RC NV_SetBitsSync(const TPMI_RH_NV_AUTH& auth_handle,
                                const std::string& auth_handle_name,
                                const TPMI_RH_NV_INDEX& nv_index,
                                const std::string& nv_index_name,
                                const UINT64& bits,
                                AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)> NV_WriteLockResponse;
  static TPM_RC SerializeCommand_NV_WriteLock(
      const TPMI_RH_NV_AUTH& auth_handle,
      const std::string& auth_handle_name,
      const TPMI_RH_NV_INDEX& nv_index,
      const std::string& nv_index_name,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_NV_WriteLock(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void NV_WriteLock(const TPMI_RH_NV_AUTH& auth_handle,
                            const std::string& auth_handle_name,
                            const TPMI_RH_NV_INDEX& nv_index,
                            const std::string& nv_index_name,
                            AuthorizationDelegate* authorization_delegate,
                            NV_WriteLockResponse callback);
  virtual TPM_RC NV_WriteLockSync(
      const TPMI_RH_NV_AUTH& auth_handle,
      const std::string& auth_handle_name,
      const TPMI_RH_NV_INDEX& nv_index,
      const std::string& nv_index_name,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)>
      NV_GlobalWriteLockResponse;
  static TPM_RC SerializeCommand_NV_GlobalWriteLock(
      const TPMI_RH_PROVISION& auth_handle,
      const std::string& auth_handle_name,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_NV_GlobalWriteLock(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void NV_GlobalWriteLock(const TPMI_RH_PROVISION& auth_handle,
                                  const std::string& auth_handle_name,
                                  AuthorizationDelegate* authorization_delegate,
                                  NV_GlobalWriteLockResponse callback);
  virtual TPM_RC NV_GlobalWriteLockSync(
      const TPMI_RH_PROVISION& auth_handle,
      const std::string& auth_handle_name,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM2B_MAX_NV_BUFFER& data)>
      NV_ReadResponse;
  static TPM_RC SerializeCommand_NV_Read(
      const TPMI_RH_NV_AUTH& auth_handle,
      const std::string& auth_handle_name,
      const TPMI_RH_NV_INDEX& nv_index,
      const std::string& nv_index_name,
      const UINT16& size,
      const UINT16& offset,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_NV_Read(
      const std::string& response,
      TPM2B_MAX_NV_BUFFER* data,
      AuthorizationDelegate* authorization_delegate);
  virtual void NV_Read(const TPMI_RH_NV_AUTH& auth_handle,
                       const std::string& auth_handle_name,
                       const TPMI_RH_NV_INDEX& nv_index,
                       const std::string& nv_index_name,
                       const UINT16& size,
                       const UINT16& offset,
                       AuthorizationDelegate* authorization_delegate,
                       NV_ReadResponse callback);
  virtual TPM_RC NV_ReadSync(const TPMI_RH_NV_AUTH& auth_handle,
                             const std::string& auth_handle_name,
                             const TPMI_RH_NV_INDEX& nv_index,
                             const std::string& nv_index_name,
                             const UINT16& size,
                             const UINT16& offset,
                             TPM2B_MAX_NV_BUFFER* data,
                             AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)> NV_ReadLockResponse;
  static TPM_RC SerializeCommand_NV_ReadLock(
      const TPMI_RH_NV_AUTH& auth_handle,
      const std::string& auth_handle_name,
      const TPMI_RH_NV_INDEX& nv_index,
      const std::string& nv_index_name,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_NV_ReadLock(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void NV_ReadLock(const TPMI_RH_NV_AUTH& auth_handle,
                           const std::string& auth_handle_name,
                           const TPMI_RH_NV_INDEX& nv_index,
                           const std::string& nv_index_name,
                           AuthorizationDelegate* authorization_delegate,
                           NV_ReadLockResponse callback);
  virtual TPM_RC NV_ReadLockSync(const TPMI_RH_NV_AUTH& auth_handle,
                                 const std::string& auth_handle_name,
                                 const TPMI_RH_NV_INDEX& nv_index,
                                 const std::string& nv_index_name,
                                 AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code)> NV_ChangeAuthResponse;
  static TPM_RC SerializeCommand_NV_ChangeAuth(
      const TPMI_RH_NV_INDEX& nv_index,
      const std::string& nv_index_name,
      const TPM2B_AUTH& new_auth,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_NV_ChangeAuth(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void NV_ChangeAuth(const TPMI_RH_NV_INDEX& nv_index,
                             const std::string& nv_index_name,
                             const TPM2B_AUTH& new_auth,
                             AuthorizationDelegate* authorization_delegate,
                             NV_ChangeAuthResponse callback);
  virtual TPM_RC NV_ChangeAuthSync(
      const TPMI_RH_NV_INDEX& nv_index,
      const std::string& nv_index_name,
      const TPM2B_AUTH& new_auth,
      AuthorizationDelegate* authorization_delegate);
  typedef base::OnceCallback<void(TPM_RC response_code,
                                  const TPM2B_ATTEST& certify_info,
                                  const TPMT_SIGNATURE& signature)>
      NV_CertifyResponse;
  static TPM_RC SerializeCommand_NV_Certify(
      const TPMI_DH_OBJECT& sign_handle,
      const std::string& sign_handle_name,
      const TPMI_RH_NV_AUTH& auth_handle,
      const std::string& auth_handle_name,
      const TPMI_RH_NV_INDEX& nv_index,
      const std::string& nv_index_name,
      const TPM2B_DATA& qualifying_data,
      const TPMT_SIG_SCHEME& in_scheme,
      const UINT16& size,
      const UINT16& offset,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_NV_Certify(
      const std::string& response,
      TPM2B_ATTEST* certify_info,
      TPMT_SIGNATURE* signature,
      AuthorizationDelegate* authorization_delegate);
  virtual void NV_Certify(const TPMI_DH_OBJECT& sign_handle,
                          const std::string& sign_handle_name,
                          const TPMI_RH_NV_AUTH& auth_handle,
                          const std::string& auth_handle_name,
                          const TPMI_RH_NV_INDEX& nv_index,
                          const std::string& nv_index_name,
                          const TPM2B_DATA& qualifying_data,
                          const TPMT_SIG_SCHEME& in_scheme,
                          const UINT16& size,
                          const UINT16& offset,
                          AuthorizationDelegate* authorization_delegate,
                          NV_CertifyResponse callback);
  virtual TPM_RC NV_CertifySync(const TPMI_DH_OBJECT& sign_handle,
                                const std::string& sign_handle_name,
                                const TPMI_RH_NV_AUTH& auth_handle,
                                const std::string& auth_handle_name,
                                const TPMI_RH_NV_INDEX& nv_index,
                                const std::string& nv_index_name,
                                const TPM2B_DATA& qualifying_data,
                                const TPMT_SIG_SCHEME& in_scheme,
                                const UINT16& size,
                                const UINT16& offset,
                                TPM2B_ATTEST* certify_info,
                                TPMT_SIGNATURE* signature,
                                AuthorizationDelegate* authorization_delegate);

  typedef base::OnceCallback<void(TPM_RC response_code)>
      PolicyFidoSignedResponse;
  static TPM_RC SerializeCommand_PolicyFidoSigned(
      const TPMI_DH_OBJECT& auth_object,
      const std::string& auth_object_name,
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      const std::string& auth_data,
      const std::vector<FIDO_DATA_RANGE>& auth_data_descr,
      const TPMT_SIGNATURE& auth,
      std::string* serialized_command,
      AuthorizationDelegate* authorization_delegate);
  static TPM_RC ParseResponse_PolicyFidoSigned(
      const std::string& response,
      AuthorizationDelegate* authorization_delegate);
  virtual void PolicyFidoSigned(
      const TPMI_DH_OBJECT& auth_object,
      const std::string& auth_object_name,
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      const std::string& auth_data,
      const std::vector<FIDO_DATA_RANGE>& auth_data_descr,
      const TPMT_SIGNATURE& auth,
      AuthorizationDelegate* authorization_delegate,
      PolicyFidoSignedResponse callback);
  virtual TPM_RC PolicyFidoSignedSync(
      const TPMI_DH_OBJECT& auth_object,
      const std::string& auth_object_name,
      const TPMI_SH_POLICY& policy_session,
      const std::string& policy_session_name,
      const std::string& auth_data,
      const std::vector<FIDO_DATA_RANGE>& auth_data_descr,
      const TPMT_SIGNATURE& auth,
      AuthorizationDelegate* authorization_delegate);

 private:
  CommandTransceiver* transceiver_;
};

}  // namespace trunks

#endif  // TRUNKS_TPM_GENERATED_H_
