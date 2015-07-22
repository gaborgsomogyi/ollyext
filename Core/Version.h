#pragma once


#define DBGEXT_VERSION L"1.9"


extern int g_ollydbgVersion;
extern wchar_t* g_versionStr;
extern wchar_t* g_copyrightStr;


void versionInit( void );
void versionDestroy( void );
