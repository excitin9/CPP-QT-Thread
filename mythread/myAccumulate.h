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
	if (!length) // 1 �������ķ�ΧΪ�բ٣��ͻ�õ�init��ֵ
		return init;
	unsigned long const min_per_thread = 25;
	unsigned long const max_threads =
		(length + min_per_thread - 1) / min_per_thread; // 2 ��Χ��Ԫ�ص������������߳�(��)����С���������Ӷ�ȷ�������̵߳��������
	unsigned long const hardware_threads =
		std::thread::hardware_concurrency();
	unsigned long const num_threads = // 3 �����������ֵ��Ӳ��֧���߳����У���С��ֵΪ�����̵߳�����
		std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
	unsigned long const block_size = length / num_threads; // 4 ÿ���̴߳����Ԫ���������Ƿ�Χ��Ԫ�ص����������̵߳ĸ����ó���
	std::vector<T> results(num_threads);
	std::vector<std::thread> threads(num_threads - 1); // 5 ������һ���߳�����,�������߳�Ҫ-1����Ϊ�Ѿ�����һ�����߳�
	Iterator block_start = first;
	for (unsigned long i = 0; i < (num_threads - 1); ++i)
	{
		Iterator block_end = block_start;
		std::advance(block_end, block_size); // 6 block_endָ��ǰ���ĩβ
		threads[i] = std::thread( // 7 ����һ�����̣߳���Ϊ��ǰ���ۼӽ��
			accumulate_block<Iterator, T>(),
			block_start, block_end, std::ref(
				[i]));
		block_start = block_end; // 8 ������һ����
	}
	accumulate_block<Iterator, T>()(
		block_start, last, results[num_threads - 1]); // 9 �������տ�Ľ��
	std::for_each(threads.begin(), threads.end(),
		std::mem_fn(&std::thread::join)); // 10 �ȴ��̴߳������
	return std::accumulate(results.begin(), results.end(), init); // 11 ������ۼ�
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
		func(data); // 1 ���ݡ����������ݸ��û�����
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
	x.process_data(malicious_function); // 2 ����һ�����⺯��
	unprotected->do_something(); // 3 ���ޱ���������·��ʱ�������
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
		data = other.data; // 1 �ڹ��캯�����е�ִ�п���
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
		if (data.empty()) throw empty_stack(); // �ڵ���popǰ�����ջ�Ƿ�Ϊ��
		std::shared_ptr<T> const res(std::make_shared<T>(data.top())); // ���޸Ķ�ջǰ�����������ֵ
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
		//��ǰ�����ͣ��޷�������
		if (this_thread_hierarchy_value <= hierarchy_value) //2
		{
			throw std::logic_error("mutex hierarchy violated");
		}
	}
	void update_hierarchy_value()
	{
		//������
		previous_hierarchy_value = this_thread_hierarchy_value;//3
		this_thread_hierarchy_value = hierarchy_value;
	}
public:
	explicit hierarchical_mutex(unsigned long value) :
		hierarchy_value(value),
		previous_hierarchy_value(0)
	{}
	//����
	void lock()
	{
		check_for_hierarchy_violation();
		internal_mutex.lock(); //4
		update_hierarchy_value(); //5
	}
	//����
	void unlock()
	{
		this_thread_hierarchy_value = previous_hierarchy_value;  //6
		internal_mutex.unlock();
	}
	//��ͼ����
	bool try_lock()
	{
		check_for_hierarchy_violation();
		if (!internal_mutex.try_lock()) //7
			return false;
		update_hierarchy_value();
		return true;
	}
	//��ȡ��ǰ�߳���
	static unsigned long get_thread_hierarchy_value()
	{
		return this_thread_hierarchy_value;
	}
};
thread_local unsigned long
hierarchical_mutex::this_thread_hierarchy_value(ULONG_MAX); //8

//�����
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
