#include "stdafx.h"
#include "Core/DBG.h"
#include "Core/Hook.h"
#include "Core/Assembler.h"
#include "KiUserExceptionDispatcher.h"


#define ASSEMBLER_ERROR_HANDLER() \
	delete [] localHookProc; \
	hookFree( dbgGetProcessHandle(), LIBNAME, PROCNAME, g_hook ); \
	return;

#define IS_AT_LEAST_ONE_PROTECTION_ENABLED() \
	( protectDRX || \
		hideDRX )

#define IS_CONFIG_CHANGED() \
	( protectDRX != g_protectDRX || \
		hideDRX != g_hideDRX )

#define LIBNAME ( L"ntdll.dll" )
#define PROCNAME ( L"KiUserExceptionDispatcher" )
#define HOOK_SIZE ( 0x100 )


static bool g_applied = false;
static bool g_protectDRX = false;
static bool g_hideDRX = false;
static sHook g_hook = { 0 };
DWORD g_drxRegistersChangedAddress = 0;
DWORD g_drxRegistersAddress = 0;


void kiUserExceptionDispatcherReset( void )
{
	g_applied = false;
}


void kiUserExceptionDispatcherApply( bool protectDRX, bool hideDRX )
{
	if( IS_AT_LEAST_ONE_PROTECTION_ENABLED() )
	{
		// If it's already applied but the config not changed just return
		if( g_applied &&
			!IS_CONFIG_CHANGED() ) return;

		// Here we have 2 chances
		// 1. Hook not applied -> just apply it
		// 2. Hook applied and config changed -> remove old hook and apply new one
		if( g_applied )
		{
			bool freeResult = hookFree( dbgGetProcessHandle(), LIBNAME, PROCNAME, g_hook );
			PLUGIN_CHECK_ERROR( freeResult, return;, UNABLE_TO_FREE_HOOK, LIBNAME, PROCNAME );

			g_applied = false;
		}

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

		// Save DRX registers
		if( protectDRX )
		{
			ASSEMBLE_AND_STEP( L"push esi", localEip );
			ASSEMBLE_AND_STEP( L"push edi", localEip );

			ASSEMBLE_AND_STEP( L"mov esi mem4:esp,-,-,0c", localEip );
			ASSEMBLE_AND_STEP( L"add esi imm:00000004", localEip );
			ASSEMBLER_SET_LABEL( "registersPatch", localEip );
			ASSEMBLE_AND_STEP( L"mov edi imm:00000000", localEip );
			ASSEMBLE_AND_STEP( L"mov ecx imm:00000006", localEip );
			ASSEMBLE_AND_STEP( L"REP movsd", localEip );

			ASSEMBLE_AND_STEP( L"pop edi", localEip );
			ASSEMBLE_AND_STEP( L"pop esi", localEip );

			ASSEMBLER_SET_LABEL( "registersChangedPatch", localEip );
			ASSEMBLE_AND_STEP( L"mov mem1:-,-,-,00000000 imm:01", localEip );
		}

		// Reset DRX registers
		if( hideDRX )
		{
			ASSEMBLE_AND_STEP( L"mov eax mem4:esp,-,-,04", localEip );
			ASSEMBLE_AND_STEP( L"mov mem4:eax,-,-,04 imm:00000000", localEip );
			ASSEMBLE_AND_STEP( L"mov mem4:eax,-,-,08 imm:00000000", localEip );
			ASSEMBLE_AND_STEP( L"mov mem4:eax,-,-,0C imm:00000000", localEip );
			ASSEMBLE_AND_STEP( L"mov mem4:eax,-,-,10 imm:00000000", localEip );
			ASSEMBLE_AND_STEP( L"mov mem4:eax,-,-,14 imm:ffff0ff0", localEip );
			ASSEMBLE_AND_STEP( L"mov mem4:eax,-,-,18 imm:00000100", localEip );
		}

		// Go to original handler with faked DRX registers
		brDisp = ASSEMBLER_CALC_RELATIVE_ADDRESS( codeBase + localEip, 5, g_hook.trampolineProc );
		wsprintf( cmd, L"jmp brdisp:%08X", brDisp );
		ASSEMBLE_AND_STEP( cmd, localEip );

		// Original DRX registers
		ASSEMBLER_SET_LABEL( "registersLabel", localEip );
		ASSEMBLER_ADD_DWORD_AND_STEP( 0, localEip );
		ASSEMBLER_ADD_DWORD_AND_STEP( 0, localEip );
		ASSEMBLER_ADD_DWORD_AND_STEP( 0, localEip );
		ASSEMBLER_ADD_DWORD_AND_STEP( 0, localEip );
		ASSEMBLER_ADD_DWORD_AND_STEP( 0, localEip );
		ASSEMBLER_ADD_DWORD_AND_STEP( 0, localEip );

		// Stored flag
		ASSEMBLER_SET_LABEL( "registersChangedLabel", localEip );
		ASSEMBLER_ADD_BYTE_AND_STEP( 0, localEip );

		// Resolve addresses which only known at the end
		patchEip = ASSEMBLER_GET_LABEL( "registersPatch" );
		if( patchEip )
		{
			labelEip = ASSEMBLER_GET_LABEL( "registersLabel" );
			wsprintf( cmd, L"mov edi imm:%08X", codeBase + labelEip );
			ASSEMBLE( cmd, patchEip );
			g_drxRegistersAddress = codeBase + labelEip;
		}

		patchEip = ASSEMBLER_GET_LABEL( "registersChangedPatch" );
		if( patchEip )
		{
			labelEip = ASSEMBLER_GET_LABEL( "registersChangedLabel" );
			wsprintf( cmd, L"mov mem1:-,-,-,%08X imm:01", codeBase + labelEip );
			ASSEMBLE( cmd, patchEip );
			g_drxRegistersChangedAddress = codeBase + labelEip;
		}

		ASSEMBLER_FLUSH();

		bool attachResult = hookAttach( dbgGetProcessHandle(), LIBNAME, PROCNAME, localHookProc, g_hook );
		PLUGIN_CHECK_ERROR( attachResult,
			delete [] localHookProc; \
			hookFree( dbgGetProcessHandle(), LIBNAME, PROCNAME, g_hook ); \
			return;,
			UNABLE_TO_ATTACH_HOOK, LIBNAME, PROCNAME );

		delete [] localHookProc;

		g_protectDRX = protectDRX;
		g_hideDRX = hideDRX;
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
