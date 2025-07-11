// SPDX-License-Identifier: GPL-3.0-or-later

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <powrprof.h>
#include <winreg.h>
#include <stdio.h>
#include <stdlib.h>

#include "awake.h"
#include "utils.h"
#include "version.h"

// Set the thread execution state to prevent or allow sleep/screen off
void AW_UpdateExecState(void)
{
	EXECUTION_STATE esFlags = ES_CONTINUOUS;

	if (g_awState.isKeepingAwake)
	{
		esFlags |= ES_SYSTEM_REQUIRED;
		if (g_awState.isKeepingScreenOn)
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

typedef struct
{
	HWND hParentWnd;
} SCR_OFF_CTX;

static BOOL
SetActivePowerPlanValue(const GUID* pActiveSchemeGuid, const GUID* pSubGroupGuid, const GUID* pSettingGuid, DWORD value)
{
	if (PowerWriteACValueIndex(NULL, pActiveSchemeGuid, pSubGroupGuid, pSettingGuid, value) != ERROR_SUCCESS)
		return FALSE;

	if (PowerWriteDCValueIndex(NULL, pActiveSchemeGuid, pSubGroupGuid, pSettingGuid, value) != ERROR_SUCCESS)
		return FALSE;

	if (PowerSetActiveScheme(NULL, pActiveSchemeGuid) != ERROR_SUCCESS)
		return FALSE;

	return TRUE;
}

static DWORD WINAPI
ScreenOffThreadProc(LPVOID lpParam)
{
	SCR_OFF_CTX* params = (SCR_OFF_CTX*)lpParam;
	HWND hParentWnd = params ? params->hParentWnd : NULL;
	free(params);

	if (!hParentWnd)
		return 1;

	GUID* pActiveSchemeGuid = NULL;
	DWORD originalTimeoutAC = 0;
	DWORD originalTimeoutDC = 0;

	if (PowerGetActiveScheme(NULL, &pActiveSchemeGuid) != ERROR_SUCCESS)
	{
		PostMessageW(hParentWnd, WM_FALLBACK_SCREEOFF, 0, 0);
		return 1;
	}

	PowerReadACValueIndex(NULL, pActiveSchemeGuid, &GUID_VIDEO_SUBGROUP, &GUID_VIDEO_POWERDOWN_TIMEOUT, &originalTimeoutAC);
	PowerReadDCValueIndex(NULL, pActiveSchemeGuid, &GUID_VIDEO_SUBGROUP, &GUID_VIDEO_POWERDOWN_TIMEOUT, &originalTimeoutDC);

	if (!SetActivePowerPlanValue(pActiveSchemeGuid, &GUID_VIDEO_SUBGROUP, &GUID_VIDEO_POWERDOWN_TIMEOUT, 1))
	{
		PostMessageW(hParentWnd, WM_FALLBACK_SCREEOFF, 0, 0);
		goto restore;
	}

	Sleep(2000);

restore:
	SetActivePowerPlanValue(pActiveSchemeGuid, &GUID_VIDEO_SUBGROUP, &GUID_VIDEO_POWERDOWN_TIMEOUT, originalTimeoutAC);
	PowerWriteDCValueIndex(NULL, pActiveSchemeGuid, &GUID_VIDEO_SUBGROUP, &GUID_VIDEO_POWERDOWN_TIMEOUT, originalTimeoutDC);
	PowerSetActiveScheme(NULL, pActiveSchemeGuid);

	if (pActiveSchemeGuid)
		LocalFree(pActiveSchemeGuid);

	// If we reach here, the primary method either succeeded or has already posted a fallback message.
	return 0;
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

	SCR_OFF_CTX* params = (SCR_OFF_CTX*)malloc(sizeof(SCR_OFF_CTX));
	if (!params)
	{
		SendMessageW(hWnd, WM_SYSCOMMAND, SC_MONITORPOWER, 2);
		return;
	}
	params->hParentWnd = hWnd;

	HANDLE hThread = CreateThread(NULL, 0, ScreenOffThreadProc, params, 0, NULL);

	if (hThread == NULL)
	{
		SendMessageW(hWnd, WM_SYSCOMMAND, SC_MONITORPOWER, 2);
		free(params);
		return;
	}

	CloseHandle(hThread);
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
		MessageBoxW(g_awState.hMainWnd, AW_STR(IDS_ERR_GET_PRIVILEGE), AW_STR(IDS_ERR), MB_ICONWARNING | MB_OK);
	}
}

// Activate a power scheme based on its enumeration index.
void AW_ActivatePowerScheme(ULONG index)
{
	// Validate index against the number of cached schemes
	if (index >= 0 && index < g_awState.powerSchemeCount)
	{
		// Directly use the cached GUID
		GUID* schemeGuid = &g_awState.powerSchemes[index].guid;
		if (PowerSetActiveScheme(NULL, schemeGuid) != ERROR_SUCCESS)
		{
			MessageBoxW(g_awState.hMainWnd, AW_STR(IDS_ERR_ACTIVATE_SCHEME), AW_STR(IDS_ERR), MB_ICONWARNING | MB_OK);
		}
	}
}

// Enumerates all system power schemes and stores them in the global state cache.
void AW_GetPowerSchemes(void)
{
	GUID* activeSchemeGuid = NULL;
	g_awState.powerSchemeCount = 0;
	SecureZeroMemory(g_awState.powerSchemes, sizeof(g_awState.powerSchemes));

	PowerGetActiveScheme(NULL, &activeSchemeGuid);

	for (ULONG index = 0; index < MAX_POWER_SCHEMES; ++index)
	{
		// Use the cache directly as the destination buffer
		AW_POWER_INFO* currentScheme = &g_awState.powerSchemes[g_awState.powerSchemeCount];
		DWORD guidSize = sizeof(GUID);

		if (PowerEnumerate(NULL, NULL, NULL, ACCESS_SCHEME, index, (UCHAR*)&currentScheme->guid, &guidSize) == ERROR_SUCCESS)
		{
			DWORD nameSize = sizeof(currentScheme->name);
			// TODO: Add XP support.
			// PowerReadFriendlyName requires Windows Vista or later
			if (PowerReadFriendlyName(NULL, &currentScheme->guid, NULL, NULL, (UCHAR*)currentScheme->name, &nameSize) == ERROR_SUCCESS)
			{
				if (activeSchemeGuid && IsEqualGUID(&currentScheme->guid, activeSchemeGuid))
					currentScheme->active = TRUE;
				// Both GUID and Name were successfully retrieved, increment count
				g_awState.powerSchemeCount++;
			}
		}
		else
		{
			// ERROR_NO_MORE_ITEMS is the expected end of enumeration
			break;
		}
	}

	if (activeSchemeGuid)
		LocalFree(activeSchemeGuid);
}

// Check if the application is configured to run at startup.
BOOL AW_IsAutoStartEnabled(void)
{
	HKEY hKey = NULL;
	LONG lResult = RegOpenKeyExW(
		HKEY_CURRENT_USER,
		L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
		0,
		KEY_QUERY_VALUE,
		&hKey
	);

	if (lResult != ERROR_SUCCESS)
		return FALSE; // Cannot open key, assume not enabled

	// Check if our specific value exists
	lResult = RegQueryValueExW(hKey, L""AW_NAME, NULL, NULL, NULL, NULL);
	RegCloseKey(hKey);

	return (lResult == ERROR_SUCCESS);
}

// Enable or disable running the application at startup by modifying the registry.
BOOL AW_SetAutoStart(BOOL enable)
{
	HKEY hKey = NULL;
	LONG lResult = RegOpenKeyExW(
		HKEY_CURRENT_USER,
		L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
		0,
		KEY_SET_VALUE, // We need write access
		&hKey
	);

	if (lResult != ERROR_SUCCESS)
		return FALSE; // Cannot open key

	if (enable)
	{
		WCHAR szPath[MAX_PATH];
		// Get the full path of the current executable
		if (GetModuleFileNameW(NULL, szPath, MAX_PATH) == 0)
		{
			RegCloseKey(hKey);
			return FALSE;
		}

		// Set the registry value
		lResult = RegSetValueExW(
			hKey,
			L""AW_NAME,
			0,
			REG_SZ,
			(const BYTE*)szPath,
			((DWORD)wcslen(szPath) + 1) * sizeof(WCHAR)
		);
	}
	else
	{
		// Delete the registry value
		lResult = RegDeleteValueW(hKey, L""AW_NAME);
	}

	RegCloseKey(hKey);
	return (lResult == ERROR_SUCCESS);
}
