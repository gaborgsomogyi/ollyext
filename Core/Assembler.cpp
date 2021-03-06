#include "stdafx.h"
#include "xed/xed-enc-lang.H"
#include "Assembler.h"


static xed_state_t g_state;
static ascii_encode_request_t g_request;
std::hash_map<std::string, DWORD> g_asmLabels;
DWORD g_asmCodeBase = NULL;
BYTE* g_asmBuffer = NULL;


void assemblerInit( void )
{
	xed_state_zero( &g_state );
	xed_state_init2( &g_state, XED_MACHINE_MODE_LEGACY_32, XED_ADDRESS_WIDTH_32b );
	xed_tables_init();
}


DWORD assemblerAssemble( const wchar_t* pCmd, DWORD localEip, wchar_t* pAssemblyError )
{
	return assemblerAssemble( pCmd, g_asmBuffer, localEip, pAssemblyError );
}


DWORD assemblerAssemble( const wchar_t* pCmd, BYTE* pBuffer, DWORD localEip, wchar_t* pAssemblyError )
{
    unsigned int inputLen = XED_MAX_INSTRUCTION_BYTES;
    unsigned int outputLen = 0;

	// Prepare request
	g_request.dstate = g_state;
	char cmdA[MAX_PATH] = { 0 };
	wcstombs( cmdA, pCmd, MAX_PATH );
	g_request.command = cmdA;

	// Assemble it
	xed_encoder_request_t req = parse_encode_request( g_request );
    xed_error_enum_t r = xed_encode( &req, &pBuffer[localEip], inputLen, &outputLen );
	if( r != XED_ERROR_NONE )
	{
		outputLen = 0;
		const char* pError = xed_error_enum_t2str( r );
		mbstowcs( pAssemblyError, pError, MAX_PATH );
	}

	return outputLen;
}


DWORD assemblerGetLen( const BYTE* pBuffer, wchar_t* pAssemblyError )
{
	PLUGIN_CHECK_ERROR( *pBuffer != 0xcc, return 0;, BREAKPOINT_FOUND_IN_ASSEMBLY );

	unsigned int instLen = 0;

	xed_decoded_inst_t xde = { 0 };
	xed_decoded_inst_zero_set_mode( &xde, &g_state );

	xed_error_enum_t r = xed_ild_decode( &xde, pBuffer, XED_MAX_INSTRUCTION_BYTES );
	if( r == XED_ERROR_NONE )
	{
		instLen = xed_decoded_inst_get_length( &xde );
	}
	else
	{
		instLen = 0;
		const char* pError = xed_error_enum_t2str( r );
		mbstowcs( pAssemblyError, pError, MAX_PATH );
	}

	return instLen;
}


void assemblerDestroy( void )
{
}
