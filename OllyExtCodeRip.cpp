#include "stdafx.h"
#include <hash_map>
#include <set>
#include "Core/Log.h"
#include "General/General.h"
#include "OllyExtRip.h"
#include "OllyExtCodeRip.h"


//#define RIP_DEBUG


class Assembly
{
public:
	Assembly():
	label(),
	jumpAddress( 0 ),
	jumpAddressStr(),
	command()
	{}

	Assembly( DWORD jumpAddress_, const std::string& jumpAddressStr_, const std::string& command_ ):
	label(),
	jumpAddress( jumpAddress_ ),
	jumpAddressStr( jumpAddressStr_ ),
	command( command_ )
	{}

	std::string label;
	DWORD jumpAddress;
	std::string jumpAddressStr;
	std::string command;
};


static bool codeRipIsJumpHandled( std::hash_map<DWORD, Assembly>& rAssemblies,
									DWORD jumpTarget )
{
	if( rAssemblies.find( jumpTarget ) != rAssemblies.end() )
		return true;
	else
		return false;
}


static void codeRipGetFunctionRange( DWORD jumpTarget,
										DWORD& rStart,
										DWORD& rSize )
{
	rStart = jumpTarget;
	rSize = 0;

	DWORD ptr = rStart;
	BYTE buffer[MAXCMDSIZE];
	DISASM da;
	memset( &da, 0, sizeof( da ) );
	do
	{
		// Read the assembly from the target binary
		PLUGIN_CHECK_ERROR( Readmemory( buffer, ptr, MAXCMDSIZE, MM_SILENT ),
			return;,
			UNABLE_TO_READ_REMOTE_PROCESS_MEMORY, ptr, MAXCMDSIZE );

		// Reverse the code
		da.EIP = (DWORD)buffer;
		da.VirtualAddr = ptr;
		DWORD cmdSize = BeaEngineDisasm( &da );
		PLUGIN_CHECK_ERROR( cmdSize != OUT_OF_BLOCK && cmdSize != UNKNOWN_OPCODE,
			return;,
			UNABLE_TO_DISASSEMBLE_CODE, da.VirtualAddr, L"" );

		// Step
		ptr += cmdSize;
	} while( da.Instruction.BranchType != JmpType &&
				da.Instruction.BranchType != RetType );

	rSize = ptr - rStart;

#ifdef RIP_DEBUG
	logMessage( L"codeRipGetFunctionRange jumpTarget: %08X start: %08X size: %08X", jumpTarget, rStart, rSize );
#endif
}


static bool codeRipGetAsmFromRange( DWORD start,
									DWORD size,
									std::set<DWORD>& rIndices,
									std::hash_map<DWORD, Assembly>& rAssemblies,
									bool isRecursive )
{
	PLUGIN_CHECK_ERROR_NO_MB( start && size, return false; );

#ifdef RIP_DEBUG
	logMessage( L"codeRipGetAsmFromRange start: %08X size: %08X isRecursive: %ws", start, size, isRecursive ? L"yes" : L"no" );
#endif

	// Allocate the necessary memory
	BYTE* pBuffer = new BYTE[size];
	PLUGIN_CHECK_ERROR( pBuffer, return false;, UNABLE_TO_ALLOCATE_LOCAL_MEMORY );

	// Read the assembly from the target binary
	PLUGIN_CHECK_ERROR( Readmemory( pBuffer, start, size, MM_SILENT ),
		delete [] pBuffer;
		return false;,
		UNABLE_TO_READ_REMOTE_PROCESS_MEMORY, start, size );

	// Update indices and the assembly map
	DWORD ptr = 0;
	DISASM da;
	std::list<DWORD> outerJumps;

	memset( &da, 0, sizeof( da ) );
	switch( g_generalOptions.codeRipperSyntax )
	{
	default:
	case CODE_MASMSYNTAX:
		da.Options |= MasmSyntax;
	break;

	case CODE_GOASMSYNTAX:
		da.Options |= GoAsmSyntax;
	break;

	case CODE_NASMSYNTAX:
		da.Options |= NasmSyntax;
	break;

	case CODE_ATSYNTAX:
		da.Options |= ATSyntax;
	break;
	}
	da.Options |= SuffixedNumeral;

	while( ptr < size )
	{
		da.EIP = (DWORD)( pBuffer + ptr );
		da.VirtualAddr = start + ptr;
		DWORD cmdSize = BeaEngineDisasm( &da );
#ifdef RIP_DEBUG
		logMessage( L"codeRipGetAsmFromRange disasm address: %08X cmdSize: %d", (DWORD)da.VirtualAddr, cmdSize );
#endif
		PLUGIN_CHECK_ERROR( cmdSize != OUT_OF_BLOCK && cmdSize != UNKNOWN_OPCODE,
			delete [] pBuffer;
			return false;,
			UNABLE_TO_DISASSEMBLE_CODE, da.VirtualAddr, L"" );

		// Store index
		rIndices.insert( (DWORD)da.VirtualAddr );

		// Calculate jump target and store assembly
		char jumpAddressStr[16] = { 0 };
		if( da.Instruction.AddrValue )
			sprintf_s( jumpAddressStr, sizeof( jumpAddressStr ), "%08Xh", (DWORD)da.Instruction.AddrValue );
		rAssemblies[(DWORD)da.VirtualAddr] = Assembly( (DWORD)da.Instruction.AddrValue, jumpAddressStr, da.CompleteInstr );

#ifdef RIP_DEBUG
		{
			wchar_t c[MAX_PATH];
			mbstowcs( c, da.CompleteInstr, MAX_PATH );
			logMessage( L"codeRipGetAsmFromRange command: \"%ws\"", c );
		}
#endif

		// If jump is not in this function then store the pointer
		if( isRecursive &&
			da.Instruction.AddrValue &&
			!codeRipIsJumpHandled( rAssemblies, (DWORD)da.Instruction.AddrValue ) )
		{
#ifdef RIP_DEBUG
			logMessage( L"codeRipGetAsmFromRange outer jump at: %08X target: %08X", (DWORD)da.VirtualAddr, (DWORD)da.Instruction.AddrValue );
#endif
			outerJumps.push_back( (DWORD)da.Instruction.AddrValue );
		}

		ptr += cmdSize;
	}

	// Remove buffer
	delete [] pBuffer;

	// Rip other functions recursively
	for( std::list<DWORD>::iterator it = outerJumps.begin(); it != outerJumps.end(); ++it )
	{
		DWORD subStart = 0;
		DWORD subSize = 0;

		codeRipGetFunctionRange( *it, subStart, subSize );

		if( subStart && subSize )
		{
			bool result = codeRipGetAsmFromRange( subStart,
													subSize,
													rIndices,
													rAssemblies,
													isRecursive );
			if( !result ) return false;
		}
	}

	return true;
}


static bool codeRipCalculateJumps( std::set<DWORD>& rIndices,
									std::hash_map<DWORD, Assembly>& rAssemblies )
{
	for( std::set<DWORD>::iterator it = rIndices.begin(); it != rIndices.end(); ++it )
	{
		Assembly& rAssembly = rAssemblies[*it];

		// If it's jump or call
		if( rAssembly.jumpAddress )
		{
#ifdef RIP_DEBUG
			{
				wchar_t j[MAX_PATH];
				mbstowcs( j, rAssembly.jumpAddressStr.c_str(), MAX_PATH );
				wchar_t c[MAX_PATH];
				mbstowcs( c, rAssembly.command.c_str(), MAX_PATH );
				logMessage( L"codeRipCalculateJumps address: %08X pattern: \"%ws\" command: \"%ws\"", *it, j, c );
			}
#endif
			// Replace ADDRESS with LOC_ADDRESS
			int pos = rAssembly.command.find( rAssembly.jumpAddressStr );
			PLUGIN_CHECK_ERROR( pos != -1, return false;, UNABLE_TO_FIND_TARGET_ADDRESS, *it, rAssembly.command.c_str() );
			rAssembly.command.insert( pos, "LOC_" );

			// Set the label for the target
			std::hash_map<DWORD, Assembly>::iterator targetIt = rAssemblies.find( rAssembly.jumpAddress );
			if( targetIt != rAssemblies.end() )
			{
				char labelStr[16] = { 0 };
				sprintf_s( labelStr, sizeof( labelStr ), "%08Xh", (*targetIt).first );
				(*targetIt).second.label = labelStr;
			}
		}
	}

	return true;
}


static void codeRipAddInitialJump( DWORD initAddress,
									std::set<DWORD>& rIndices,
									std::hash_map<DWORD, Assembly>& rAssemblies )
{
#ifdef RIP_DEBUG
	logMessage( L"codeRipAddInitialJump initAddress: %08X", initAddress );
#endif
	rIndices.insert( 0 );
	char jumpAddressStr[16] = { 0 };
	sprintf_s( jumpAddressStr, sizeof( jumpAddressStr ), "%08Xh", initAddress );
	char asmStr[16] = { 0 };
	sprintf_s( asmStr, sizeof( asmStr ), "jmp %08Xh", initAddress );
	rAssemblies[0] = Assembly( initAddress, jumpAddressStr, asmStr );
	rAssemblies[0].label = "FUNCTION_ENTRY_POINT";
}


static bool codeRipBuildAssembly( std::set<DWORD>& rIndices,
									std::hash_map<DWORD, Assembly>& rAssemblies,
									std::string& rAsmCode )
{
	for( std::set<DWORD>::iterator it = rIndices.begin(); it != rIndices.end(); ++it )
	{
		Assembly& rAssembly = rAssemblies[*it];

		if( !rAssembly.label.empty() )
		{
			rAsmCode += "LOC_";
			rAsmCode += rAssembly.label;
			rAsmCode += ":\r\n";
		}

		rAsmCode += "\t";
		rAsmCode += rAssembly.command;
		rAsmCode += "\r\n";
	}

	return true;
}


static bool codeRipGetAsmFromRange( std::string& rAsmCode, bool isRecursive )
{
	rAsmCode = "";

	t_dump* pDisasmDump = Getcpudisasmdump();
	PLUGIN_CHECK_ERROR( pDisasmDump, return false;, UNABLE_TO_GET_DISASSEMBLER_DUMP );

	DWORD bufferSize = pDisasmDump->sel1 - pDisasmDump->sel0;

	std::set<DWORD> indices;
	std::hash_map<DWORD, Assembly> assemblies;

	PLUGIN_CHECK_ERROR_NO_MB( codeRipGetAsmFromRange( pDisasmDump->sel0, bufferSize, indices, assemblies, isRecursive ),
		return false; );

	if( indices.find( pDisasmDump->sel0 ) != indices.begin() )
	{
		codeRipAddInitialJump( pDisasmDump->sel0, indices, assemblies );
	}

	PLUGIN_CHECK_ERROR_NO_MB( codeRipCalculateJumps( indices, assemblies ), return false; );
	PLUGIN_CHECK_ERROR_NO_MB( codeRipBuildAssembly( indices, assemblies, rAsmCode ), return false; );

	return true;
}


void codeRipExecute( bool isRecursive )
{
	std::string asmCode = "";

	PLUGIN_CHECK_ERROR_NO_MB( codeRipGetAsmFromRange( asmCode, isRecursive ), return; );
	PLUGIN_CHECK_ERROR_NO_MB( ripSendToClipboard( asmCode ), return; );

	logMessage( L"%ws: Code ripped successfully", dbgGetPluginName() );
}
