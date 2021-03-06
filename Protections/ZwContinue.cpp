#include "stdafx.h"
#include "Core/DBG.h"
#include "Core/Hook.h"
#include "Core/Assembler.h"
#include "ZwContinue.h"
#include "KiUserExceptionDispatcher.h"


#define ASSEMBLER_ERROR_HANDLER() \
	delete [] localHookProc; \
	hookFree( dbgGetProcessHandle(), LIBNAME, PROCNAME, g_hook ); \
	return;

#define LIBNAME ( L"ntdll.dll" )
#define PROCNAME ( L"ZwContinue" )
#define HOOK_SIZE ( 0x100 )


static bool g_applied = false;
static sHook g_hook = { 0 };


void zwContinueReset( void )
{
	g_applied = false;
}


void zwContinueApply( bool protect )
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

		// Check wheather we saved something
		ASSEMBLER_SET_LABEL( "registersChangedPatch", localEip );
		ASSEMBLE_AND_STEP( L"cmp mem1:-,-,-,00000000 imm:00", localEip );
		ASSEMBLER_SET_LABEL( "notSavedPatch", localEip );
		ASSEMBLE_AND_STEP( L"jz brdisp:00000000", localEip );
		ASSEMBLER_SET_LABEL( "registersChangedResetPatch", localEip );
		ASSEMBLE_AND_STEP( L"mov mem1:-,-,-,00000000 imm:00", localEip );

		// Restore DRX registers
		ASSEMBLE_AND_STEP( L"push esi", localEip );
		ASSEMBLE_AND_STEP( L"push edi", localEip );

		ASSEMBLER_SET_LABEL( "registersPatch", localEip );
		ASSEMBLE_AND_STEP( L"mov esi imm:00000000", localEip );
		ASSEMBLE_AND_STEP( L"mov edi mem4:esp,-,-,0c", localEip );
		ASSEMBLE_AND_STEP( L"add edi imm:00000004", localEip );
		ASSEMBLE_AND_STEP( L"mov ecx imm:00000006", localEip );
		ASSEMBLE_AND_STEP( L"REP movsd", localEip );

		ASSEMBLE_AND_STEP( L"pop edi", localEip );
		ASSEMBLE_AND_STEP( L"pop esi", localEip );

		ASSEMBLER_SET_LABEL( "notSavedLabel", localEip );

		// Go to original handler with faked DRX registers
		brDisp = ASSEMBLER_CALC_RELATIVE_ADDRESS( codeBase + localEip, 5, g_hook.trampolineProc );
		wsprintf( cmd, L"jmp brdisp:%08X", brDisp );
		ASSEMBLE_AND_STEP( cmd, localEip );

		// Resolve addresses which only known at the end
		patchEip = ASSEMBLER_GET_LABEL( "registersChangedPatch" );
		wsprintf( cmd, L"cmp mem1:-,-,-,%08X imm:00", g_drxRegistersChangedAddress );
		ASSEMBLE( cmd, patchEip );

		patchEip = ASSEMBLER_GET_LABEL( "notSavedPatch" );
		labelEip = ASSEMBLER_GET_LABEL( "notSavedLabel" );
		brDisp = ASSEMBLER_CALC_RELATIVE_ADDRESS( codeBase + patchEip, 6, codeBase + labelEip );
		wsprintf( cmd, L"jz brdisp:%08X", brDisp );
		ASSEMBLE( cmd, patchEip );

		patchEip = ASSEMBLER_GET_LABEL( "registersChangedResetPatch" );
		wsprintf( cmd, L"mov mem1:-,-,-,%08X imm:00", g_drxRegistersChangedAddress );
		ASSEMBLE( cmd, patchEip );

		patchEip = ASSEMBLER_GET_LABEL( "registersPatch" );
		wsprintf( cmd, L"mov esi imm:%08X", g_drxRegistersAddress );
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
