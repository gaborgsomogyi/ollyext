#include "stdafx.h"
#include <dbghelp.h>
#include "Log.h"
#include "Reg.h"
#include "OS.h"


#define REG_KEY_NAME ( L"Software\\Microsoft\\Windows NT\\CurrentVersion" )


// ntdll->_PEB->BeingDebugged
static DWORD g_osBeingDebuggedOffset = 0;

// ntdll->_PEB->ProcessHeaps
static DWORD g_osProcessHeapsOffset = 0;

// ntdll->_PEB->NtGlobalFlag
static DWORD g_osNtGlobalFlagOffset = 0;

// ntdll->_HEAP->Flags
static DWORD g_osFlagsOffset = 0;

// ntdll->_HEAP->ForceFlags
static DWORD g_osForceFlagsOffset = 0;


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
	const wchar_t* productName = regReadValue( HKEY_LOCAL_MACHINE, REG_KEY_NAME, L"ProductName" );
	const wchar_t* currentVersion = regReadValue( HKEY_LOCAL_MACHINE, REG_KEY_NAME, L"CurrentVersion" );
	const wchar_t* currentBuild = regReadValue( HKEY_LOCAL_MACHINE, REG_KEY_NAME, L"CurrentBuild" );
	logMessage( L"%ws: Detected windows: %ws %ws.%ws", dbgGetPluginName(),
		productName != NULL ? productName : L"UNKNOWN",
		currentVersion != NULL ? currentVersion : L"UNKNOWN",
		currentBuild != NULL ? currentBuild : L"UNKNOWN" );
	if( productName )
	{
		delete [] productName;
		productName = NULL;
	}
	if( currentVersion )
	{
		delete [] currentVersion;
		currentVersion = NULL;
	}
	if( currentBuild )
	{
		delete [] currentBuild;
		currentBuild = NULL;
	}

	g_osBeingDebuggedOffset = osGetModuleOffsetInStruct( L"ntdll.dll", L"_PEB", L"BeingDebugged" );
	logMessage(L"%ws: BeingDebugged offset: %08X", dbgGetPluginName(), g_osBeingDebuggedOffset);

	g_osProcessHeapsOffset = osGetModuleOffsetInStruct( L"ntdll.dll", L"_PEB", L"ProcessHeaps" );
	logMessage(L"%ws: ProcessHeaps offset: %08X", dbgGetPluginName(), g_osProcessHeapsOffset);

	g_osNtGlobalFlagOffset = osGetModuleOffsetInStruct( L"ntdll.dll", L"_PEB", L"NtGlobalFlag" );
	logMessage(L"%ws: NtGlobalFlag offset: %08X", dbgGetPluginName(), g_osNtGlobalFlagOffset);

	g_osFlagsOffset = osGetModuleOffsetInStruct( L"ntdll.dll", L"_HEAP", L"Flags" );
	logMessage(L"%ws: Flags offset: %08X", dbgGetPluginName(), g_osFlagsOffset );

	g_osForceFlagsOffset = osGetModuleOffsetInStruct( L"ntdll.dll", L"_HEAP", L"ForceFlags" );
	logMessage(L"%ws: ForceFlags offset: %08X", dbgGetPluginName(), g_osForceFlagsOffset );
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
	return g_osBeingDebuggedOffset;
}


// ntdll->_PEB->ProcessHeaps
DWORD osGetProcessHeapsOffset( void )
{
	return g_osProcessHeapsOffset;
}


// ntdll->_PEB->NtGlobalFlag
DWORD osGetNtGlobalFlagOffset( void )
{
	return g_osNtGlobalFlagOffset;
}


// ntdll->_HEAP->Flags
DWORD osGetFlagsOffset( void )
{
	return g_osFlagsOffset;
}


// ntdll->_HEAP->ForceFlags
DWORD osGetForceFlagsOffset( void )
{
	return g_osForceFlagsOffset;
}
