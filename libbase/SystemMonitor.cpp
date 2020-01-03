#include <base/Utils.h>
#include <base/Log.h>
#include <base/SystemMonitor.h>

#include <iomanip>
#include<iostream>


INIT_SINGLE(SystemMonitor);

SystemMonitor::SystemMonitor()
{
	initSysInfo();
	initCpuInfo();
	initMemoryInfo();
}

SystemMonitor::~SystemMonitor()
{

}

void SystemMonitor::printAllInfo()
{
	LOGI("\n" <<
		"====================================" << "\n" <<
		"=== System information ===" << "\n" <<
		"= Name: " << _sysInfo.name << "\n" <<
		"= Version: " << _sysInfo.version << "\n" <<
		"=== Cpu information ===" << "\n" <<
		"= ProcessNum: " << _cpuInfo.processNum << "\n" <<
		"= Frequency: " << _cpuInfo.frequency << " MHz" << "\n" <<
		"= Manufacture: " << _cpuInfo.manufacture << "\n" <<
		"= Type: " << _cpuInfo.type << "\n" <<
		"=== Memory infomation ===" << "\n" <<
		"= Total: " << _memoryInfo.total << " GB" << "\n" <<
		"= Available: " << _memoryInfo.avail << " GB" << "\n" <<
		"===================================="
		);
}

unsigned int SystemMonitor::getMemory()
{
	unsigned int mem = 0;

#if defined _WINDOWS_ || defined WIN32

	HANDLE handle = GetCurrentProcess();
	PROCESS_MEMORY_COUNTERS pmc;
	GetProcessMemoryInfo(handle, &pmc, sizeof(pmc));
	mem = (unsigned int)(pmc.PeakWorkingSetSize / DKBYTES);

#else

	std::vector<std::string> lines;
	int pid = getpid();
	char file[64] = { 0 };
	sprintf(file, "/proc/%d/status", pid);

	if(!readFile(file, lines))
		return 0;

	std::string tmp = "";
	for(unsigned int i = 0; i < lines.size(); i++)
	{
		if(isStringBeginOf(lines[i], "VmRSS:"))
		{
			tmp = lines[i];
			break;
		}
	}

	if (!tmp.empty())
	{
		trimString(tmp, ' ');
		trimString(tmp, '\n');
		trimString(tmp, '\r');
		trimString(tmp, '\t');
		replaceAllChar(tmp, "VmRSS:", "");

		mem = atoi(tmp.c_str());
	}
	
#endif
	return mem;
}

float SystemMonitor::getCpuLoad()
{
	return 0.0;
}

void SystemMonitor::initSysInfo()
{

	_sysInfo.name = "unknown";
	_sysInfo.version = "unknown";

#if defined _WINDOWS_ || defined WIN32

	OSVERSIONINFO osver = { sizeof(OSVERSIONINFO) };
	GetVersionEx(&osver);
	_sysInfo.version = std::to_string(osver.dwMajorVersion) + "." + std::to_string(osver.dwMinorVersion);

	if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 0)
		_sysInfo.name = "Windows 2000";
	else if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 1)
		_sysInfo.name = "Windows XP";
	else if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 0)
		_sysInfo.name = "Windows 2003";
	else if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 2)
		_sysInfo.name = "windows vista";
	else if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 1)
		_sysInfo.name = "windows 7";
	else if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 2)
		_sysInfo.name = "windows 10";
	else
		_sysInfo.name = "windows unknown";
	
#else

	_sysInfo.name = "Linux";

	std::vector<std::string> lines;
	if(!readFile("/proc/version", lines))
		return;

	std::string tmp = "";
	for(unsigned int i = 0; i < lines.size(); i++)
	{
		tmp += lines[i];
	}
	trimString(tmp, '\n');
	trimString(tmp, '\r');
	trimString(tmp, '\t');
	_sysInfo.version = tmp;

#endif
}

void SystemMonitor::initCpuInfo()
{
	_cpuInfo.processNum = 0;
	_cpuInfo.frequency = "unknown";
	_cpuInfo.manufacture = "unknown";
	_cpuInfo.type = "unknown";

#if defined WIN32

	SYSTEM_INFO si;
	memset(&si, 0, sizeof(SYSTEM_INFO));
	GetNativeSystemInfo(&si);
	_cpuInfo.processNum = si.dwNumberOfProcessors;

	// 我怀疑这里有问题, 获取到的type竟然是 AMD X86 64 Processor
	// 但机子实际是intel的
	switch (si.dwProcessorType)
	{
	case PROCESSOR_INTEL_386:
		_cpuInfo.type = "Intel 386 processor";
		break;
	case PROCESSOR_INTEL_486:
		_cpuInfo.type = "Intel 486 Processor";
		break;
	case PROCESSOR_INTEL_PENTIUM:
		_cpuInfo.type = "Intel Pentium Processor";
		break;
	case PROCESSOR_INTEL_IA64:
		_cpuInfo.type = "Intel IA64 Processor";
		break;
	case PROCESSOR_AMD_X8664:
		_cpuInfo.type = "AMD X86 64 Processor";
		break;
	default:
		_cpuInfo.type = "unknown";
		break;
	}
	
#else

	std::vector<std::string> lines;
	if (!readFile("/proc/cpuinfo", lines))
		return;

	for(unsigned int i = 0; i < lines.size(); i++)
	{
		if (isStringBeginOf(lines[i], "processor"))
		{
			_cpuInfo.processNum++;
		}
		else if (isStringBeginOf(lines[i], "cpu MHz"))
		{
			_cpuInfo.frequency = lines[i];
		}
		else if (isStringBeginOf(lines[i], "vendor_id"))
		{
			_cpuInfo.manufacture = lines[i];
		}
		else if (isStringBeginOf(lines[i], "model name"))
		{
			_cpuInfo.type = lines[i];
		}
	}

	trimString(_cpuInfo.frequency, '\t');
	trimString(_cpuInfo.frequency, '\n');
	trimString(_cpuInfo.frequency, '\r');
	replaceAllChar(_cpuInfo.frequency, "cpu MHz:", "");
	trimHeadString(_cpuInfo.frequency, ' ');
	
	trimString(_cpuInfo.manufacture, '\t');
	trimString(_cpuInfo.manufacture, '\n');
	trimString(_cpuInfo.manufacture, '\r');
	replaceAllChar(_cpuInfo.manufacture, "vendor_id:", "");
	trimHeadString(_cpuInfo.manufacture, ' ');
	
	trimString(_cpuInfo.type, '\t');
	trimString(_cpuInfo.type, '\n');
	trimString(_cpuInfo.type, '\r');
	replaceAllChar(_cpuInfo.type, "model name:", "");
	trimHeadString(_cpuInfo.type, ' ');
	
#endif
}

void SystemMonitor::initMemoryInfo()
{
	_memoryInfo.total = 0;
	_memoryInfo.avail = 0;

#if defined _WINDOWS_ || defined WIN32

	MEMORYSTATUSEX statusex;  
	statusex.dwLength = sizeof(statusex);  
	if (GlobalMemoryStatusEx(&statusex))
	{
		unsigned long long total = 0, remain_total = 0, avl = 0, remain_avl = 0;
		double decimal_total = 0, decimal_avl = 0;
		remain_total = statusex.ullTotalPhys % GBYTES;
		total = statusex.ullTotalPhys / GBYTES;
		avl = statusex.ullAvailPhys / GBYTES;
		remain_avl = statusex.ullAvailPhys % GBYTES;
		if (remain_total > 0)
			decimal_total = (remain_total / MBYTES) / DKBYTES;
		if (remain_avl > 0)
			decimal_avl = (remain_avl / MBYTES) / DKBYTES;

		decimal_total += (double)total;
		decimal_avl += (double)avl;

		_memoryInfo.total = (float)decimal_total;
		_memoryInfo.avail = (float)decimal_avl;
	}

#else

	std::vector<std::string> lines;
	if (!readFile("/proc/meminfo", lines))
		return;

	for(unsigned int i = 0; i < lines.size(); i++)
	{
		std::string tmp = lines[i];
		trimString(tmp, ' ');
		replaceAllChar(tmp, "kb", "");

		if (isStringBeginOf(tmp, "MemTotal:"))
		{
			replaceAllChar(tmp, "MemTotal:", "");
			long total = atol(tmp.c_str());
			_memoryInfo.total = total / DKBYTES / DKBYTES;
		}
		else if (isStringBeginOf(tmp, "MemFree:"))
		{
			replaceAllChar(tmp, "MemFree:", "");
			long avail = atol(tmp.c_str());
			_memoryInfo.avail = avail / DKBYTES / DKBYTES;
		}
	}
	
#endif
}

bool SystemMonitor::readFile(std::string filename, std::vector<std::string>& lines)
{
	bool ret = false;
	lines.clear();

#if defined _WINDOWS_ || defined WIN32

		// nothing to do

#else

	FILE* fp = fopen(filename.c_str(), "r");
	if (!fp) return false;

	char getstr[1024] = { 0 };
	while (!feof(fp))
	{
		memset(getstr, 0, sizeof(getstr));
		fgets(getstr, sizeof(getstr) - 1, fp);
		lines.push_back(getstr);
	}

	fclose(fp);
	ret = true;

#endif
	return ret;
}
