#include "stdafx.h"
#include <winternl.h>
#include <psapi.h>
#include "Assembler.h"
#include "Local.h"
#include "Remote.h"


typedef LONG( WINAPI NTQUERYINFORMATIONPROCESS )( HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG );


DWORD remoteGetPEB( HANDLE hProcess )
{
	PLUGIN_CHECK_ERROR( hProcess, return 0;, INVALID_PROCESS_ID );

	NTQUERYINFORMATIONPROCESS* NtQueryInformationProcess = (NTQUERYINFORMATIONPROCESS*)localGetProcAddress( L"ntdll.dll", L"NtQueryInformationProcess" );
	if( !NtQueryInformationProcess ) return 0;

	ULONG bytesWritten;
	PROCESS_BASIC_INFORMATION info = { 0 };
	NTSTATUS status = (NtQueryInformationProcess)( hProcess, ProcessBasicInformation, &info, sizeof(PROCESS_BASIC_INFORMATION), &bytesWritten );
	PLUGIN_CHECK_ERROR( status == 0, return 0;, UNABLE_TO_GET_PROCESS_PEB );

	return (DWORD)info.PebBaseAddress;
}


void* remoteAlloc( HANDLE hProcess, DWORD size )
{
	PLUGIN_CHECK_ERROR( hProcess, return false;, INVALID_PROCESS_ID );

	void* remoteAddress = VirtualAllocEx( hProcess, NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE );
	PLUGIN_CHECK_ERROR( remoteAddress, return NULL;, UNABLE_TO_ALLOCATE_REMOTE_MEMORY );

	return remoteAddress;
}


void remoteFree( HANDLE hProcess, void* remoteAddress )
{
	PLUGIN_CHECK_ERROR( hProcess, return;, INVALID_PROCESS_ID );

	BOOL freeResult = VirtualFreeEx( hProcess, remoteAddress, 0, MEM_RELEASE );
	PLUGIN_CHECK_ERROR( freeResult, return;, UNABLE_TO_FREE_REMOTE_MEMORY, remoteAddress );
}


bool remoteRead( HANDLE hProcess, void* remoteAddress, DWORD remoteSize, void* localAddress )
{
	PLUGIN_CHECK_ERROR( hProcess, return false;, INVALID_PROCESS_ID );

	MEMORY_BASIC_INFORMATION memInfo;
	SIZE_T queryResult = VirtualQueryEx( hProcess, remoteAddress, &memInfo, sizeof( memInfo ) );
	PLUGIN_CHECK_ERROR( queryResult, return false;, UNABLE_TO_READ_PROTECTION_AT, remoteAddress );

	BOOL protectResult = FALSE;
	DWORD oldProtect = 0;

	if( memInfo.Protect != PAGE_EXECUTE_READ &&
		memInfo.Protect != PAGE_EXECUTE_READWRITE &&
		memInfo.Protect != PAGE_EXECUTE_WRITECOPY &&
		memInfo.Protect != PAGE_READONLY &&
		memInfo.Protect != PAGE_READWRITE &&
		memInfo.Protect != PAGE_WRITECOPY )
	{
		protectResult = VirtualProtectEx( hProcess, remoteAddress, remoteSize, PAGE_READONLY, &oldProtect );
		PLUGIN_CHECK_ERROR( protectResult, return false;, UNABLE_TO_CHANGE_PROTECTION_AT, remoteAddress );
	}

	BOOL readResult = ReadProcessMemory( hProcess, remoteAddress, localAddress, remoteSize, NULL );
	PLUGIN_CHECK_ERROR( readResult,
		if( protectResult && oldProtect ) \
		{ \
			protectResult = VirtualProtectEx( hProcess, remoteAddress, remoteSize, oldProtect, &oldProtect ); \
			PLUGIN_CHECK_ERROR( protectResult, return false;, UNABLE_TO_RESTORE_PROTECTION_AT, remoteAddress ); \
		} \
		return false;,
		UNABLE_TO_READ_MEMORY_AT, remoteAddress );

	if( protectResult && oldProtect )
	{
		protectResult = VirtualProtectEx( hProcess, remoteAddress, remoteSize, oldProtect, &oldProtect );
		PLUGIN_CHECK_ERROR( protectResult, return false;, UNABLE_TO_RESTORE_PROTECTION_AT, remoteAddress );
	}

	return true;
}


bool remoteWrite( HANDLE hProcess, const void* localAddress, DWORD localSize, void* remoteAddress )
{
	PLUGIN_CHECK_ERROR( hProcess, return false;, INVALID_PROCESS_ID );

	MEMORY_BASIC_INFORMATION memInfo;
	SIZE_T queryResult = VirtualQueryEx( hProcess, remoteAddress, &memInfo, sizeof( memInfo ) );
	PLUGIN_CHECK_ERROR( queryResult, return false;, UNABLE_TO_READ_PROTECTION_AT, remoteAddress );

	BOOL protectResult = FALSE;
	DWORD oldProtect = 0;

	if( memInfo.Protect != PAGE_EXECUTE_READWRITE &&
		memInfo.Protect != PAGE_READWRITE )
	{
		protectResult = VirtualProtectEx( hProcess, remoteAddress, localSize, PAGE_READWRITE, &oldProtect );
		PLUGIN_CHECK_ERROR( protectResult, return false;, UNABLE_TO_CHANGE_PROTECTION_AT, remoteAddress );
	}

	BOOL writeResult = WriteProcessMemory( hProcess, remoteAddress, localAddress, localSize, NULL );
	PLUGIN_CHECK_ERROR( writeResult,
		if( protectResult && oldProtect ) \
		{ \
			protectResult = VirtualProtectEx( hProcess, remoteAddress, localSize, oldProtect, &oldProtect ); \
			PLUGIN_CHECK_ERROR( protectResult, return false;, UNABLE_TO_RESTORE_PROTECTION_AT, remoteAddress ); \
		} \
		return false;,
		UNABLE_TO_WRITE_MEMORY_AT, remoteAddress );

	if( protectResult && oldProtect )
	{
		protectResult = VirtualProtectEx( hProcess, remoteAddress, localSize, oldProtect, &oldProtect );
		PLUGIN_CHECK_ERROR( protectResult, return false;, UNABLE_TO_RESTORE_PROTECTION_AT, remoteAddress );
	}

	return true;
}


bool remoteExec( HANDLE hProcess, void* remoteParamAddress, void* remoteCodeAddress )
{
	PLUGIN_CHECK_ERROR( hProcess, return false;, INVALID_PROCESS_ID );

	HANDLE hThread = CreateRemoteThread
						(
							hProcess,
							NULL,
							0,
							(LPTHREAD_START_ROUTINE)remoteCodeAddress,
							remoteParamAddress,
							0,
							NULL
						);
	PLUGIN_CHECK_ERROR( hThread, return false;, UNABLE_TO_EXECUTE_REMOTE_THREAD );

	WaitForSingleObject( hThread, INFINITE );
	CloseHandle( hThread );

	return true;
}


HMODULE remoteGetModuleHandle( HANDLE hProcess, const wchar_t* libName )
{
	PLUGIN_CHECK_ERROR( hProcess, return NULL;, INVALID_PROCESS_ID );

	HMODULE hModule = localLoadLibrary( libName );
	if( !hModule ) return NULL;

	MODULEINFO modInfo = { 0 };
	BOOL moduleResult = GetModuleInformation( hProcess, hModule, &modInfo, sizeof( MODULEINFO ) );
	PLUGIN_CHECK_ERROR_NO_MB( moduleResult, return NULL; );

	return (HMODULE)modInfo.lpBaseOfDll;
}


FARPROC remoteGetProcAddress( HANDLE hProcess, const wchar_t* libName, const wchar_t* procName )
{
	PLUGIN_CHECK_ERROR( hProcess, return NULL;, INVALID_PROCESS_ID );

	HMODULE hLocalModule = localLoadLibrary( libName );
	if( !hLocalModule ) return NULL;

	HMODULE hRemoteModule = remoteGetModuleHandle( hProcess, libName );
	if( !hRemoteModule ) return NULL;

	FARPROC localProcAddress = localGetProcAddress( libName, procName );
	if( !localProcAddress ) return NULL;

	FARPROC remoteProcAddress = localProcAddress;
	if( (DWORD)hRemoteModule < (DWORD)hLocalModule )
	{
		remoteProcAddress = (FARPROC)( (DWORD)remoteProcAddress - ( (DWORD)hLocalModule - (DWORD)hRemoteModule ) );
	}
	else if( (DWORD)hRemoteModule > (DWORD)hLocalModule )
	{
		remoteProcAddress = (FARPROC)( (DWORD)remoteProcAddress + ( (DWORD)hRemoteModule - (DWORD)hLocalModule ) );
	}

	return remoteProcAddress;
}


bool remotePatchApply( HANDLE hProcess, void* remoteAddress, BYTE* origCode, BYTE* patchCode, DWORD size )
{
	PLUGIN_CHECK_ERROR( hProcess, return false;, INVALID_PROCESS_ID );

	if( origCode )
	{
		bool readSuccess = remoteRead( hProcess, remoteAddress, size, origCode );
		if( !readSuccess ) return false;
	}

	bool writeSuccess = remoteWrite( hProcess, patchCode, size, remoteAddress );
	if( !writeSuccess ) return false;

	return true;
}


bool remotePatchApply( HANDLE hProcess, const wchar_t* libName, const wchar_t* procName, BYTE* origCode, BYTE* patchCode, DWORD size )
{
	PLUGIN_CHECK_ERROR( hProcess, return false;, INVALID_PROCESS_ID );

	FARPROC procAddress = remoteGetProcAddress( hProcess, libName, procName );
	if( !procAddress ) return false;

	return remotePatchApply( hProcess, (void*)procAddress, origCode, patchCode, size );
}
