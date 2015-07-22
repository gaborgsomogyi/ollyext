#include "stdafx.h"
#include "plugin.h"
#include "Core/Log.h"
#include "OllyExtLog.h"


void logMessageCB( const wchar_t* pMsg )
{
	Addtolist( 0, BLACK, (wchar_t*)pMsg );
}


void logErrorCB( const wchar_t* pMsg )
{
	Addtolist( 0, RED, (wchar_t*)pMsg );
}


void logInit( void )
{
	registerMessageCB( logMessageCB );
	registerErrorCB( logErrorCB );
}
