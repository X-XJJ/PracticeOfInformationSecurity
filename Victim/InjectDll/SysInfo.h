

#include <stdio.h>
#include<stdlib.h>
#include <windows.h>
#include <ntsecapi.h>



typedef DWORD(WINAPI *NTQUERYSYSTEMINFORMATION)(DWORD, PVOID, DWORD, PDWORD);

typedef struct _SYSTEM_PROCESS_INFORMATION

{

	DWORD NextEntryDelta;

	DWORD ThreadCount;

	DWORD Reserved1[6];

	FILETIME ftCreateTime;

	FILETIME ftUserTime;

	FILETIME ftKernelTime;

	UNICODE_STRING ProcessName;

	DWORD BasePriority;

	DWORD ProcessId;

	DWORD InheritedFromProcessId;

	DWORD HandleCount;

	DWORD Reserved2[2];

	DWORD VmCounters;

	DWORD dCommitCharge;

	PVOID ThreadInfos[1];

}SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;


typedef enum _SYSTEM_INFORMATION_CLASS {
	SystemBasicInformation,  //0
	SystemProcessorInformation,  //1
	SystemPerformanceInformation,  //2
	SystemTimeOfDayInformation,  //3
	SystemPathInformation,  //4
	SystemProcessInformation, //5  进程信息
	SystemCallCountInformation,
	SystemDeviceInformation,
	SystemProcessorPerformanceInformation,
	SystemFlagsInformation,
	SystemCallTimeInformation,  //10
	SystemModuleInformation,  //模块信息
	SystemLocksInformation,
	SystemStackTraceInformation,
	SystemPagedPoolInformation,
	SystemNonPagedPoolInformation,
	SystemHandleInformation,
	SystemObjectInformation,
	SystemPageFileInformation,
	SystemVdmInstemulInformation,
	SystemVdmBopInformation,  //20
	SystemFileCacheInformation,
	SystemPoolTagInformation,
	SystemInterruptInformation,
	SystemDpcBehaviorInformation,
	SystemFullMemoryInformation,
	SystemLoadGdiDriverInformation,
	SystemUnloadGdiDriverInformation,
	SystemTimeAdjustmentInformation,
	SystemSummaryMemoryInformation,
	SystemNextEventIdInformation,  //30
	SystemEventIdsInformation,
	SystemCrashDumpInformation,
	SystemExceptionInformation,
	SystemCrashDumpStateInformation,
	SystemKernelDebuggerInformation,
	SystemContextSwitchInformation,
	SystemRegistryQuotaInformation,
	SystemExtendServiceTableInformation,
	SystemPrioritySeperation,
	SystemPlugPlayBusInformation,  //40
	SystemDockInformation,
	SystemPowerInformation2,
	SystemProcessorSpeedInformation,
	SystemCurrentTimeZoneInformation,
	SystemLookasideInformation
} SYSTEM_INFORMATION_CLASS, *PSYSTEM_INFORMATION_CLASS;
