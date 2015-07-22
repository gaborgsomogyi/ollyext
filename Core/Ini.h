#pragma once


#include <string>


void iniSaveString( const wchar_t* pFileName, const wchar_t* pName, const wchar_t* pValue );
std::wstring iniReadString( const wchar_t* pFileName, const wchar_t* pName, const wchar_t* pDefaultValue = L"" );

void iniSaveBool( const wchar_t* pFileName, const wchar_t* pName, bool value );
bool iniReadBool( const wchar_t* pFileName, const wchar_t* pName, bool defaultValue = false );

void iniSaveInt( const wchar_t* pFileName, const wchar_t* pName, int value );
int iniReadInt( const wchar_t* pFileName, const wchar_t* pName, int defaultValue = 0 );
