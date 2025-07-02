// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#define QUOTE_(x) #x
#define QUOTE(x) QUOTE_(x)

#define AW_MAJOR_VERSION 0
#define AW_MINOR_VERSION 0
#define AW_MICRO_VERSION 1
#define AW_BUILD_VERSION 0

#define AW_VERSION      AW_MAJOR_VERSION,AW_MINOR_VERSION,AW_MICRO_VERSION,AW_BUILD_VERSION
#define AW_VERSION_STR  QUOTE(AW_MAJOR_VERSION.AW_MINOR_VERSION.AW_MICRO_VERSION.AW_BUILD_VERSION)

#define AW_COMPANY      "A1ive"
#define AW_COPYRIGHT    "Copyright (c) 2025 A1ive"
#define AW_FILEDESC     "Power Control Utility"
#define AW_NAME         "WinAwake"
