#include <base/Base.h>
#include <base/Utils.h>
#include <base/MallocManager.h>

void daemon()
{
#if defined _WINDOWS_ || defined WIN32
#else
	int pid;
	if( (pid=fork())>0 )	exit(0);
	else if( pid<0 )		exit(-1);

	signal(SIGHUP,		SIG_IGN );	// SIGHUP (挂起) 当运行进程的用户注销时通知该进程，使进程终止
	signal(SIGALRM,		SIG_IGN );	// SIGALRM (超时) alarm函数使用该信号，时钟定时器超时响应
	//signal(SIGTERM,	SIG_IGN );	// SIGTERM 程序结束(terminate)信号, 与SIGKILL不同的是该信号可以被阻塞和处理. 
									// 通常用来要求程序自己正常退出. shell命令kill缺省产生这个信号. 
	signal(SIGPIPE,		SIG_IGN );	// SIGPIPE 写至无读进程的管道, 或者Socket通信SOCT_STREAM的读进程已经终止，而再写入。
	signal(SIGXCPU,		SIG_IGN );	// SIGXCPU 超过CPU时间资源限制. 这个限制可以由getrlimit/setrlimit来读取/改变
	signal(SIGXFSZ,		SIG_IGN );	// SIGXFSZ 当进程企图扩大文件以至于超过文件大小资源限制
	signal(SIGPROF,		SIG_IGN );	// SIGPROF (梗概时间超时) setitimer(2)函数设置的梗概统计间隔计时器(profiling interval timer)
	//signal(SIGUSR1,	SIG_IGN );
	//signal(SIGUSR2,	SIG_IGN );
	signal(SIGVTALRM,	SIG_IGN );	// SIGVTALRM 虚拟时钟信号. 类似于SIGALRM, 但是计算的是该进程占用的CPU时间
	signal(SIGQUIT,		SIG_IGN );	// SIGQUIT (退出) 用户按下或时通知进程，使进程终止
	signal(SIGINT,		SIG_IGN );	// SIGINT (中断) 当用户按下时,通知前台进程组终止进程
	//signal(SIGPOLL,	SIG_IGN );
	//signal(SIGSYS,	SIG_IGN );	// SIGSYS 非法的系统调用
#endif
}

int initNet()
{
#if defined _WINDOWS_ || defined WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD(1, 1);

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err < 0)
		return err;

	if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1) {
		WSACleanup();
		return -1;
	}
#endif

	return 0;
}

int getRand(int rangeMin, int rangeMax)
{
	return rand() % (rangeMax - rangeMin + 1) + rangeMin;
}

std::string memToString(const void *mem, int size)
{
	//int newsize = size * 4;
	//char *tmp = __new(newsize+1);

	unsigned char *memByte = (unsigned char*)mem;
	char temp[4] = { 0 };
	std::string s;

	for (int i = 0, j = 0; i < size; ++i, ++j)
	{
		unsigned char ch = memByte[i];
		sprintf(temp, "%02x ", ch);
		s += temp;

		//if (ch >= 32 && ch <= 125)
		//	tmp[j] = ch;
		//else
		//{
		//	j += (sprintf(tmp + j, "%02x ", ch) - 1);
		//}
	}

	//std::string s(tmp);
	//__delete(tmp);
	return s;
}

int time_to_tm(time_t *time_input, struct tm* tm_result)
{
	static const char month_days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	static const bool leap_year[4] = { false, false, true, false };

	unsigned int leave_for_fouryear = 0;
	unsigned short four_year_count = 0;
	unsigned int temp_value = 0;

	tm_result->tm_sec = *time_input % 60;
	temp_value = (unsigned int)*time_input / 60;// 分钟
	tm_result->tm_min = temp_value % 60;
	temp_value /= 60; // 小时

	temp_value += 8;// 加上时区

	tm_result->tm_hour = temp_value % 24;
	temp_value /= 24; // 天

	tm_result->tm_wday = (temp_value + 4) % 7;// 1970-1-1是4

	four_year_count = temp_value / (365 * 4 + 1);
	leave_for_fouryear = temp_value % (365 * 4 + 1);
	int leave_for_year_days = leave_for_fouryear;

	int day_count = 0;
	int i = 0;

	for (i = 0; i < 4; i++)
	{
		day_count = leap_year[i] ? 366 : 365;

		if (leave_for_year_days <= day_count)
		{
			break;
		}
		else
		{
			leave_for_year_days -= day_count;
		}
	}

	tm_result->tm_year = four_year_count * 4 + i + 70;
	tm_result->tm_yday = leave_for_year_days;// 这里不是天数，而是标记，从0开始

	int leave_for_month_days = leave_for_year_days;

	int j = 0;
	for (j = 0; j < 12; j++)
	{
		if (leap_year[i] && j == 1)
		{
			if (leave_for_month_days < 29)
			{
				break;
			}
			else if (leave_for_month_days == 29)
			{
				j++;
				leave_for_month_days = 0;
				break;
			}
			else
			{
				leave_for_month_days -= 29;
			}

			continue;
		}

		if (leave_for_month_days < month_days[j])
		{
			break;
		}
		else if (leave_for_month_days == month_days[j]){
			j++;
			leave_for_month_days = 0;
			break;
		}
		else
		{
			leave_for_month_days -= month_days[j];
		}
	}

	tm_result->tm_mday = leave_for_month_days + 1;
	tm_result->tm_mon = j;
	if (tm_result->tm_mon >= 12)
	{
		tm_result->tm_year++;
		tm_result->tm_mon -= 12;
	}
	tm_result->tm_isdst = -1;

	return 0;
}
int tm_to_time(struct tm* tm_input, time_t *time_result)
{
	static short monthlen[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	static short monthbegin[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

	time_t t;

	t = monthbegin[tm_input->tm_mon]
		+ tm_input->tm_mday - 1
		+ (!(tm_input->tm_year & 3) && tm_input->tm_mon > 1);

	tm_input->tm_yday = (int)t;
	t += 365 * (tm_input->tm_year - 70)
		+ (tm_input->tm_year - 69) / 4;

	tm_input->tm_wday = (t + 4) % 7;

	t = t * 86400 + (tm_input->tm_hour - 8) * 3600 + tm_input->tm_min * 60 + tm_input->tm_sec;

	if (tm_input->tm_mday > monthlen[tm_input->tm_mon] + (!(tm_input->tm_year & 3) && tm_input->tm_mon == 1))
	{
		*time_result = mktime(tm_input);
	}
	else
	{
		*time_result = t;
	}

	return 0;
}

unsigned long long getTimeStampMsec()
{
	unsigned long long ret_time = 0;
#if defined _WINDOWS_ || defined WIN32

	struct tm	tm;
	time_t		now;
	SYSTEMTIME	wtm;

	GetLocalTime(&wtm);
	tm.tm_year = wtm.wYear - 1900;
	tm.tm_mon = wtm.wMonth - 1;
	tm.tm_mday = wtm.wDay;
	tm.tm_hour = wtm.wHour;
	tm.tm_min = wtm.wMinute;
	tm.tm_sec = wtm.wSecond;
	tm.tm_isdst = -1;

	tm_to_time(&tm, &now);

	ret_time = (unsigned long long)now*1000UL + wtm.wMilliseconds;
#else
	struct timeval now;
	gettimeofday( &now , 0 );
	ret_time = now.tv_sec;
	ret_time = ret_time * 1000000;
	ret_time += now.tv_usec;

	ret_time = ret_time / 1000ULL;
#endif

	return ret_time;
}

std::string getTimeString()
{
	time_t now = time(nullptr);
	struct tm t;
	time_to_tm(&now, &t);
	
	char curtime[256] = {0};
	sprintf( curtime, "[%02d:%02d:%02d]", t.tm_hour, t.tm_min, t.tm_sec);
	std::string s = std::string( curtime );
	return s;
}

unsigned int getNowDay()
{
	time_t now = time(nullptr);
	struct tm t;
	time_to_tm(&now, &t);
	return (t.tm_year+1900) * 10000 + t.tm_mon * 100 + t.tm_mday;
}

unsigned int getNowTime()
{
	time_t now = time(nullptr);
	struct tm t;
	time_to_tm(&now, &t);
	return t.tm_hour * 10000 + t.tm_min * 100 + t.tm_sec;
}

std::string getNowTimeStr()
{
	time_t now = time(nullptr);
	struct tm t;
	time_to_tm(&now, &t);
	char str[16] = { 0 };
	sprintf(str, "%02d%02d%02d", t.tm_hour, t.tm_min, t.tm_sec);
	return str;
}

void sleepMillisecond(unsigned long msec)
{
#if defined _WINDOWS_ || defined WIN32
	::Sleep(msec);
#else
	::usleep(msec*1000);
#endif
}

std::string getModulePath()
{
#if defined _WINDOWS_ || defined WIN32

	char szFile[MAX_PATH];
	GetModuleFileNameA(GetModuleHandle(nullptr), szFile, MAX_PATH);

	char szDrive[MAX_PATH], szPath[MAX_PATH];

	_splitpath(szFile, szDrive, szPath, nullptr, nullptr);
	strncat(szDrive, szPath, sizeof(szDrive)-strlen(szDrive)-1);

	std::string ret = szDrive;
	if (ret.find_last_of(PF) != ret.length() - 1 && ret.find_first_of('/') != ret.length() - 1)
	{
		ret.append(PF);
	}

	return ret;

#else

	char dir[1024] = {0};
	int count = readlink( "/proc/self/exe", dir, 1024 );
	if ( count < 0 || count >= 1024 )
		return "./";
	else
		dir[count] = 0;

	std::string ret = dir;

	// 去掉路径后面的程序名
	std::string::size_type pos = ret.find_last_of(PF);
	if( pos == std::string::npos ) return ret;
	return ret.substr(0, pos+1);

#endif
}

bool pathExist(const char * path)
{
	bool ret = false;
#if defined _WINDOWS_ || defined WIN32
	ret =  _access(path, ACCESS_EXIST) == 0 ? true : false;
#else
	ret = access(path, ACCESS_EXIST) == 0 ? true : false;
#endif
	return ret;
}

bool createPath(const char * path)
{
#if defined _WINDOWS_ || defined WIN32
	if (pathExist(path)) return true;
	return (_mkdir(path) == TRUE);
#else
	mkdir(path, 0755);
	return true;
#endif
}

void trimString(std::string& str, const unsigned char c)
{
	int index = 0;
	if( !str.empty())
	{
		while( (index = (int)str.find(c,index)) != (int)std::string::npos)
		{
			str.erase(index,1);
		}
	}
}

void trimTailAllString(std::string& str, const unsigned char c)
{
	if(str.empty())
		return;
	std::string::size_type pos = str.find(c);
	if(pos != std::string::npos)
		str.erase(pos, str.size()-pos);
}

bool isStringBeginOf(const std::string& str, const std::string s)
{
	if (str.empty() || s.empty())
		return false;
	std::string::size_type pos = str.find(s);
	return pos == 0 ? true : false;
}

void trimHeadString(std::string& str, const unsigned char c)
{
	if(str.empty())
		return;
	str.erase(0,str.find_first_not_of(c));
}

void trimTailString(std::string& str, const unsigned char c)
{
	if(str.empty())
		return;
	str.erase(str.find_last_not_of(c) + 1);
}

void replaceAllChar(std::string& str,
	const std::string from, 
	const std::string to)
{
	if (from == to)
		return;
	if (str.empty())
		return;
	std::string::size_type index = str.find(from);
	while (std::string::npos != index)
	{
		str = str.replace(index, from.length(), to);
		index = str.find(from, index+from.length());
	}
}

void splitString(const std::string& str,
	std::vector<std::string>& list,
	const std::string& c,
	const unsigned int index)
{
	if(str.empty())
		return;
	std::string::size_type left,right;
	left = 0;
	right = str.find(c);
	unsigned int ct = 0;
	while(std::string::npos != right)
	{
		list.push_back(str.substr(left, right-left));
		left = right+1;
		right = str.find(c, left);
		ct++;
		if (index != 0 && ct >= index)
			break;
	}
	if(left != str.length())
		list.push_back(str.substr(left));
}

void stringUp(std::string& str)
{
	std::transform(str.begin(),str.end(),str.begin(),::toupper);
}

void stringLow(std::string& str)
{
	std::transform(str.begin(),str.end(),str.begin(),::tolower);
}
