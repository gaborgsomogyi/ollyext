#pragma once


extern HICON g_hIconSmall;
extern HICON g_hIconBig;


void iconInit( HINSTANCE instance, DWORD iconSmallID, DWORD iconNormalID );
void iconDestroy( void );
bool iconApply( bool enabled );
