#include "stdafx.h"
#include "Core/DBG.h"
#include "Core/Local.h"
#include "Core/Hook.h"
#include "Core/Assembler.h"
#include "NtQueryInformationProcess.h"


#define ASSEMBLER_ERROR_HANDLER() \
	delete [] localHookProc; \
	hookFree( dbgGetProcessHandle(), LIBNAME, PROCNAME, g_hook ); \
	return;

#define IS_AT_LEAST_ONE_PROTECTION_ENABLED() \
	( checkRemoteDebuggerPresent || \
		debugProcessFlags || \
		processDebugObjectHandle || \
		parentProcess )

#define IS_CONFIG_CHANGED() \
	( checkRemoteDebuggerPresent != g_checkRemoteDebuggerPresent || \
		debugProcessFlags != g_debugProcessFlags || \
		processDebugObjectHandle != g_processDebugObjectHandle || \
		parentProcess != g_parentProcess )

#define LIBNAME ( L"ntdll.dll" )
#define PROCNAME ( L"NtQueryInformationProcess" )
#define HOOK_SIZE ( 0x100 )


static bool g_applied = false;
static bool g_checkRemoteDebuggerPresent = false;
static bool g_debugProcessFlags = false;
static bool g_processDebugObjectHandle = false;
static bool g_parentProcess = false;
static sHook g_hook = { 0 };


void ntQueryInformationProcessReset( void )
{
	g_applied = false;
}


void ntQueryInformationProcessApply
(
	bool checkRemoteDebuggerPresent,
	bool processDebugObjectHandle,
	bool debugProcessFlags,
	bool parentProcess
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

		// Call original function
		ASSEMBLE_AND_STEP( L"mov eax esp", localEip );
		ASSEMBLE_AND_STEP( L"mov ecx imm:00000005", localEip );
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

		if( checkRemoteDebuggerPresent )
		{
			// If ProcessInformationClass == 0x07
			ASSEMBLE_AND_STEP( L"cmp mem4:esp,-,-,08 imm:00000007", localEip );
			ASSEMBLER_SET_LABEL( "notPortPatch", localEip );
			ASSEMBLE_AND_STEP( L"jnz brdisp:00000000", localEip );

			// Remove debug port :)
			ASSEMBLE_AND_STEP( L"mov edx mem4:esp,-,-,0c", localEip );
			ASSEMBLE_AND_STEP( L"mov mem4:edx imm:00000000", localEip );
		}

		ASSEMBLER_SET_LABEL( "notPortLabel", localEip );

		if( debugProcessFlags )
		{
			// If ProcessInformationClass == 0x1F
			ASSEMBLE_AND_STEP( L"cmp mem4:esp,-,-,08 imm:0000001F", localEip );
			ASSEMBLER_SET_LABEL( "notFlagsPatch", localEip );
			ASSEMBLE_AND_STEP( L"jnz brdisp:00000000", localEip );

			// Remove debug flags :)
			ASSEMBLE_AND_STEP( L"mov edx mem4:esp,-,-,0c", localEip );
			ASSEMBLE_AND_STEP( L"mov mem4:edx imm:00000001", localEip );
		}

		ASSEMBLER_SET_LABEL( "notFlagsLabel", localEip );

		if( processDebugObjectHandle )
		{
			// If ProcessInformationClass == 0x1E
			ASSEMBLE_AND_STEP( L"cmp mem4:esp,-,-,08 imm:0000001E", localEip );
			ASSEMBLER_SET_LABEL( "notObjectPatch", localEip );
			ASSEMBLE_AND_STEP( L"jnz brdisp:00000000", localEip );

			// Remove debug flags :)
			ASSEMBLE_AND_STEP( L"mov edx mem4:esp,-,-,0c", localEip );
			ASSEMBLE_AND_STEP( L"mov mem4:edx imm:00000000", localEip );

			// Return with STATUS_PORT_NOT_SET
			ASSEMBLE_AND_STEP( L"mov eax imm:c0000353", localEip );
		}

		ASSEMBLER_SET_LABEL( "notObjectLabel", localEip );

		if( parentProcess )
		{
			// If ProcessInformationClass == 0x00
			ASSEMBLE_AND_STEP( L"cmp mem4:esp,-,-,08 imm:00000000", localEip );
			ASSEMBLER_SET_LABEL( "notParentPatch", localEip );
			ASSEMBLE_AND_STEP( L"jnz brdisp:00000000", localEip );

			// Change parent :)
			ASSEMBLE_AND_STEP( L"mov edx mem4:esp,-,-,0c", localEip );
			std::vector<DWORD> procIdList;
			localGetProcIdList( L"explorer.exe", procIdList );
			DWORD pidOfExplorer = 0;
			if( procIdList.size() > 0 ) pidOfExplorer = procIdList[0];
			wsprintf( cmd, L"mov mem4:edx,-,-,14 imm:%08X", pidOfExplorer );
			ASSEMBLE_AND_STEP( cmd, localEip );
		}

		ASSEMBLER_SET_LABEL( "notParentLabel", localEip );

		// Return and clean the stack
		ASSEMBLE_AND_STEP( L"ret_near imm:0014", localEip );

		// Resolve addresses which only known at the end
		patchEip = ASSEMBLER_GET_LABEL( "notPortPatch" );
		if( patchEip )
		{
			labelEip = ASSEMBLER_GET_LABEL( "notPortLabel" );
			brDisp = ASSEMBLER_CALC_RELATIVE_ADDRESS( codeBase + patchEip, 6, codeBase + labelEip );
			wsprintf( cmd, L"jnz brdisp:%08X", brDisp );
			ASSEMBLE( cmd, patchEip );
		}
		patchEip = ASSEMBLER_GET_LABEL( "notFlagsPatch" );
		if( patchEip )
		{
			labelEip = ASSEMBLER_GET_LABEL( "notFlagsLabel" );
			brDisp = ASSEMBLER_CALC_RELATIVE_ADDRESS( codeBase + patchEip, 6, codeBase + labelEip );
			wsprintf( cmd, L"jnz brdisp:%08X", brDisp );
			ASSEMBLE( cmd, patchEip );
		}
		patchEip = ASSEMBLER_GET_LABEL( "notObjectPatch" );
		if( patchEip )
		{
			labelEip = ASSEMBLER_GET_LABEL( "notObjectLabel" );
			brDisp = ASSEMBLER_CALC_RELATIVE_ADDRESS( codeBase + patchEip, 6, codeBase + labelEip );
			wsprintf( cmd, L"jnz brdisp:%08X", brDisp );
			ASSEMBLE( cmd, patchEip );
		}
		patchEip = ASSEMBLER_GET_LABEL( "notParentPatch" );
		if( patchEip )
		{
			labelEip = ASSEMBLER_GET_LABEL( "notParentLabel" );
			brDisp = ASSEMBLER_CALC_RELATIVE_ADDRESS( codeBase + patchEip, 6, codeBase + labelEip );
			wsprintf( cmd, L"jnz brdisp:%08X", brDisp );
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

		g_debugProcessFlags = debugProcessFlags;
		g_checkRemoteDebuggerPresent = checkRemoteDebuggerPresent;
		g_processDebugObjectHandle = processDebugObjectHandle;
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
