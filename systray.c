// SPDX-License-Identifier: GPL-3.0-or-later

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#include "awake.h"
#include "utils.h"
#include "systray.h"

// Add the icon to the system tray
void AW_AddTrayIcon(HWND hWnd)
{
	NOTIFYICONDATAW nid = { 0 };
	nid.cbSize = sizeof(NOTIFYICONDATAW);
	nid.hWnd = hWnd;
	nid.uID = 1; // Unique ID for the icon
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = WM_TRAYICON;
	nid.hIcon = g_awState.hIconDisabled;
	wcscpy_s(nid.szTip, ARRAYSIZE(nid.szTip), AW_DESC);

	Shell_NotifyIconW(NIM_ADD, &nid);
}

// Remove the icon from the system tray
void AW_RemoveTrayIcon(HWND hWnd)
{
	NOTIFYICONDATAW nid = { 0 };
	nid.cbSize = sizeof(NOTIFYICONDATAW);
	nid.hWnd = hWnd;
	nid.uID = 1;
	Shell_NotifyIconW(NIM_DELETE, &nid);
}

// Choose and set the appropriate tray icon based on current state.
void AW_UpdateTrayIcon(HWND hWnd)
{
	NOTIFYICONDATAW nid = { 0 };
	nid.cbSize = sizeof(NOTIFYICONDATAW);
	nid.hWnd = hWnd;
	nid.uID = 1;
	nid.uFlags = NIF_ICON | NIF_TIP;

	// Determine which icon to display.
	if (g_awState.isKeepingAwake)
	{
		if (g_awState.isKeepingScreenOn)
		{
			wcscpy_s(nid.szTip, ARRAYSIZE(nid.szTip), AW_STR(IDS_MENU_KEEP_SCREEN_ON));
			nid.hIcon = g_awState.hIconScreenOn;
		}
		else
		{
			wcscpy_s(nid.szTip, ARRAYSIZE(nid.szTip), AW_STR(IDS_MENU_KEEP_AWAKE));
			nid.hIcon = g_awState.hIconEnabled;
		}
	}
	else
	{
		wcscpy_s(nid.szTip, ARRAYSIZE(nid.szTip), AW_DESC);
		nid.hIcon = g_awState.hIconDisabled;
	}

	// Modify the existing notification icon.
	Shell_NotifyIconW(NIM_MODIFY, &nid);
}

// Populate a menu with power schemes read from the global cache.
static void PopulatePowerSchemesMenu(HMENU hSubMenu)
{
	for (ULONG i = 0; i < g_awState.powerSchemeCount; i++)
	{
		UINT flags = MF_STRING;
		// Check if the cached scheme is the active one
		if (g_awState.powerSchemes[i].active)
			flags |= MF_CHECKED;

		// The menu ID corresponds to the scheme's index in our cache array.
		AppendMenuW(hSubMenu, flags, IDM_POWER_SCHEME_BASE + i, g_awState.powerSchemes[i].name);
	}
}

// Create and display the right-click context menu
void AW_ShowTrayMenu(HWND hWnd)
{
	POINT pt;
	GetCursorPos(&pt);

	g_awState.hMenu = CreatePopupMenu();
	if (g_awState.hMenu)
	{
		// Add menu items, checking state for the toggles
		UINT uFlags = MF_STRING;
		if (g_awState.isKeepingAwake)
			uFlags |= MF_CHECKED;
		AppendMenuW(g_awState.hMenu, uFlags, IDM_KEEP_AWAKE, AW_STR(IDS_MENU_KEEP_AWAKE));

		uFlags = MF_STRING;
		if (g_awState.isKeepingScreenOn)
			uFlags |= MF_CHECKED;
		if (!g_awState.isKeepingAwake)
			uFlags |= MF_GRAYED;
		AppendMenuW(g_awState.hMenu, uFlags, IDM_KEEP_SCREEN_ON, AW_STR(IDS_MENU_KEEP_SCREEN_ON));

		AppendMenuW(g_awState.hMenu, MF_SEPARATOR, 0, NULL);

		AppendMenuW(g_awState.hMenu, MF_STRING, IDM_TURN_OFF_SCREEN, AW_STR(IDS_MENU_TURN_OFF_SCREEN));
		AppendMenuW(g_awState.hMenu, MF_STRING, IDM_SLEEP, AW_STR(IDS_MENU_SLEEP));

		AppendMenuW(g_awState.hMenu, MF_SEPARATOR, 0, NULL);

	
		// Add the submenu to the main menu
		// Create a submenu for power schemes
		HMENU hSubMenu = CreatePopupMenu();
		if (hSubMenu)
		{
			AppendMenuW(g_awState.hMenu, MF_STRING, IDM_OPEN_POWER_MENU, AW_STR(IDS_MENU_OPEN_POWER_MENU));
			// Populate the submenu with available power schemes
			AW_GetPowerSchemes(); // Ensure we have the latest schemes
			PopulatePowerSchemesMenu(hSubMenu);
			// Add the submenu to the main menu
			AppendMenuW(g_awState.hMenu, MF_POPUP, (UINT_PTR)hSubMenu, AW_STR(IDS_MENU_POWER_SCHEMES));
		}

		AppendMenuW(g_awState.hMenu, MF_SEPARATOR, 0, NULL);

		AppendMenuW(g_awState.hMenu, MF_STRING, IDM_EXIT, AW_STR(IDS_MENU_EXIT));

		// This is required for the menu to behave correctly
		SetForegroundWindow(hWnd);

		// Display the menu
		TrackPopupMenuEx(g_awState.hMenu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN, pt.x, pt.y, hWnd, NULL);

		// Cleanup
		DestroyMenu(g_awState.hMenu);
		g_awState.hMenu = NULL; // Avoid dangling handle
	}
}
