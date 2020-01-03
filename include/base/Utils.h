#pragma once

/*=============================================
 =	一些常用的公共接口

 +	NOTE
		一些函数要保证线程安全,调用可重入系统函数

==============================================*/

#include <base/Base.h>

#define ACCESS_READ		4 // 是否可读
#define ACCESS_WRITE	2 // 是否可写
#define ACCESS_EXECUTE	1 // 是否可执行
#define ACCESS_EXIST	0 // 是否存在

/**
 * 开启守护进程
 * 仅在linux有效
 */
void daemon();

/**
 * 初始化网络
 * 仅在Windows有效
 * 返回0成功,非0失败
 */
int initNet();

/**
 * 获取随机数[min,max]
 */
int getRand(int rangeMin, int rangeMax);

/**
 * 将内存转换成16进制字符串
 */
std::string memToString(const void *mem, int size);

/**
 * 类似于localtime_r
 * 为了线程安全,自己实现了
 */
int time_to_tm(time_t *time_input, struct tm* tm_result);
int tm_to_time(struct tm* tm_input, time_t *time_result);

/**
 * 获取当前时间戳（毫秒）
 */
unsigned long long getTimeStampMsec();

/**
 * 获取当前时间字符串: "[2019-01-01]"
 */
std::string getTimeString();

/**
 * 获取今天日期: 20190101
 */
unsigned int getNowDay();

/**
 * 获取当前时间: 235959
 */
unsigned int getNowTime();
std::string getNowTimeStr();

/**
 * 睡眠函数（毫秒）
 */
void sleepMillisecond(unsigned long msec);

/**
 * 获取程序绝对路径
 */
std::string getModulePath();

/**
 * 判断路劲是否存在
 */
bool pathExist(const char * path);

/**
 * 创建路劲
 */
bool createPath(const char * path);

/**
 * 去掉字符串所有的特定字符
 */
void trimString(std::string& str, const unsigned char c);

/**
 * 去掉匹配字符后面所有的内容
 * 为了支持ini配置字段后面加#注释
 */
void trimTailAllString(std::string& str, const unsigned char c);

/**
 * 判断字符串是否以子串为开头
 * 比如:
 *		isStringBeginOf("ABC:123123", "ABC") => return true
 */
bool isStringBeginOf(const std::string& str, const std::string s);

/**
 * 去掉字符串头\尾的特定字符,没有则不变
 */
void trimHeadString(std::string& str, const unsigned char c);
void trimTailString(std::string& str, const unsigned char c);

/**
 * 替换字符串中所有的from,替换成to
 */
void replaceAllChar(std::string& str,
	const std::string from,
	const std::string to);

/**
 * 分割字符串,c可一个是一个子串
 * index: 0分割全部, >0分割前N个
 */
void splitString(const std::string& str,
	std::vector<std::string>& list, const
	std::string& c,
	const unsigned int index = 0);

/**
 * 字符串转为大写/小写
 */
void stringUp(std::string& str);
void stringLow(std::string& str);
