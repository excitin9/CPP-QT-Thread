#pragma once
#include <vector>
#include <thread>
#include <atomic>
#include <future>
#include "mythreadpool.h"
template<typename Iterator, typename T>
struct accumulate_block
{
	void operator()(Iterator first, Iterator last, T& result)
	{
		result = std::accumulate(first, last, result); // 1 �����׳��쳣
	}
};
template<typename Iterator, typename T>
T parallel_accumulate(Iterator first, Iterator last, T init)
{
	unsigned long const length = std::distance(first, last); // 2
	if (!length)
		return init;
	unsigned long const min_per_thread = 25;
	unsigned long const max_threads =
		(length + min_per_thread - 1) / min_per_thread;
	unsigned long const hardware_threads =
		std::thread::hardware_concurrency();
	unsigned long const num_threads =
		std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
	unsigned long const block_size = length / num_threads;
	std::vector<T> results(num_threads); // 3 
	std::vector<std::thread> threads(num_threads - 1); // 4 ����ڹ���threads�׳��쳣�������������������һ��
	Iterator block_start = first; // 5
	for (unsigned long i = 0; i < (num_threads - 1); ++i)
	{//block_start��ʼ��
		Iterator block_end = block_start; // 6
		std::advance(block_end, block_size);
		threads[i] = std::thread( // 7 �����˵�һ���߳�
			accumulate_block<Iterator, T>(), //���ܻ��׳��쳣
			block_start, block_end, std::ref(results[i]));
		block_start = block_end; // 8
	}
	accumulate_block()(block_start, last, results[num_threads - 1]); // 9 �����׳��쳣
	std::for_each(threads.begin(), threads.end(),
		std::mem_fn(&std::thread::join));
	return std::accumulate(results.begin(), results.end(), init); // 10 ���ܻ��׳��쳣
}

template<typename Iterator, typename T>
struct accumulate_block2
{
	T operator()(Iterator first, Iterator last) // 1
	{
		return std::accumulate(first, last, T()); // 2
	}
};
template<typename Iterator, typename T>
T parallel_accumulate2(Iterator first, Iterator last, T init)
{
	unsigned long const length = std::distance(first, last);
	if (!length)
		return init;
	unsigned long const min_per_thread = 25;
	unsigned long const max_threads =
		(length + min_per_thread - 1) / min_per_thread;
	unsigned long const hardware_threads =
		std::thread::hardware_concurrency();
	unsigned long const num_threads =
		std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
	unsigned long const block_size = length / num_threads;
	std::vector<std::future<T> > futures(num_threads - 1); // 3
	std::vector<std::thread> threads(num_threads - 1);
	Iterator block_start = first;
	for (unsigned long i = 0; i < (num_threads - 1); ++i)
	{
		Iterator block_end = block_start;
		std::advance(block_end, block_size);
		std::packaged_task<T(Iterator, Iterator)> task( // 4
			accumulate_block<Iterator, T>());
		futures[i] = task.get_future(); // 5
		threads[i] = std::thread(std::move(task), block_start, block_end); // 6
		block_start = block_end;
	}
	T last_result = accumulate_block2()(block_start, last); // 7
	std::for_each(threads.begin(), threads.end(),
		std::mem_fn(&std::thread::join));
	T result = init; // 8
	for (unsigned long i = 0; i < (num_threads - 1); ++i)
	{
		result += futures[i].get(); // 9
	}
	result += last_result; // 10
	return result;
}

class join_threads
{
	std::vector<std::thread>& threads;
public:
	explicit join_threads(std::vector<std::thread>& threads_) :
		threads(threads_)
	{}
	~join_threads()
	{
		for (unsigned long i = 0; i < threads.size(); ++i)
		{
			if (threads[i].joinable())
				threads[i].join();
		}
	}
};

template<typename Iterator, typename T>
T parallel_accumulate3(Iterator first, Iterator last, T init)
{
	unsigned long const length = std::distance(first, last);
	if (!length)
		return init;
	unsigned long const min_per_thread = 25;
	unsigned long const max_threads =
		(length + min_per_thread - 1) / min_per_thread;
	unsigned long const hardware_threads =
		std::thread::hardware_concurrency();
	unsigned long const num_threads =
		std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
	unsigned long const block_size = length / num_threads;
	std::vector<std::future<T> > futures(num_threads - 1);
	std::vector<std::thread> threads(num_threads - 1);
	join_threads joiner(threads); // 1
	Iterator block_start = first;
	for (unsigned long i = 0; i < (num_threads - 1); ++i)
	{
		Iterator block_end = block_start;
		std::advance(block_end, block_size);
		std::packaged_task<T(Iterator, Iterator)> task(
			accumulate_block<Iterator, T>());
		futures[i] = task.get_future();
		threads[i] = std::thread(std::move(task), block_start, block_end);
		block_start = block_end;
	}
	T last_result = accumulate_block()(block_start, last);
	T result = init;
	for (unsigned long i = 0; i < (num_threads - 1); ++i)
	{
		result += futures[i].get(); // 2
	}
	result += last_result;
	return result;
}//ʹ��һ���ɵȴ�������̳߳������м���template<typename Iterator, typename T>
T parallel_accumulate4(Iterator first, Iterator last, T init)
{
	unsigned long const length = std::distance(first, last);

	if (!length)
		return init;

	unsigned long const block_size = 25;
	//������������ʹ�õĿ���(num_blocks)���������̵߳�����
	unsigned long const num_blocks = (length + block_size - 1) / block_size; //1 

	std::vector<std::future<T> > futures(num_blocks - 1);
	thread_pool pool;
	
	Iterator block_start = first;
	for (unsigned long i = 0; i < (num_threads - 1); ++i)
	{
		Iterator block_end = block_start;
		std::advance(block_end, block_size);
		futures[i] = pool.submit(accumulate_block<Iterator, T>());//2 ����submit���ύ����
		block_start = block_end;
	}
	T last_result = accumulate_block<Iterator,T>(block_start, last);
	T result = init;
	for (unsigned long i = 0; i < (num_threads - 1); ++i)
	{
		result += futures[i].get(); 
	}
	result += last_result;
	return result;
}