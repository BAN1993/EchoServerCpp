#pragma once

/*=============================================
=	内存池

+	NOTE
		1.非线程安全
		2.一般由上层(mallocManager)管理
		  和保证线程安全

==============================================*/

#include <base/Base.h>
#include <tool/Uncopyable.h>

class mallocPool : public unCopyable
{
public:
	/**
	 * 创建一个内存池
	 * size:每块内存大小
	 * count:每次拆分成多少块内存
	 *       因为每次总大小是一样的
	 */
	mallocPool(unsigned int size, unsigned int count);
	~mallocPool();

public:
	// 失败返回空
	char* malloc();
	bool free(char* ptr);

private:
	bool doExpand();

private:
	unsigned int _size;						// 每块内存大小
	unsigned int _count;					// 每次扩展几块
	std::list<char*> _all_mem_list;			// 整块内存列表
	std::list<char*> _free_list;			// 空闲内存列表

private:
	unsigned long long _history_count;		// 一共分配了多少块内存
	unsigned long long _history_get;		// 一共获取了几次
	unsigned long long _history_free;		// 一共释放了几次
};
