#include "stdafx.h"
#include "Core/Log.h"
#include "Core/DBG.h"
#include "Core/Config.h"
#include "Protect.h"

#include "IsDebuggerPresent.h"
#include "NtGlobalFlag.h"
#include "HeapFlags.h"
#include "ForceFlags.h"
#include "NtQueryInformationProcess.h"
#include "OutputDebugStringA.h"
#include "OutputDebugStringW.h"
#include "NtClose.h"
#include "NtOpenProcess.h"
#include "BlockInput.h"
#include "TerminateProcess.h"
#include "SetInformationThread.h"
#include "NtQueryObject.h"
#include "FindWindowA.h"
#include "FindWindowExA.h"
#include "FindWindowW.h"
#include "FindWindowExW.h"

#include "Process32First.h"
#include "Process32FirstW.h"
#include "Process32Next.h"
#include "Process32NextW.h"
#include "GetTickCount.h"
#include "TimeGetTime.h"
#include "QueryPerformanceCounter.h"
#include "ZwGetContextThread.h"
#include "NtSetContextThread.h"
#include "NtQuerySystemInformation.h"
#include "NtSetDebugFilterState.h"
#include "NtSetDebugFilterState.h"
#include "KiUserExceptionDispatcher.h"
#include "ZwContinue.h"
#include "DbgPrompt.h"
#include "CreateThread.h"
#include "NtSystemDebugControl.h"

#include "Custom.h"


sProtectOptions g_protectOptions = { 0 };


#define ISDEBUGGERPRESENT L"IsDebuggerPresent"
#define NTGLOBALFLAG L"NtGlobalFlag"
#define HEAPFLAGS L"HeapFlags"
#define FORCEFLAGS L"ForceFlags"
#define CHECKREMOTEDEBUGGERPRESENT L"CheckRemoteDebuggerPresent"
#define OUTPUTDEBUGSTRING L"OutputDebugString"
#define NTCLOSE L"NtClose"
#define SEDEBUGPRIVILEGE L"SeDebugPrivilege"
#define BLOCKINPUT L"BlockInput"
#define PROCESSDEBUGFLAGS L"ProcessDebugFlags"
#define PROCESSDEBUGOBJECTHANDLE L"ProcessDebugObjectHandle"
#define TERMINATEPROCESS L"TerminateProcess"
#define NTSETINFORMATIONTHREAD L"NtSetInformationThread"
#define NTQUERYOBJECT L"NtQueryObject"
#define FINDWINDOW L"FindWindow"
#define NTOPENPROCESS L"NtOpenProcess"
#define PARENTPROCESS L"ParentProcess"

#define PROCESS32FIRST L"Process32First"
#define PROCESS32NEXT L"Process32Next"
#define GETTICKCOUNT L"GetTickCount"
#define TIMEGETTIME L"TimeGetTime"
#define QUERYPERFORMANCECOUNTER L"QueryPerformanceCounter"
#define ZWGETCONTEXTTHREAD L"ZwGetContextThread"
#define NTSETCONTEXTTHREAD L"NtSetContextThread"
#define KDDEBUGGERNOTPRESENT L"KdDebuggerNotPresent"
#define KDDEBUGGERENABLED L"KdDebuggerEnabled"
#define NTSETDEBUGFILTERSTATE L"NtSetDebugFilterState"
#define PROTECTDRX L"ProtectDRX"
#define HIDEDRX L"HideDRX"
#define DBGPROMPT L"DbgPrompt"
#define CREATETHREAD L"CreateThread"
#define NTSYSTEMDEBUGCONTROL L"NtSystemDebugControl"

#define CUSTOM L"Custom"


void protectInit( HMODULE hMod )
{
	logMessage( L"%ws: Protection init", dbgGetPluginName() );

	customInit( hMod );
}


void protectReadOptions( void )
{
	g_protectOptions.isDebuggerPresent = configReadBool( ISDEBUGGERPRESENT );
	g_protectOptions.ntGlobalFlag = configReadBool( NTGLOBALFLAG );
	g_protectOptions.heapFlags = configReadBool( HEAPFLAGS );
	g_protectOptions.forceFlags = configReadBool( FORCEFLAGS );
	g_protectOptions.checkRemoteDebuggerPresent = configReadBool( CHECKREMOTEDEBUGGERPRESENT );
	g_protectOptions.outputDebugString = configReadBool( OUTPUTDEBUGSTRING );
	g_protectOptions.ntClose = configReadBool( NTCLOSE );
	g_protectOptions.seDebugPrivilege = configReadBool( SEDEBUGPRIVILEGE );
	g_protectOptions.blockInput = configReadBool( BLOCKINPUT );
	g_protectOptions.processDebugFlags = configReadBool( PROCESSDEBUGFLAGS );
	g_protectOptions.processDebugObjectHandle = configReadBool( PROCESSDEBUGOBJECTHANDLE );
	g_protectOptions.terminateProcess = configReadBool( TERMINATEPROCESS );
	g_protectOptions.ntSetInformationThread = configReadBool( NTSETINFORMATIONTHREAD );
	g_protectOptions.ntQueryObject = configReadBool( NTQUERYOBJECT );
	g_protectOptions.findWindow = configReadBool( FINDWINDOW );
	g_protectOptions.ntOpenProcess = configReadBool( NTOPENPROCESS );

	g_protectOptions.process32First = configReadBool( PROCESS32FIRST );
	g_protectOptions.process32Next = configReadBool( PROCESS32NEXT );
	g_protectOptions.parentProcess = configReadBool( PARENTPROCESS );
	g_protectOptions.getTickCount = configReadBool( GETTICKCOUNT );
	g_protectOptions.timeGetTime = configReadBool( TIMEGETTIME );
	g_protectOptions.queryPerformanceCounter = configReadBool( QUERYPERFORMANCECOUNTER );
	g_protectOptions.zwGetContextThread = configReadBool( ZWGETCONTEXTTHREAD );
	g_protectOptions.ntSetContextThread = configReadBool( NTSETCONTEXTTHREAD );
	g_protectOptions.kdDebuggerNotPresent = configReadBool( KDDEBUGGERNOTPRESENT );
	g_protectOptions.kdDebuggerEnabled = configReadBool( KDDEBUGGERENABLED );
	g_protectOptions.ntSetDebugFilterState = configReadBool( NTSETDEBUGFILTERSTATE );
	g_protectOptions.protectDRX = configReadBool( PROTECTDRX );
	g_protectOptions.hideDRX = configReadBool( HIDEDRX );
	g_protectOptions.dbgPrompt = configReadBool( DBGPROMPT );
	g_protectOptions.createThread = configReadBool( CREATETHREAD );
	g_protectOptions.ntSystemDebugControl = configReadBool( NTSYSTEMDEBUGCONTROL );

	g_protectOptions.custom = configReadBool( CUSTOM );
}


void protectWriteOptions( void )
{
	configSaveBool( ISDEBUGGERPRESENT, g_protectOptions.isDebuggerPresent );
	configSaveBool( ISDEBUGGERPRESENT, g_protectOptions.isDebuggerPresent );
	configSaveBool( NTGLOBALFLAG, g_protectOptions.ntGlobalFlag );
	configSaveBool( HEAPFLAGS, g_protectOptions.heapFlags );
	configSaveBool( FORCEFLAGS, g_protectOptions.forceFlags );
	configSaveBool( CHECKREMOTEDEBUGGERPRESENT, g_protectOptions.checkRemoteDebuggerPresent );
	configSaveBool( OUTPUTDEBUGSTRING, g_protectOptions.outputDebugString );
	configSaveBool( NTCLOSE, g_protectOptions.ntClose );
	configSaveBool( SEDEBUGPRIVILEGE, g_protectOptions.seDebugPrivilege );
	configSaveBool( BLOCKINPUT, g_protectOptions.blockInput );
	configSaveBool( PROCESSDEBUGFLAGS, g_protectOptions.processDebugFlags );
	configSaveBool( PROCESSDEBUGOBJECTHANDLE, g_protectOptions.processDebugObjectHandle );
	configSaveBool( TERMINATEPROCESS, g_protectOptions.terminateProcess );
	configSaveBool( NTSETINFORMATIONTHREAD, g_protectOptions.ntSetInformationThread );
	configSaveBool( NTQUERYOBJECT, g_protectOptions.ntQueryObject );
	configSaveBool( FINDWINDOW, g_protectOptions.findWindow );
	configSaveBool( NTOPENPROCESS, g_protectOptions.ntOpenProcess );

	configSaveBool( PROCESS32FIRST, g_protectOptions.process32Next );
	configSaveBool( PROCESS32NEXT, g_protectOptions.process32Next );
	configSaveBool( PARENTPROCESS, g_protectOptions.parentProcess );
	configSaveBool( GETTICKCOUNT, g_protectOptions.getTickCount );
	configSaveBool( TIMEGETTIME, g_protectOptions.timeGetTime );
	configSaveBool( QUERYPERFORMANCECOUNTER, g_protectOptions.queryPerformanceCounter );
	configSaveBool( ZWGETCONTEXTTHREAD, g_protectOptions.zwGetContextThread );
	configSaveBool( NTSETCONTEXTTHREAD, g_protectOptions.ntSetContextThread );
	configSaveBool( KDDEBUGGERNOTPRESENT, g_protectOptions.kdDebuggerNotPresent );
	configSaveBool( KDDEBUGGERENABLED, g_protectOptions.kdDebuggerEnabled );
	configSaveBool( NTSETDEBUGFILTERSTATE, g_protectOptions.ntSetDebugFilterState );
	configSaveBool( PROTECTDRX, g_protectOptions.protectDRX );
	configSaveBool( HIDEDRX, g_protectOptions.hideDRX );
	configSaveBool( DBGPROMPT, g_protectOptions.dbgPrompt );
	configSaveBool( CREATETHREAD, g_protectOptions.createThread );
	configSaveBool( NTSYSTEMDEBUGCONTROL, g_protectOptions.ntSystemDebugControl );

	configSaveBool( CUSTOM, g_protectOptions.custom );
}


void protectPreHook( void )
{
	logMessage( L"%ws: Protection pre hook", dbgGetPluginName() );
	ntGlobalFlagPreHook( g_protectOptions.ntGlobalFlag );
}


void protectPostHook( void )
{
	logMessage( L"%ws: Protection post hook", dbgGetPluginName() );
	ntGlobalFlagPostHook( g_protectOptions.ntGlobalFlag );
}


void protectReset( void )
{
	logMessage( L"%ws: Protection reset", dbgGetPluginName() );

	isDebuggerPresentReset();
	ntGlobalFlagReset();
	heapFlagsReset();
	forceFlagsReset();
	ntQueryInformationProcessReset();
	outputDebugStringAReset();
	outputDebugStringWReset();
	ntCloseReset();
	ntOpenProcessReset();
	blockInputReset();
	terminateProcessReset();
	setInformationThreadReset();
	ntQueryObjectReset();
	findWindowAReset();
	findWindowExAReset();
	findWindowWReset();
	findWindowExWReset();

	process32FirstReset();
	process32FirstWReset();
	process32NextReset();
	process32NextWReset();
	getTickCountReset();
	timeGetTimeReset();
	queryPerformanceCounterReset();
	zwGetContextThreadReset();
	ntSetContextThreadReset();
	ntQuerySystemInformationReset();
	ntSetDebugFilterStateReset();
	kiUserExceptionDispatcherReset();
	zwContinueReset();
	dbgPromptReset();
	createThreadReset();
	ntSystemDebugControlReset();

	customReset();
}


bool protectApply( void )
{
	logMessage( L"%ws: Protection apply", dbgGetPluginName() );

	PLUGIN_CHECK_ERROR( dbgGetProcessHandle(), return false;, NO_DEBUGGE );

	isDebuggerPresentApply( g_protectOptions.isDebuggerPresent );
	ntGlobalFlagApply( g_protectOptions.ntGlobalFlag );
	heapFlagsApply( g_protectOptions.heapFlags );
	forceFlagsApply( g_protectOptions.forceFlags );
	ntQueryInformationProcessApply
	(
		g_protectOptions.checkRemoteDebuggerPresent,
		g_protectOptions.processDebugObjectHandle,
		g_protectOptions.processDebugFlags,
		g_protectOptions.parentProcess
	);
	outputDebugStringAApply( g_protectOptions.outputDebugString );
	outputDebugStringWApply( g_protectOptions.outputDebugString );
	ntCloseApply( g_protectOptions.ntClose );
	ntOpenProcessApply
	(
		g_protectOptions.seDebugPrivilege,
		g_protectOptions.ntOpenProcess
	);
	blockInputApply( g_protectOptions.blockInput );
	terminateProcessApply( g_protectOptions.terminateProcess );
	setInformationThreadApply( g_protectOptions.ntSetInformationThread );
	ntQueryObjectApply( g_protectOptions.ntQueryObject );
	findWindowAApply( g_protectOptions.findWindow );
	findWindowExAApply( g_protectOptions.findWindow );
	findWindowWApply( g_protectOptions.findWindow );
	findWindowExWApply( g_protectOptions.findWindow );

	process32FirstApply( g_protectOptions.process32First );
	process32FirstWApply( g_protectOptions.process32First );
	process32NextApply( g_protectOptions.process32Next );
	process32NextWApply( g_protectOptions.process32Next );
	getTickCountApply( g_protectOptions.getTickCount );
	timeGetTimeApply( g_protectOptions.timeGetTime );
	queryPerformanceCounterApply( g_protectOptions.queryPerformanceCounter );
	zwGetContextThreadApply( g_protectOptions.zwGetContextThread );
	ntSetContextThreadApply( g_protectOptions.ntSetContextThread );
	ntQuerySystemInformationApply
	(
		g_protectOptions.kdDebuggerNotPresent,
		g_protectOptions.kdDebuggerEnabled
	);
	ntSetDebugFilterStateApply( g_protectOptions.ntSetDebugFilterState );
	kiUserExceptionDispatcherApply
	(
		g_protectOptions.protectDRX,
		g_protectOptions.hideDRX
	);
	zwContinueApply( g_protectOptions.protectDRX );
	dbgPromptApply( g_protectOptions.dbgPrompt );
	createThreadApply( g_protectOptions.createThread );
	ntSystemDebugControlApply( g_protectOptions.ntSystemDebugControl );

	customApply( g_protectOptions.custom );

	return true;
}
