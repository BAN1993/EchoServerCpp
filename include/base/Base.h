#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sstream>
#include <time.h>
#include <algorithm>

#include <map>
#include <vector>
#include <unordered_map>
#include <list>
#include <queue>

#if defined _WINDOWS_ || defined WIN32

	#pragma warning(disable: 4996)

	// 要把 <windows.h> 放在 <Winsock2.h> 后面,否则会报
	// socket.error C2011: “sockaddr”: “struct”类型重定义
	// 本来应该放在 <SocketBase.h> 头文件里的
	#include <Winsock2.h>
	#include <windows.h>
	#include <process.h>
	#include <io.h>
	#include <direct.h>
	#include <Psapi.h>
	
	inline unsigned int gettid()
	{
		return GetCurrentThreadId();
	}

#else

	#include <unistd.h>
	#include <sys/time.h>
	#include <sys/syscall.h>
	#include <sys/stat.h>
	#include <signal.h>
	#include <pthread.h>
	
	#define gettid() syscall(__NR_gettid)

	#define byte unsigned char

#endif

// 路劲分隔符
#if defined _WINDOWS_ || defined WIN32
#define PF "\\"
#else
#define PF "/"
#endif

enum eServerType
{
	unknown=0,
	client,
	webServer,
	datbaseServer,
	gameServer,
};