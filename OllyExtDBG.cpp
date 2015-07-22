#include "stdafx.h"
#include "Core/DBG.h"
#include "OllyExtDBG.h"


static const wchar_t* dbgGetPluginNameCB( void )
{
	return L"OllyExt";
}


static HWND dbgGetMainWindowHWNDCB( void )
{
	return hwollymain;
}


static const wchar_t* dbgGetProcessNameCB( void )
{
	return executable;
}


static HANDLE dbgGetProcessHandleCB( void )
{
	return process;
}


static DWORD dbgGetProcessIDCB( void )
{
	return processid;
}


void dbgInit( void )
{
	registerGetPluginNameCB( dbgGetPluginNameCB );
	registerGetMainWindowHWNDCB( dbgGetMainWindowHWNDCB );
	registerGetProcessNameCB( dbgGetProcessNameCB );
	registerGetProcessHandleCB( dbgGetProcessHandleCB );
	registerGetProcessIDCB( dbgGetProcessIDCB );
}
