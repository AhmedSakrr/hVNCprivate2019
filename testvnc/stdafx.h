#pragma once

#define _CRT_SECURE_NO_WARNINGS
#include <sdkddkver.h>
#include <stdlib.h>
#include <tchar.h>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>
#include <psapi.h>
#include <ShlObj.h>

#define countof(array) sizeof(array) / sizeof(array[0])

#pragma warning(push)
#pragma warning(disable:4005) // macro redefinition
#include "../inc/ntdll.h"
#include <ntstatus.h>
#pragma warning(pop)

typedef INT	WINERROR;