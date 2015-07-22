#pragma once


#include <string>


typedef struct
{
	bool caption;
	std::wstring captionValue;
	bool killAntiAttach;
} sBugfixOptions;


extern sBugfixOptions g_bugfixOptions;


void bugfixInit( void );
void bugfixReadOptions( void );
void bugfixWriteOptions( void );
bool bugfixApply( void );
