#include "stdafx.h"
#include "Core/DBG.h"
#include "detours.h"
#include "Caption.h"


#define APPLY_SET_WINDOW_TEXT_HOOK() \
	LONG beginResult = DetourTransactionBegin(); \
	PLUGIN_CHECK_ERROR( beginResult == NO_ERROR, return;, UNABLE_TO_START_DETOURS_TRANSACTION, beginResult ); \
 \
	LONG updateThreadResult = DetourUpdateThread( GetCurrentThread() ); \
	PLUGIN_CHECK_ERROR( updateThreadResult == NO_ERROR, return;, UNABLE_TO_UPDATE_DETOURS_THREAD, updateThreadResult ); \
 \
	LONG attachResult = DetourAttach( &(PVOID&)SetWindowTextWTrampoline, SetWindowTextWHook ); \
	PLUGIN_CHECK_ERROR( attachResult == NO_ERROR, return;, UNABLE_TO_ATTACH_DETOURS, SetWindowTextWTrampoline, attachResult ); \
 \
	LONG commitResult = DetourTransactionCommit(); \
	PLUGIN_CHECK_ERROR( commitResult == NO_ERROR, return;, UNABLE_TO_COMMIT_DETOURS_TRANSACTION, commitResult );

#define RESTORE_SET_WINDOW_TEXT_HOOK() \
	LONG beginResult = DetourTransactionBegin(); \
	PLUGIN_CHECK_ERROR( beginResult == NO_ERROR, return;, UNABLE_TO_START_DETOURS_TRANSACTION, beginResult ); \
 \
	LONG updateThreadResult = DetourUpdateThread( GetCurrentThread() ); \
	PLUGIN_CHECK_ERROR( updateThreadResult == NO_ERROR, return;, UNABLE_TO_UPDATE_DETOURS_THREAD, updateThreadResult ); \
 \
	LONG detachResult = DetourDetach( &(PVOID&)SetWindowTextWTrampoline, SetWindowTextWHook ); \
	PLUGIN_CHECK_ERROR( detachResult == NO_ERROR, return;, UNABLE_TO_DETACH_DETOURS, SetWindowTextWTrampoline, detachResult ); \
 \
	LONG commitResult = DetourTransactionCommit(); \
	PLUGIN_CHECK_ERROR( commitResult == NO_ERROR, return;, UNABLE_TO_COMMIT_DETOURS_TRANSACTION, commitResult );


typedef BOOL ( WINAPI* SETWINDOWTEXTW )
(
	HWND hWnd,
	LPCWSTR lpString
);


static bool g_applied = false;
static wchar_t g_captionOrig[MAX_PATH] = { 0 };
static SETWINDOWTEXTW SetWindowTextWTrampoline = SetWindowTextW;


BOOL WINAPI SetWindowTextWHook( HWND hWnd, LPCWSTR lpString )
{
	if( hWnd == dbgGetMainWindowHWND() )
		return TRUE;

	return SetWindowTextWTrampoline( hWnd, lpString );
}


void captionApply( bool protect, const std::wstring& rCaption )
{
	if( protect )
	{
		if( g_applied )
		{
			RESTORE_SET_WINDOW_TEXT_HOOK();
		}
		else
		{
			BOOL getSuccess = GetWindowText( dbgGetMainWindowHWND(), g_captionOrig, MAX_PATH );
			PLUGIN_CHECK_ERROR( getSuccess, return;, UNABLE_TO_GET_WINDOW_TEXT );
		}

		BOOL setSuccess = SetWindowText( dbgGetMainWindowHWND(), rCaption.c_str() );
		PLUGIN_CHECK_ERROR( setSuccess, return;, UNABLE_TO_SET_WINDOW_TEXT );

		APPLY_SET_WINDOW_TEXT_HOOK();

		g_applied = true;
	}
	else
	{
		if( !g_applied ) return;

		RESTORE_SET_WINDOW_TEXT_HOOK();

		BOOL setSuccess = SetWindowText( dbgGetMainWindowHWND(), g_captionOrig );
		PLUGIN_CHECK_ERROR( setSuccess, return;, UNABLE_TO_SET_WINDOW_TEXT );

		g_applied = false;
	}
}
