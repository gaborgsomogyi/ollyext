#include "stdafx.h"
#include "Core/Config.h"
#include "Bugfix.h"

#include "Caption.h"
#include "KillAntiAttach.h"


sBugfixOptions g_bugfixOptions = { 0 };


#define DEFAULT_CAPTION L"Ferrit"

#define CAPTION L"Caption"
#define CAPTIONVALUE L"CaptionValue"
#define KILLANTIATTACH L"KillAntiAttach"


void bugfixInit( void )
{
}


void bugfixReadOptions( void )
{
	g_bugfixOptions.caption = configReadBool( CAPTION );
	g_bugfixOptions.captionValue = configReadString( CAPTIONVALUE, DEFAULT_CAPTION );
	g_bugfixOptions.killAntiAttach = configReadBool( KILLANTIATTACH );
}


void bugfixWriteOptions( void )
{
	configSaveBool( CAPTION, g_bugfixOptions.caption );
	configSaveString( CAPTIONVALUE, g_bugfixOptions.captionValue.c_str() );
	configSaveBool( KILLANTIATTACH, g_bugfixOptions.killAntiAttach );
}


bool bugfixApply( void )
{
	captionApply( g_bugfixOptions.caption, g_bugfixOptions.captionValue.empty() ? DEFAULT_CAPTION : g_bugfixOptions.captionValue );
	killAntiAttachApply( g_bugfixOptions.killAntiAttach );

	return true;
}
