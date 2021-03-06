#include "stdafx.h"
#include "Core/DBG.h"
#include "Core/Local.h"
#include "Core/Hook.h"
#include "Core/Assembler.h"
#include "Process32NextW.h"


#define ASSEMBLER_ERROR_HANDLER() \
	delete [] localHookProc; \
	hookFree( dbgGetProcessHandle(), LIBNAME, PROCNAME, g_hook ); \
	return;

#define LIBNAME ( L"kernel32.dll" )
#define PROCNAME ( L"Process32NextW" )
#define HOOK_SIZE ( 0x100 )


static bool g_applied = false;
static sHook g_hook = { 0 };


void process32NextWReset( void )
{
	g_applied = false;
}


void process32NextWApply( bool protect )
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

		// If lppe->th32ModuleID is protected
		ASSEMBLE_AND_STEP( L"mov edx mem4:esp,-,-,08", localEip );
		DWORD pid = GetCurrentProcessId();
		wsprintf( cmd, L"cmp mem4:edx,-,-,08 imm:%08X", pid );
		ASSEMBLE_AND_STEP( cmd, localEip );
		ASSEMBLER_SET_LABEL( "processNoSkipPatch", localEip );
		ASSEMBLE_AND_STEP( L"jnz brdisp:00000000", localEip );
		ASSEMBLER_SET_LABEL( "processSkipPatch", localEip );
		ASSEMBLE_AND_STEP( L"call_near brdisp:00000000", localEip );

		// If lppe->th32ParentProcessID is protected
		ASSEMBLER_SET_LABEL( "processNoSkipLabel", localEip );
		ASSEMBLE_AND_STEP( L"mov edx mem4:esp,-,-,08", localEip );
		pid = dbgGetProcessID();
		wsprintf( cmd, L"cmp mem4:edx,-,-,08 imm:%08X", pid );
		ASSEMBLE_AND_STEP( cmd, localEip );
		ASSEMBLER_SET_LABEL( "processParentPatch", localEip );
		ASSEMBLE_AND_STEP( L"jz brdisp:00000000", localEip );

		// Return and clean the stack
		ASSEMBLE_AND_STEP( L"ret_near imm:0008", localEip );

		// Call original function once again in order to skip the protected process
		ASSEMBLER_SET_LABEL( "processSkipLabel", localEip );
		ASSEMBLE_AND_STEP( L"mov eax esp", localEip );
		ASSEMBLE_AND_STEP( L"add eax imm:00000004", localEip );
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
		ASSEMBLE_AND_STEP( L"ret_near", localEip );

		// Change parent :)
		ASSEMBLER_SET_LABEL( "processParentLabel", localEip );
		std::vector<DWORD> procIdList;
		localGetProcIdList( L"explorer.exe", procIdList );
		DWORD pidOfExplorer = 0;
		if( procIdList.size() > 0 ) pidOfExplorer = procIdList[0];
		wsprintf( cmd, L"mov mem4:edx,-,-,18 imm:%08X", pidOfExplorer );
		ASSEMBLE_AND_STEP( cmd, localEip );
		ASSEMBLE_AND_STEP( L"ret_near imm:0008", localEip );

		// Resolve addresses which only known at the end
		patchEip = ASSEMBLER_GET_LABEL( "processNoSkipPatch" );
		labelEip = ASSEMBLER_GET_LABEL( "processNoSkipLabel" );
		brDisp = ASSEMBLER_CALC_RELATIVE_ADDRESS( codeBase + patchEip, 6, codeBase + labelEip );
		wsprintf( cmd, L"jnz brdisp:%08X", brDisp );
		ASSEMBLE( cmd, patchEip );

		patchEip = ASSEMBLER_GET_LABEL( "processSkipPatch" );
		labelEip = ASSEMBLER_GET_LABEL( "processSkipLabel" );
		brDisp = ASSEMBLER_CALC_RELATIVE_ADDRESS( codeBase + patchEip, 5, codeBase + labelEip );
		wsprintf( cmd, L"call_near brdisp:%08X", brDisp );
		ASSEMBLE( cmd, patchEip );

		patchEip = ASSEMBLER_GET_LABEL( "processParentPatch" );
		labelEip = ASSEMBLER_GET_LABEL( "processParentLabel" );
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
