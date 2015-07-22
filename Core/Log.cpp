#include "stdafx.h"
#include "Log.h"


static wchar_t g_msg[1024 * 16];
static MessageCB g_messageCB = NULL;
static ErrorCB g_errorCB = NULL;


void registerMessageCB( MessageCB cb )
{
	g_messageCB = cb;
}


void registerErrorCB( ErrorCB cb )
{
	g_errorCB = cb;
}


void logMessage( const wchar_t* format, ... )
{
	va_list argptr;

	va_start( argptr, format );
	vswprintf_s( g_msg, sizeof( g_msg ), format, argptr );
	va_end( argptr );

	if( g_messageCB )
		g_messageCB( g_msg );
}


void logError( const wchar_t* format, ... )
{
	va_list argptr;

	va_start( argptr, format );
	vswprintf_s( g_msg, sizeof( g_msg ), format, argptr );
	va_end( argptr );

	if( g_errorCB )
		g_errorCB( g_msg );
}
