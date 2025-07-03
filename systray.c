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
	nid.uFlags = NIF_ICON; // We are only modifying the icon

	// Determine which icon to display.
	if (g_awState.isKeepingScreenOn)
		nid.hIcon = g_awState.hIconIndefinite;
	else if (g_awState.isKeepingAwake)
		nid.hIcon = g_awState.hIconNormal;
	else
		nid.hIcon = g_awState.hIconDisabled;

	// Modify the existing notification icon.
	Shell_NotifyIconW(NIM_MODIFY, &nid);
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
		AppendMenuW(g_awState.hMenu, MF_STRING | (g_awState.isKeepingAwake ? MF_CHECKED : MF_UNCHECKED),
			IDM_KEEP_AWAKE, AW_STR(IDS_MENU_KEEP_AWAKE));
		AppendMenuW(g_awState.hMenu, MF_STRING | (g_awState.isKeepingScreenOn ? MF_CHECKED : MF_UNCHECKED),
			IDM_KEEP_SCREEN_ON, AW_STR(IDS_MENU_KEEP_SCREEN_ON));
		AppendMenuW(g_awState.hMenu, MF_SEPARATOR, 0, NULL);
		AppendMenuW(g_awState.hMenu, MF_STRING, IDM_TURN_OFF_SCREEN, AW_STR(IDS_MENU_TURN_OFF_SCREEN));
		AppendMenuW(g_awState.hMenu, MF_STRING, IDM_SLEEP, AW_STR(IDS_MENU_SLEEP));
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
