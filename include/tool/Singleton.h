#pragma once

/*=============================================
 =	单例

==============================================*/

/**
 * 定义单例
 */
#define DEFINE_SINGLE(classname)          \
	private:                              \
		static classname* _instance;      \
	public:                               \
		static classname* getInstance()   \
		{                                 \
			return _instance;             \
		}

/**
 * 初始化单例
 * 为了线程安全,初始化放在函数外
 */
#define INIT_SINGLE(classname) \
	classname* classname::_instance = new classname();

/**
 * 获取单例
 */
#define GET_SINGLE(classname) \
	classname::getInstance()

