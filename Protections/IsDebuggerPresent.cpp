#include "stdafx.h"
#include "Core/DBG.h"
#include "Core/OS.h"
#include "Core/Remote.h"
#include "IsDebuggerPresent.h"


static bool g_applied = false;
static BYTE g_patch[] = { 0 };
static BYTE g_orig[sizeof( g_patch )] = { 0 };


void isDebuggerPresentReset( void )
{
	g_applied = false;
}


void isDebuggerPresentApply( bool protect )
{
	if( protect )
	{
		if( g_applied ) return;

		DWORD peb = remoteGetPEB( dbgGetProcessHandle() );
		if( !peb ) return;

		if( !remotePatchApply( dbgGetProcessHandle(), (void*)( peb + osGetBeingDebuggedOffset() ), g_orig, g_patch, sizeof( g_patch ) ) )
			return;

		g_applied = true;
	}
	else
	{
		if( !g_applied ) return;

		DWORD peb = remoteGetPEB( dbgGetProcessHandle() );
		if( !peb ) return;

		if( !remotePatchApply( dbgGetProcessHandle(), (void*)( peb + osGetBeingDebuggedOffset() ), NULL, g_orig, sizeof( g_orig ) ) )
			return;

		g_applied = false;
	}
}
