#include "stdafx.h"
#include "OllyExtCodeRip.h"
#include "OllyExtSignatureRip.h"
#include "OllyExtDisasmMenu.h"


t_menu g_disasmSubMenu[] =
{
	{ L"Code Rip to Clipboard", L"Rip selected code to clipboard", K_NONE, menuCodeRip, NULL, 0 },
	{ L"Code Rip to Clipboard Recursive", L"Rip selected code to clipboard recursive", K_NONE, menuCodeRipRecursive, NULL, 0 },
	{ L"Signature Rip to Clipboard", L"Rip selected signature to clipboard", K_NONE, menuCodeRipSignature, NULL, 0 },
	{ NULL, NULL, K_NONE, NULL, NULL, 0 }
};

t_menu g_disasmMenu[] =
{
	{ L"OllyExt", L"OllyExt", K_NONE, NULL, g_disasmSubMenu, 0 },
	{ NULL, NULL, K_NONE, NULL, NULL, 0 }
};


int menuCodeRip( t_table* pt, wchar_t* name, ulong index, int mode )
{
	if( mode == MENU_VERIFY )
	{
		return MENU_NORMAL;
	}
	else if( mode == MENU_EXECUTE )
	{
		codeRipExecute( false );
		return MENU_NOREDRAW;
	}

	return MENU_ABSENT;
}


int menuCodeRipRecursive( t_table* pt, wchar_t* name, ulong index, int mode )
{
	if( mode == MENU_VERIFY )
	{
		return MENU_NORMAL;
	}
	else if( mode == MENU_EXECUTE )
	{
		codeRipExecute( true );
		return MENU_NOREDRAW;
	}

	return MENU_ABSENT;
}


int menuCodeRipSignature( t_table* pt, wchar_t* name, ulong index, int mode )
{
	if( mode == MENU_VERIFY )
	{
		return MENU_NORMAL;
	}
	else if( mode == MENU_EXECUTE )
	{
		signatureRipExecute();
		return MENU_NOREDRAW;
	}

	return MENU_ABSENT;
}
