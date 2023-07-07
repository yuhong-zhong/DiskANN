/*
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT license.
 */
#![cfg_attr(
    not(test),
    warn(clippy::panic, clippy::unwrap_used, clippy::expect_used)
)]


pub mod message {
    include!(concat!(env!("OUT_DIR"), "/diskann_logger.rs"));
}

pub use self::telemetry::TelemetryProcessor;
