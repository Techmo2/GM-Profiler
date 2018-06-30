#ifndef STOOL_h
#define STOOL_h

#include "windows.h"
#include "psapi.h"
#include "TCHAR.h"
#include "pdh.h"

MEMORYSTATUSEX memStat;
PPROCESS_MEMORY_COUNTERS pmc;
static ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
static HANDLE self;
static PDH_HQUERY cpuQuery;
static PDH_HCOUNTER cpuTotal;

bool cpu_system_stat_init = false;
bool cpu_proc_stat_init = false;

namespace STool {

	struct ProfilerNode {
		const char* name;
		__int64 num_calls;
		__int64 time_start;
		__int64 time_end;
		__int64 time_elapsed;
	};

	struct SysStats {
		double total_phys_mem; // Total physical memory in a system
		double system_mem_usage; // Total physical memory used in a system
		long proc_mem_usage; // Physical memory currently used by this process
		double proc_pagefile_usage; // The amount of pagefile used by the current process
		double total_cpu_usage; // Current CPU utilization by all processes
		int num_processors; // The number of logical processors
		double proc_cpu_usage; // The CPU utilization of just this process
	};

	// Query the memory statistics, then get the amount of physical memory present in the system
	double queryTotalPhysMemory(SysStats &stat) {
		memStat.dwLength = sizeof(MEMORYSTATUSEX);
		GlobalMemoryStatusEx(&memStat);
		stat.total_phys_mem = (double)memStat.ullTotalPhys;
		return stat.total_phys_mem;
	}

	// Query the memory statistics, then get the amount of physical memory used by the system
	double querySystemMemoryUsage(SysStats &stat) {
		memStat.dwLength = sizeof(MEMORYSTATUSEX);
		GlobalMemoryStatusEx(&memStat);
		stat.system_mem_usage = (double) (memStat.ullTotalPhys - memStat.ullAvailPhys);
		return stat.system_mem_usage;
	}

	// Query the memory statistics, then get the amount of physical memory used by the current process
	long queryProcessMemoryUsage(SysStats &stat) {
		GetProcessMemoryInfo(GetCurrentProcess(), pmc, sizeof(pmc));
		stat.proc_mem_usage = (long)pmc->WorkingSetSize;
		return stat.proc_mem_usage;
	}

	double querySystemCPUUsage(SysStats &stat) {
		if (!cpu_system_stat_init) {
			PdhOpenQuery(NULL, NULL, &cpuQuery);
			// You can also use L"\\Processor(*)\\% Processor Time" and get individual CPU values with PdhGetFormattedCounterArray()
			PdhAddEnglishCounter(cpuQuery, (const char*)L"\\Processor(_Total)\\% Processor Time", NULL, &cpuTotal);
			PdhCollectQueryData(cpuQuery);
			cpu_system_stat_init = true;
		}

		PDH_FMT_COUNTERVALUE counterVal;
		PdhCollectQueryData(cpuQuery);
		PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal);
		stat.total_cpu_usage = counterVal.doubleValue;

		return stat.total_cpu_usage;
	}

	double queryProcCPUUsage(SysStats &stat) {
		if (!cpu_proc_stat_init) {
			SYSTEM_INFO sysInfo;
			FILETIME ftime, fsys, fuser;

			GetSystemInfo(&sysInfo);
			stat.num_processors = sysInfo.dwNumberOfProcessors;

			GetSystemTimeAsFileTime(&ftime);
			memcpy(&lastCPU, &ftime, sizeof(FILETIME));

			self = GetCurrentProcess();
			GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
			memcpy(&lastSysCPU, &fsys, sizeof(FILETIME));
			memcpy(&lastUserCPU, &fuser, sizeof(FILETIME));
		}

		FILETIME ftime, fsys, fuser;
		ULARGE_INTEGER now, sys, user;
		double percent;

		GetSystemTimeAsFileTime(&ftime);
		memcpy(&now, &ftime, sizeof(FILETIME));

		GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
		memcpy(&sys, &fsys, sizeof(FILETIME));
		memcpy(&user, &fuser, sizeof(FILETIME));
		percent = (sys.QuadPart - lastSysCPU.QuadPart) +
			(user.QuadPart - lastUserCPU.QuadPart);
		percent /= (now.QuadPart - lastCPU.QuadPart);
		percent /= stat.num_processors;
		lastCPU = now;
		lastUserCPU = user;
		lastSysCPU = sys;

		stat.proc_cpu_usage = percent * 100;
		return stat.proc_cpu_usage;
	}

	int queryNumProcessors(SysStats &stat) {
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		stat.num_processors = sysInfo.dwNumberOfProcessors;

		return stat.num_processors;
	}
}

#endif STOOL_h
