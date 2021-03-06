#include "stdafx.h"
#include "Core/DBG.h"
#include "Core/Remote.h"
#include "Core/Hook.h"
#include "Core/Assembler.h"
#include "NtClose.h"


#define ASSEMBLER_ERROR_HANDLER() \
	delete [] localHookProc; \
	hookFree( dbgGetProcessHandle(), LIBNAME, PROCNAME, g_hook ); \
	return;

#define LIBNAME ( L"ntdll.dll" )
#define PROCNAME ( L"NtClose" )
#define HOOK_SIZE ( 0x100 )


static bool g_applied = false;
static sHook g_hook = { 0 };


void ntCloseReset( void )
{
	g_applied = false;
}


void ntCloseApply( bool protect )
{
	if( protect )
	{
		if( g_applied ) return;

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
		DWORD brDisp = 0;

		// Assembler init
		ASSEMBLER_FLUSH();
		ASSEMBLER_SET_CODEBASE( codeBase );
		ASSEMBLER_SET_TARGET_BUFFER( localHookProc );

		// Check whether the handle is valid :)
		ASSEMBLER_SET_LABEL( "getHandlerInfoPatch", localEip );
		ASSEMBLE_AND_STEP( L"push imm:00000000", localEip );
		ASSEMBLE_AND_STEP( L"push mem4:esp,-,-,08", localEip );
		FARPROC getHandleInformation = remoteGetProcAddress( dbgGetProcessHandle(), L"kernel32.dll", L"GetHandleInformation" );
		if( !getHandleInformation )
		{
			ASSEMBLER_ERROR_HANDLER();
		}
		brDisp = ASSEMBLER_CALC_RELATIVE_ADDRESS( codeBase + localEip, 5, getHandleInformation );
		wsprintf( cmd, L"call_near brdisp:%08X", brDisp );
		ASSEMBLE_AND_STEP( cmd, localEip );
		ASSEMBLE_AND_STEP( L"cmp eax imm:00000000", localEip );
		ASSEMBLER_SET_LABEL( "noValidHandlePatch", localEip );
		ASSEMBLE_AND_STEP( L"jz brdisp:00000000", localEip );

		// Call original function
		ASSEMBLE_AND_STEP( L"mov eax esp", localEip );
		ASSEMBLE_AND_STEP( L"mov ecx imm:00000001", localEip );
		ASSEMBLER_SET_LABEL( "loopLabel", localEip );
		ASSEMBLE_AND_STEP( L"mov edx mem4:eax,ecx,4", localEip );
		ASSEMBLE_AND_STEP( L"push edx", localEip );
		ASSEMBLE_AND_STEP( L"dec ecx", localEip );
		ASSEMBLE_AND_STEP( L"cmp ecx imm:00000000", localEip );
		labelEip = ASSEMBLER_GET_LABEL( "loopLabel" );
		brDisp = ASSEMBLER_CALC_RELATIVE_ADDRESS( codeBase + localEip, 6, codeBase + labelEip );
		wsprintf( cmd, L"jnz brdisp:%08X", brDisp );
		ASSEMBLE_AND_STEP( cmd, localEip );
		brDisp = ASSEMBLER_CALC_RELATIVE_ADDRESS( codeBase + localEip, 5, g_hook.trampolineProc );
		wsprintf( cmd, L"call_near brdisp:%08X", brDisp );
		ASSEMBLE_AND_STEP( cmd, localEip );
		ASSEMBLE_AND_STEP( L"ret_near imm:0004", localEip );

		// Return and clean the stack
		ASSEMBLER_SET_LABEL( "noValidHandleLabel", localEip );
		ASSEMBLE_AND_STEP( L"mov eax imm:c0000008", localEip );
		ASSEMBLE_AND_STEP( L"ret_near imm:0004", localEip );

		// Add space for a DWORD to get handle info
		ASSEMBLER_SET_LABEL( "getHandlerInfoLabel", localEip );
		ASSEMBLER_ADD_DWORD_AND_STEP( 0, localEip );

		// Resolve addresses which only known at the end
		patchEip = ASSEMBLER_GET_LABEL( "getHandlerInfoPatch" );
		labelEip = ASSEMBLER_GET_LABEL( "getHandlerInfoLabel" );
		wsprintf( cmd, L"push imm:%08X", codeBase + labelEip );
		ASSEMBLE( cmd, patchEip );

		patchEip = ASSEMBLER_GET_LABEL( "noValidHandlePatch" );
		labelEip = ASSEMBLER_GET_LABEL( "noValidHandleLabel" );
		brDisp = ASSEMBLER_CALC_RELATIVE_ADDRESS( codeBase + patchEip, 6, codeBase + labelEip );
		wsprintf( cmd, L"jz brdisp:%08X", brDisp );
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
