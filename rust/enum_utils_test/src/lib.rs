// Copyright 2023 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

use enum_utils::enum_as;
use enum_utils::passthru_to;

#[derive(Debug)]
#[enum_as(u8)]
pub enum TestU8 {
    /// One Value
    One = 1,
    // Eight with other kind of comment
    Eight = 0o10,
    ThirtyTwo = 0x20, // inline comment afterwards
}

#[test]
fn test_u8_enum_conversion() {
    assert_eq!(TestU8::from_u8(1), Some(TestU8::One));
    assert_eq!(TestU8::from_u8(8), Some(TestU8::Eight));
    assert_eq!(TestU8::from_u8(32), Some(TestU8::ThirtyTwo));
    assert_eq!(TestU8::END, 33);

    for i in 0..u8::MAX {
        if matches!(i, 1 | 8 | 32) {
            continue;
        }
        assert!(TestU8::from_u8(i).is_none());
    }
}

#[test]
fn test_u8_enum_equals() {
    assert_eq!(TestU8::One, TestU8::One);
    assert_eq!(TestU8::Eight, TestU8::Eight);
    assert_eq!(TestU8::ThirtyTwo, TestU8::ThirtyTwo);

    assert_ne!(TestU8::One, TestU8::Eight);
    assert_ne!(TestU8::Eight, TestU8::ThirtyTwo);
    assert_ne!(TestU8::ThirtyTwo, TestU8::One);
}

#[derive(Debug)]
#[enum_as(u16)]
pub enum TestU16 {
    Zero,
    One,
    Ten = 10,
    Max = 0xffff,
}

#[test]
fn test_u16_enum_conversion() {
    assert_eq!(TestU16::from_u16(0), Some(TestU16::Zero));
    assert_eq!(TestU16::from_u16(1), Some(TestU16::One));
    assert_eq!(TestU16::from_u16(10), Some(TestU16::Ten));
    assert_eq!(TestU16::from_u16(u16::MAX), Some(TestU16::Max));
    assert_eq!(TestU16::END, (u16::MAX as usize) + 1);

    for i in 0..u16::MAX {
        if matches!(i, 0 | 1 | 10) {
            continue;
        }
        assert!(TestU16::from_u16(i).is_none());
    }
}

#[test]
fn test_u16_enum_equals() {
    assert_eq!(TestU16::Zero, TestU16::Zero);
    assert_eq!(TestU16::One, TestU16::One);
    assert_eq!(TestU16::Ten, TestU16::Ten);

    assert_ne!(TestU16::Zero, TestU16::One);
    assert_ne!(TestU16::One, TestU16::Ten);
    assert_ne!(TestU16::Ten, TestU16::Zero);
}

#[derive(Debug)]
#[enum_as(usize)]
pub enum TestUsize {
    Zero,
    One,
    Two,
}

#[test]
fn test_usize_enum_conversion() {
    assert_eq!(TestUsize::from_usize(0), Some(TestUsize::Zero));
    assert_eq!(TestUsize::from_usize(1), Some(TestUsize::One));
    assert_eq!(TestUsize::from_usize(2), Some(TestUsize::Two));
    assert_eq!(TestUsize::END, 3);

    // Too expensive to test all cases
    for i in 3..100 {
        assert!(TestUsize::from_usize(i).is_none());
    }
    assert!(TestUsize::from_usize(usize::MAX).is_none());
}

#[test]
fn test_usize_enum_equals() {
    assert_eq!(TestUsize::Zero, TestUsize::Zero);
    assert_eq!(TestUsize::One, TestUsize::One);
    assert_eq!(TestUsize::Two, TestUsize::Two);

    assert_ne!(TestUsize::Zero, TestUsize::One);
    assert_ne!(TestUsize::One, TestUsize::Two);
    assert_ne!(TestUsize::Two, TestUsize::Zero);
}

#[derive(Debug)]
#[enum_as(u64)]
pub enum TestU64 {
    Zero,
    MaxU32 = 0xFFFF_FFFF,
    U64,
}

#[test]
fn test_u64_enum_conversion() {
    assert_eq!(TestU64::from_u64(0), Some(TestU64::Zero));
    assert_eq!(TestU64::from_u64(0xFFFF_FFFF), Some(TestU64::MaxU32));
    assert_eq!(TestU64::from_u64(0x1_0000_0000), Some(TestU64::U64));
    assert_eq!(TestU64::END, 0x1_0000_0001);

    // Too expensive to test all cases
    for i in 1..100 {
        assert!(TestU64::from_u64(i).is_none());
    }
    assert!(TestU64::from_u64(u64::MAX).is_none());
}

#[test]
fn test_u64_enum_equals() {
    assert_eq!(TestU64::Zero, TestU64::Zero);
    assert_eq!(TestU64::MaxU32, TestU64::MaxU32);
    assert_eq!(TestU64::U64, TestU64::U64);

    assert_ne!(TestU64::Zero, TestU64::MaxU32);
    assert_ne!(TestU64::MaxU32, TestU64::U64);
    assert_ne!(TestU64::U64, TestU64::Zero);
}

#[derive(Debug)]
#[enum_as(i32)]
pub enum TestI32 {
    NegativeFour = -4,
    NegativeOne = -1,
    One = 1,
    Two = 2,
}

#[test]
fn test_ui32_enum_conversion() {
    assert_eq!(TestI32::from_i32(-4), Some(TestI32::NegativeFour));
    assert_eq!(TestI32::from_i32(-1), Some(TestI32::NegativeOne));
    assert_eq!(TestI32::from_i32(1), Some(TestI32::One));
    assert_eq!(TestI32::from_i32(2), Some(TestI32::Two));
    assert_eq!(TestI32::END, 3);

    // Too expensive to test all cases
    for i in [-6, -5, -3, -2, 0, 3, 4] {
        assert!(TestI32::from_i32(i).is_none());
    }
}

#[test]
fn test_ui32_enum_equals() {
    assert_eq!(TestI32::NegativeFour, TestI32::NegativeFour);
    assert_eq!(TestI32::NegativeOne, TestI32::NegativeOne);
    assert_eq!(TestI32::One, TestI32::One);
    assert_eq!(TestI32::Two, TestI32::Two);

    assert_ne!(TestI32::NegativeFour, TestI32::NegativeOne);
    assert_ne!(TestI32::NegativeFour, TestI32::One);
    assert_ne!(TestI32::NegativeFour, TestI32::Two);
    assert_ne!(TestI32::One, TestI32::Two);
}

pub struct Inner(usize);

impl Inner {
    pub fn increment(&mut self, by: usize) -> usize {
        self.0 += by;
        self.0
    }
    pub fn read(&self) -> usize {
        self.0
    }
}

pub struct Outer {
    pub inner: Inner,
}

impl Outer {
    #[passthru_to(inner)]
    pub fn increment(&mut self, by: usize) -> usize {}
    #[passthru_to(inner)]
    pub fn read(&self) -> usize {}
}

#[test]
fn test_passthru() {
    let mut outer = Outer { inner: Inner(1) };

    assert_eq!(outer.increment(4), 5);
    assert_eq!(outer.increment(5), 10);
    assert_eq!(outer.read(), 10);
}
