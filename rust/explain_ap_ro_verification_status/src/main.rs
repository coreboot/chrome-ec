// Copyright 2023 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//! Explains AP RO verification status codes from plain integers.
//!
//! AP RO verification result integers take two forms:
//!
//! - A `u32` derived from `ApRoVerificationResult` in capsules.
//!   This puts a `VerifyError` in the top 8 bits, and stores extra
//!   detail in the lower 24 bits. This is what is found in the
//!   Platform.TPM.ExpandedApRoVerificationStatus UMA metric.
//!
//! - A `u8` derived from `ApRoVerificationTpmvStatus`, in syscalls.
//!   This is the output of `gsctool -B` and found in the
//!   Platform.Ti50.ARVStatus metric. It follows a different set of
//!   statuses from Cr50, but is in the same namespace.

use ap_ro_errs::{
    ApRoVerificationResult, ApRoVerificationTpmvStatus, BadValue, CryptoAlgorithmSource,
    DigestLocation, InternalErrorSource, Result, SignatureLocation, VerifyError, VerifyErrorCode,
    VersionMismatchSource,
};
use std::fmt::Display;

#[derive(Debug)]
enum ExplainError {
    WrongArgCount,
    InvalidArg,
}

impl ExplainError {
    fn to_str(self) -> &'static str {
        match self {
            ExplainError::WrongArgCount => "Expected a single arg, hex or decimal",
            ExplainError::InvalidArg => {
                "Unrecognized argument format, expected a single hex or decimal argument"
            }
        }
    }
}
fn parse_arg(mut arg: &str) -> core::result::Result<u32, ExplainError> {
    if let Some(stripped) = arg.strip_prefix("0x").or_else(|| arg.strip_prefix('x')) {
        arg = stripped;
    } else {
        // First try to parse as base-10, unless there's a hex prefix.
        if let Ok(out) = arg.parse() {
            return Ok(out);
        } else if let Ok(out) = arg.parse::<i32>() {
            // Try parse negative numbers, and if successful, cast to u32
            return Ok(out as u32);
        }
    }
    // If parsing as base-10 failed, try base-16.
    u32::from_str_radix(arg, 16).map_err(|_| ExplainError::InvalidArg)
}

fn get_code_from_args() -> core::result::Result<u32, ExplainError> {
    let mut args = std::env::args();
    let _app_name = args.next();
    let Some(out) = args.next() else {
        return Err(ExplainError::WrongArgCount);
    };
    if args.next().is_some() {
        return Err(ExplainError::WrongArgCount);
    }
    parse_arg(&out)
}

fn expected_less_detail(detail: &[u8], at: &str) {
    println!(
        "This tool expects the expanded status to have no detail in the {at},\n\
        but instead it has {detail:#02x?}.\n\
        Consider updating this tool."
    );
}

fn explain_code(code: u32) {
    /// Bit that indicates that AP RO verification has succeeded at least once on device.
    const LATCH_SUCCESS: u32 = 0x80000000;

    println!("Code: 0x{code:08x}");
    if let Ok(bottom) = u8::try_from(code) {
        println!("This is likely a status code from TPM");
        let text = ApRoVerificationTpmvStatus::from_u8(bottom)
            .map_or("Unknown to this tool", |x| x.to_str());
        println!("Description: {text}");
        return;
    }
    if code == ApRoVerificationResult::from(Ok(())).into() {
        println!("It is a SUCCESS status");
        return;
    }
    // Remove latch value before trying to parse the result
    let result = ApRoVerificationResult::from(code & !LATCH_SUCCESS);
    let err = Result::from(result).expect_err("Already checked for success.");
    if err == VerifyError::Internal(InternalErrorSource::CouldNotDeserialize, 0) {
        println!("The code is not recognized by this tool");
        return;
    }
    println!("This is likely an expanded error code");
    if code & LATCH_SUCCESS != 0 {
        println!("THIS ERROR OCURRED AFTER SUCCESS WAS LATCHED!")
    }
    let code = err.code();
    let detail = err.detail();

    // TODO(kupiakos): Instead of displaying `Debug` for codes, display the
    //                 contents of the docstrings.
    //                 This would be best done with a proc macro on the enums.

    println!("Top level code: {:?} ({})", code, code as u8);
    match code {
        VerifyErrorCode::TooBig
        | VerifyErrorCode::InconsistentGscvd
        | VerifyErrorCode::InconsistentKeyblock
        | VerifyErrorCode::MissingGscvd
        | VerifyErrorCode::SettingNotProvisioned
        | VerifyErrorCode::OutOfMemory
        | VerifyErrorCode::InconsistentKey
        | VerifyErrorCode::SpiRead
        | VerifyErrorCode::NonZeroGbbFlags
        | VerifyErrorCode::WrongRootKey => {
            if let [0, 0, 0] = detail {
                println!("This expanded status carries no extra detail as expected.");
            } else {
                expected_less_detail(&detail, "bottom 24 bits");
            }
        }

        VerifyErrorCode::FailedVerification => {
            let top = detail[0];
            if top == 0 {
                println!("Failed due to signature verification failure.");
                let sig_location = detail[1];
                if let Some(sig_location) = SignatureLocation::from_u8(sig_location) {
                    println!("Where was the failing signature: {sig_location:?}");
                } else {
                    println!("Unrecognized signature location ({sig_location})");
                }

                if detail[2] != 0 {
                    expected_less_detail(&detail[2..], "bottom 8 bits");
                }
                return;
            }
            println!("Failed due to a digest mismatch with the contents in AP flash.");
            let num_root_key_hashes = (top >> 4) - 1;
            println!(
                "The Ti50 image this came from had {num_root_key_hashes} hardcoded root key hashes."
            );
            let digest_location = top & 0x0F;
            if let Some(digest_location) = DigestLocation::from_u8(digest_location) {
                println!("Which digest failed: {digest_location:?}");
            } else {
                println!("Unrecognized digest location ({digest_location})");
            }
            let [_, got, expected] = detail;
            println!("The first octet of the calculated hash: 0x{got:02x}");
            println!("The first octet of the expected hash:   0x{expected:02x}");
        }
        VerifyErrorCode::VersionMismatch => {
            let [source, none0, none1] = detail;
            if none0 != 0 || none1 != 0 {
                expected_less_detail(&[none0, none1], "bottom 16 bits");
            }
            if let Some(known) = VersionMismatchSource::from_u8(source) {
                println!("Version mismatch source: {:?}", known);
            } else {
                println!("Unknown version mismatch source ({})", source);
            }
        }
        VerifyErrorCode::FailedStatusRegister1
        | VerifyErrorCode::FailedStatusRegister2
        | VerifyErrorCode::FailedStatusRegister3 => {
            let register_index = match code {
                VerifyErrorCode::FailedStatusRegister1 => 1,
                VerifyErrorCode::FailedStatusRegister2 => 2,
                VerifyErrorCode::FailedStatusRegister3 => 3,
                _ => unreachable!(),
            };
            let [got, value, mask] = detail;
            println!("Status Register {register_index} did not have an expected value.");
            println!("The flash chip on the system returned 0x{got:02x}");
            println!("The provisioned value is: 0x{value:02x}");
            println!("The provisioned mask is:  0x{mask:02x}");
            // Mask of 0 is a special case for an error getting provisioned value
            if mask == 0 {
                if let Some(e) = BadValue::from_u8(value) {
                    println!(
                        "Error getting provisioned value due to it being {}",
                        e.to_string()
                    );
                } else {
                    println!("Unknown error getting provision value: {value}");
                }
            } else if got & mask == value & mask {
                println!("This doesn't make sense, because it should have passed.");
            } else {
                println!(
                    "It failed because 0x{got:02x} & 0x{mask:02x} != 0x{value:02x} & 0x{mask:02x}"
                );
            }
        }
        VerifyErrorCode::UnsupportedCryptoAlgorithm => {
            let [source, hi, lo] = detail;
            let unknown_code = u16::from_be_bytes([hi, lo]);
            println!("An unknown crypto algorithm was encountered: {unknown_code}");
            if let Some(source) = CryptoAlgorithmSource::from_u8(source) {
                println!("Location of unknown crypto algorithm: {source:?}");
            } else {
                println!("Unknown crypto algorithm location ({source})");
            }
        }
        VerifyErrorCode::Internal => {
            let [source, hi, lo] = detail;
            let Some(source) = InternalErrorSource::from_u8(source) else {
                println!("Unknown internal error source ({source})");
                return;
            };
            let code = u16::from_be_bytes([hi, lo]);
            match source {
                InternalErrorSource::Kernel => {
                    println!("Internal error caused by the kernel");
                    let code_str = match code {
                        1 => Some("FAIL"),
                        2 => Some("BUSY"),
                        3 => Some("ALREADY"),
                        4 => Some("OFF"),
                        5 => Some("RESERVE"),
                        6 => Some("INVAL"),
                        7 => Some("SIZE"),
                        8 => Some("CANCEL"),
                        9 => Some("NOMEM"),
                        10 => Some("NOSUPPORT"),
                        11 => Some("NODEVICE"),
                        12 => Some("UNINSTALLED"),
                        13 => Some("NOACK"),
                        _ => None,
                    };
                    if let Some(code_str) = code_str {
                        println!("Kernel error code {code} ({code_str})");
                    } else {
                        println!("Unknown kernel error code {code}");
                    }
                }
                InternalErrorSource::Crypto => {
                    println!("Error caused by the crypto engine");
                    let code = code as i16 as i32;
                    println!("Raw code: {code}");
                    println!("TODO: print out friendlier status, requires `enum_as` rework");
                }
                InternalErrorSource::VerifyFlashRanges => {
                    println!("Error occurred while verifying flash ranges");
                }
                InternalErrorSource::CouldNotDeserialize => {
                    unreachable!("Handled by \"code is not recognized by this tool\"")
                }
            }
        }
        VerifyErrorCode::BoardIdMismatch => {
            let [got_byte, got_nibble_expected_nibble, expected_byte] = detail;
            let got = (got_byte as u32) << 4 | (got_nibble_expected_nibble as u32) >> 4;
            let expected =
                ((got_nibble_expected_nibble & 0x0f) as u32) << 8 | (expected_byte as u32);

            struct Display3BoardIdNibbles(u32);
            impl Display for Display3BoardIdNibbles {
                fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
                    use core::fmt::Write as _;

                    write!(f, "{:03x} (first char: ", self.0)?;
                    for b in std::ascii::escape_default((self.0 >> 4) as u8) {
                        f.write_char(b.into())?;
                    }
                    if self.0 == 0x5a5 {
                        write!(f, ", probably ZZCR")?;
                    }
                    write!(f, ")")
                }
            }

            println!("There was a mismatch between the board ID in the GSCVD and on the board.");
            println!("The detailed code carries the top 12 bits of the ID on AP flash and GSC.");
            println!("In GSCVD: {}", Display3BoardIdNibbles(got));
            println!("On GSC:   {}", Display3BoardIdNibbles(expected));
        }
    }
}

fn main() {
    if let Err(e) = get_code_from_args().map(explain_code) {
        println!("{}", e.to_str());
    }
}
