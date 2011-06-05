/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#if !defined(REGISTRYHOOKS_INCL) && !defined(UNITY_BUILD)
#define REGISTRYHOOKS_INCL


#include "../global.h"
#include "../../shared/ipc.h"
#include "../tls.h"
#include "../wintasee.h"

typedef struct _KEY_NAME_INFORMATION {
  ULONG NameLength;
  WCHAR Name[1];
} KEY_NAME_INFORMATION, *PKEY_NAME_INFORMATION;
//NTSYSAPI NTSTATUS NTAPI NtQueryKey(IN HANDLE KeyHandle, IN DWORD KeyInformationClass, OUT PVOID KeyInformation, IN ULONG Length, OUT PULONG ResultLength);

static const char* RegistryKeyToName(HKEY hKey)
{
	switch((LONG)(ULONG_PTR)hKey)
	{
	case 0: return "NULL";
	case HKEY_CLASSES_ROOT: return "HKEY_CLASSES_ROOT";
	case HKEY_CURRENT_USER: return "HKEY_CURRENT_USER";
	case HKEY_LOCAL_MACHINE: return "HKEY_LOCAL_MACHINE";
	case HKEY_USERS: return "HKEY_USERS";
	case HKEY_PERFORMANCE_DATA: return "HKEY_PERFORMANCE_DATA";
	case HKEY_PERFORMANCE_TEXT: return "HKEY_PERFORMANCE_TEXT";
	case HKEY_PERFORMANCE_NLSTEXT: return "HKEY_PERFORMANCE_NLSTEXT";
	case HKEY_CURRENT_CONFIG: return "HKEY_CURRENT_CONFIG";
	case HKEY_DYN_DATA: return "HKEY_DYN_DATA";
	default:
		{
			//return "?";
			static const int MAXLEN = 512;
			char basicInfoBytes [sizeof(KEY_NAME_INFORMATION) + (MAXLEN-1)*sizeof(WCHAR)] = {0};
			ULONG resultLength = 0;
			NtQueryKey(hKey, /*KeyNameInformation*/3, basicInfoBytes, MAXLEN, &resultLength);
			int length = (int)(((KEY_NAME_INFORMATION*)basicInfoBytes)->NameLength);
			length = min(length, (int)resultLength);
			length = min(length, MAXLEN);
			static char rvtemp [MAXLEN];
			rvtemp[sizeof(rvtemp)-1] = 0;
			char* rvtempPtr = rvtemp;
			WCHAR* wname = ((KEY_NAME_INFORMATION*)basicInfoBytes)->Name;
			do{
				*rvtempPtr++ = (char)*wname++;
			} while(rvtempPtr[-1]);
			return rvtemp;
		}
	}
} 

HOOKFUNC LONG APIENTRY MyRegOpenKeyA(
HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult)
{
	debuglog(LCF_REGISTRY|LCF_TODO, __FUNCTION__ "(\"%s\", \"%s\") called.\n", RegistryKeyToName(hKey), lpSubKey ? lpSubKey : "");
	LONG rv = RegOpenKeyA(hKey, lpSubKey, phkResult);
	debuglog(LCF_REGISTRY|LCF_TODO, __FUNCTION__" returned \"%s\".\n", RegistryKeyToName(phkResult ? *phkResult : 0));
	return rv;
}
HOOKFUNC LONG APIENTRY MyRegOpenKeyW(
HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult)
{
	debuglog(LCF_REGISTRY|LCF_TODO, __FUNCTION__ "(\"%s\", \"%S\") called.\n", RegistryKeyToName(hKey), lpSubKey ? lpSubKey : L"");
	LONG rv = RegOpenKeyW(hKey, lpSubKey, phkResult);
	debuglog(LCF_REGISTRY|LCF_TODO, __FUNCTION__" returned \"%s\".\n", RegistryKeyToName(phkResult ? *phkResult : 0));
	return rv;
}
HOOKFUNC LONG APIENTRY MyRegOpenKeyExA(
HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)
{
	debuglog(LCF_REGISTRY|LCF_TODO, __FUNCTION__ "(\"%s\", \"%s\") called.\n", RegistryKeyToName(hKey), lpSubKey ? lpSubKey : "");
	LONG rv = RegOpenKeyExA(hKey, lpSubKey, ulOptions, samDesired, phkResult);
	debuglog(LCF_REGISTRY|LCF_TODO, __FUNCTION__" returned \"%s\".\n", RegistryKeyToName(phkResult ? *phkResult : 0));
	return rv;
}
HOOKFUNC LONG APIENTRY MyRegOpenKeyExW(
HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)
{
	debuglog(LCF_REGISTRY|LCF_TODO|LCF_FREQUENT, __FUNCTION__ "(\"%s\", \"%S\") called.\n", RegistryKeyToName(hKey), lpSubKey ? lpSubKey : L"");
	LONG rv = RegOpenKeyExW(hKey, lpSubKey, ulOptions, samDesired, phkResult);
	debuglog(LCF_REGISTRY|LCF_TODO|LCF_FREQUENT, __FUNCTION__" returned \"%s\".\n", RegistryKeyToName(phkResult ? *phkResult : 0));
	return rv;
}
HOOKFUNC LONG APIENTRY MyRegEnumKeyA(HKEY hKey, DWORD dwIndex, LPCSTR lpName, DWORD cchName)
{
	debuglog(LCF_REGISTRY|LCF_TODO, __FUNCTION__ "(\"%s\", %d) called.\n", RegistryKeyToName(hKey), dwIndex);
	LONG rv = RegEnumKeyA(hKey, dwIndex, lpName, cchName);
	debuglog(LCF_REGISTRY|LCF_TODO, __FUNCTION__" returned \"%s\".\n", lpName);
	return rv;
}
HOOKFUNC LONG APIENTRY MyRegEnumKeyW(HKEY hKey, DWORD dwIndex, LPCWSTR lpName, DWORD cchName)
{
	debuglog(LCF_REGISTRY|LCF_TODO, __FUNCTION__ "(\"%s\", %d) called.\n", RegistryKeyToName(hKey), dwIndex);
	LONG rv = RegEnumKeyW(hKey, dwIndex, lpName, cchName);
	debuglog(LCF_REGISTRY|LCF_TODO, __FUNCTION__" returned \"%S\".\n", lpName);
	return rv;
}
HOOKFUNC LONG APIENTRY MyRegQueryValueExA(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved,
													 LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
	debuglog(LCF_REGISTRY|LCF_TODO, __FUNCTION__ "(\"%s\", \"%s\") called.\n", RegistryKeyToName(hKey), lpValueName ? lpValueName : "");
	LONG rv = RegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
	debuglog(LCF_REGISTRY|LCF_TODO, __FUNCTION__" returned \"%s\".\n", lpValueName, lpData); // FIXME: lpData isn't always a string
	if(tasflags.forceSoftware && lpValueName && !strcmp(lpValueName, "EmulationOnly"))
		lpData[0] = 1; // disable hardware acceleration, maybe not needed anymore but it gets rid of some savestate error messages
	//if(lpValueName && !strcmp(lpValueName, "FontSmoothing"))
	//	lpData[0] = 0;
	//if(lpValueName && !strcmp(lpValueName, "FontSmoothingType"))
	//	lpData[0] = 0;
	//if(lpValueName && !strcmp(lpValueName, "ClearTypeLevel"))
	//	lpData[0] = 0;
	return rv;
}

HOOKFUNC LONG APIENTRY MyRegQueryValueExW(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved,
													 LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
	debuglog(LCF_REGISTRY|LCF_TODO, __FUNCTION__ "(\"%s\", \"%S\") called.\n", RegistryKeyToName(hKey), lpValueName ? lpValueName : L"");
	LONG rv = RegQueryValueExW(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
	debuglog(LCF_REGISTRY|LCF_TODO, __FUNCTION__" returned \"%s\".\n", lpValueName, lpData);
	if(tasflags.forceSoftware && lpValueName && !wcscmp(lpValueName, L"EmulationOnly"))
		lpData[0] = 1; // disable hardware acceleration, maybe not needed anymore but it gets rid of some savestate error messages
	//if(lpValueName && !wcscmp(lpValueName, L"FontSmoothing"))
	//	lpData[0] = 0;
	//if(lpValueName && !wcscmp(lpValueName, L"FontSmoothingType"))
	//	lpData[0] = 0;
	//if(lpValueName && !wcscmp(lpValueName, L"ClearTypeLevel"))
	//	lpData[0] = 0;
	return rv;
}

HOOKFUNC NTSTATUS NTAPI MyNtQueryKey(IN HANDLE KeyHandle, IN DWORD KeyInformationClass, OUT PVOID KeyInformation, IN ULONG Length, OUT PULONG ResultLength)
{
	return NtQueryKey(KeyHandle, KeyInformationClass, KeyInformation, Length, ResultLength);
}


HOOKFUNC int WINAPI MyGetSystemMetrics(int nIndex)
{
	ThreadLocalStuff& curtls = tls;
	curtls.callerisuntrusted++;
	curtls.treatDLLLoadsAsClient++;
	int systemResult = GetSystemMetrics(nIndex);
	curtls.treatDLLLoadsAsClient--;
	curtls.callerisuntrusted--;
	int rv = systemResult;
	switch(nIndex)
	{
	case SM_CLEANBOOT:
		rv = 0; // normal boot
		break;
	case SM_CMOUSEBUTTONS:
		rv = 8; // upper limit... any more would take a new byte per frame to record anyway
		break;
	case SM_CXDOUBLECLK:
		rv = 4;
		break;
	case SM_CXDRAG:
		rv = 4;
		break;
	case SM_IMMENABLED:
	case SM_DBCSENABLED: // todo?
	case /*SM_DIGITIZER*/94:
	case /*SM_MAXIMUMTOUCHES*/95:
	case /*SM_MEDIACENTER*/87:
	case SM_DEBUG:
	case SM_MENUDROPALIGNMENT:
	case SM_MIDEASTENABLED:
	case SM_NETWORK:
	case SM_PENWINDOWS:
	case SM_REMOTECONTROL:
	case SM_REMOTESESSION:
	case /*SM_SERVERR2*/89:
	case SM_SHOWSOUNDS:
	case /*SM_SHUTTINGDOWN*/0x2000:
	case SM_SLOWMACHINE:
	case /*SM_STARTER*/88:
	case /*SM_TABLETPC*/86:
		rv = 0; // disabled unnecessary things
		break;
	case SM_MOUSEPRESENT:
		rv = 1; // "This value is rarely zero"
		break;
	case /*SM_MOUSEHORIZONTALWHEELPRESENT*/91:
	case SM_MOUSEWHEELPRESENT:
	case SM_SWAPBUTTON:
		rv = 0; // FIXME: TODO: customize some of these depending on mouse settings
		break;
	case SM_CMONITORS:
		if(tasflags.forceWindowed)
			rv = 1;
		// else allow the system setting to be used here
	case SM_CXSCREEN:
	case SM_CXFULLSCREEN:
	case SM_CXVIRTUALSCREEN:
		if(fakeDisplayValid)
			rv = fakeDisplayWidth;
		// else allow the system setting to be used here
		break;
	case SM_CYSCREEN:
	case SM_CYFULLSCREEN:
	case SM_CYVIRTUALSCREEN:
		if(fakeDisplayValid)
			rv = fakeDisplayHeight;
		// else allow the system setting to be used here
		break;
	case SM_SAMEDISPLAYFORMAT:
		if(tasflags.forceWindowed)
			rv = 1;
		// else allow the system setting to be used here
		break;
	case SM_ARRANGE:
	case SM_CXBORDER:
	case SM_CYBORDER:
	case SM_CXCURSOR:
	case SM_CYCURSOR:
	case SM_CXDLGFRAME:
	case SM_CYDLGFRAME:
	case SM_CXEDGE:
	case SM_CYEDGE:
	case /*SM_CXFOCUSBORDER*/83:
	case /*SM_CYFOCUSBORDER*/84:
	case SM_CXFRAME:
	case SM_CYFRAME:
	case SM_CXHSCROLL:
	case SM_CYHSCROLL:
	case SM_CXHTHUMB:
	case SM_CYVTHUMB:
	case SM_CXICON:
	case SM_CYICON:
	case SM_CXICONSPACING:
	case SM_CYICONSPACING:
	case SM_CXMAXIMIZED:
	case SM_CYMAXIMIZED:
	case SM_CXMAXTRACK:
	case SM_CYMAXTRACK:
	case SM_CXMENUCHECK:
	case SM_CYMENUCHECK:
	case SM_CXMENUSIZE:
	case SM_CYMENUSIZE:
	case SM_CXMIN:
	case SM_CYMIN:
	case SM_CXMINIMIZED:
	case SM_CYMINIMIZED:
	case SM_CXMINSPACING:
	case SM_CYMINSPACING:
	case SM_CXMINTRACK:
	case SM_CYMINTRACK:
	case /*SM_CXPADDEDBORDER*/92:
	case SM_CXSIZE:
	case SM_CYSIZE:
	case SM_CXSMICON:
	case SM_CYSMICON:
	case SM_CXSMSIZE:
	case SM_CYSMSIZE:
	case SM_CXVSCROLL:
	case SM_CYVSCROLL:
	case SM_XVIRTUALSCREEN:
	case SM_YVIRTUALSCREEN:
	default:
		// allow system setting
		// hopefully these aren't used in sync-affecting ways...
		// they easily could be, but the idea here is that
		// I don't want to force everyone to use the same desktop resolution or window positions,
		// and most games don't have any game logic affected by those things,
		// and if they did then this definitely isn't the only place that needs to change to handle it.
		// TODO: probably some of these will need to be handled eventually to avoid desyncs in some games.
		break;
	}
	if(rv != systemResult)
		debuglog(LCF_REGISTRY|LCF_TODO|LCF_FREQUENT, __FUNCTION__ "(%d) returned %d (0x%X) instead of %d (0x%X).\n", nIndex, rv, rv, systemResult, systemResult);
	else
		debuglog(LCF_REGISTRY|LCF_TODO|LCF_FREQUENT, __FUNCTION__ "(%d) returned %d (0x%X).\n", nIndex, rv, rv);
	return rv;
}





void ApplyRegistryIntercepts()
{
	static const InterceptDescriptor intercepts [] = 
	{
		//MAKE_INTERCEPT(1, ADVAPI32, RegQueryValueExA),
		//MAKE_INTERCEPT(1, ADVAPI32, RegOpenKeyA),
		//MAKE_INTERCEPT(1, ADVAPI32, RegOpenKeyExA),
		//MAKE_INTERCEPT(1, ADVAPI32, RegEnumKeyA),
		//MAKE_INTERCEPT(-1, NTDLL, NtQueryKey),

		//MAKE_INTERCEPT(1, ADVAPI32, RegOpenKeyW),
		//MAKE_INTERCEPT(1, ADVAPI32, RegOpenKeyExW),
		//MAKE_INTERCEPT(1, ADVAPI32, RegEnumKeyW),
		//MAKE_INTERCEPT(1, ADVAPI32, RegQueryValueExW),
		//TODO: should be possible to replace those with: NtCreateKey, NtOpenKey, NtOpenKeyEx, NtQueryValueKey ... refer to article h ttp://www.codeproject.com/KB/system/NtRegistry.aspx

		MAKE_INTERCEPT(1, USER32, GetSystemMetrics),
	};
	ApplyInterceptTable(intercepts, ARRAYSIZE(intercepts));
}


#else
#pragma message(__FILE__": (skipped compilation)")
#endif
