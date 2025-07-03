// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "resource.h"

#define AW_DESC L"Power Control Utility"

// Define a custom message for the notification icon
#define WM_TRAYICON (WM_APP + 1)

// Define menu item identifiers
#define IDM_KEEP_AWAKE        1001
#define IDM_KEEP_SCREEN_ON    1002
#define IDM_TURN_OFF_SCREEN   1003
#define IDM_SLEEP             1004
#define IDM_EXIT              1005

// Global structure to hold all application state.
// This is the only global variable as requested.
typedef struct
{
	HWND  hMainWnd;
	HICON hMainIcon;
	HICON hIconDisabled;
	HICON hIconScreenOn;
	HICON hIconEnabled;
	HMENU hMenu;
	BOOL  isKeepingAwake;
	BOOL  isKeepingScreenOn;
	WCHAR* uiText[COUNT_IDS_TXT];
} AWAKE_APP_STATE;

#define AW_STR(id) (g_awState.uiText[(id) - MIN_IDS_TXT])

extern AWAKE_APP_STATE g_awState;
