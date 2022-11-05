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
	result.splice(result.begin(), input, input.begin()); // 1 使用splice()①将输入的首个元素(中间值)放入结果列表中
	T const& pivot = *result.begin(); // 2 中间值
	auto divide_point = std::partition(input.begin(), input.end(),
		[&](T const& t) {return t < pivot; }); // 3  std::partition 将序列中的值分成小于“中间”值的组和大于“中间”值的组
	std::list<T> lower_part;
	lower_part.splice(lower_part.end(), input, input.begin(),
		divide_point); // 4 将input列表小于divided_point的值移动到新列表lower_part④中
	auto new_lower(
		sequential_quick_sort(std::move(lower_part))); // 5
	auto new_higher(
		sequential_quick_sort(std::move(input))); // 6
	result.splice(result.end(), new_higher); // 7 new_higher指向的值放在“中间”值的后面⑦
	result.splice(result.begin(), new_lower); // 8  new_lower指向的值放在“中间”值的前面⑧
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
	std::future<std::list<T> > new_lower( // 1 ，使用 std::async() ①在另一线程对其进行排序
		std::async(&parallel_quick_sort<T>, std::move(lower_part)));
	auto new_higher(
		parallel_quick_sort(std::move(input))); // 2 如同之前一样，使用递归的方式进行排序
	result.splice(result.end(), new_higher); // 3
	result.splice(result.begin(), new_lower.get()); // 4
	return result;
}
/*
	比起使用 std::async() ，你可以写一个spawn_task()函数
	对 std::packaged_task 和 std::thread 做简单的包装
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


