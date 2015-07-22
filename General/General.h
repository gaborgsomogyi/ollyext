#pragma once


typedef enum
{
	CODE_MASMSYNTAX,
	CODE_GOASMSYNTAX,
	CODE_NASMSYNTAX,
	CODE_ATSYNTAX
} eCodeRipperSyntax;

typedef enum
{
	DATA_CSYNTAX,
	DATA_MASMSYNTAX
} eDataRipperSyntax;

typedef struct
{
	eCodeRipperSyntax codeRipperSyntax;
	eDataRipperSyntax dataRipperSyntax;
	bool applyIcon;
} sGeneralOptions;


extern sGeneralOptions g_generalOptions;


void generalInit( void );
void generalReadOptions( void );
void generalWriteOptions( void );
void generalApply( void );
