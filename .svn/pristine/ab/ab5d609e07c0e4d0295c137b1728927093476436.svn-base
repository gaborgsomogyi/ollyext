#pragma once


typedef const wchar_t* ( *GetPluginNameCB )( void );
typedef HWND ( *GetMainWindowHWNDCB )( void );
typedef const wchar_t* ( *GetProcessNameCB )( void );
typedef HANDLE ( *GetProcessHandleCB )( void );
typedef DWORD ( *GetProcessIDCB )( void );


void registerGetPluginNameCB( GetPluginNameCB cb );
void registerGetMainWindowHWNDCB( GetMainWindowHWNDCB cb );
void registerGetProcessNameCB( GetProcessNameCB cb );
void registerGetProcessHandleCB( GetProcessHandleCB cb );
void registerGetProcessIDCB( GetProcessIDCB cb );


const wchar_t* dbgGetPluginName( void );
HWND dbgGetMainWindowHWND( void );
const wchar_t* dbgGetProcessName( void );
HANDLE dbgGetProcessHandle( void );
DWORD dbgGetProcessID( void );
