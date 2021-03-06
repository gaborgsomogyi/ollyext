#include "stdafx.h"
#include "Core/DBG.h"
#include "Core/OS.h"
#include "Core/Remote.h"
#include "HeapFlags.h"


static bool g_applied = false;
static BYTE g_patch[] = { 0x02, 0x00, 0x00, 0x00 };
static BYTE g_orig[sizeof( g_patch )] = { 0 };


void heapFlagsReset( void )
{
	g_applied = false;
}


void heapFlagsApply( bool protect )
{
	if( protect )
	{
		if( g_applied ) return;

		DWORD peb = remoteGetPEB( dbgGetProcessHandle() );
		if( !peb ) return;

		// Shift to ProcessHeap offset
		DWORD processHeapOffset = peb + osGetProcessHeapsOffset();
		DWORD processHeap = 0;
		BOOL readSuccess = ReadProcessMemory( dbgGetProcessHandle(), (void*)processHeapOffset, &processHeap, sizeof( processHeap ), NULL );
		PLUGIN_CHECK_ERROR( readSuccess, return;, UNABLE_TO_READ_MEMORY_AT, processHeapOffset );

		DWORD flagsOffset = osGetFlagsOffset();
		if( !flagsOffset ) return;

		if( !remotePatchApply( dbgGetProcessHandle(), (void*)( processHeap + flagsOffset ), g_orig, g_patch, sizeof( g_patch ) ) )
			return;

		g_applied = true;
	}
	else
	{
		if( !g_applied ) return;

		DWORD peb = remoteGetPEB( dbgGetProcessHandle() );
		if( !peb ) return;

		DWORD processHeapOffset = peb + osGetProcessHeapsOffset();
		DWORD processHeap = 0;
		BOOL readSuccess = ReadProcessMemory( dbgGetProcessHandle(), (void*)processHeapOffset, &processHeap, sizeof( processHeap ), NULL );
		PLUGIN_CHECK_ERROR( readSuccess, return;, UNABLE_TO_READ_MEMORY_AT, processHeapOffset );

		DWORD flagsOffset = osGetFlagsOffset();
		if( !flagsOffset ) return;

		if( !remotePatchApply( dbgGetProcessHandle(), (void*)( processHeap + flagsOffset ), NULL, g_orig, sizeof( g_orig ) ) )
			return;

		g_applied = false;
	}
}
