// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ArduinoString.h"
#include "Common.h"
#include "Stream.h"
#include "debug.h"
#include "math.h"
#include "pgmspace.h"

#include <assert.h>
#include <stdexcept>
#include <stdint.h>
#include <string>

#define A0 0

#define sem_acquire_blocking(sem)
#define sem_release(sem)

using namespace arduino;
