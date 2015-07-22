#include "stdafx.h"
extern "C"
{
#include "xed/xed-interface.h"
};
#include "Core/OS.h"
#include "Core/Log.h"
#include "Core/Version.h"
#include "Core/Icon.h"
#include "Core/Assembler.h"
#include "Core/Integrity.h"
#include "Protections/Protect.h"
#include "Bugfixes/Bugfix.h"
#include "General/General.h"
#include "OllyExtDBG.h"
#include "OllyExtLog.h"
#include "OllyExtAbout.h"
#include "OllyExtMenu.h"
#include "OllyExtDisasmMenu.h"
#include "OllyExtDumpMenu.h"
#include "OllyExt.h"


HINSTANCE g_instance = NULL;


BOOL WINAPI DllMain( HINSTANCE new_instance, DWORD fdwReason, LPVOID lpvReserved )
{
	g_instance = new_instance;
	return TRUE;
}


extc int __cdecl ODBG2_Pluginquery( int ollydbgversion, ulong* features, wchar_t pluginname[SHORTNAME], wchar_t pluginversion[SHORTNAME] )
{
	if( ollydbgversion < 201 )
	{
		PLUGIN_CHECK_ERROR( false, return 0;, INVALID_OLLY_VERSION, 201, ollydbgversion );
	}
	g_ollydbgVersion = ollydbgversion;

	dbgInit();
	logInit();
	osInit();
	versionInit();

	wcscpy_s( pluginname, SHORTNAME, dbgGetPluginName() );
	wcscpy_s( pluginversion, SHORTNAME, DBGEXT_VERSION );

	logMessage( g_versionStr );

    wchar_t v[MAX_PATH];

	mbstowcs( v, xed_get_version(), MAX_PATH );
	logMessage( L"%ws: XED version: %ws", dbgGetPluginName(), v );

	mbstowcs( v, BeaEngineVersion(), MAX_PATH );
	logMessage( L"%ws: BeaEngine version: %ws", dbgGetPluginName(), v );

	wchar_t r[MAX_PATH];
    mbstowcs( r, BeaEngineRevision(), MAX_PATH );
	logMessage( L"%ws: BeaEngine revision: %ws", dbgGetPluginName(), r );

	return PLUGIN_VERSION;
}


extc int __cdecl ODBG2_Plugininit( void )
{
	aboutInit();

	iconInit( g_instance, IDI_ICON_SMALL, IDI_ICON );

	assemblerInit();

	integrityInit();
	integritySnapshot( L"ntdll.dll" );

	protectInit( g_instance );
	protectReadOptions();

	bugfixInit();
	bugfixReadOptions();
	bugfixApply();

	generalInit();
	generalReadOptions();
	generalApply();

	return 0;
}


extc void ODBG2_Pluginnotify( int code, void* data, ulong parm1, ulong parm2 )
{
	switch( code )
	{
		case PN_STATUS:
			switch( parm1 )
			{
				case STAT_LOADING:
					protectPreHook();
				break;

				case STAT_IDLE:
					protectPostHook();
				break;
			}
		break;

		case PN_NEWMOD:
			protectApply();
		break;
	}
}


extc void __cdecl ODBG2_Pluginreset( void )
{
	protectReset();
}


extc t_menu* __cdecl ODBG2_Pluginmenu( wchar_t* type )
{
	if( !wcscmp( type, PWM_MAIN ) )
		return g_mainMenu;
	else if( !wcscmp( type, PWM_DISASM ) )
		return g_disasmMenu;
	else if( !wcscmp( type, PWM_DUMP ) )
		return g_dumpMenu;
	return NULL;
};


extc void __cdecl ODBG2_Plugindestroy( void )
{
	integrityDestroy();
	assemblerDestroy();
	iconDestroy();
	aboutDestroy();
	versionDestroy();
}
