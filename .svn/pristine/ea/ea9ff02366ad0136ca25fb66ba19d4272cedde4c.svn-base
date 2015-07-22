#include "stdafx.h"
#include "Core/Version.h"
#include "Core/Log.h"
#include "General/General.h"
#include "OllyExtRip.h"
#include "OllyExtDataRip.h"


//#define RIP_DEBUG


static bool signatureRipGetDataFromRange( std::string& rSignature )
{
	rSignature = "";

	PLUGIN_CHECK_ERROR( g_ollydbgVersion == 201, return false;, UNABLE_TO_RIP_DATA );

	t_dump* pDisasmDump = Getcpudisasmdump();
	PLUGIN_CHECK_ERROR( pDisasmDump, return false;, UNABLE_TO_GET_DISASSEMBLER_DUMP );

	DWORD bufferSize = pDisasmDump->sel1 - pDisasmDump->sel0;

#ifdef RIP_DEBUG
	logMessage( L"signatureRipGetDataFromRange start: %08X size: %08X", pDisasmDump->sel0, bufferSize );
#endif

	PLUGIN_CHECK_ERROR_NO_MB( pDisasmDump->sel0 && bufferSize, return false; );

	// Allocate the necessary memory
	BYTE* pBuffer = new BYTE[bufferSize];
	PLUGIN_CHECK_ERROR( pBuffer, return false;, UNABLE_TO_ALLOCATE_LOCAL_MEMORY );

	// Read the assembly from the target binary
	PLUGIN_CHECK_ERROR( Readmemory( pBuffer, pDisasmDump->sel0, bufferSize, MM_SILENT ),
		delete [] pBuffer;
		return false;,
		UNABLE_TO_READ_REMOTE_PROCESS_MEMORY, pDisasmDump->sel0, bufferSize );

	DWORD ptr = 0;
	char numStr[4] = { 0 };
	while( ptr < bufferSize )
	{
#ifdef RIP_DEBUG
		logMessage( L"signatureRipGetDataFromRange byte: %02X", pBuffer[ptr] );
#endif
		sprintf_s( numStr, sizeof( numStr ), "%02X", pBuffer[ptr] );
		rSignature += numStr;
		++ptr;
	}

	// Remove buffer
	delete [] pBuffer;

	return true;
}


void signatureRipExecute( void )
{
	std::string signatureStr = "";

	PLUGIN_CHECK_ERROR_NO_MB( signatureRipGetDataFromRange( signatureStr ), return; );
	PLUGIN_CHECK_ERROR_NO_MB( ripSendToClipboard( signatureStr ), return; );

	logMessage( L"%ws: Signature ripped successfully", dbgGetPluginName() );
}
