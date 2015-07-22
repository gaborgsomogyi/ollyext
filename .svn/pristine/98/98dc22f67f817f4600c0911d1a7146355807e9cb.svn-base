#include "stdafx.h"
#include "Core/Config.h"
#include "Core/Icon.h"
#include "General/General.h"


sGeneralOptions g_generalOptions = { CODE_MASMSYNTAX, DATA_CSYNTAX, false };


#define CODERIPPERSYNTAX L"CodeRipperSyntax"
#define DATARIPPERSYNTAX L"DataRipperSyntax"
#define APPLYICON L"ApplyIcon"


void generalInit( void )
{
}


void generalReadOptions( void )
{
	int data = configReadInt( CODERIPPERSYNTAX );
	g_generalOptions.codeRipperSyntax = (eCodeRipperSyntax)data;
	if( g_generalOptions.codeRipperSyntax < CODE_MASMSYNTAX )
		g_generalOptions.codeRipperSyntax = CODE_MASMSYNTAX;
	if( g_generalOptions.codeRipperSyntax > CODE_ATSYNTAX )
		g_generalOptions.codeRipperSyntax = CODE_MASMSYNTAX;

	data = configReadInt( DATARIPPERSYNTAX );
	g_generalOptions.dataRipperSyntax = (eDataRipperSyntax)data;
	if( g_generalOptions.dataRipperSyntax < DATA_CSYNTAX )
		g_generalOptions.dataRipperSyntax = DATA_CSYNTAX;
	if( g_generalOptions.dataRipperSyntax > DATA_MASMSYNTAX )
		g_generalOptions.dataRipperSyntax = DATA_CSYNTAX;

	g_generalOptions.applyIcon = configReadBool( APPLYICON, true );
}


void generalWriteOptions( void )
{
	configSaveInt( CODERIPPERSYNTAX, g_generalOptions.codeRipperSyntax );
	configSaveInt( DATARIPPERSYNTAX, g_generalOptions.dataRipperSyntax );
	configSaveBool( APPLYICON, g_generalOptions.applyIcon );
}


void generalApply( void )
{
	iconApply( g_generalOptions.applyIcon );
}
