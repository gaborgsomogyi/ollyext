#include "stdafx.h"
#include "Core/DBG.h"
#include "Icon.h"


HICON g_hOldIconSmall = NULL;
HICON g_hOldIconBig = NULL;

HICON g_hIconSmall = NULL;
HICON g_hIconBig = NULL;


void iconInit( HINSTANCE instance, DWORD iconSmallID, DWORD iconNormalID )
{
	g_hOldIconSmall = (HICON)SendMessage( dbgGetMainWindowHWND(), WM_GETICON, ICON_SMALL, 0 );
	g_hOldIconBig = (HICON)SendMessage( dbgGetMainWindowHWND(), WM_GETICON, ICON_BIG, 0 );

	g_hIconSmall = LoadIcon( instance, MAKEINTRESOURCE( iconSmallID ) );
	g_hIconBig = LoadIcon( instance, MAKEINTRESOURCE( iconNormalID ) );
}


void iconDestroy( void )
{
	DestroyIcon( g_hIconSmall );
	g_hIconSmall = NULL;

	DestroyIcon( g_hIconBig );
	g_hIconBig = NULL;
}


bool iconApply( bool enabled )
{
	if( enabled )
	{
		SendMessage( dbgGetMainWindowHWND(), WM_SETICON, ICON_SMALL, (LPARAM)g_hIconSmall );
		SendMessage( dbgGetMainWindowHWND(), WM_SETICON, ICON_BIG, (LPARAM)g_hIconBig );
	}
	else
	{
		SendMessage( dbgGetMainWindowHWND(), WM_SETICON, ICON_SMALL, (LPARAM)g_hOldIconSmall );
		SendMessage( dbgGetMainWindowHWND(), WM_SETICON, ICON_BIG, (LPARAM)g_hOldIconBig );
	}

	return true;
}
