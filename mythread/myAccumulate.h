#include <list>
#include <mutex>
#include <algorithm>
#include "threadsafe_queue.h"

#pragma once
template<typename Iterator, typename T>
struct accumulate_block
{
	void operator()(Iterator first, Iterator last, T& result)
	{
		result = std::accumulate(first, last, result);
	}
};
template<typename Iterator, typename T>
T parallel_accumulate(Iterator first, Iterator last, T init)
{
	unsigned long const length = std::distance(first, last);
	if (!length) // 1 如果输入的范围为空①，就会得到init的值
		return init;
	unsigned long const min_per_thread = 25;
	unsigned long const max_threads =
		(length + min_per_thread - 1) / min_per_thread; // 2 范围内元素的总数量除以线程(块)中最小任务数，从而确定启动线程的最大数量
	unsigned long const hardware_threads =
		std::thread::hardware_concurrency();
	unsigned long const num_threads = // 3 计算量的最大值和硬件支持线程数中，较小的值为启动线程的数量
		std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
	unsigned long const block_size = length / num_threads; // 4 每个线程处理的元素数量，是范围中元素的总量除以线程的个数得出的
	std::vector<T> results(num_threads);
	std::vector<std::thread> threads(num_threads - 1); // 5 创建了一个线程容器,启动的线程要-1，因为已经有了一个主线程
	Iterator block_start = first;
	for (unsigned long i = 0; i < (num_threads - 1); ++i)
	{
		Iterator block_end = block_start;
		std::advance(block_end, block_size); // 6 block_end指向当前块的末尾
		threads[i] = std::thread( // 7 启动一个新线程，并为当前块累加结果
			accumulate_block<Iterator, T>(),
			block_start, block_end, std::ref(
				[i]));
		block_start = block_end; // 8 启动下一个块
	}
	accumulate_block<Iterator, T>()(
		block_start, last, results[num_threads - 1]); // 9 处理最终块的结果
	std::for_each(threads.begin(), threads.end(),
		std::mem_fn(&std::thread::join)); // 10 等待线程创建完成
	return std::accumulate(results.begin(), results.end(), init); // 11 将结果累加
}

class some_data
{
	int a;
	std::string b;
public:
	void do_something();
};
class data_wrapper
{
private:
	some_data data;
	std::mutex m;
public:
	template<typename Function>
	void process_data(Function func)
	{
		std::lock_guard<std::mutex> l(m);
		func(data); // 1 传递“保护”数据给用户函数
	}
};
some_data* unprotected;
void malicious_function(some_data& protected_data)
{
	unprotected = &protected_data;
}
data_wrapper x;
void foo()
{
	x.process_data(malicious_function); // 2 传递一个恶意函数
	unprotected->do_something(); // 3 在无保护的情况下访问保护数据
}

#include <exception>
#include <memory>
#include <mutex>
#include <stack>

struct empty_stack : std::exception
{
	const char* what() const throw() {
		return "empty stack!";
	};
};
template<typename T>
class threadsafe_stack
{
private:
	std::stack<T> data;
	mutable std::mutex m;
public:
	threadsafe_stack()
		: data(std::stack<int>()) {}
	threadsafe_stack(const threadsafe_stack& other)
	{
		std::lock_guard<std::mutex> lock(other.m);
		data = other.data; // 1 在构造函数体中的执行拷贝
	}
	threadsafe_stack& operator=(const threadsafe_stack&) = delete;
	void push(T new_value)
	{
		std::lock_guard<std::mutex> lock(m);
		data.push(new_value);
	}
	std::shared_ptr<T> pop()
	{
		std::lock_guard<std::mutex> lock(m);
		if (data.empty()) throw empty_stack(); // 在调用pop前，检查栈是否为空
		std::shared_ptr<T> const res(std::make_shared<T>(data.top())); // 在修改堆栈前，分配出返回值
			data.pop();
		return res;
	}
	void pop(T& value)
	{
		std::lock_guard<std::mutex> lock(m);
		if (data.empty()) throw empty_stack();
		value = data.top();
		data.pop();
	}

	bool empty() const
	{
		std::lock_guard<std::mutex> lock(m);
		return data.empty();
	}
};
#include <mutex>

class hierarchical_mutex
{
	std::mutex internal_mutex;
	unsigned long const hierarchy_value;
	unsigned long previous_hierarchy_value;
	static thread_local unsigned long this_thread_hierarchy_value; // 1
	void check_for_hierarchy_violation()
	{
		//当前锁更低，无法上新锁
		if (this_thread_hierarchy_value <= hierarchy_value) //2
		{
			throw std::logic_error("mutex hierarchy violated");
		}
	}
	void update_hierarchy_value()
	{
		//更新锁
		previous_hierarchy_value = this_thread_hierarchy_value;//3
		this_thread_hierarchy_value = hierarchy_value;
	}
public:
	explicit hierarchical_mutex(unsigned long value) :
		hierarchy_value(value),
		previous_hierarchy_value(0)
	{}
	//上锁
	void lock()
	{
		check_for_hierarchy_violation();
		internal_mutex.lock(); //4
		update_hierarchy_value(); //5
	}
	//解锁
	void unlock()
	{
		this_thread_hierarchy_value = previous_hierarchy_value;  //6
		internal_mutex.unlock();
	}
	//试图上锁
	bool try_lock()
	{
		check_for_hierarchy_violation();
		if (!internal_mutex.try_lock()) //7
			return false;
		update_hierarchy_value();
		return true;
	}
	//获取当前线程锁
	static unsigned long get_thread_hierarchy_value()
	{
		return this_thread_hierarchy_value;
	}
};
thread_local unsigned long
hierarchical_mutex::this_thread_hierarchy_value(ULONG_MAX); //8

//层次锁
hierarchical_mutex high_level_mutex(10000); // 1
hierarchical_mutex low_level_mutex(5000); // 2
int do_low_level_stuff();
int low_level_func()
{
	std::lock_guard<hierarchical_mutex> lk(low_level_mutex); // 3
	return do_low_level_stuff();
}
void high_level_stuff(int some_param);
void high_level_func()
{
	std::lock_guard<hierarchical_mutex> lk(high_level_mutex); // 4
	high_level_stuff(low_level_func()); // 5
}
void thread_a() // 6
{
	high_level_func();
}
hierarchical_mutex other_mutex(100); // 7
void do_other_stuff();
void other_stuff()
{
	high_level_func(); // 8
	do_other_stuff();
}
void thread_b() // 9
{
	std::lock_guard<hierarchical_mutex> lk(other_mutex); // 10
	other_stuff();
}
