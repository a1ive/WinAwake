// SPDX-License-Identifier: GPL-3.0-or-later

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <powrprof.h>

#include "awake.h"
#include "utils.h"

// Set the thread execution state to prevent or allow sleep/screen off
void AW_UpdateExecState(void)
{
	EXECUTION_STATE esFlags = ES_CONTINUOUS;

	if (g_awState.isKeepingAwake)
	{
		esFlags |= ES_SYSTEM_REQUIRED;
	}
	if (g_awState.isKeepingScreenOn)
	{
		esFlags |= ES_DISPLAY_REQUIRED;
	}

	// If either flag is set, this call prevents sleep/display off.
	// If no flags are set (back to ES_CONTINUOUS), it allows normal power management.
	SetThreadExecutionState(esFlags);
}

// Enable the SE_SHUTDOWN_NAME privilege required for SetSuspendState
BOOL AW_EnableShutdownPrivilege(void)
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tokenPrivileges;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		return FALSE;
	}

	if (!LookupPrivilegeValueW(NULL, SE_SHUTDOWN_NAME, &tokenPrivileges.Privileges[0].Luid))
	{
		CloseHandle(hToken);
		return FALSE;
	}

	tokenPrivileges.PrivilegeCount = 1;
	tokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (!AdjustTokenPrivileges(hToken, FALSE, &tokenPrivileges, 0, (PTOKEN_PRIVILEGES)NULL, 0))
	{
		CloseHandle(hToken);
		return FALSE;
	}

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
	{
		CloseHandle(hToken);
		return FALSE;
	}

	CloseHandle(hToken);
	return TRUE;
}

// Turn off the monitor immediately
void AW_TurnOffScreen(HWND hWnd)
{
	// SC_MONITORPOWER - Sets the state of the display.
	// This command supports devices that have power-saving features,
	// such as a battery-powered personal computer.
	// The lParam parameter can have the following values :
	//     -1 (the display is powering on)
	//      1 (the display is going to low power)
	//      2 (the display is being shut off)
	SendMessageW(hWnd, WM_SYSCOMMAND, SC_MONITORPOWER, 2);
}

// Put the system to sleep immediately
void AW_EnterSleep(void)
{
	// Attempt to get the required privilege. This might fail if the user is not an admin.
	if (AW_EnableShutdownPrivilege())
	{
		// SetSuspendState(Hibernate, Force, WakeupEventsDisabled)
		// FALSE for first parameter means sleep, not hibernate.
		SetSuspendState(FALSE, FALSE, FALSE);
	}
	else
	{
		MessageBoxW(g_awState.hMainWnd, L"Failed to get privilege for sleeping. Try running as administrator.", L"Permission Denied", MB_ICONWARNING | MB_OK);
	}
}
