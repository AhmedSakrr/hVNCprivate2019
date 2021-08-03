#ifndef __COPY_H_
#define __COPY_H_

DWORD XCopyDirectoryW( LPWSTR Src, LPWSTR Dst );
DWORD XCopyDirectoryA( LPSTR Src, LPSTR Dst );
DWORD XCopyDirectorySpecifyProcessW( LPWSTR ProcessName, LPWSTR Src, LPWSTR Dst );

BOOL XRemoveDirectoryW( LPWSTR szPath, LPWSTR Pattern, BOOL bRemoveDir);
BOOL XRemoveDirectoryA( LPSTR szPath, LPSTR Pattern, BOOL bRemoveDir);

#endif