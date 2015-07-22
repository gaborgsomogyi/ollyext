#include "stdafx.h"
#include "DBG.h"
#include "Ini.h"


void iniSaveString( const wchar_t* pFileName, const wchar_t* pName, const wchar_t* pValue )
{
	WritePrivateProfileString( dbgGetPluginName(), pName, pValue, pFileName );
}


std::wstring iniReadString( const wchar_t* pFileName, const wchar_t* pName, const wchar_t* pDefaultValue )
{
	wchar_t s[MAX_PATH] = { 0 };
	GetPrivateProfileString( dbgGetPluginName(), pName, pDefaultValue, s, sizeof( s ), pFileName );
	return s;
}


void iniSaveBool( const wchar_t* pFileName, const wchar_t* pName, bool value )
{
	iniSaveInt( pFileName, pName, value );
}


bool iniReadBool( const wchar_t* pFileName, const wchar_t* pName, bool defaultValue )
{
	return ( iniReadInt( pFileName, pName, defaultValue ) != 0 );
}


void iniSaveInt( const wchar_t* pFileName, const wchar_t* pName, int value )
{
	wchar_t s[MAX_PATH] = { 0 };
	_itow( value, s, 10 );
	WritePrivateProfileString( dbgGetPluginName(), pName, s, pFileName );
}


int iniReadInt( const wchar_t* pFileName, const wchar_t* pName, int defaultValue )
{
	return GetPrivateProfileInt( dbgGetPluginName(), pName, defaultValue, pFileName );
}
