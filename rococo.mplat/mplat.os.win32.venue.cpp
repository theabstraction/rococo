#include <rococo.api.h>
#include <rococo.os.win32.h>
#include <rococo.window.h>
#include <rococo.os.h>

#include <Processthreadsapi.h>
#include <psapi.h>

using namespace Rococo;

struct OSVenue : public IMathsVenue
{
	int numProcessors;
	HANDLE hCurrentProcess;

	FILETIME createdAt;
	FILETIME exitAt;
	FILETIME kernelTime;
	FILETIME userTime;

	ULARGE_INTEGER lastCPU;
	ULARGE_INTEGER  lastSysCPU;
	ULARGE_INTEGER  lastUserCPU;

	OSVenue()
	{
		SYSTEM_INFO sysInfo;
		FILETIME ftime;

		GetSystemInfo(&sysInfo);
		numProcessors = sysInfo.dwNumberOfProcessors;

		GetSystemTimeAsFileTime(&ftime);
		memcpy(&lastCPU, &ftime, sizeof(FILETIME));

		hCurrentProcess = GetCurrentProcess();
		GetProcessTimes(hCurrentProcess, &createdAt, &exitAt, &kernelTime, &userTime);
		memcpy(&lastSysCPU, &kernelTime, sizeof(FILETIME));
		memcpy(&lastUserCPU, &userTime, sizeof(FILETIME));

		lastCPU = lastSysCPU;
	}

	double usage = 0;

	double getCurrentValue()
	{
		FILETIME ftime, fsys, fuser;
		ULARGE_INTEGER now, sys, user;
		double percent;

		GetSystemTimeAsFileTime(&ftime);
		memcpy(&now, &ftime, sizeof(FILETIME));

		if (!GetProcessTimes(hCurrentProcess, &ftime, &ftime, &fsys, &fuser))
		{
			return usage;
		}

		memcpy(&sys, &fsys, sizeof(FILETIME));
		memcpy(&user, &fuser, sizeof(FILETIME));
		percent = (double)((sys.QuadPart - lastSysCPU.QuadPart) + (user.QuadPart - lastUserCPU.QuadPart));
		percent /= (now.QuadPart - lastCPU.QuadPart);
		percent /= numProcessors;

		if ((now.QuadPart - lastCPU.QuadPart) < 20000000)
		{
			return usage;
		}

		lastCPU = now;
		lastUserCPU = user;
		lastSysCPU = sys;

		usage = percent * 100;
		return usage;
	}

	virtual void ShowVenue(IMathsVisitor& visitor)
	{
		visitor.ShowString("OS", "Windows");

		SYSTEMTIME sysTime;
		GetSystemTime(&sysTime);

		visitor.ShowString("Time", "%.2d/%.2d/%.4d %.2d:%.2d:%.2d", sysTime.wDay, sysTime.wMonth, sysTime.wYear, sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
		visitor.ShowDecimal("CPU Time", OS::CpuTicks());
		visitor.ShowDecimal("CPU Hz", OS::CpuHz());
		visitor.ShowDecimal("UTC Ticks", OS::UTCTime());

		MEMORYSTATUSEX statex = { 0 };
		statex.dwLength = sizeof(statex);
		GlobalMemoryStatusEx(&statex);

		visitor.ShowString("Memory used", "%lld MB of %lld MB. Load %d%%", (statex.ullTotalPhys - statex.ullAvailPhys) / 1048576, statex.ullTotalPhys / 1048576, statex.dwMemoryLoad);

		int32 processorNumber = (int32)GetCurrentProcessorNumber();
		visitor.ShowString("Cpu # (Main Thread)", "%d", processorNumber);
		visitor.ShowString("Process Id", "%d", (int32)GetCurrentProcessId());

		HANDLE hProcess = GetCurrentProcess();

		DWORD handleCount;
		GetProcessHandleCount(hProcess, &handleCount);

		visitor.ShowString("Handle Count", "%d", (int32)handleCount);

		visitor.ShowString("Command Line", "%s", (cstr)GetCommandLineA());

		PROCESS_MEMORY_COUNTERS pmc;
		if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
		{
			visitor.ShowString("Page Faults", "%u", pmc.PageFaultCount);
			visitor.ShowString("PageFile usage", "%llu kb", pmc.PagefileUsage / 1024);
			visitor.ShowString("Working Set Size", "%llu kb", pmc.WorkingSetSize / 1024);
			visitor.ShowString("Peak PageFile usage", "%llu kb", pmc.PeakPagefileUsage / 1024);
			visitor.ShowString("Peak Working Set Size", "%llu kb", pmc.PeakWorkingSetSize / 1024);
		}

		double pc = getCurrentValue();

		visitor.ShowString("User CPU Load", "%3.1lf%%", pc);
	}
};

namespace Rococo {
	namespace MPlatImpl {
		IMathsVenue &GetOSVenue()
		{
			static OSVenue venue;
			return venue;
		}
} }