#include "stdafx.h"
#include "Core/DBG.h"
#include "Core/Hook.h"
#include "Core/Assembler.h"
#include "Core/Remote.h"
#include "FindWindowExA.h"


#define ASSEMBLER_ERROR_HANDLER() \
	delete [] localHookProc; \
	hookFree( dbgGetProcessHandle(), LIBNAME, PROCNAME, g_hook ); \
	return;

#define LIBNAME ( L"user32.dll" )
#define PROCNAME ( L"FindWindowExA" )
#define HOOK_SIZE ( 0x200 )
#define CLASS_NAME ( "OLLYDBG" )

static bool g_applied = false;
static sHook g_hook = { 0 };


void findWindowExAReset( void )
{
	g_applied = false;
}


void findWindowExAApply( bool protect )
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
		DWORD brDisp = 0;

		// Assembler init
		ASSEMBLER_FLUSH();
		ASSEMBLER_SET_CODEBASE( codeBase );
		ASSEMBLER_SET_TARGET_BUFFER( localHookProc );

		// Push used registers
		ASSEMBLE_AND_STEP( L"push esi", localEip );
		ASSEMBLE_AND_STEP( L"push edi", localEip );

		// If lpClassName is OLLYDBG
		ASSEMBLE_AND_STEP( L"mov esi mem4:esp,-,-,14", localEip );
		ASSEMBLE_AND_STEP( L"cmp esi imm:00000000", localEip );
		ASSEMBLER_SET_LABEL( "nullClassNamePatch", localEip );
		ASSEMBLE_AND_STEP( L"jz brdisp:00000000", localEip );
		ASSEMBLER_SET_LABEL( "classNamePatch", localEip );
		ASSEMBLE_AND_STEP( L"mov edi imm:00000000", localEip );
		ASSEMBLER_SET_LABEL( "loopLabel", localEip );
		ASSEMBLE_AND_STEP( L"mov al mem1:esi", localEip );
		ASSEMBLE_AND_STEP( L"mov dl mem1:edi", localEip );
		ASSEMBLE_AND_STEP( L"inc esi", localEip );
		ASSEMBLE_AND_STEP( L"inc edi", localEip );
		ASSEMBLE_AND_STEP( L"and al imm:DF", localEip );
		ASSEMBLE_AND_STEP( L"cmp al dl", localEip );
		ASSEMBLER_SET_LABEL( "notSameClassPatch", localEip );
		ASSEMBLE_AND_STEP( L"jnz brdisp:00000000", localEip );
		ASSEMBLE_AND_STEP( L"cmp al imm:00", localEip );
		ASSEMBLER_SET_LABEL( "sameClassPatch", localEip );
		ASSEMBLE_AND_STEP( L"jz brdisp:00000000", localEip );
		labelEip = ASSEMBLER_GET_LABEL( "loopLabel" );
		brDisp = ASSEMBLER_CALC_RELATIVE_ADDRESS( codeBase + localEip, 5, codeBase + labelEip );
		wsprintf( cmd, L"jmp brdisp:%08X", brDisp );
		ASSEMBLE_AND_STEP( cmd, localEip );

		// Pop used registers
		ASSEMBLER_SET_LABEL( "nullClassNameLabel", localEip );
		ASSEMBLER_SET_LABEL( "notSameClassLabel", localEip );
		ASSEMBLE_AND_STEP( L"pop edi", localEip );
		ASSEMBLE_AND_STEP( L"pop esi", localEip );

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

		// Return and clean the stack
		ASSEMBLER_SET_LABEL( "sameClassLabel", localEip );
		ASSEMBLE_AND_STEP( L"pop edi", localEip );
		ASSEMBLE_AND_STEP( L"pop esi", localEip );
		ASSEMBLE_AND_STEP( L"xor eax eax", localEip );
		ASSEMBLE_AND_STEP( L"ret_near imm:0010", localEip );

		// Add strings
		ASSEMBLER_SET_LABEL( "classNameLabel", localEip );
		ASSEMBLER_ADD_STRING_AND_STEP( CLASS_NAME, localEip );

		// Resolve addresses which only known at the end
		patchEip = ASSEMBLER_GET_LABEL( "nullClassNamePatch" );
		labelEip = ASSEMBLER_GET_LABEL( "nullClassNameLabel" );
		brDisp = ASSEMBLER_CALC_RELATIVE_ADDRESS( codeBase + patchEip, 6, codeBase + labelEip );
		wsprintf( cmd, L"jz brdisp:%08X", brDisp );
		ASSEMBLE( cmd, patchEip );

		patchEip = ASSEMBLER_GET_LABEL( "classNamePatch" );
		labelEip = ASSEMBLER_GET_LABEL( "classNameLabel" );
		wsprintf( cmd, L"mov edi imm:%08X", codeBase + labelEip );
		ASSEMBLE( cmd, patchEip );

		patchEip = ASSEMBLER_GET_LABEL( "notSameClassPatch" );
		labelEip = ASSEMBLER_GET_LABEL( "notSameClassLabel" );
		brDisp = ASSEMBLER_CALC_RELATIVE_ADDRESS( codeBase + patchEip, 6, codeBase + labelEip );
		wsprintf( cmd, L"jnz brdisp:%08X", brDisp );
		ASSEMBLE( cmd, patchEip );

		patchEip = ASSEMBLER_GET_LABEL( "sameClassPatch" );
		labelEip = ASSEMBLER_GET_LABEL( "sameClassLabel" );
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
