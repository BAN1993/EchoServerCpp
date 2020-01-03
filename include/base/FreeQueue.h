#pragma once

/*=============================================
 =	无锁队列

 +	TODO
		-现在还不能用!!!!!!
		-期望传入指针,因为Pop为空时会返回0

==============================================*/

#include <atomic>

#if defined _WINDOWS_ || defined WIN32
#include <windows.h>
#define __sync_bool_compare_and_swap(ptr, oldv, newv) \
	(InterlockedCompareExchangePointer((void*volatile*)ptr,newv,oldv), (*ptr)==(newv))
#endif

#include <base/MallocManager.h>

template <typename T>
struct FreeQueueNode
{
	T data;
	FreeQueueNode<T> *next;
};

template <typename T>
class FreeQueue
{
public:
	FreeQueue()
		: _pHead(new FreeQueueNode<T>), _pTail(_pHead)
	{
		_pTail->next = nullptr;
	};

	~FreeQueue()
	{
		delete _pHead;
	};

	void Push(T data)
	{
		__new_obj(FreeQueueNode<T>, n);
		n->data = data;
		n->next = nullptr;

		FreeQueueNode<T>* p;
		// 将p挂载到末尾
		do
		{
			p = _pTail;
		} while (!__sync_bool_compare_and_swap(&p->next, nullptr, n));

		// 将p置为尾节点
		// 假设这里肯定会成功
		__sync_bool_compare_and_swap(&_pTail, p, n);
	};

	T Pop()
	{
		// 获取头节点,并赋为next
		FreeQueueNode<T>* p;
		do
		{
			p = _pHead->next;
			if (!p)
				return 0;
		} while (!__sync_bool_compare_and_swap(&_pHead->next, p, p->next));

		// 如果队列中没数据了,则将head赋给tail
		// TODO 这里应该有问题,怎么保证判空后_pTail没有被其他线程赋值
		if (!_pHead->next)
		{
			do{} while (!__sync_bool_compare_and_swap(&_pTail, p, _pHead));
		}

		T value = p->data;
		__delete_obj(FreeQueueNode<T>, p);
		return value;
	};

private:
	FreeQueueNode<T>* _pHead;
	FreeQueueNode<T>* _pTail;
};
