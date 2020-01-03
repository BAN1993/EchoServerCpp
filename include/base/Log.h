#pragma once

/*=============================================
 =	多线程日志模块

 +	NOTE
		支持按日,文件大小切文件

==============================================*/

#include <base/Base.h>
#include <tool/Uncopyable.h>
#include <tool/Singleton.h>
#include <base/Utils.h>
#include <base/Thread.h>

#include <fstream>

enum eLogLevel
{
	LOGLEVEL_DEBUG = 0,
	LOGLEVEL_INFO = 10,
	LOGLEVEL_ERROR = 20,
	LOGLEVEL_SYSTEM = 30,
};

extern unsigned int g_loglevel;

#define LOGSTREAM(level, s)              \
	if(g_loglevel <= level)              \
	{                                    \
		std::ostringstream os;           \
		os << getTimeString() << s;      \
		std::string str(os.str());       \
		GET_SINGLE(Log)->log(            \
			(const char *)str.c_str(),   \
			(unsigned int)str.length()); \
	}

/**
 * 外部调用的写日志宏
 * 会附带: [level][file:line][function][threadid]...
 */
#define LOG(os)    LOGSTREAM(LOGLEVEL_DEBUG,  "[Debug][" <<__FILE__<<":"<<__LINE__<<"]["<<__FUNCTION__<<"()]["<<gettid()<<"]"<<os<<"\n")
#define LOGD(os)   LOGSTREAM(LOGLEVEL_DEBUG,  "[Debug][" <<__FILE__<<":"<<__LINE__<<"]["<<__FUNCTION__<<"()]["<<gettid()<<"]"<<os<<"\n")
#define LOGI(os)   LOGSTREAM(LOGLEVEL_INFO,   "[Info]["  <<__FILE__<<":"<<__LINE__<<"]["<<__FUNCTION__<<"()]["<<gettid()<<"]"<<os<<"\n")
#define LOGE(os)   LOGSTREAM(LOGLEVEL_ERROR,  "[Error][" <<__FILE__<<":"<<__LINE__<<"]["<<__FUNCTION__<<"()]["<<gettid()<<"]"<<os<<"\n")
#define LOGSYS(os) LOGSTREAM(LOGLEVEL_SYSTEM, "[System]["<<__FILE__<<":"<<__LINE__<<"]["<<__FUNCTION__<<"()]["<<gettid()<<"]"<<os<<"\n")

class Log : public Thread
{
	DEFINE_UNCOPYABLE(Log);
	DEFINE_SINGLE(Log);

public:
	const static int MAX_LOG_BUFFER_SIZE = 614400;

public:
	void setConfig(std::string filename, unsigned int filesize, unsigned int loglevel, bool screenout, bool fileout);
	void setScreenOut(bool flag);
	bool doStart();
	void log(const char* buf, unsigned int len);

private:
	Log();
	int run();

	/**
	* 尝试打开文件
	* 如果日期变更或者大小超限都会重新打开一个文件
	* 返回false就不要写了
	*/
	bool openFile();

	/**
	* 写日志
	* 将缓存的内容写入文件和屏幕
	*/
	bool logFile();

private:
	inline int length() { return int(_pLogCur - _data); }

private:
	bool _bRuning;
	std::string _sFilename;
	std::vector<std::string> _vecPathList;	// 路劲列表,因为要生成的日志文件可能不在当前目录下
	unsigned int _nFileSize;				// 日志文件大小限制
	bool _bStdOut;							// 是否输出屏幕
	bool _bFileOut;							// 是否输出文件
	unsigned int _nNowDay;					// 当前日期
	unsigned int _nLastDay;					// 上次日期

	std::fstream _fp;
	Lock _bufLock;
	char _data[MAX_LOG_BUFFER_SIZE];
	char* _pLogCur;
	char _tmpData[MAX_LOG_BUFFER_SIZE];

	unsigned int _nNowUseSize;			// 当前日志文件大小
	unsigned long long _lDropCount;		// 被丢弃的日志数量
};

/**
 * 初始化日志模块并开启线程
 * filename:日志路劲(无则视为当前目录)+文件名,会自动创建不存在的文件夹或文件
 * filesize:日志文件大小(K)上限,超过上限则新建文件
 * loglevel:日志等级
 * screenout:是否打印屏幕
 */
inline void initLog(std::string filename, unsigned int filesize, unsigned int loglevel, bool screenout, bool fileout = true)
{ 
	GET_SINGLE(Log)->setConfig(filename, filesize, loglevel, screenout, fileout);
	assert(GET_SINGLE(Log)->doStart());
};
