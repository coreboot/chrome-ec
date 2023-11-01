// Copyright 2023 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
use std::cmp::Ordering;

use proc_macro::*;

fn get_enum_and_stream(stream: TokenStream) -> Option<(TokenTree, TokenStream)> {
    let mut enum_name = None;
    let mut cap_next = false;
    for tree in stream {
        match tree {
            TokenTree::Ident(i) if i.to_string() == "enum" => cap_next = true,
            TokenTree::Ident(i) if cap_next => {
                enum_name = Some(TokenTree::Ident(i));
                cap_next = false;
            }
            TokenTree::Group(g) => {
                if let Some(name) = enum_name {
                    return Some((name, g.stream()));
                } else {
                    let result = get_enum_and_stream(g.stream());
                    if result.is_some() {
                        return result;
                    }
                };
            }
            _ => {}
        }
    }
    None
}

fn generate_enum_array(name: TokenStream, mut item: TokenStream, header: &str) -> TokenStream {
    let (enum_name, enum_stream) = get_enum_and_stream(item.clone()).expect("Must use on enum");
    // Check that enums do not have associated fields, but see still want to all doc comments.
    for tree in enum_stream.clone() {
        match tree {
            TokenTree::Group(group) => match group.stream().into_iter().next() {
                Some(TokenTree::Ident(ident)) if ident.to_string() == "doc" => {
                    // This is a doc comment; this is the only allowed group right now since we
                    // do not support associated fields on any enum value
                }
                _ => panic!("Only enums without associated fields are supported"),
            },
            _ => {
                // If no groups, then this enum should just be a normal "C-Style" enum.
            }
        }
    }

    let qualified_list = enum_stream
        .into_iter()
        .map(|tree| {
            if let TokenTree::Ident(det_name) = tree {
                format!("{}::{},", enum_name, det_name).parse().unwrap()
            } else {
                TokenStream::new()
            }
        })
        .collect::<TokenStream>();
    let array_stream: TokenStream = format!(
        r#"{header}
        pub const {name}: &[{enum_name}] = &[{qualified_list}];"#,
        header = header,
        name = name,
        enum_name = enum_name,
        qualified_list = qualified_list,
    )
    .parse()
    .unwrap();

    // Emitted exactly what was written, then add the test array
    item.extend(array_stream);
    item
}

/// Generates an test cfg array with the specified name containing all enum values. This is only
/// valid for enums where all the variants do not have fields associated with them.
/// Access to array is possible only in test builds
///
/// # Example
///
/// ```
/// # #[macro_use] extern crate enum_utils;
/// #[gen_test_enum_array(MyTestArrayName)]
/// enum MyEnum {
///     ValueOne,
///     ValueTwo,
/// }
///
/// #[cfg(test)]
/// mod tests {
///     #[test]
///     fn test_two_values() {
///         assert_eq!(MyTestArrayName.len(), 2);
///     }
/// }
/// ```
#[proc_macro_attribute]
pub fn gen_test_enum_array(name: TokenStream, item: TokenStream) -> TokenStream {
    generate_enum_array(name, item, "#[cfg(test)]")
}

/// Generates an array with the specified name containing all enum values. This is only valid for
/// enums where all the variants do not have fields associated with them.
///
/// # Example
///
/// ```
/// # #[macro_use] extern crate enum_utils;
/// #[gen_enum_array(MyArrayName)]
/// pub enum MyEnum {
///     ValueOne,
///     ValueTwo,
/// }
///
/// fn check() {
///         assert_eq!(MyArrayName.len(), 2);
/// }
/// ```
#[proc_macro_attribute]
pub fn gen_enum_array(name: TokenStream, item: TokenStream) -> TokenStream {
    generate_enum_array(name, item, "")
}

/// Generates an impl to_string for enum which returns &str.
/// Method is implemended using match for every enum variant.
/// Valid for enums where all the variants do not have fields associated with them.
///
/// # Example
///
/// ```
/// # #[macro_use] extern crate enum_utils;
/// #[gen_to_string]
/// enum MyEnum {
///     ValueOne,
///     ValueTwo,
/// }
///
/// fn main() {
///     let e = MyEnum::ValueOne;
///     println!("{}", e.to_string());
/// }
///
/// ```
#[proc_macro_attribute]
pub fn gen_to_string(_attr: TokenStream, mut input: TokenStream) -> TokenStream {
    let (enum_name, enum_stream) = get_enum_and_stream(input.clone()).expect("Must use on enum");

    let mut match_arms = TokenStream::new();
    let enums_items = enum_stream.into_iter().filter_map(|tt| match tt {
        TokenTree::Ident(id) => Some(id.to_string()),
        _ => None,
    });
    for item in enums_items {
        let arm: TokenStream = format!(
            r#"{enum_name}::{item} => "{item}","#,
            enum_name = enum_name,
            item = item,
        )
        .parse()
        .unwrap();
        match_arms.extend(arm);
    }

    let implementation: TokenStream = format!(
        r#"impl {enum_name} {{
        pub fn to_string(&self) -> &'static str {{
            match *self {{
                {match_arms}
            }}
        }}
    }}"#,
        enum_name = enum_name,
        match_arms = match_arms
    )
    .parse()
    .unwrap();

    // Emit input as is and add the to_string implementation
    input.extend(implementation);
    input
}

fn parse_to_i128(val: &str, negative: bool) -> i128 {
    let (first_pos, base) = match val.get(0..2) {
        Some("0x") => (2, 16),
        Some("0o") => (2, 8),
        Some("0b") => (2, 2),
        _ => (0, 10),
    };

    let sign = if negative { -1 } else { 1 };

    // Remove any helper _ in the string literal, then convert from base
    sign * i128::from_str_radix(
        &val[first_pos..]
            .chars()
            .filter(|c| *c != '_')
            .collect::<String>(),
        base,
    )
    .unwrap_or_else(|_| panic!("Invalid number {}", val))
}

/// Generates a exclusive `END` const and `from_<repr>` function for an enum
///
/// # Example
///
/// ```
/// # #[macro_use] extern crate enum_utils;
/// #[enum_as(u8)]
/// enum MyEnum {
///     ValueZero,
///     ValueOne,
/// }
///
/// # fn main() {
/// assert!(matches!(MyEnum::from_u8(1), Some(MyEnum::ValueOne)));
/// assert_eq!(MyEnum::END, 2);
/// # }
///
/// ```
#[proc_macro_attribute]
pub fn enum_as(repr: TokenStream, input: TokenStream) -> TokenStream {
    let repr = repr.to_string();
    let (enum_name, enum_stream) = get_enum_and_stream(input.clone()).expect("Must use on enum");

    #[derive(Debug, Copy, Clone)]
    enum WantState {
        Determinant,
        Punc,
        NegativeVal,
        Val,
        CommentBlock,
    }
    let mut state = WantState::Determinant;
    let mut skipped_ranges = vec![];
    let mut start: Option<i128> = None;
    let mut end: Option<i128> = None;
    for tt in enum_stream {
        use WantState::*;
        state = match (tt, state) {
            (TokenTree::Ident(_), Determinant) => Punc,
            // If we are expecting a Determinant, but get a # instead, it must be a comment
            (TokenTree::Punct(p), Determinant) if p.as_char() == '#' => CommentBlock,
            (TokenTree::Punct(p), Val) if p.as_char() == '-' => NegativeVal,
            (TokenTree::Group(_), CommentBlock) => Determinant,
            (TokenTree::Punct(p), Punc) => match p.as_char() {
                ',' => {
                    start = Some(start.unwrap_or(0));
                    end = Some(end.unwrap_or_else(|| start.unwrap()) + 1);
                    Determinant
                }
                '=' => Val,
                other => panic!("Unexpected punctuation '{}'", other),
            },
            (TokenTree::Literal(l), Val | NegativeVal) => {
                let val = parse_to_i128(&l.to_string(), matches!(state, NegativeVal));
                start = Some(start.unwrap_or(val));
                let expected = end.unwrap_or_else(|| start.unwrap());
                match val.cmp(&expected) {
                    Ordering::Greater => {
                        skipped_ranges.push((expected, val));
                        end = Some(val);
                    }
                    Ordering::Less => panic!("Discriminants must increase in value"),
                    Ordering::Equal => (),
                }
                Punc
            }
            (tt, want) => {
                panic!("Want {:?} but got {:?}", want, tt)
            }
        };
    }

    let skipped_ranges = if skipped_ranges.is_empty() {
        "".to_string()
    } else {
        format!(
            r#"for r in &{ranges:?} {{
                if (r.0..r.1).contains(&val) {{
                    return None;
                }}
            }}"#,
            ranges = skipped_ranges
        )
    };

    // Ensure that there is at least one discriminant
    let start = start.expect("Enum needs at least one discriminant");
    let end = end.expect("Enum needs at least one discriminant");

    // Ensure that END will fit into usize or u64
    u64::try_from(end).unwrap_or_else(|_| panic!("Value after last discriminant must be unsigned"));

    let implementation: TokenStream = format!(
        r#"impl {enum_name} {{
            pub const END: {end_type} = {end};

            pub fn from_{repr}(val: {repr}) -> Option<Self> {{
                if val < {start} {{
                    return None;
                }}
                if val > ((Self::END - 1) as {repr}) {{
                    return None;
                }}
                {skipped_ranges}
                Some( unsafe {{ core::mem::transmute(val) }})
            }}
        }}

        impl PartialEq for {enum_name} {{
            fn eq(&self, other: &Self) -> bool {{
                *self as {repr} == *other as {repr}
            }}
        }}

        impl Eq for {enum_name} {{ }}

        #[cfg(not(target_arch = "riscv32"))]
        impl core::hash::Hash for {enum_name} {{
            fn hash<H: core::hash::Hasher>(&self, state: &mut H) {{
                (*self as {repr}).hash(state);
            }}
         }}"#,
        enum_name = enum_name,
        start = start,
        end = end,
        end_type = if repr == "u64" { "u64" } else { "usize " },
        repr = repr,
        skipped_ranges = skipped_ranges,
    )
    .parse()
    .unwrap();

    // Attribute input with a repr, Clone, Clone, and allow(dead_code), then add the custom
    // implementation. We allow dead code since the these enums are typically interface enums that
    // define an API boundary.
    let mut res: TokenStream = format!(
        "#[repr({})]\n#[derive(Copy, Clone)]\n#[allow(dead_code)]",
        repr
    )
    .parse()
    .unwrap();
    res.extend(input);
    res.extend(implementation);
    res
}

fn get_param_list_without_self(params: TokenStream) -> String {
    let mut result = String::new();

    #[derive(Debug, Copy, Clone)]
    enum WantState {
        SelfParam,
        FirstIdentifier,
        Comma,
    }
    let mut state = WantState::SelfParam;
    for tt in params {
        use WantState::*;
        state = match (tt, state) {
            (TokenTree::Ident(ident), SelfParam) if ident.to_string() == "self" => FirstIdentifier,
            (TokenTree::Ident(ident), FirstIdentifier) => {
                result.push_str(&ident.to_string());
                Comma
            }
            (TokenTree::Punct(p), Comma) if p.to_string() == "," => {
                result.push(',');
                FirstIdentifier
            }
            (_, other) => {
                // Do nothing; keep watching for what we are looking for
                other
            }
        };
    }

    result
}

/// Replaces the function body with a single statement that pass all parameters thru to same
/// function name on the variable specified in the passthru_to macro
///
/// # Example
///
/// ```
/// # #[macro_use] extern crate enum_utils;
/// pub struct Inner(usize);
///
/// impl Inner {
///     pub fn read_plus(&self, plus: usize) -> usize {
///         self.0 + plus
///     }
/// }
///
/// pub struct Outer {
///     pub my_inner: Inner,
/// }
///
/// impl Outer {
///     #[passthru_to(my_inner)]
///     pub fn read_plus(&self, plus: usize) -> usize {}
/// }
///
/// ```
#[proc_macro_attribute]
pub fn passthru_to(passthru_var: TokenStream, input: TokenStream) -> TokenStream {
    #[derive(Debug, Copy, Clone)]
    enum WantState {
        FunctionKeyword,
        FunctionName,
        Parameters,
        Body,
        End,
    }
    let mut state = WantState::FunctionKeyword;
    let mut name = None;
    let mut params = None;
    let mut body_num = None;
    for (i, tt) in input.clone().into_iter().enumerate() {
        use WantState::*;
        state = match (tt, state) {
            (TokenTree::Ident(ident), FunctionKeyword) if ident.to_string() == "fn" => FunctionName,
            (_, FunctionKeyword) => FunctionKeyword,
            (TokenTree::Ident(ident), FunctionName) => {
                name = Some(ident.to_string());
                Parameters
            }
            (TokenTree::Group(group), Parameters) => {
                params = Some(get_param_list_without_self(group.stream()));
                Body
            }
            (TokenTree::Group(group), Body) if group.delimiter() == Delimiter::Brace => {
                body_num = Some(i);
                End
            }
            (_, Body) => Body,
            (tt, want) => {
                panic!("Want {:?} but got {:?}", want, tt)
            }
        };
    }

    let implementation: TokenStream = format!(
        r#"{{ self.{passthru_var}.{name}({params}) }}"#,
        passthru_var = passthru_var,
        name = name.expect("Cannot find function name"),
        params = params.expect("Could find parameters"),
    )
    .parse()
    .unwrap();

    // Take everything up to body, then replace body with the passthru implementation
    input
        .into_iter()
        .take(body_num.expect("Cannot find body"))
        .chain(implementation)
        .collect()
}
