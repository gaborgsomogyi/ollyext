#include "stdafx.h"
#include "Core/DBG.h"
#include "Core/Local.h"
#include "Core/Remote.h"
#include "Core/Hook.h"
#include "Core/Assembler.h"
#include "NtOpenProcess.h"


#define ASSEMBLER_ERROR_HANDLER() \
	delete [] localHookProc; \
	hookFree( dbgGetProcessHandle(), LIBNAME, PROCNAME, g_hook ); \
	return;

#define IS_AT_LEAST_ONE_PROTECTION_ENABLED() \
	( seDebugPrivilege || \
		ntOpenProcess )

#define IS_CONFIG_CHANGED() \
	( seDebugPrivilege != g_seDebugPrivilege || \
		ntOpenProcess != g_ntOpenProcess )

#define LIBNAME ( L"ntdll.dll" )
#define PROCNAME ( L"NtOpenProcess" )
#define HOOK_SIZE ( 0x100 )


static bool g_applied = false;
static bool g_seDebugPrivilege = false;
static bool g_ntOpenProcess = false;
static sHook g_hook = { 0 };


void ntOpenProcessReset( void )
{
	g_applied = false;
}


void ntOpenProcessApply
(
	bool seDebugPrivilege,
	bool ntOpenProcess
)
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

		char label[64] = { 0 };
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

		// If ClientId->UniqueProcess is protected
		if( seDebugPrivilege )
		{
			ASSEMBLE_AND_STEP( L"mov edx mem4:esp,-,-,10", localEip );
			std::vector<DWORD> procIdList;
			localGetProcIdList( L"csrss.exe", procIdList );
			for( DWORD i = 0; i < procIdList.size(); ++i )
			{
				wsprintf( cmd, L"cmp mem4:edx imm:%08X", procIdList[i] );
				ASSEMBLE_AND_STEP( cmd, localEip );
				sprintf_s( label, sizeof( label ), "processAccessDeniedPatch%d", i );
				ASSEMBLER_SET_LABEL( label, localEip );
				ASSEMBLE_AND_STEP( L"jz brdisp:00000000", localEip );
			}
		}
		if( ntOpenProcess )
		{
			DWORD pid = GetCurrentProcessId();
			ASSEMBLE_AND_STEP( L"mov edx mem4:esp,-,-,10", localEip );
			wsprintf( cmd, L"cmp mem4:edx imm:%08X", pid );
			ASSEMBLE_AND_STEP( cmd, localEip );
			ASSEMBLER_SET_LABEL( "processInvalidCIDPatch", localEip );
			ASSEMBLE_AND_STEP( L"jz brdisp:00000000", localEip );
		}

		// Call original function
		ASSEMBLE_AND_STEP( L"mov eax esp", localEip );
		ASSEMBLE_AND_STEP( L"mov ecx imm:00000004", localEip );
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
		ASSEMBLE_AND_STEP( L"ret_near imm:0010", localEip );

		// Return STATUS_INVALID_CID and clean the stack
		ASSEMBLER_SET_LABEL( "processInvalidCIDLabel", localEip );
		ASSEMBLE_AND_STEP( L"xor eax eax", localEip );
		ASSEMBLE_AND_STEP( L"mov edx mem4:esp,-,-,04", localEip );
		ASSEMBLE_AND_STEP( L"mov mem4:edx eax", localEip );
		ASSEMBLE_AND_STEP( L"mov eax imm:c000000b", localEip );
		ASSEMBLE_AND_STEP( L"ret_near imm:0010", localEip );

		// Return STATUS_ACCESS_DENIED and clean the stack
		ASSEMBLER_SET_LABEL( "processAccessDeniedLabel", localEip );
		ASSEMBLE_AND_STEP( L"xor eax eax", localEip );
		ASSEMBLE_AND_STEP( L"mov edx mem4:esp,-,-,04", localEip );
		ASSEMBLE_AND_STEP( L"mov mem4:edx eax", localEip );
		ASSEMBLE_AND_STEP( L"mov eax imm:c0000022", localEip );
		ASSEMBLE_AND_STEP( L"ret_near imm:0010", localEip );

		// Resolve addresses which only known at the end
		patchEip = ASSEMBLER_GET_LABEL( "processInvalidCIDPatch" );
		labelEip = ASSEMBLER_GET_LABEL( "processInvalidCIDLabel" );
		brDisp = ASSEMBLER_CALC_RELATIVE_ADDRESS( codeBase + patchEip, 6, codeBase + labelEip );
		wsprintf( cmd, L"jz brdisp:%08X", brDisp );
		ASSEMBLE( cmd, patchEip );

		labelEip = ASSEMBLER_GET_LABEL( "processAccessDeniedLabel" );
		DWORD i = 0;
		while( true )
		{
			sprintf_s( label, sizeof( label ), "processAccessDeniedPatch%d", i++ );
			patchEip = ASSEMBLER_GET_LABEL( label );
			if( !patchEip ) break;
			brDisp = ASSEMBLER_CALC_RELATIVE_ADDRESS( codeBase + patchEip, 6, codeBase + labelEip );
			wsprintf( cmd, L"jz brdisp:%08X", brDisp );
			ASSEMBLE( cmd, patchEip );
		}

		ASSEMBLER_FLUSH();

		bool attachResult = hookAttach( dbgGetProcessHandle(), LIBNAME, PROCNAME, localHookProc, g_hook );
		PLUGIN_CHECK_ERROR( attachResult,
			delete [] localHookProc; \
			hookFree( dbgGetProcessHandle(), LIBNAME, PROCNAME, g_hook ); \
			return;,
			UNABLE_TO_ATTACH_HOOK, LIBNAME, PROCNAME );

		delete [] localHookProc;

		g_seDebugPrivilege = seDebugPrivilege;
		g_ntOpenProcess = ntOpenProcess;
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
