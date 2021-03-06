#include "stdafx.h"
extern "C"
{
#include "xed/xed-interface.h"
};
#include "Log.h"
#include "Assembler.h"
#include "Remote.h"
#include "Hook.h"


#define UNCONDITIONAL_JMP_SIZE ( 5 )


bool hookAlloc( HANDLE hProcess, wchar_t* libName, wchar_t* procName, sHook& rHook )
{
	BOOL writeResult = FALSE;
	wchar_t cmd[MAX_PATH] = { 0 };
	DWORD codeLen = 0;
	wchar_t assemblyError[MAX_PATH] = { 0 };

	// Get API address
	rHook.remoteTargetProc = remoteGetProcAddress( hProcess, libName, procName );
	if( !rHook.remoteTargetProc )
		return false;

	// Get number of bytes has to be saved into trampoline
	DWORD apiOrigCodeSize = 0;
	DWORD ptr = (DWORD)rHook.remoteTargetProc;
	while( apiOrigCodeSize < UNCONDITIONAL_JMP_SIZE )
	{
		BYTE buffer[XED_MAX_INSTRUCTION_BYTES];

		// Read the assembly from the target binary
		PLUGIN_CHECK_ERROR( remoteRead( hProcess, (void*)ptr, XED_MAX_INSTRUCTION_BYTES, buffer ),
			return false;,
			UNABLE_TO_READ_REMOTE_PROCESS_MEMORY, ptr, XED_MAX_INSTRUCTION_BYTES );

		// Reverse the code
		DWORD cmdSize = assemblerGetLen( buffer, assemblyError );
		PLUGIN_CHECK_ERROR( cmdSize, return false;, UNABLE_TO_DISASSEMBLE_CODE, ptr, assemblyError );

		ptr += cmdSize;
		apiOrigCodeSize += cmdSize;
	}

	// Calculate trampoline proc size
	rHook.trampolineProcSize = apiOrigCodeSize + UNCONDITIONAL_JMP_SIZE;

	// Allocate trampoline proc
	rHook.trampolineProc = VirtualAllocEx( hProcess, NULL, rHook.trampolineProcSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE );
	PLUGIN_CHECK_ERROR( rHook.trampolineProc, return false;, UNABLE_TO_ALLOCATE_REMOTE_MEMORY );

	// Copy the original data from target API function to trampoline
	writeResult = WriteProcessMemory( hProcess, rHook.trampolineProc, rHook.remoteTargetProc, apiOrigCodeSize, NULL );
	PLUGIN_CHECK_ERROR( writeResult,
		VirtualFreeEx( hProcess, rHook.trampolineProc, 0, MEM_RELEASE ); \
		return false;,
		UNABLE_TO_WRITE_MEMORY_AT, rHook.trampolineProc );

	// Assemble an unconditional jump back to the target API function
	BYTE jmpToOrigFunc[UNCONDITIONAL_JMP_SIZE] = { 0 };
	DWORD brDisp = ASSEMBLER_CALC_RELATIVE_ADDRESS( (DWORD)rHook.trampolineProc + apiOrigCodeSize, UNCONDITIONAL_JMP_SIZE, (DWORD)rHook.remoteTargetProc + apiOrigCodeSize );
	wsprintf( cmd, L"jmp brdisp:%08X", brDisp );
	codeLen = assemblerAssemble( cmd, jmpToOrigFunc, 0, assemblyError );
	PLUGIN_CHECK_ERROR( codeLen,
		VirtualFreeEx( hProcess, rHook.trampolineProc, 0, MEM_RELEASE ); \
		return false;,
		UNABLE_TO_ASSEMBLE_CODE, cmd, (DWORD)(rHook.trampolineProc) + apiOrigCodeSize, assemblyError );

	// Copy this unconditional jump into trampoline
	writeResult = WriteProcessMemory( hProcess, (void*)( (DWORD)rHook.trampolineProc + apiOrigCodeSize ), jmpToOrigFunc, UNCONDITIONAL_JMP_SIZE, NULL );
	PLUGIN_CHECK_ERROR( writeResult,
		VirtualFreeEx( hProcess, rHook.trampolineProc, 0, MEM_RELEASE ); \
		return false;,
		UNABLE_TO_WRITE_MEMORY_AT, (DWORD)rHook.trampolineProc + apiOrigCodeSize );

	// Allocate hook proc
	rHook.remoteHookProc = VirtualAllocEx( hProcess, NULL, rHook.hookSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE );
	PLUGIN_CHECK_ERROR( rHook.remoteHookProc,
		VirtualFreeEx( hProcess, rHook.trampolineProc, 0, MEM_RELEASE ); \
		return false;,
		UNABLE_TO_ALLOCATE_REMOTE_MEMORY );

	return true;
}


bool hookAttach( HANDLE hProcess, wchar_t* libName, wchar_t* procName, void* localHookProc, sHook& rHook )
{
	BOOL writeResult = FALSE;
	wchar_t cmd[64] = { 0 };
	DWORD codeLen = 0;
	wchar_t assemblyError[MAX_PATH] = { 0 };

	// Copy the hook function into the target process
	writeResult = WriteProcessMemory( hProcess, rHook.remoteHookProc, localHookProc, rHook.hookSize, NULL );
	PLUGIN_CHECK_ERROR( writeResult, return false;, UNABLE_TO_WRITE_MEMORY_AT, rHook.remoteHookProc );

	// Assemble an unconditional jump to the hook function
	BYTE jmpToHookFunc[UNCONDITIONAL_JMP_SIZE] = { 0 };
	DWORD brDisp = ASSEMBLER_CALC_RELATIVE_ADDRESS( (DWORD)rHook.remoteTargetProc, UNCONDITIONAL_JMP_SIZE, rHook.remoteHookProc );
	wsprintf( cmd, L"jmp brdisp:%08X", brDisp );
	codeLen = assemblerAssemble( cmd, jmpToHookFunc, 0, assemblyError );
	PLUGIN_CHECK_ERROR( codeLen, return false;, UNABLE_TO_ASSEMBLE_CODE, cmd, rHook.remoteHookProc, assemblyError );

	// Copy this unconditional jump into target API function
	writeResult = WriteProcessMemory( hProcess, rHook.remoteTargetProc, jmpToHookFunc, UNCONDITIONAL_JMP_SIZE, NULL );
	PLUGIN_CHECK_ERROR( writeResult, return false;, UNABLE_TO_WRITE_MEMORY_AT, rHook.remoteTargetProc );

	return true;
}


bool hookFree( HANDLE hProcess, wchar_t* libName, wchar_t* procName, sHook& rHook )
{
	BOOL readResult = FALSE;
	BOOL writeResult = FALSE;
	BOOL freeResult = FALSE;

	if( rHook.trampolineProcSize &&
		rHook.trampolineProc )
	{
		void* localTrampolineProc = new BYTE[rHook.trampolineProcSize];
		PLUGIN_CHECK_ERROR( localTrampolineProc, return false;, UNABLE_TO_ALLOCATE_LOCAL_MEMORY );

		// Copy the trampoline to a local buffer
		readResult = ReadProcessMemory( hProcess, rHook.trampolineProc, localTrampolineProc, rHook.trampolineProcSize, NULL );
		PLUGIN_CHECK_ERROR( readResult,
			delete [] localTrampolineProc; \
			return false;,
			UNABLE_TO_READ_MEMORY_AT, rHook.trampolineProc );

		// Check API function
		if( rHook.remoteTargetProc )
		{
			// Copy the saved trampoline instructions back to target API function
			writeResult = WriteProcessMemory( hProcess, rHook.remoteTargetProc, localTrampolineProc, UNCONDITIONAL_JMP_SIZE, NULL );
			PLUGIN_CHECK_ERROR( writeResult,
				delete [] localTrampolineProc; \
				return false;,
				UNABLE_TO_WRITE_MEMORY_AT, rHook.remoteTargetProc );
		}

		delete [] localTrampolineProc;
		localTrampolineProc = NULL;
	}

	// Free hook proc
	if( rHook.remoteHookProc )
	{
		freeResult = VirtualFreeEx( hProcess, rHook.remoteHookProc, 0, MEM_RELEASE );
		PLUGIN_CHECK_ERROR( freeResult, return false;, UNABLE_TO_FREE_REMOTE_MEMORY, rHook.remoteHookProc );
	}

	// Free trampoline proc
	if( rHook.trampolineProc )
	{
		freeResult = VirtualFreeEx( hProcess, rHook.trampolineProc, 0, MEM_RELEASE );
		PLUGIN_CHECK_ERROR( freeResult, return false;, UNABLE_TO_FREE_REMOTE_MEMORY, rHook.trampolineProc );
	}

	memset( &rHook, 0, sizeof( sHook ) );

	return true;
}
