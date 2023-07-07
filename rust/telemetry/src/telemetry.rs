/*
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT license.
 */

use crate::telemetry::message::Log;

trait TelemetryProcessor {
    fn emit(&self, message: Log) -> Result<(), LogError>;
}