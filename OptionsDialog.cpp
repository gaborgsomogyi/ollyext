#include "stdafx.h"
#include <commctrl.h>
#include "Core/Version.h"
#include "Core/Icon.h"
#include "Protections/Protect.h"
#include "Bugfixes/Bugfix.h"
#include "General/General.h"
#include "OllyExt.h"
#include "OllyExtAbout.h"
#include "OptionsDialog.h"


#define PROTECT_ENABLE_DLG_FROM_OPTION( tab, control, option ) \
{ \
	HWND hwnd = GetDlgItem( g_tabs[tab].hwnd, control ); \
	EnableWindow( hwnd, option ); \
}

#define PROTECT_GET_VALUE_FROM_EDITBOX( tab, control, option ) \
{ \
	HWND hwnd = GetDlgItem( g_tabs[tab].hwnd, control ); \
	wchar_t s[MAX_PATH] = { 0 }; \
	SendMessage( hwnd, WM_GETTEXT, sizeof( s ), (LPARAM)s ); \
	option = s; \
}

#define PROTECT_SET_EDITBOX_FROM_VALUE( tab, control, option ) \
{ \
	HWND hwnd = GetDlgItem( g_tabs[tab].hwnd, control ); \
	SendMessage( hwnd, WM_SETTEXT, 0, (LPARAM)option ); \
}

#define PROTECT_GET_VALUE_FROM_CHECKBOX( tab, control, option ) \
{ \
	HWND hwnd = GetDlgItem( g_tabs[tab].hwnd, control ); \
	option = SendMessage( hwnd, BM_GETSTATE, 0, 0 ) == BST_CHECKED; \
}

#define PROTECT_SET_CHECKBOX_FROM_VALUE( tab, control, option ) \
{ \
	HWND hwnd = GetDlgItem( g_tabs[tab].hwnd, control ); \
	SendMessage( hwnd, BM_SETCHECK, option ? BST_CHECKED : BST_UNCHECKED, 0 ); \
}

#define GENERAL_ADD_COMBO_ITEM( tab, control, item ) \
{ \
	HWND hwnd = GetDlgItem( g_tabs[tab].hwnd, control ); \
	SendMessage( hwnd, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)item ); \
}

#define GENERAL_COMBO_SETSEL( tab, control, selection ) \
{ \
	HWND hwnd = GetDlgItem( g_tabs[tab].hwnd, control ); \
	SendMessage( hwnd, CB_SETCURSEL, (WPARAM)selection, (LPARAM)0); \
}

#define GENERAL_COMBO_GETSEL( tab, control, result ) \
{ \
	HWND hwnd = GetDlgItem( g_tabs[tab].hwnd, control ); \
	result = SendMessage( hwnd, CB_GETCURSEL, (WPARAM)0, (LPARAM)0); \
}


typedef enum
{
	TAB_ANTI_DEBUG1,
	TAB_ANTI_DEBUG2,
	TAB_ANTI_DEBUG3,
	TAB_BUGFIXES,
	TAB_GENERAL,
	TAB_NUM
} eTabType;

typedef struct
{
	wchar_t* name;
	DWORD id;
	HWND hwnd;
} sTab;

static sTab g_tabs[TAB_NUM] =
{
	{ L"Anti-Debug 1", IDD_TAB1, NULL },
	{ L"Anti-Debug 2", IDD_TAB2, NULL },
	{ L"Anti-Debug 3", IDD_TAB3, NULL },
	{ L"Bugfixes", IDD_TAB4, NULL },
	{ L"General", IDD_TAB5, NULL }
};

static int g_selectedTab = 0;


static void createTabs( HWND hDlg )
{
	HWND hTab = GetDlgItem( hDlg, IDC_OPTIONS_TAB );
	if( !hTab ) return;

	RECT tabRect = { 0 };
	GetWindowRect( hTab, &tabRect );

	LONG itemYMax = 0;
	LONG w = tabRect.right - tabRect.left;
	LONG h = tabRect.bottom- tabRect.top;

	TCITEM tcItem;
	for( DWORD i = 0; i < sizeof( g_tabs ) / sizeof( g_tabs[0] ); ++i )
	{
		memset( &tcItem, 0, sizeof( TCITEM ) );
		tcItem.mask = TCIF_TEXT;
		tcItem.pszText = g_tabs[i].name;

		if( TabCtrl_InsertItem( hTab, i, &tcItem ) == -1 ) return;

		RECT itemRect = { 0 };
		TabCtrl_GetItemRect( hTab, i, &itemRect );
		if( itemYMax < itemRect.bottom )
			itemYMax = itemRect.bottom;
	}

	for( DWORD i = 0; i < sizeof( g_tabs ) / sizeof( g_tabs[0] ); ++i )
	{
		g_tabs[i].hwnd = CreateDialog( g_instance, MAKEINTRESOURCE( g_tabs[i].id ), hTab, NULL );
		SetWindowPos( g_tabs[i].hwnd,
						NULL,
						0,
						itemYMax,
						w,
						h,
						0 );
		ShowWindow( g_tabs[i].hwnd, SW_HIDE );
	}

	g_selectedTab = 0;
	ShowWindow( g_tabs[g_selectedTab].hwnd, SW_SHOW );
}


static void setOptions( HWND hDlg )
{
	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_ANTI_DEBUG1, IDC_ISDEBUGGERPRESENT, g_protectOptions.isDebuggerPresent );
	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_ANTI_DEBUG1, IDC_NTGLOBALFLAG, g_protectOptions.ntGlobalFlag );
	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_ANTI_DEBUG1, IDC_HEAPFLAGS, g_protectOptions.heapFlags );
	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_ANTI_DEBUG1, IDC_FORCEFLAGS, g_protectOptions.forceFlags );
	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_ANTI_DEBUG1, IDC_CHECKREMOTEDEBUGGERPRESENT, g_protectOptions.checkRemoteDebuggerPresent );
	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_ANTI_DEBUG1, IDC_OUTPUTDEBUGSTRING, g_protectOptions.outputDebugString );
	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_ANTI_DEBUG1, IDC_NTCLOSE, g_protectOptions.ntClose );
	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_ANTI_DEBUG1, IDC_SEDEBUGPRIVILEGE, g_protectOptions.seDebugPrivilege );
	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_ANTI_DEBUG1, IDC_BLOCKINPUT, g_protectOptions.blockInput );
	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_ANTI_DEBUG1, IDC_PROCESSDEBUGFLAGS, g_protectOptions.processDebugFlags );
	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_ANTI_DEBUG1, IDC_PROCESSDEBUGOBJECTHANDLE, g_protectOptions.processDebugObjectHandle );
	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_ANTI_DEBUG1, IDC_TERMINATEPROCESS, g_protectOptions.terminateProcess );
	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_ANTI_DEBUG1, IDC_NTSETINFORMATIONTHREAD, g_protectOptions.ntSetInformationThread );
	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_ANTI_DEBUG1, IDC_NTQUERYOBJECT, g_protectOptions.ntQueryObject );
	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_ANTI_DEBUG1, IDC_FINDWINDOW, g_protectOptions.findWindow );
	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_ANTI_DEBUG1, IDC_NTOPENPROCESS, g_protectOptions.ntOpenProcess );

	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_ANTI_DEBUG2, IDC_PROCESS32FIRST, g_protectOptions.process32First );
	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_ANTI_DEBUG2, IDC_PROCESS32NEXT, g_protectOptions.process32Next );
	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_ANTI_DEBUG2, IDC_PARENTPROCESS, g_protectOptions.parentProcess );
	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_ANTI_DEBUG2, IDC_GETTICKCOUNT, g_protectOptions.getTickCount );
	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_ANTI_DEBUG2, IDC_TIMEGETTIME, g_protectOptions.timeGetTime );
	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_ANTI_DEBUG2, IDC_QUERYPERFORMANCECOUNTER, g_protectOptions.queryPerformanceCounter );
	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_ANTI_DEBUG2, IDC_ZWGETCONTEXTTHREAD, g_protectOptions.zwGetContextThread );
	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_ANTI_DEBUG2, IDC_NTSETCONTEXTTHREAD, g_protectOptions.ntSetContextThread );
	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_ANTI_DEBUG2, IDC_KDDEBUGGERNOTPRESENT, g_protectOptions.kdDebuggerNotPresent );
	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_ANTI_DEBUG2, IDC_KDDEBUGGERENABLED, g_protectOptions.kdDebuggerEnabled );
	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_ANTI_DEBUG2, IDC_NTSETDEBUGFILTERSTATE, g_protectOptions.ntSetDebugFilterState );
	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_ANTI_DEBUG2, IDC_PROTECTDRX, g_protectOptions.protectDRX );
	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_ANTI_DEBUG2, IDC_HIDEDRX, g_protectOptions.hideDRX );
	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_ANTI_DEBUG2, IDC_DBGPROMPT, g_protectOptions.dbgPrompt );
	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_ANTI_DEBUG2, IDC_CREATETHREAD, g_protectOptions.createThread );
	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_ANTI_DEBUG2, IDC_NTSYSTEMDEBUGCONTROL, g_protectOptions.ntSystemDebugControl );

	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_ANTI_DEBUG3, IDC_CUSTOM, g_protectOptions.custom );

	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_BUGFIXES, IDC_CAPTION, g_bugfixOptions.caption );
	PROTECT_SET_EDITBOX_FROM_VALUE( TAB_BUGFIXES, IDC_CAPTION_EDIT, g_bugfixOptions.captionValue.c_str() );
	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_BUGFIXES, IDC_KILLANTIATTACH, g_bugfixOptions.killAntiAttach );

	GENERAL_ADD_COMBO_ITEM( TAB_GENERAL, IDC_CODE_RIPPER_SYNTAX_COMBO, L"Masm" );
	GENERAL_ADD_COMBO_ITEM( TAB_GENERAL, IDC_CODE_RIPPER_SYNTAX_COMBO, L"Goasm" );
	GENERAL_ADD_COMBO_ITEM( TAB_GENERAL, IDC_CODE_RIPPER_SYNTAX_COMBO, L"Nasm" );
	GENERAL_ADD_COMBO_ITEM( TAB_GENERAL, IDC_CODE_RIPPER_SYNTAX_COMBO, L"AT" );
	GENERAL_COMBO_SETSEL( TAB_GENERAL, IDC_CODE_RIPPER_SYNTAX_COMBO, g_generalOptions.codeRipperSyntax );

	GENERAL_ADD_COMBO_ITEM( TAB_GENERAL, IDC_DATA_RIPPER_SYNTAX_COMBO, L"C" );
	GENERAL_ADD_COMBO_ITEM( TAB_GENERAL, IDC_DATA_RIPPER_SYNTAX_COMBO, L"Masm" );
	GENERAL_COMBO_SETSEL( TAB_GENERAL, IDC_DATA_RIPPER_SYNTAX_COMBO, g_generalOptions.dataRipperSyntax );

	PROTECT_SET_CHECKBOX_FROM_VALUE( TAB_GENERAL, IDC_APPLYICON, g_generalOptions.applyIcon );
}


BOOL CALLBACK OptionsDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
	case WM_INITDIALOG:
	{
		SetWindowText( hDlg, g_versionStr );

		SendMessage( hDlg, WM_SETICON, ICON_SMALL, (LPARAM)g_hIconSmall );
		SendMessage( hDlg, WM_SETICON, ICON_BIG, (LPARAM)g_hIconBig );

		createTabs( hDlg );
		setOptions( hDlg );

		return TRUE;
	}
	break;

	case WM_DESTROY:
	case WM_CLOSE:
		EndDialog( hDlg, 0 );
		return TRUE;
	break;

	case WM_COMMAND:
        switch( HIWORD( wParam ) )
        {
        case BN_CLICKED:       
			switch( LOWORD( wParam ) )
			{
			case IDC_OK:
			{
				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_ANTI_DEBUG1, IDC_ISDEBUGGERPRESENT, g_protectOptions.isDebuggerPresent );
				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_ANTI_DEBUG1, IDC_NTGLOBALFLAG, g_protectOptions.ntGlobalFlag );
				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_ANTI_DEBUG1, IDC_HEAPFLAGS, g_protectOptions.heapFlags );
				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_ANTI_DEBUG1, IDC_FORCEFLAGS, g_protectOptions.forceFlags );
				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_ANTI_DEBUG1, IDC_CHECKREMOTEDEBUGGERPRESENT, g_protectOptions.checkRemoteDebuggerPresent );
				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_ANTI_DEBUG1, IDC_OUTPUTDEBUGSTRING, g_protectOptions.outputDebugString );
				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_ANTI_DEBUG1, IDC_NTCLOSE, g_protectOptions.ntClose );
				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_ANTI_DEBUG1, IDC_SEDEBUGPRIVILEGE, g_protectOptions.seDebugPrivilege );
				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_ANTI_DEBUG1, IDC_BLOCKINPUT, g_protectOptions.blockInput );
				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_ANTI_DEBUG1, IDC_PROCESSDEBUGFLAGS, g_protectOptions.processDebugFlags );
				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_ANTI_DEBUG1, IDC_PROCESSDEBUGOBJECTHANDLE, g_protectOptions.processDebugObjectHandle );
				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_ANTI_DEBUG1, IDC_TERMINATEPROCESS, g_protectOptions.terminateProcess );
				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_ANTI_DEBUG1, IDC_NTSETINFORMATIONTHREAD, g_protectOptions.ntSetInformationThread );
				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_ANTI_DEBUG1, IDC_NTQUERYOBJECT, g_protectOptions.ntQueryObject );
				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_ANTI_DEBUG1, IDC_FINDWINDOW, g_protectOptions.findWindow );
				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_ANTI_DEBUG1, IDC_NTOPENPROCESS, g_protectOptions.ntOpenProcess );

				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_ANTI_DEBUG2, IDC_PROCESS32FIRST, g_protectOptions.process32First );
				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_ANTI_DEBUG2, IDC_PROCESS32NEXT, g_protectOptions.process32Next );
				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_ANTI_DEBUG2, IDC_PARENTPROCESS, g_protectOptions.parentProcess );
				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_ANTI_DEBUG2, IDC_GETTICKCOUNT, g_protectOptions.getTickCount );
				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_ANTI_DEBUG2, IDC_TIMEGETTIME, g_protectOptions.timeGetTime );
				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_ANTI_DEBUG2, IDC_QUERYPERFORMANCECOUNTER, g_protectOptions.queryPerformanceCounter );
				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_ANTI_DEBUG2, IDC_ZWGETCONTEXTTHREAD, g_protectOptions.zwGetContextThread );
				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_ANTI_DEBUG2, IDC_NTSETCONTEXTTHREAD, g_protectOptions.ntSetContextThread );
				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_ANTI_DEBUG2, IDC_KDDEBUGGERNOTPRESENT, g_protectOptions.kdDebuggerNotPresent );
				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_ANTI_DEBUG2, IDC_KDDEBUGGERENABLED, g_protectOptions.kdDebuggerEnabled );
				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_ANTI_DEBUG2, IDC_NTSETDEBUGFILTERSTATE, g_protectOptions.ntSetDebugFilterState );
				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_ANTI_DEBUG2, IDC_PROTECTDRX, g_protectOptions.protectDRX );
				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_ANTI_DEBUG2, IDC_HIDEDRX, g_protectOptions.hideDRX );
				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_ANTI_DEBUG2, IDC_DBGPROMPT, g_protectOptions.dbgPrompt );
				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_ANTI_DEBUG2, IDC_CREATETHREAD, g_protectOptions.createThread );
				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_ANTI_DEBUG2, IDC_NTSYSTEMDEBUGCONTROL, g_protectOptions.ntSystemDebugControl );

				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_ANTI_DEBUG3, IDC_CUSTOM, g_protectOptions.custom );

				protectWriteOptions();
				if( dbgGetProcessHandle() ) protectApply();

				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_BUGFIXES, IDC_CAPTION, g_bugfixOptions.caption );
				PROTECT_GET_VALUE_FROM_EDITBOX( TAB_BUGFIXES, IDC_CAPTION_EDIT, g_bugfixOptions.captionValue );
				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_BUGFIXES, IDC_KILLANTIATTACH, g_bugfixOptions.killAntiAttach );

				bugfixWriteOptions();
				bugfixApply();

				int index = 0;

				GENERAL_COMBO_GETSEL( TAB_GENERAL, IDC_CODE_RIPPER_SYNTAX_COMBO, index );
				g_generalOptions.codeRipperSyntax = (eCodeRipperSyntax)index;
				if( g_generalOptions.codeRipperSyntax < CODE_MASMSYNTAX )
					g_generalOptions.codeRipperSyntax = CODE_MASMSYNTAX;
				if( g_generalOptions.codeRipperSyntax > CODE_ATSYNTAX )
					g_generalOptions.codeRipperSyntax = CODE_MASMSYNTAX;

				GENERAL_COMBO_GETSEL( TAB_GENERAL, IDC_DATA_RIPPER_SYNTAX_COMBO, index );
				g_generalOptions.dataRipperSyntax = (eDataRipperSyntax)index;
				if( g_generalOptions.dataRipperSyntax < DATA_CSYNTAX )
					g_generalOptions.dataRipperSyntax = DATA_CSYNTAX;
				if( g_generalOptions.dataRipperSyntax > DATA_MASMSYNTAX )
					g_generalOptions.dataRipperSyntax = DATA_CSYNTAX;

				PROTECT_GET_VALUE_FROM_CHECKBOX( TAB_GENERAL, IDC_APPLYICON, g_generalOptions.applyIcon );

				generalWriteOptions();
				generalApply();

				EndDialog( hDlg, 0 );
				return TRUE;
			}
			break;

			case IDC_CANCEL:
				EndDialog( hDlg, 0 );
				return TRUE;
			break;

			case IDC_ABOUT:
				aboutShow();
				return TRUE;
			break;
			}
        break;
        }
	break;

	case WM_NOTIFY:
	{
		LPNMHDR lpnmhdr = (LPNMHDR)lParam;
		switch( lpnmhdr->code )
        {
        case TCN_SELCHANGE:
			switch( lpnmhdr->idFrom )
			{
			case IDC_OPTIONS_TAB:
				int pageID = TabCtrl_GetCurSel( lpnmhdr->hwndFrom );
				if( pageID >= 0 )
				{
					ShowWindow( g_tabs[g_selectedTab].hwnd, SW_HIDE );
					g_selectedTab = pageID;
					ShowWindow( g_tabs[g_selectedTab].hwnd, SW_SHOW );
				}
				return TRUE;
			break;
			}
        break;
        }
	}
	break;
	}

	return FALSE;
}
