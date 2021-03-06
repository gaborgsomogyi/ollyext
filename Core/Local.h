#pragma once


#include <vector>


HMODULE localLoadLibrary( const wchar_t* libName );
HMODULE localGetModuleHandle( const wchar_t* libName );
FARPROC localGetProcAddress( const wchar_t* libName, const wchar_t* procName );
void localGetProcIdList( const wchar_t* procName, std::vector<DWORD>& rList );
