#pragma once


typedef void ( *MessageCB )( const wchar_t* pMsg );
typedef void ( *ErrorCB )( const wchar_t* pMsg );


void registerMessageCB( MessageCB cb );
void registerErrorCB( ErrorCB cb );

void logMessage( const wchar_t* format, ... );
void logError( const wchar_t* format, ... );
