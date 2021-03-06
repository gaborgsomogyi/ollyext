#include "stdafx.h"
#include "Core/DBG.h"
#include "Core/Hook.h"
#include "Core/Assembler.h"
#include "Core/Remote.h"
#include "TimeGetTime.h"


#define ASSEMBLER_ERROR_HANDLER() \
	delete [] localHookProc; \
	hookFree( dbgGetProcessHandle(), LIBNAME, PROCNAME, g_hook ); \
	return;

#define LIBNAME ( L"winmm.dll" )
#define PROCNAME ( L"timeGetTime" )
#define HOOK_SIZE ( 0x100 )


static bool g_applied = false;
static sHook g_hook = { 0 };


void timeGetTimeReset( void )
{
	g_applied = false;
}


void timeGetTimeApply( bool protect )
{
	if( protect )
	{
		if( g_applied ) return;

		HMODULE hModule = remoteGetModuleHandle( dbgGetProcessHandle(), LIBNAME );
		if( !hModule ) return;

		g_hook.remoteTargetProc = NULL;
		g_hook.trampolineProcSize = 0;
		g_hook.trampolineProc = NULL;
		g_hook.hookSize = HOOK_SIZE;
		g_hook.remoteHookProc = NULL;

		bool allocResult = hookAlloc( dbgGetProcessHandle(), LIBNAME, PROCNAME, g_hook );
		PLUGIN_CHECK_ERROR( allocResult, return;, UNABLE_TO_ALLOC_HOOK, LIBNAME, PROCNAME );

		BYTE* localHookProc = new BYTE[HOOK_SIZE];
		PLUGIN_CHECK_ERROR( localHookProc,
			hookFree( dbgGetProcessHandle(), LIBNAME, PROCNAME, g_hook ); \
			return;,
			UNABLE_TO_ALLOCATE_LOCAL_MEMORY );

		memset( localHookProc, 0x90, HOOK_SIZE );

		wchar_t cmd[64] = { 0 };
		DWORD codeBase = (DWORD)g_hook.remoteHookProc;
		DWORD localEip = 0;
		DWORD patchEip = 0;
		DWORD labelEip = 0;

		// Assembler init
		ASSEMBLER_FLUSH();
		ASSEMBLER_SET_CODEBASE( codeBase );
		ASSEMBLER_SET_TARGET_BUFFER( localHookProc );

		// Return and clean the stack
		ASSEMBLER_SET_LABEL( "movCounterPatch", localEip );
		ASSEMBLE_AND_STEP( L"mov eax mem4:-,-,-,00000000", localEip );
		ASSEMBLER_SET_LABEL( "readIncPatch", localEip );
		ASSEMBLE_AND_STEP( L"mov edx mem4:-,-,-,00000000", localEip );
		ASSEMBLER_SET_LABEL( "incCounterPatch", localEip );
		ASSEMBLE_AND_STEP( L"add mem4:-,-,-,00000000 edx", localEip );
		ASSEMBLE_AND_STEP( L"ret_near", localEip );
		ASSEMBLER_SET_LABEL( "counterAddress", localEip );
		ASSEMBLER_ADD_DWORD_AND_STEP( timeGetTime(), localEip );
		ASSEMBLER_SET_LABEL( "incAddress", localEip );
		ASSEMBLER_ADD_DWORD_AND_STEP( 1, localEip );

		// Resolve addresses which only known at the end
		patchEip = ASSEMBLER_GET_LABEL( "movCounterPatch" );
		labelEip = ASSEMBLER_GET_LABEL( "counterAddress" );
		wsprintf( cmd, L"mov eax mem4:-,-,-,%08X", codeBase + labelEip );
		ASSEMBLE( cmd, patchEip );

		patchEip = ASSEMBLER_GET_LABEL( "readIncPatch" );
		labelEip = ASSEMBLER_GET_LABEL( "incAddress" );
		wsprintf( cmd, L"mov edx mem4:-,-,-,%08X", codeBase + labelEip );
		ASSEMBLE( cmd, patchEip );

		patchEip = ASSEMBLER_GET_LABEL( "incCounterPatch" );
		labelEip = ASSEMBLER_GET_LABEL( "counterAddress" );
		wsprintf( cmd, L"add mem4:-,-,-,%08X edx", codeBase + labelEip );
		ASSEMBLE( cmd, patchEip );

		ASSEMBLER_FLUSH();

		bool attachResult = hookAttach( dbgGetProcessHandle(), LIBNAME, PROCNAME, localHookProc, g_hook );
		PLUGIN_CHECK_ERROR( attachResult,
			delete [] localHookProc; \
			hookFree( dbgGetProcessHandle(), LIBNAME, PROCNAME, g_hook ); \
			return;,
			UNABLE_TO_ATTACH_HOOK, LIBNAME, PROCNAME );

		delete [] localHookProc;

		g_applied = true;
	}
	else
	{
		if( !g_applied ) return;

		bool freeResult = hookFree( dbgGetProcessHandle(), LIBNAME, PROCNAME, g_hook );
		PLUGIN_CHECK_ERROR( freeResult, return;, UNABLE_TO_FREE_HOOK, LIBNAME, PROCNAME );

		g_applied = false;
	}
}
