#ifndef __ACTIVEDLL_H_
#define __ACTIVEDLL_H_

BOOL WINAPI CallCreateProcessA(
	PCHAR lpApplicationName,
	PCHAR lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	PCHAR lpCurrentDirectory,
	LPSTARTUPINFOA lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation
);

BOOL WINAPI CallCreateProcessW(
	LPWSTR lpApplicationName,
	LPWSTR lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPWSTR lpCurrentDirectory,
	LPSTARTUPINFOW lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation
);

typedef BOOL (__stdcall * AC_ON_CREATE_PROCESSW_CALLBACK)(
	LPWSTR lpApplicationName,
	LPWSTR lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPWSTR lpCurrentDirectory,
	LPSTARTUPINFOW lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation
);

typedef BOOL (__stdcall * AC_ON_CREATE_PROCESSA_CALLBACK)(
	LPSTR lpApplicationName,
	LPSTR lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPSTR lpCurrentDirectory,
	LPSTARTUPINFOA lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation
);

typedef LPWSTR (__stdcall * AC_CHANGE_DESKTOP_NAMEW_CALLBACK)(LPSTARTUPINFOW lpStartupInfo);
typedef LPSTR (__stdcall * AC_CHANGE_DESKTOP_NAMEA_CALLBACK)(LPSTARTUPINFOA lpStartupInfo);

// Active DLL caller-defined callbacks
extern AC_CHANGE_DESKTOP_NAMEA_CALLBACK		g_pChangeDesktopNameA;
extern AC_CHANGE_DESKTOP_NAMEW_CALLBACK		g_pChangeDesktopNameW;
extern AC_ON_CREATE_PROCESSA_CALLBACK		g_pOnCreateProcessA; 
extern AC_ON_CREATE_PROCESSW_CALLBACK		g_pOnCreateProcessW;

WINERROR AcStartup(PVOID pContext, BOOL bSetHooks);
void AcCleanup(void);

#endif //__ACTIVEDLL_H_