#pragma once


void osInit( void );
void osDestroy( void );
bool osDirExists( const wchar_t* dirName );

// System specific values
DWORD osGetBeingDebuggedOffset( void );
DWORD osGetProcessHeapsOffset( void );
DWORD osGetNtGlobalFlagOffset( void );
DWORD osGetFlagsOffset( void );
DWORD osGetForceFlagsOffset( void );
