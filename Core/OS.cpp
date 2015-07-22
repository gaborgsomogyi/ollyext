#include "stdafx.h"
#include <dbghelp.h>
#include "Log.h"
#include "OS.h"


struct OSDETECTEDVERSION
{
	wchar_t* osName;

	// ntdll->_PEB->BeingDebugged
	DWORD BeingDebugged;

	// ntdll->_PEB->ProcessHeaps
	DWORD ProcessHeaps;

	// ntdll->_PEB->NtGlobalFlag
	DWORD NtGlobalFlag;

	// ntdll->_HEAP->Flags
	DWORD Flags;

	// ntdll->_HEAP->ForceFlags
	DWORD ForceFlags;
};

struct OSVERSIONINFOEX_
{
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
	WORD wServicePackMajor;
	BYTE wProductType;

	OSDETECTEDVERSION detectedVersion;
};


const OSVERSIONINFOEX_ g_windowsVersions[] =
{
	{ 5, 1, 0, 0, { L"XP", 0x00000002, 0x00000018, 0x00000068, 0x0000000C, 0x00000010 } },
	{ 5, 1, 1, 0, { L"XP SP1", 0x00000002, 0x00000018, 0x00000068, 0x0000000C, 0x00000010 } },
	{ 5, 1, 2, 0, { L"XP SP2", 0x00000002, 0x00000018, 0x00000068, 0x0000000C, 0x00000010 } },
	{ 5, 1, 3, 0, { L"XP SP3", 0x00000002, 0x00000018, 0x00000068, 0x0000000C, 0x00000010 } },

//	{ 5, 2, 0, 0 }, // Server 2003
	{ 5, 2, 1, 0, { L"Server 2003 SP1", 0x00000002, 0x00000018, 0x00000068, 0x0000000C, 0x00000010 } },
	{ 5, 2, 2, 0, { L"Server 2003 SP2", 0x00000002, 0x00000018, 0x00000068, 0x0000000C, 0x00000010 } },

//	{ 6, 0, 0, VER_NT_WORKSTATION }, // Vista
//	{ 6, 0, 1, VER_NT_WORKSTATION }, // Vista SP1
//	{ 6, 0, 2, VER_NT_WORKSTATION }, // Vista SP2

//	{ 6, 0, 0, VER_NT_SERVER }, // Server 2008
//	{ 6, 0, 1, VER_NT_SERVER }, // Server 2008 SP1
//	{ 6, 0, 2, VER_NT_SERVER }, // Server 2008 SP1

	{ 6, 1, 0, VER_NT_SERVER, { L"Server 2008 R2", 0x00000002, 0x00000018, 0x00000068, 0x00000040, 0x00000044 } },
	{ 6, 1, 1, VER_NT_SERVER, { L"Server 2008 R2 SP1", 0x00000002, 0x00000018, 0x00000068, 0x00000040, 0x00000044 } },

	{ 6, 1, 0, VER_NT_WORKSTATION, { L"7", 0x00000002, 0x00000018, 0x00000068, 0x00000040, 0x00000044 } },
	{ 6, 1, 1, VER_NT_WORKSTATION, { L"7 SP1", 0x00000002, 0x00000018, 0x00000068, 0x00000040, 0x00000044 } },

	{ 6, 2, 0, VER_NT_SERVER, { L"Server 2012", 0x00000002, 0x00000018, 0x00000068, 0x00000040, 0x00000044 } },
	{ 6, 2, 0, VER_NT_WORKSTATION, { L"8", 0x00000002, 0x00000018, 0x00000068, 0x00000040, 0x00000044 } },

	{ 6, 3, 0, VER_NT_SERVER, { L"Server 2012 R2", 0x00000002, 0x00000018, 0x00000068, 0x00000040, 0x00000044 } },
	{ 6, 3, 0, VER_NT_WORKSTATION, { L"8.1", 0x00000002, 0x00000018, 0x00000068, 0x00000040, 0x00000044 } },
};


static OSVERSIONINFOEX_ g_osVersion = { 0, 0, 0, 0, { L"UNKNOWN", 0x00000000, 0x00000000, 0x00000000, 0x00000000 } };


static BOOL osEqualsMajorVersion( DWORD majorVersion )
{
    OSVERSIONINFOEX osVersionInfo;
    ZeroMemory( &osVersionInfo, sizeof( OSVERSIONINFOEX ) );
    osVersionInfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );
    osVersionInfo.dwMajorVersion = majorVersion;
    ULONGLONG maskCondition = VerSetConditionMask( 0, VER_MAJORVERSION, VER_EQUAL );
    return VerifyVersionInfo( &osVersionInfo, VER_MAJORVERSION, maskCondition );
}


static BOOL osEqualsMinorVersion( DWORD minorVersion )
{
    OSVERSIONINFOEX osVersionInfo;
    ZeroMemory( &osVersionInfo, sizeof( OSVERSIONINFOEX ) );
    osVersionInfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );
    osVersionInfo.dwMinorVersion = minorVersion;
    ULONGLONG maskCondition = VerSetConditionMask( 0, VER_MINORVERSION, VER_EQUAL );
    return VerifyVersionInfo( &osVersionInfo, VER_MINORVERSION, maskCondition );
}


static BOOL osEqualsServicePack( WORD servicePackMajor )
{
    OSVERSIONINFOEX osVersionInfo;
    ZeroMemory( &osVersionInfo, sizeof( OSVERSIONINFOEX ) );
    osVersionInfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );
    osVersionInfo.wServicePackMajor = servicePackMajor;
    ULONGLONG maskCondition = VerSetConditionMask( 0, VER_SERVICEPACKMAJOR, VER_EQUAL );
    return VerifyVersionInfo( &osVersionInfo, VER_SERVICEPACKMAJOR, maskCondition );
}


static BOOL osEqualsProductType( BYTE productType )
{
    OSVERSIONINFOEX osVersionInfo;
    ZeroMemory( &osVersionInfo, sizeof( OSVERSIONINFOEX ) );
    osVersionInfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );
    osVersionInfo.wProductType = productType;
    ULONGLONG maskCondition = VerSetConditionMask( 0, VER_PRODUCT_TYPE, VER_EQUAL );
    return VerifyVersionInfo( &osVersionInfo, VER_PRODUCT_TYPE, maskCondition );
}


static BYTE osGetProductType( void )
{
    if( osEqualsProductType( VER_NT_WORKSTATION ) )
    {
        return VER_NT_WORKSTATION;
    }
    else if( osEqualsProductType( VER_NT_DOMAIN_CONTROLLER ) )
    {
        return VER_NT_DOMAIN_CONTROLLER;
    }
    else if( osEqualsProductType( VER_NT_SERVER ) )
    {
        return VER_NT_SERVER;
    }
    return 0;
}


static BOOL GetVersionEx_( OSVERSIONINFOEX_& rOSVersionInfoEx )
{
	WORD wProductType = osGetProductType();

	for( DWORD i = 0; i < sizeof( g_windowsVersions ) / sizeof( OSVERSIONINFOEX_ ); ++i )
	{
		if( osEqualsMajorVersion( g_windowsVersions[i].dwMajorVersion ) )
		{
			if( osEqualsMinorVersion( g_windowsVersions[i].dwMinorVersion ) )
			{
				if( osEqualsServicePack( g_windowsVersions[i].wServicePackMajor ) )
				{
					if( !g_windowsVersions[i].wProductType || g_windowsVersions[i].wProductType == wProductType )
					{
						rOSVersionInfoEx.dwMajorVersion = g_windowsVersions[i].dwMajorVersion;
						rOSVersionInfoEx.dwMinorVersion = g_windowsVersions[i].dwMinorVersion;
						rOSVersionInfoEx.wServicePackMajor = g_windowsVersions[i].wServicePackMajor;

						rOSVersionInfoEx.detectedVersion = g_windowsVersions[i].detectedVersion;

						return TRUE;
					}
				}
			}
		}
	}
	return FALSE;
}


static DWORD osGetModuleOffsetInStruct( const std::wstring& rModule, const std::wstring& rStruct, const std::wstring& rMember )
{
	DWORD result = 0;
	
	HANDLE hModule = GetCurrentProcess();

	DWORD options = SymGetOptions() | SYMOPT_UNDNAME | SYMOPT_DEBUG | SYMOPT_LOAD_ANYTHING;
    PLUGIN_CHECK_ERROR( options == SymSetOptions( options ), return 0;, UNABLE_TO_SET_SYMBOL_SERVER_OPTIONS );

	wchar_t appPath[MAX_PATH] = { 0 };
	GetModuleFileName( 0, appPath, sizeof( appPath ) );
	PathRemoveFileSpec( appPath );

	wchar_t sysPath[MAX_PATH] = { 0 };
	PLUGIN_CHECK_ERROR( GetSystemDirectory( sysPath, sizeof( sysPath ) ), return 0;, UNABLE_TO_GET_SYSTEM_DIRECTORY );

	std::wstring symbolPath;
	symbolPath += L"SRV*";
	symbolPath += appPath;
	symbolPath += L"\\OllyExtSymbols";
	symbolPath += L"*http://msdl.microsoft.com/download/symbols";

	PLUGIN_CHECK_ERROR( SymInitializeW( hModule, symbolPath.c_str(), FALSE ), return 0;, UNABLE_TO_INITIALIZE_SYMBOL_SERVER );

	std::wstring modulePath;
	modulePath += sysPath;
	modulePath += L"\\";
	modulePath += rModule.c_str();
	DWORD64 hDll = (DWORD)SymLoadModuleExW( hModule, NULL, modulePath.c_str(), NULL, 0, 0, NULL, 0 );
	PLUGIN_CHECK_ERROR( hDll,
		SymCleanup( hModule );
		return 0;,
		UNABLE_TO_LOAD_MODULE, modulePath.c_str() );

    BYTE symbolInfoBuf[MAX_SYM_NAME];
    ZeroMemory( symbolInfoBuf, sizeof( symbolInfoBuf ) );
    SYMBOL_INFOW* pSymbolInfo = (SYMBOL_INFOW*)symbolInfoBuf;
    pSymbolInfo->SizeOfStruct = sizeof( SYMBOL_INFOW );
    pSymbolInfo->MaxNameLen = MAX_SYM_NAME;
    pSymbolInfo->ModBase = hDll;
	PLUGIN_CHECK_ERROR_NO_MB( SymGetTypeFromNameW( hModule, hDll, rStruct.c_str(), pSymbolInfo ),
		SymUnloadModule64( hModule, hDll );
		SymCleanup( hModule );
		return 0; );

    DWORD childCount = 0;
    PLUGIN_CHECK_ERROR_NO_MB( SymGetTypeInfo( hModule, hDll, pSymbolInfo->TypeIndex, TI_GET_CHILDRENCOUNT, &childCount ),
		SymUnloadModule64( hModule, hDll );
		SymCleanup( hModule );
		return 0; );

    DWORD childrenParamsSize = sizeof( TI_FINDCHILDREN_PARAMS ) + sizeof( DWORD ) * childCount;
    std::auto_ptr<BYTE> pChildrenParamsBuf = std::auto_ptr<BYTE>( new BYTE[childrenParamsSize] );
	PLUGIN_CHECK_ERROR( pChildrenParamsBuf.get(),
		SymUnloadModule64( hModule, hDll );
		SymCleanup( hModule );
		return 0;,
		UNABLE_TO_ALLOCATE_LOCAL_MEMORY );
    ZeroMemory( pChildrenParamsBuf.get(), childrenParamsSize );
    TI_FINDCHILDREN_PARAMS* pChildrenParams = (TI_FINDCHILDREN_PARAMS*)pChildrenParamsBuf.get();
    pChildrenParams->Count = childCount;
    PLUGIN_CHECK_ERROR_NO_MB( SymGetTypeInfo( hModule, hDll, pSymbolInfo->TypeIndex, TI_FINDCHILDREN, pChildrenParams ),
		SymUnloadModule64( hModule, hDll );
		SymCleanup( hModule );
		return 0;
		);

    for( DWORD i = pChildrenParams->Start; i < pChildrenParams->Count; ++i )
    {
        wchar_t* pSymName = NULL;
        PLUGIN_CHECK_ERROR_NO_MB( SymGetTypeInfo( hModule, hDll, pChildrenParams->ChildId[i], TI_GET_SYMNAME, &pSymName ),
			SymUnloadModule64( hModule, hDll );
			SymCleanup( hModule );
			return 0;
			);

        DWORD symbolOffset = 0;
        PLUGIN_CHECK_ERROR_NO_MB( SymGetTypeInfo( hModule, hDll, pChildrenParams->ChildId[i], TI_GET_OFFSET, &symbolOffset ),
			SymUnloadModule64( hModule, hDll );
			SymCleanup( hModule );
			return 0;
			);

		if( !wcscmp( pSymName, rMember.c_str() ) )
		{
			result = symbolOffset;
			break;
		}

        LocalFree( pSymName );
    }

	PLUGIN_CHECK_ERROR( SymUnloadModule64( hModule, hDll ),
		SymCleanup( hModule );
		return 0;,
		UNABLE_TO_LOAD_MODULE, modulePath.c_str() );
	PLUGIN_CHECK_ERROR( SymCleanup( hModule ), return 0;, UNABLE_TO_CLEANUP_SYMBOL_SERVER );

	return result;
}


void osInit( void )
{
	OSVERSIONINFOEX_ osvi;

	// Get the Windows version.
	ZeroMemory( &osvi, sizeof( osvi ) );
	if( !GetVersionEx_( osvi ) )
	{
		logMessage( L"%ws: This version considered as UNKNOWN", dbgGetPluginName() );
		MessageBox( NULL, UNABLE_TO_GET_WIN_VERSION, dbgGetPluginName(), MB_OK | MB_ICONERROR );
		return;
	}

	g_osVersion = osvi;

	if( !g_osVersion.wServicePackMajor )
	{
		logMessage( L"%ws: Detected windows version: %d.%d No Service Pack",
					dbgGetPluginName(),
					g_osVersion.dwMajorVersion,
					g_osVersion.dwMinorVersion );
	}
	else
	{
		logMessage( L"%ws: Detected windows version: %d.%d Service Pack %d",
					dbgGetPluginName(),
					g_osVersion.dwMajorVersion,
					g_osVersion.dwMinorVersion,
					g_osVersion.wServicePackMajor );
	}

	logMessage( L"%ws: This version considered as Windows %s", dbgGetPluginName(), g_osVersion.detectedVersion.osName );

	DWORD offset = osGetModuleOffsetInStruct( L"ntdll.dll", L"_PEB", L"BeingDebugged" );
	logMessage( L"%ws: BeingDebugged offset: %08X", dbgGetPluginName(), offset );

	logMessage( L"%ws: BeingDebugged offset: %08X", dbgGetPluginName(), g_osVersion.detectedVersion.BeingDebugged );
	logMessage( L"%ws: ProcessHeaps offset: %08X", dbgGetPluginName(), g_osVersion.detectedVersion.ProcessHeaps );
	logMessage( L"%ws: NtGlobalFlag offset: %08X", dbgGetPluginName(), g_osVersion.detectedVersion.NtGlobalFlag );
	logMessage( L"%ws: Flags offset: %08X", dbgGetPluginName(), g_osVersion.detectedVersion.Flags );
	logMessage( L"%ws: ForceFlags offset: %08X", dbgGetPluginName(), g_osVersion.detectedVersion.ForceFlags );
}


void osDestroy( void )
{
}


bool osDirExists( const wchar_t* dirName )
{
	DWORD attr = GetFileAttributes( dirName );
	if( attr == INVALID_FILE_ATTRIBUTES )
		return false;

	return ( attr & FILE_ATTRIBUTE_DIRECTORY ) != 0;
}


// ntdll->_PEB->BeingDebugged
DWORD osGetBeingDebuggedOffset( void )
{
	return g_osVersion.detectedVersion.BeingDebugged;
}


// ntdll->_PEB->ProcessHeaps
DWORD osGetProcessHeapsOffset( void )
{
	return g_osVersion.detectedVersion.ProcessHeaps;
}


// ntdll->_PEB->NtGlobalFlag
DWORD osGetNtGlobalFlagOffset( void )
{
	return g_osVersion.detectedVersion.NtGlobalFlag;
}


// ntdll->_HEAP->Flags
DWORD osGetFlagsOffset( void )
{
	return g_osVersion.detectedVersion.Flags;
}


// ntdll->_HEAP->ForceFlags
DWORD osGetForceFlagsOffset( void )
{
	return g_osVersion.detectedVersion.ForceFlags;
}
