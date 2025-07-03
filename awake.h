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
#define IDM_OPEN_POWER_MENU   1006
#define IDM_AUTO_START        1007

#define IDM_POWER_SCHEME_BASE 2000 // Base ID for dynamic power schemes
#define MAX_POWER_SCHEMES     32   // A reasonable limit for schemes

// A structure to hold cached information for a single power scheme
typedef struct
{
	BOOL active;
	GUID  guid;
	WCHAR name[256]; // friendly name
} AW_POWER_INFO;

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
	AW_POWER_INFO powerSchemes[MAX_POWER_SCHEMES];
	ULONG powerSchemeCount;
	WCHAR* uiText[COUNT_IDS_TXT];
} AWAKE_APP_STATE;

#define AW_STR(id) (g_awState.uiText[(id) - MIN_IDS_TXT])

extern AWAKE_APP_STATE g_awState;
