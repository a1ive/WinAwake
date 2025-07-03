// SPDX-License-Identifier: GPL-3.0-or-later

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <shellapi.h>

#include "awake.h"
#include "utils.h"
#include "systray.h"
#include "resource.h"

// Link necessary libraries
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "powrprof.lib")

AWAKE_APP_STATE g_awState;

// Free the memory allocated for the cached strings.
static void FreeUIStrings(void)
{
	for (int i = 0; i < COUNT_IDS_TXT; ++i)
	{
		if (g_awState.uiText[i])
		{
			free(g_awState.uiText[i]);
			g_awState.uiText[i] = NULL;
		}
	}
}

// Clean up resources
static void
DestroyAppState(void)
{
	DestroyIcon(g_awState.hMainIcon);
	DestroyIcon(g_awState.hIconDisabled);
	DestroyIcon(g_awState.hIconScreenOn);
	DestroyIcon(g_awState.hIconEnabled);
	FreeUIStrings();
}

// Main window procedure to handle messages
static LRESULT CALLBACK
WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		AW_AddTrayIcon(hWnd);
		break;

	case WM_DESTROY:
		AW_RemoveTrayIcon(hWnd);
		DestroyAppState();
		PostQuitMessage(0);
		break;

	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// Handle dynamic power scheme menu items
		if (wmId >= IDM_POWER_SCHEME_BASE && wmId < (IDM_POWER_SCHEME_BASE + MAX_POWER_SCHEMES))
		{
			AW_ActivatePowerScheme((ULONG)wmId - IDM_POWER_SCHEME_BASE);
			break;
		}
		// Parse the menu selections
		switch (wmId)
		{
		case IDM_KEEP_AWAKE:
			g_awState.isKeepingAwake = !g_awState.isKeepingAwake;
			AW_UpdateExecState();
			AW_UpdateTrayIcon(hWnd);
			break;

		case IDM_KEEP_SCREEN_ON:
			g_awState.isKeepingScreenOn = !g_awState.isKeepingScreenOn;
			AW_UpdateExecState();
			AW_UpdateTrayIcon(hWnd);
			break;

		case IDM_TURN_OFF_SCREEN:
			AW_TurnOffScreen(hWnd);
			break;

		case IDM_SLEEP:
			AW_EnterSleep();
			break;

		case IDM_OPEN_POWER_MENU:
			// Open the classic Control Panel power options applet
			ShellExecuteW(NULL, L"open", L"control.exe",
				L"/name Microsoft.PowerOptions /page pagePlanSettings", NULL, SW_SHOWNORMAL);
			break;

		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;

		default:
			return DefWindowProcW(hWnd, uMsg, wParam, lParam);
		}
	}
	break;

	case WM_TRAYICON:
		// Handle tray icon messages (e.g., right-click)
		if (lParam == WM_RBUTTONUP)
		{
			AW_ShowTrayMenu(hWnd);
		}
		break;

	default:
		return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}

// Load an icon from resources or uses a default icon if not found
static HICON
LoadIconFromRes(HINSTANCE hInstance, WORD id)
{
	HICON hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(id));
	if (!hIcon)
		hIcon = LoadIconW(NULL, IDI_APPLICATION);
	return hIcon;
}

// Load all necessary strings from resources
static BOOL LoadUIStrings(HINSTANCE hInstance)
{
	for (UINT id = MIN_IDS_TXT; id <= MAX_IDS_TEXT; ++id)
	{
		const WCHAR* pStringResource = NULL;
		int len = LoadStringW(hInstance, id, (LPWSTR)&pStringResource, 0);

		if (len > 0)
		{
			WCHAR* buffer = (WCHAR*)calloc((size_t)len + 1, sizeof(WCHAR));
			if (!buffer)
			{
				FreeUIStrings();
				return FALSE;
			}
			// Copy the string content.
			// The null terminator is already in place thanks to calloc.
			memcpy(buffer, pStringResource, (size_t)len * sizeof(WCHAR));
			g_awState.uiText[id - MIN_IDS_TXT] = buffer;
		}
		else
		{
			FreeUIStrings();
			return FALSE;
		}
	}
	return TRUE;
}

// Create the main (hidden) window and registers its class
static BOOL
CreateMainWindow(HINSTANCE hInstance)
{
	const WCHAR* CLASS_NAME = L"SystemTrayAppAwakeWindowClass";

	WNDCLASSEXW wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEXW);
	wcex.style = 0;
	wcex.lpfnWndProc = WindowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;

	g_awState.hMainIcon = LoadIconFromRes(hInstance, IDI_ICON_MAIN);
	g_awState.hIconDisabled = LoadIconFromRes(hInstance, IDI_ICON_DISABLED);
	g_awState.hIconScreenOn = LoadIconFromRes(hInstance, IDI_ICON_SCREEN_ON);
	g_awState.hIconEnabled = LoadIconFromRes(hInstance, IDI_ICON_ENABLED);
	LoadUIStrings(hInstance);

	wcex.hIcon = g_awState.hMainIcon;
	wcex.hCursor = LoadCursorW(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = CLASS_NAME;
	wcex.hIconSm = g_awState.hMainIcon;

	if (!RegisterClassExW(&wcex))
		return FALSE;

	// Create a hidden window to receive messages
	g_awState.hMainWnd = CreateWindowExW(
		0,                              // Optional window styles.
		CLASS_NAME,                     // Window class
		AW_DESC,      // Window text
		WS_OVERLAPPEDWINDOW,            // Window style
		CW_USEDEFAULT, CW_USEDEFAULT,   // Position
		CW_USEDEFAULT, CW_USEDEFAULT,   // Size
		NULL,                           // Parent window
		NULL,                           // Menu
		hInstance,                      // Instance handle
		NULL                            // Additional application data
	);

	return g_awState.hMainWnd != NULL;
}

// Entry point of the application
int WINAPI
wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nShowCmd)
{
	// Unused parameters
	(void)hPrevInstance;
	(void)lpCmdLine;
	(void)nShowCmd;

#ifdef _DEBUG
	SetThreadUILanguage(GetUserDefaultUILanguage());
#endif

	// Initialize the global state structure
	SecureZeroMemory(&g_awState, sizeof(AWAKE_APP_STATE));

	if (!CreateMainWindow(hInstance))
	{
		MessageBoxW(NULL, AW_STR(IDS_ERR_CREATE_WINDOW), AW_STR(IDS_ERR), MB_ICONERROR | MB_OK);
		DestroyAppState();
		return 1;
	}

	// Main message loop
	MSG msg;
	while (GetMessageW(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	return (int)msg.wParam;
}
