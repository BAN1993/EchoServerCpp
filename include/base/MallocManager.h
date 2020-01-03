#pragma once

/*=============================================
=	多线程内存池管理类

+	NOTE
		1.可以开启 _TEST_MEM_ 定义，检测哪里申请的内存没有释放
		  需要程序正常退出
		  未做优化，严重影响性能，谨慎开启

==============================================*/

#include <base/Base.h>
#include <tool/Uncopyable.h>
#include <tool/Singleton.h>
#include <base/MallocPool.h>
#include <base/Lock.h>

//#define _TEST_MEM_ // or define in MakeFile

#if defined _TEST_MEM_
#define __new(size) GET_SINGLE(mallocManager)->malloc(size, __FILE__, __LINE__)
#define __delete(p) GET_SINGLE(mallocManager)->free((char*)(p), __FILE__, __LINE__)
#else
#define __new(size) GET_SINGLE(mallocManager)->malloc(size)
#define __delete(p) GET_SINGLE(mallocManager)->free((char*)(p))
#endif


#define __new_obj(classname, p)						classname* (p) = (classname*)__new(sizeof(classname));new((p)) classname()
#define __new_obj_1(classname, p, v1)				classname* (p) = (classname*)__new(sizeof(classname));new((p)) classname((v1))
#define __new_obj_2(classname, p, v1, v2)			classname* (p) = (classname*)__new(sizeof(classname));new((p)) classname((v1), (v2))
#define __new_obj_3(classname, p, v1, v2, v3)		classname* (p) = (classname*)__new(sizeof(classname));new((p)) classname((v1), (v2), (v3))
#define __new_obj_4(classname, p, v1, v2, v3, v4)	classname* (p) = (classname*)__new(sizeof(classname));new((p)) classname((v1), (v2), (v3), (v4))
#define __delete_obj(classname, p)					{(p)->~classname();__delete((p));}

#define __new_copy_char(str, p)				\
	char* p = __new(str.length()+1);		\
	memcpy(p, str.c_str(), str.length());	\
	p[str.length()] = '\0'

#define __new_copy_int(num, p)		\
	char* p = __new(sizeof(int));	\
	*(int*)p = num

#define __new_copy_double(num, p)	\
	char* p = __new(sizeof(double));\
	*(double*)p = num

static const unsigned int mem_max_size = 1024 * 10;

struct Recode
{
	std::string filename;
	int line;
};

typedef unsigned long long ptrlen;
typedef std::unordered_map<unsigned int, mallocPool*>		SIZE_POOL_MAP;
typedef std::unordered_map<char*, mallocPool*>				PTR_POOL_UNMAP;
typedef std::unordered_map<char*, unsigned int>				PTR_SIZE_MAP;
typedef std::unordered_map<char*, Recode>					PTR_RECODE_MAP;

class mallocManager : public unCopyable
{
	DEFINE_SINGLE(mallocManager);

private:
	mallocManager(){};
	~mallocManager();

public:
	// 失败返回空
	char* malloc(unsigned int size, const char* filename, unsigned int line);
	char* malloc(unsigned int size);
	bool free(char* ptr);
	bool free(char* ptr, const char* filename, unsigned int line);
	bool avail(void* ptr); // 判断内存是否有效(必须是我申请的内存)
	void printMemRecode(); // 打印所有申请的内存

private:
	unsigned int fixSize(unsigned int fsize);		// 调整内存大小为整数
	unsigned int getCount(unsigned int fixSize);	// 根据内存大小调整连续分配几块内存

private:
	SIZE_POOL_MAP	_small_list;		// 小内存大小映射表 <size, pool>
	PTR_POOL_UNMAP	_small_ptr_list;	// 小内存指针映射表 <char*, pool>
	PTR_SIZE_MAP	_big_ptr_list;		// 大内指针存映射表 <char*, size>
	PTR_RECODE_MAP	_ptr_recode;		// TEST 记录哪行申请的内存
	Lock _lock;
};
