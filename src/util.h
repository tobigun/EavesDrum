// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#define ENUM_TO_STRING(v) (#v)
#define MAP_ENUM_TO_STRING(v) \
  if (value == v) \
    return ENUM_TO_STRING(v);

#define MAP_STRING_TO_ENUM(v) \
  if (value.equalsIgnoreCase(ENUM_TO_STRING(v))) \
    return v;
