#pragma once


#include <string>


void configSaveString( const wchar_t* pName, const wchar_t* pValue );
std::wstring configReadString( const wchar_t* pName, const wchar_t* pDefaultValue = L"" );

void configSaveBool( const wchar_t* pName, bool value );
bool configReadBool( const wchar_t* pName, bool defaultValue = false );

void configSaveInt( const wchar_t* pName, int value );
int configReadInt( const wchar_t* pName, int defaultValue = 0 );
