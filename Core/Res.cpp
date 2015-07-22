#include "stdafx.h"
#include "Core/vlx.h"
#include "Core/Log.h"


bool resourceSave( HMODULE hMod, DWORD resourceID, const wchar_t* resourceType, const wchar_t* fileName )
{
	logMessage( L"%ws: Saving resource: %ws", dbgGetPluginName(), fileName );

	HRSRC res = FindResource( hMod, MAKEINTRESOURCE( resourceID ), resourceType );
	PLUGIN_CHECK_ERROR( res, return false;, UNABLE_TO_FIND_RESOURCE );

	unsigned int resSize = SizeofResource( hMod, res );
	PLUGIN_CHECK_ERROR( resSize, return false;, UNABLE_TO_GET_RESOURCE_SIZE );

	HGLOBAL resData = LoadResource( hMod, res );
	PLUGIN_CHECK_ERROR( resData, return false;, UNABLE_TO_LOAD_RESOURCE );

	void* resBinaryData = LockResource( resData );
	PLUGIN_CHECK_ERROR( resBinaryData, return false;, UNABLE_TO_LOCK_RESOURCE );

	void* resBinaryDataUnpacked = NULL;
	unsigned int resSizeUnpacked = 0;
	bool isUnpacked = vlx_decompress( resBinaryData, resSize, &resBinaryDataUnpacked, (DWORD*)&resSizeUnpacked );
	PLUGIN_CHECK_ERROR( isUnpacked, return false;, UNABLE_TO_UNCOMPRESS_DATA );

    std::ofstream f( fileName, std::ios::out | std::ios::binary );
	PLUGIN_CHECK_ERROR( !f.fail(),
		if( resBinaryDataUnpacked )\
		{\
			delete [] resBinaryDataUnpacked;\
			resBinaryDataUnpacked = NULL;\
		}\
		return false;,
		UNABLE_TO_SAVE_FILE_BECAUSE_OF_FAILBIT );
	PLUGIN_CHECK_ERROR( !f.bad(),
		if( resBinaryDataUnpacked )\
		{\
			delete [] resBinaryDataUnpacked;\
			resBinaryDataUnpacked = NULL;\
		}\
		return false;,
		UNABLE_TO_SAVE_FILE_BECAUSE_OF_BADBIT );
    f.write( (char*)resBinaryDataUnpacked, resSizeUnpacked );
	PLUGIN_CHECK_ERROR( !f.fail(),
		if( resBinaryDataUnpacked )\
		{\
			delete [] resBinaryDataUnpacked;\
			resBinaryDataUnpacked = NULL;\
		}\
		return false;,
		UNABLE_TO_SAVE_FILE_BECAUSE_OF_FAILBIT );
	PLUGIN_CHECK_ERROR( !f.bad(),
		if( resBinaryDataUnpacked )\
		{\
			delete [] resBinaryDataUnpacked;\
			resBinaryDataUnpacked = NULL;\
		}\
		return false;,
		UNABLE_TO_SAVE_FILE_BECAUSE_OF_BADBIT );
    f.close();

	if( resBinaryDataUnpacked )
	{
		delete [] resBinaryDataUnpacked;
		resBinaryDataUnpacked = NULL;
	}

	return true;
}
