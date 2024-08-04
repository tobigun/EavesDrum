// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "log.h"

// uncomment to enable debug mode
//#define DEBUG_DRUM
#ifdef DEBUG_DRUM
#define DEBUG_PRINTF SerialDebug.printf
#else
#define DEBUG_PRINTF(...)
#endif
