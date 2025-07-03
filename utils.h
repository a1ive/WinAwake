// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

void AW_UpdateExecState(void);

BOOL AW_EnableShutdownPrivilege(void);

void AW_TurnOffScreen(HWND hWnd);

void AW_EnterSleep(void);

void AW_ActivatePowerScheme(ULONG index);

void AW_GetPowerSchemes(void);
