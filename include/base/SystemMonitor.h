#pragma once

/*=============================================
 =	系统监控类

 + NOTE
		目前先做成非线程安全,因为只有在主线程才会调用

 + TODO
		1.在win下一些info可能获取不全
		2.暂不提供获取cpu load的接口
		   一般获取cpu要另开一个线程去计算, 等以后需要再做吧


==============================================*/

#include <base/Base.h>
#include <tool/Uncopyable.h>
#include <tool/Singleton.h>

#define GBYTES  1073741824
#define MBYTES  1048576
#define KBYTES  1024
#define DKBYTES 1024.0

struct SysInfo
{
	std::string name; // 系统类型
	std::string version; // 系统版本
};

struct CpuInfo
{
	unsigned int processNum; // 核数
	std::string frequency; // 主频
	std::string manufacture; // 制造商
	std::string type; // 类型
};

struct MemoryInfo
{
	float total; // 总内存(MB)
	float avail; // 可用内存(MB)
};

class SystemMonitor : public unCopyable
{
	DEFINE_SINGLE(SystemMonitor);

public:
	SystemMonitor();
	~SystemMonitor();

public:
	/**
	 * 获取一些软硬件信息
	 */
	inline SysInfo& getSysInfo() { return _sysInfo; };
	inline CpuInfo& getCpuInfo() { return _cpuInfo; };
	inline MemoryInfo& getMemoryInfo() { return _memoryInfo; };

	/**
	 * 打印硬件信息
	 */
	void printAllInfo();

	/**
	 * 获取当前占用内存(KB)
	 */
	unsigned int getMemory();

	/**
	 * 获取cpu load
	 * 现在是无效的接口
	 */
	float getCpuLoad();

private:
	void initSysInfo();
	void initCpuInfo();
	void initMemoryInfo();
	bool readFile(std::string filename, std::vector<std::string>& lines);

private:
	SysInfo _sysInfo;
	CpuInfo _cpuInfo;
	MemoryInfo _memoryInfo;
};
