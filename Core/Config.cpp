#include "stdafx.h"
#include "DBG.h"
#include "Ini.h"
#include "Config.h"


#define INI_NAME L".\\OllyExt.ini"


void configSaveString( const wchar_t* pName, const wchar_t* pValue )
{
	iniSaveString( INI_NAME, pName, pValue );
}


std::wstring configReadString( const wchar_t* pName, const wchar_t* pDefaultValue )
{
	return iniReadString( INI_NAME, pName, pDefaultValue );
}


void configSaveBool( const wchar_t* pName, bool value )
{
	configSaveInt( pName, value );
}


bool configReadBool( const wchar_t* pName, bool defaultValue )
{
	return ( configReadInt( pName, defaultValue ) != 0 );
}


void configSaveInt( const wchar_t* pName, int value )
{
	iniSaveInt( INI_NAME, pName, value );
}


int configReadInt( const wchar_t* pName, int defaultValue )
{
	return iniReadInt( INI_NAME, pName, defaultValue );
}
