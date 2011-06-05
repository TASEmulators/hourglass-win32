/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#ifndef REGISTRYTRAMPS_H_INCL
#define REGISTRYTRAMPS_H_INCL

#define RegOpenKeyA TrampRegOpenKeyA
TRAMPFUNC LONG APIENTRY RegOpenKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult) TRAMPOLINE_DEF
#define RegOpenKeyExA TrampRegOpenKeyExA
TRAMPFUNC LONG APIENTRY RegOpenKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult) TRAMPOLINE_DEF
#define RegEnumKeyA TrampRegEnumKeyA
TRAMPFUNC LONG APIENTRY RegEnumKeyA(HKEY hKey, DWORD dwIndex, LPCSTR lpName, DWORD cchName) TRAMPOLINE_DEF
#define RegQueryValueExA TrampRegQueryValueExA
TRAMPFUNC LONG APIENTRY RegQueryValueExA(HKEY hKey, LPCTSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) TRAMPOLINE_DEF
#define RegOpenKeyW TrampRegOpenKeyW
TRAMPFUNC LONG APIENTRY RegOpenKeyW(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult) TRAMPOLINE_DEF
#define RegOpenKeyExW TrampRegOpenKeyExW
TRAMPFUNC LONG APIENTRY RegOpenKeyExW(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult) TRAMPOLINE_DEF
#define RegEnumKeyW TrampRegEnumKeyW
TRAMPFUNC LONG APIENTRY RegEnumKeyW(HKEY hKey, DWORD dwIndex, LPCWSTR lpName, DWORD cchName) TRAMPOLINE_DEF
#define RegQueryValueExW TrampRegQueryValueExW
TRAMPFUNC LONG APIENTRY RegQueryValueExW(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) TRAMPOLINE_DEF
#define NtQueryKey TrampNtQueryKey
TRAMPFUNC NTSTATUS NTAPI NtQueryKey(IN HANDLE KeyHandle, IN DWORD KeyInformationClass, OUT PVOID KeyInformation, IN ULONG Length, OUT PULONG ResultLength) TRAMPOLINE_DEF

// close enough?
#define GetSystemMetrics TrampGetSystemMetrics
TRAMPFUNC int WINAPI GetSystemMetrics(int nIndex) TRAMPOLINE_DEF

#endif
