#include "stdafx.h"
#include <hash_map>
#include "Core/Log.h"
#include "Core/OS.h"
#include "Core/Res.h"
#include "Core/Ini.h"
#include "Core/Remote.h"
#include "Custom.h"


//#define CUSTOM_DEBUG
#define CUSTOM_PATCH_DIRECTORY ( L"OllyExtPatches" )
#define CUSTOM_PATCH_FIND_PATTERN ( L"OllyExtPatches/*.ini" )
#define CUSTOM_PATCH_TYPE ( L"INI_FILE" )
#define CUSTOM_PATCH_FILENAME ( L"OllyExtPatches\\Example.ini" )


typedef struct sCustomPatch
{
	bool applied;
	void* procAddress;
	DWORD originalDataSize;
	void* originalData;

	sCustomPatch():
	applied( false ),
	procAddress( NULL ),
	originalDataSize( 0 ),
	originalData( NULL )
	{}

	~sCustomPatch()
	{
		if( originalData )
		{
			delete [] originalData;
		}
	}
} sCustomPatch;


static std::hash_map<std::wstring, sCustomPatch> g_patches;


void customInit( HMODULE hMod )
{
	if( !osDirExists( CUSTOM_PATCH_DIRECTORY ) )
	{
		if( CreateDirectory( CUSTOM_PATCH_DIRECTORY, NULL ) )
			resourceSave( hMod, IDR_INI_FILE1, CUSTOM_PATCH_TYPE, CUSTOM_PATCH_FILENAME );
	}
}


void customReset( void )
{
	g_patches.clear();
}


bool customApply( bool enabled )
{
	if( enabled )
	{
		WIN32_FIND_DATA findData;
		HANDLE hFind = FindFirstFile( CUSTOM_PATCH_FIND_PATTERN, &findData );
		if( hFind != INVALID_HANDLE_VALUE )
		{
			do
			{
				sCustomPatch& rPatch = g_patches[findData.cFileName];

				if( rPatch.applied ) continue;

#ifdef CUSTOM_DEBUG
				logMessage( L"%ws: Applying custom patch: %ws", dbgGetPluginName(), findData.cFileName );
#endif
				std::wstring iniFilePath = L".\\";
				iniFilePath.append( CUSTOM_PATCH_DIRECTORY );
				iniFilePath.append( L"\\" );
				iniFilePath.append( findData.cFileName );

				std::wstring patchName = iniReadString( iniFilePath.c_str(), L"Name" );
				std::wstring moduleName = iniReadString( iniFilePath.c_str(), L"Module" );
				std::wstring functionName = iniReadString( iniFilePath.c_str(), L"Function" );
				std::wstring patch = iniReadString( iniFilePath.c_str(), L"Patch" );

#ifdef CUSTOM_DEBUG
				logMessage( L"%ws:   Name: %ws", dbgGetPluginName(), patchName.c_str() );
				logMessage( L"%ws:   Module: %ws", dbgGetPluginName(), moduleName.c_str() );
				logMessage( L"%ws:   Function: %ws", dbgGetPluginName(), functionName.c_str() );
				logMessage( L"%ws:   Patch: %ws", dbgGetPluginName(), patch.c_str() );
#endif

				rPatch.procAddress = remoteGetProcAddress( dbgGetProcessHandle(), moduleName.c_str(), functionName.c_str() );
				if( !rPatch.procAddress ) continue;

				// Alloc space for the patch
				DWORD patchSize = patch.length() >> 1;
				if( !patchSize ) continue;
				BYTE* patchCode = new BYTE[patchSize];
				memset( patchCode, 0, patchSize );

				// Alloc space for the original bytes
				rPatch.originalDataSize = patchSize;
				rPatch.originalData = new BYTE[patchSize];
				memset( rPatch.originalData, 0, patchSize );

				// Convert string patch into byte array
				DWORD ptr = 0;
				for( DWORD i = 0; i < patch.length(); i += 2 )
				{
					std::wstring subPatch = patch.substr( i, 2 );
					swscanf( subPatch.c_str(), L"%02X", &patchCode[ptr++] );
				}

				if( !remotePatchApply( dbgGetProcessHandle(), rPatch.procAddress, (BYTE*)rPatch.originalData, patchCode, patchSize ) )
				{
					delete [] patchCode;
					rPatch.originalDataSize = 0;
					delete [] rPatch.originalData;
					rPatch.originalData = NULL;
					continue;
				}

				delete [] patchCode;

#ifdef CUSTOM_DEBUG
				logMessage( L"%ws: Applied custom patch: %ws", dbgGetPluginName(), findData.cFileName );
#endif
				rPatch.applied = true;
			} while( FindNextFile( hFind, &findData ) );
			FindClose( hFind );
		}
	}
	else
	{
		for( std::hash_map<std::wstring, sCustomPatch>::iterator it = g_patches.begin(); it != g_patches.end(); ++it )
		{
			if( !it->second.applied ) continue;

			if( !remotePatchApply( dbgGetProcessHandle(), it->second.procAddress, NULL, (BYTE*)it->second.originalData, it->second.originalDataSize ) )
			{
				continue;
			}

			it->second.applied = false;
			it->second.procAddress = NULL;
			it->second.originalDataSize = 0;
			if( it->second.originalData )
			{
				delete [] it->second.originalData;
			}
		}
	}

	return true;
}
