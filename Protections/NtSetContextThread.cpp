#include "stdafx.h"
#include "Core/DBG.h"
#include "Core/Hook.h"
#include "Core/Assembler.h"
#include "ZwGetContextThread.h"
#include "NtSetContextThread.h"


#define ASSEMBLER_ERROR_HANDLER() \
	delete [] localHookProc; \
	hookFree( dbgGetProcessHandle(), LIBNAME, PROCNAME, g_hook ); \
	return;

#define LIBNAME ( L"ntdll.dll" )
#define PROCNAME ( L"NtSetContextThread" )
#define HOOK_SIZE ( 0x400 )


static bool g_applied = false;
static sHook g_hook = { 0 };


void ntSetContextThreadReset( void )
{
	g_applied = false;
}


void ntSetContextThreadApply( bool protect )
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

		// Get current context
		void* zwGetContextThread = zwGetContextThreadOrig();
		if( !zwGetContextThread )
		{
			ASSEMBLER_ERROR_HANDLER();
		}
		ASSEMBLER_SET_LABEL( "contextPatch", localEip );
		ASSEMBLE_AND_STEP( L"push imm:00000000", localEip );
		ASSEMBLE_AND_STEP( L"push mem4:esp,-,-,08", localEip );
		brDisp = ASSEMBLER_CALC_RELATIVE_ADDRESS( codeBase + localEip, 5, zwGetContextThread );
		wsprintf( cmd, L"call_near brdisp:%08X", brDisp );
		ASSEMBLE_AND_STEP( cmd, localEip );

		// Set DRX registers with the actual values
		ASSEMBLE_AND_STEP( L"push esi", localEip );
		ASSEMBLE_AND_STEP( L"push edi", localEip );
		ASSEMBLER_SET_LABEL( "copyPatch", localEip );
		ASSEMBLE_AND_STEP( L"mov esi imm:00000000", localEip );
		ASSEMBLE_AND_STEP( L"mov edi mem4:esp,-,-,10", localEip );
		ASSEMBLE_AND_STEP( L"add edi imm:00000004", localEip );
		ASSEMBLE_AND_STEP( L"mov ecx imm:00000006", localEip );
		ASSEMBLE_AND_STEP( L"REP movsd", localEip );
		ASSEMBLE_AND_STEP( L"pop edi", localEip );
		ASSEMBLE_AND_STEP( L"pop esi", localEip );

		// Call original function
		ASSEMBLE_AND_STEP( L"mov eax esp", localEip );
		ASSEMBLE_AND_STEP( L"mov ecx imm:00000002", localEip );
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

		// Return and clean the stack
		ASSEMBLE_AND_STEP( L"ret_near imm:0008", localEip );

		// Add space for context
		localEip = ( localEip + ( 4 - 1 ) ) & -4;
		CONTEXT context = { CONTEXT_DEBUG_REGISTERS };
		ASSEMBLER_SET_LABEL( "contextLabel", localEip );
		ASSEMBLER_ADD_ARRAY_AND_STEP( &context, sizeof( CONTEXT ), localEip );

		// Resolve addresses which only known at the end
		labelEip = ASSEMBLER_GET_LABEL( "contextLabel" );

		patchEip = ASSEMBLER_GET_LABEL( "contextPatch" );
		wsprintf( cmd, L"push imm:%08X", codeBase + labelEip );
		ASSEMBLE( cmd, patchEip );

		patchEip = ASSEMBLER_GET_LABEL( "copyPatch" );
		wsprintf( cmd, L"mov esi imm:%08X", codeBase + labelEip + 0x04 );
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
