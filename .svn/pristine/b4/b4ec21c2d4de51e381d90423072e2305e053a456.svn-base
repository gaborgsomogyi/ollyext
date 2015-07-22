#include "stdafx.h"
#include "DBG.h"


GetPluginNameCB g_getPluginNameCB = NULL;
GetMainWindowHWNDCB g_getMainWindowHWNDCB = NULL;
GetProcessNameCB g_getProcessNameCB = NULL;
GetProcessHandleCB g_getProcessHandleCB = NULL;
GetProcessIDCB g_getProcessIDCB = NULL;


void registerGetPluginNameCB( GetPluginNameCB cb )
{
	g_getPluginNameCB = cb;
}


void registerGetMainWindowHWNDCB( GetMainWindowHWNDCB cb )
{
	g_getMainWindowHWNDCB = cb;
}


void registerGetProcessNameCB( GetProcessNameCB cb )
{
	g_getProcessNameCB = cb;
}


void registerGetProcessHandleCB( GetProcessHandleCB cb )
{
	g_getProcessHandleCB = cb;
}


void registerGetProcessIDCB( GetProcessIDCB cb )
{
	g_getProcessIDCB = cb;
}


const wchar_t* dbgGetPluginName( void )
{
	if( g_getPluginNameCB )
		return g_getPluginNameCB();
	return NULL;
}


HWND dbgGetMainWindowHWND( void )
{
	if( g_getMainWindowHWNDCB )
		return g_getMainWindowHWNDCB();
	return 0;
}


const wchar_t* dbgGetProcessName( void )
{
	if( g_getProcessNameCB )
		return g_getProcessNameCB();
	return NULL;
}


HANDLE dbgGetProcessHandle( void )
{
	if( g_getProcessHandleCB )
		return g_getProcessHandleCB();
	return 0;
}


DWORD dbgGetProcessID( void )
{
	if( g_getProcessIDCB )
		return g_getProcessIDCB();
	return 0;
}
