/*
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT license.
 */
use std::env;
use protoc_bin_vendored;

extern crate prost_build;

fn main() {
    env::set_var("PROTOC", protoc_bin_vendored::protoc_bin_path().unwrap());
    env::set_var("PROTOC_INCLUDE", protoc_bin_vendored::include_path().unwrap());
    prost_build::compile_protos(&["src/indexlog.proto"], &["src/"]).unwrap();
}
