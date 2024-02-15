# Copyright 2023 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Module for testing TPM2 commands."""
import time
import subcmd
import utils
TPM_SU_CLEAR = 0x0000
TPM_SU_STATE = 0x0001
TPM_RH_OWNER = 0x40000001
TPM_RH_ENDORSEMENT = 0x4000000B
TPM_RH_PLATFORM = 0x4000000C
TPM_RH_LOCKOUT = 0x4000000A
NV_INDEX_G2F_CERT =  0x013fff02

def tpm2_startup(tpm, state):
    """Send TPM2_Startup command

    Args:
        tpm: a tpm object used to communicate with the device
        state: TPM_SU_CLEAR / TPM_SU_STATE

    Raises:
        subcmd.TpmTestError: on unexpected target responses
    """
    startup = [0x80, 0x01,      # TPM_ST_NO_SESSIONS
            0x00, 0x00, 0x00, 0x0c, # commandSize = 12
            0x00, 0x00, 0x01, 0x44, # TPM_CC_Startup
            0x00, 0x00,            # TPM_SU_CLEAR / TPM_SU_STATE
      ]
    startup[10:12] = state.to_bytes(2, byteorder='big')
    cmd = bytes(startup)
    response = tpm.command(cmd)
    return response

def tpm2_shutdown(tpm, state):
    """Send TPM2_Shutdown command

    Args:
        tpm: a tpm object used to communicate with the device
        state: TPM_SU_CLEAR / TPM_SU_STATE

    Raises:
        subcmd.TpmTestError: on unexpected target responses
    """
    shutdown = [        0x80, 0x01, # tag: TPM_ST_NO_SESSIONS
        0x00, 0x00, 0x00, 0x0c, # size:
        0x00, 0x00, 0x01, 0x45, # command: TPM_CC_Shutdown
        # ======
        0x00, 0x00, # shutdown type: TPM_SU_STATE
      ]
    shutdown[10:12] = state.to_bytes(2, byteorder='big')
    cmd = bytes(shutdown)
    response = tpm.command(cmd)
    return response

def tpm2_clear(tpm, handle):
    """Send TPM2_Clear command

    Args:
        tpm: a tpm object used to communicate with the device
        handle: TPM_RH_PLATFORM, etc

    Raises:
        subcmd.TpmTestError: on unexpected target responses
    """
    clear = [   0x80, 0x02,     # tag: TPM_ST_SESSIONS
        0x00, 0x00, 0x00, 0x1b, # size:
        0x00, 0x00, 0x01, 0x26, # command: TPM_CC_Clear
        # ======
        0x00, 0x00, 0x00, 0x00,        # TPMI_RH_CLEAR handle
        0x00, 0x00, 0x00, 0x09,        # authorizationSize : UINT32
        0x40, 0x00, 0x00, 0x09,        # sessionHandle : empty password
        0x00, 0x00, 0x00, 0x00, 0x00,  # nonce, sessionAttributes, hmac
      ]
    clear[10:14] = handle.to_bytes(4, byteorder='big')
    cmd = bytes(clear)
    response = tpm.command(cmd)
    return response

def tpm2_nv_read(tpm, index, size, offset):
    """Send TPM2_NvRead command

    Args:
        tpm: a tpm object used to communicate with the device
        index: NV index
        size: number of bytes to read
        offset: starting offset to read

    Raises:
        subcmd.TpmTestError: on unexpected target responses
    """
    nv_read = [0x80, 0x02,           # TPM_ST_SESSIONS
        0x00, 0x00, 0x00, 0x23,        # size
        0x00, 0x00, 0x01, 0x4e,        # TPM_CC_NV_READ
        0x01, 0x3f, 0xff, 0x02,        # authHandle : TPMI_RH_NV_AUTH
        0x01, 0x3f, 0xff, 0x02,        # nvIndex    : TPMI_RH_NV_INDEX
        0x00, 0x00, 0x00, 0x09,        # authorizationSize : UINT32
        0x40, 0x00, 0x00, 0x09,        # sessionHandle : empty password
        0x00, 0x00, 0x00, 0x00, 0x00,  # nonce, sessionAttributes, hmac
        0x01, 0x3b,                    # nvSize   : UINT16
        0x00, 0x00                     # nvOffset : UINT16
      ]
    nv_read[10:14] = index.to_bytes(4, byteorder='big')
    nv_read[14:18] = index.to_bytes(4, byteorder='big')
    nv_read[31:33] = size.to_bytes(2, byteorder='big')
    nv_read[33:35] = offset.to_bytes(2, byteorder='big')
    cmd = bytes(nv_read)
    response = tpm.command(cmd)
    if len(response) <= 16 or response.count(0) > 100:
        raise subcmd.TpmTestError('Unexpected G2F response: '
                                       + utils.hex_dump(response))
    return response

def g2f_read_check_cert(tpm):
    """Read G2F certificate from virtual NV read index

       This also triggers creation of U2F state if not present.

    Args:
        tpm: a tpm object used to communicate with the device

    Raises:
        subcmd.TpmTestError: on unexpected target responses
    """
    response = tpm2_nv_read(tpm, NV_INDEX_G2F_CERT, 0x13b, 0)
    if len(response) <= 16 or response.count(0) > 100:
        raise subcmd.TpmTestError('Unexpected G2F response: '
                                       + utils.hex_dump(response))
    return response

def tpm2_create_ek(tpm):
    """Send TPM2_CreatePrimary with Endorsement Key policy.

    Args:
        tpm: a tpm object used to communicate with the device

    Returns:
        object handle

    Raises:
        subcmd.TpmTestError: on unexpected target responses
    """
    create_primary_ek = [
        0x80, 0x02, # tag = TPM_ST_SESSIONS
        0x00, 0x00, 0x00, 0xa3, # size = 0xd7 = 215 - 52 = 163 = a3
        0x00, 0x00, 0x01, 0x31, # code = TPM_CC_CreatePrimary
        # ------
        0x40, 0x00, 0x00, 0x0B, # primary_handle =  TPM_RH_ENDORSEMENT
        # ~~~~~~
        0x00, 0x00, 0x00, 0x09, # auth size:
        0x40, 0x00, 0x00, 0x09, # session handle: TPM_RS_PW
        0x00, 0x00, # nonce: TPM2B
        0x00, # attributes
        0x00, 0x00, # password: TPM2B
        0x00, 0x04, # sensitive.size = 4
        0x00, 0x00, # sensitive.auth.size = 0
        0x00, 0x00, # sensitive.data.size = 0
        0x00, 0x7a, # public.size = 122
        0x00, 0x23, # public.type = TPM_ALG_ECC
        0x00, 0x0b, # public.name_alg = SHA256
        0x00, 0x03, 0x00, 0xb2, # attributes
        0x00, 0x20, # public.auth_policy.size = 32 (below)
        0x83, 0x71, 0x97, 0x67, 0x44, 0x84, 0xb3, 0xf8, 0x1a, 0x90, 0xcc, 0x8d,
        0x46, 0xa5, 0xd7, 0x24, 0xfd, 0x52, 0xd7, 0x6e, 0x06, 0x52, 0x0b, 0x64,
        0xf2, 0xa1, 0xda, 0x1b, 0x33, 0x14, 0x69, 0xaa,
        # ^^^ Above 32 bytes = TPM2_PolicySecret(TPM_RH_ENDORSEMENT)
        0x00, 0x06, # public.sym.alg = TPM_ALG_AES
        0x00, 0x80, # public.sym.key_bits = 128
        0x00, 0x43, # public.sym.mode = TPM_ALG_CFB
        0x00, 0x10, #
        0x00, 0x03, 0x00, 0x10, 0x00, 0x20, # public_area->unique.ecc.x.b.size
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x20, # public_area->unique.ecc.y.b.size
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, # outside_info.size = 0
        0x00, 0x00, 0x00, 0x00, # pcr_select.size = 0
    ]
    cmd = bytes(create_primary_ek)
    response = tpm.command(cmd)
    return response[10:14]

def tpm2_flush_context(tpm, handle: bytes):
    """Send TPM2_FlushContext command

    Args:
        tpm: a tpm object used to communicate with the device
        handle: 4-byte object handle to flush

    Raises:
        subcmd.TpmTestError: on unexpected target responses
    """
    flush_context = [
        0x80, 0x01, # tag: TPM_ST_NO_SESSIONS
        0x00, 0x00, 0x00, 0x0e, # size:
        0x00, 0x00, 0x01, 0x65, # command: TPM_CC_FlushContext
        0xFF, 0xFF, 0xFF, 0xFF, # handle: filled below
    ]
    flush_context[10:14] = bytearray(handle)
    cmd = bytes(flush_context)
    response = tpm.command(cmd)
    return response

def startup_test(tpm):
    """Run TPM2 startup/shutdown in a loop tests"""
    tpm2_startup(tpm, TPM_SU_CLEAR)
    tpm2_clear(tpm, TPM_RH_PLATFORM)
    tpm2_shutdown(tpm, TPM_SU_STATE)
    tpm2_startup(tpm, TPM_SU_CLEAR)
    tpm2_clear(tpm, TPM_RH_PLATFORM)
    handle = tpm2_create_ek(tpm)
    print('EK handle =', handle.hex())
    tpm2_flush_context(tpm, handle)
    tpm2_shutdown(tpm, TPM_SU_STATE)
    while True:
        print('Startup')
        tpm2_startup(tpm, TPM_SU_STATE)
        handle1 = tpm2_create_ek(tpm)
        handle2 = tpm2_create_ek(tpm)
        handle3 = tpm2_create_ek(tpm)
        print('EK handle =', handle3.hex())
        g2f_read_check_cert(tpm)
        tpm2_flush_context(tpm, handle3)
        tpm2_flush_context(tpm, handle2)
        tpm2_flush_context(tpm, handle1)
        tpm2_clear(tpm, TPM_RH_PLATFORM)
        print('Shutdown')
        tpm2_shutdown(tpm, TPM_SU_STATE)
        time.sleep(5)
