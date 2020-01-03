#pragma once

/*=============================================
 =	不可拷贝

 +	NOTE
		一般情况下继承unCopyable即可
		但是若遇到多重继承的情况
		请使用DEFINE_UNCOPYABLE宏定义
		为了统一风格,全部使用宏也可以

==============================================*/

class unCopyable
{
protected:
	unCopyable(){};
	~unCopyable(){};
private:
	unCopyable(const unCopyable&);
	unCopyable& operator= (const unCopyable&);
};

/**
 * 不可拷贝
 * 若有多重继承可考虑使用此宏
 */
#define DEFINE_UNCOPYABLE(classname)             \
	private:                                     \
		classname(const classname&);             \
		classname& operator= (const classname&);
