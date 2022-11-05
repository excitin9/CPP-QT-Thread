#pragma once
#include <iostream>
#include <string>
#include <list>

template<typename T>
std::list<T> sequential_quick_sort(std::list<T> input)
{
	if (input.empty())
	{
		return input;
	}
	std::list<T> result;
	result.splice(result.begin(), input, input.begin()); // 1 ʹ��splice()�ٽ�������׸�Ԫ��(�м�ֵ)�������б���
	T const& pivot = *result.begin(); // 2 �м�ֵ
	auto divide_point = std::partition(input.begin(), input.end(),
		[&](T const& t) {return t < pivot; }); // 3  std::partition �������е�ֵ�ֳ�С�ڡ��м䡱ֵ����ʹ��ڡ��м䡱ֵ����
	std::list<T> lower_part;
	lower_part.splice(lower_part.end(), input, input.begin(),
		divide_point); // 4 ��input�б�С��divided_point��ֵ�ƶ������б�lower_part����
	auto new_lower(
		sequential_quick_sort(std::move(lower_part))); // 5
	auto new_higher(
		sequential_quick_sort(std::move(input))); // 6
	result.splice(result.end(), new_higher); // 7 new_higherָ���ֵ���ڡ��м䡱ֵ�ĺ����
	result.splice(result.begin(), new_lower); // 8  new_lowerָ���ֵ���ڡ��м䡱ֵ��ǰ���
	return result;
}

template<typename T>
std::list<T> parallel_quick_sort(std::list<T> input)
{
	if (input.empty())
	{
		return input;
	}
	std::list<T> result;
	result.splice(result.begin(), input, input.begin());
	T const& pivot = *result.begin();
	auto divide_point = std::partition(input.begin(), input.end(),
		[&](T const& t) {return t < pivot; });
	std::list<T> lower_part;
	lower_part.splice(lower_part.end(), input, input.begin(),
		divide_point);
	std::future<std::list<T> > new_lower( // 1 ��ʹ�� std::async() ������һ�̶߳����������
		std::async(&parallel_quick_sort<T>, std::move(lower_part)));
	auto new_higher(
		parallel_quick_sort(std::move(input))); // 2 ��֮ͬǰһ����ʹ�õݹ�ķ�ʽ��������
	result.splice(result.end(), new_higher); // 3
	result.splice(result.begin(), new_lower.get()); // 4
	return result;
}
/*
	����ʹ�� std::async() �������дһ��spawn_task()����
	�� std::packaged_task �� std::thread ���򵥵İ�װ
*/
//template<typename F, typename A>
//std::future<std::result_of<F(A&&)>::type>
//	spawn_task(F&& f, A&& a)
//{
//	typedef std::result_of<F(A&&)>::type result_type;
//	std::packaged_task<result_type(A&&)> task(std::move(f)));
//	std::future<result_type> res(task.get_future());
//	std::thread t(std::move(task), std::move(a));
//	t.detach();
//	return res;
//}

//class spinlock_mutex
//{
//	std::atomic_flag flag;
//public:
//	spinlock_mutex() :
//		flag(ATOMIC_FLAG_INIT)
//	{}
//	void lock()
//	{
//		while (flag.test_and_set(std::memory_order_acquire));
//	}
//	void unlock()
//	{
//		flag.clear(std::memory_order_release);
//	}
//};


