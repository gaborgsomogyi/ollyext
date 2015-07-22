#include "stdafx.h"
#include "Core/Log.h"


const wchar_t* regReadValue( HKEY hKey, LPCWSTR regKeyName, LPCWSTR regValueName )
{
	wchar_t* result = NULL;

	HKEY hOpenedKey = NULL;
	DWORD openResult = RegOpenKeyEx( hKey, regKeyName, REG_OPTION_VOLATILE, KEY_QUERY_VALUE, &hOpenedKey );
	PLUGIN_CHECK_ERROR( openResult == ERROR_SUCCESS,
		return result; ,
		UNABLE_TO_OPEN_REGISTRY_KEY, regKeyName, openResult );

	DWORD keyType = 0;
	DWORD resultSize = 256;
	do
	{
		result = new wchar_t[resultSize];
		PLUGIN_CHECK_ERROR( result,
			RegCloseKey(hOpenedKey); \
			return result;,
			UNABLE_TO_ALLOCATE_LOCAL_MEMORY );

		DWORD localResultSize = resultSize;
		DWORD queryResult = RegQueryValueEx( hOpenedKey, regValueName, 0, &keyType, (LPBYTE)result, &localResultSize );
		if( queryResult == ERROR_MORE_DATA )
		{
			delete [] result;
			result = NULL;
			resultSize >>= 1;
			continue;
		}

		PLUGIN_CHECK_ERROR( queryResult == ERROR_SUCCESS,
			RegCloseKey(hOpenedKey); \
			delete [] result; \
			result = NULL; \
			return result;,
			UNABLE_TO_QUERY_REGISTRY_KEY, regKeyName, regValueName, queryResult );

		PLUGIN_CHECK_ERROR( keyType == REG_SZ,
			RegCloseKey(hOpenedKey); \
			delete [] result; \
			result = NULL; \
			return result;,
			INVALID_REGISTRY_KEY_TYPE, regKeyName, regValueName, keyType, REG_SZ, queryResult );

		break;
	} while( true );

	DWORD closeResult = RegCloseKey(hOpenedKey);
	PLUGIN_CHECK_ERROR( closeResult == ERROR_SUCCESS,
		return result; ,
		UNABLE_TO_CLOSE_REGISTRY_KEY, regKeyName, closeResult );

	return result;
}
