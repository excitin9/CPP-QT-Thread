#include <future>
#include <thread>
#include <algorithm>
#include <vector>
#include <iostream>
#include <numeric>
#include "join_threads.h"

template<typename Iterator, typename Func>
void parallel_for_each(Iterator first, Iterator last, Func f)
{
	unsigned long const length = std::distance(first, last);
	if (!length)
		return;
	unsigned long const min_per_thread = 25;
	unsigned long const max_threads =
		(length + min_per_thread - 1) / min_per_thread;
	unsigned long const hardware_threads =
		std::thread::hardware_concurrency();
	unsigned long const num_threads =
		std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
	unsigned long const block_size = length / num_threads;
	std::vector<std::future<void> > futures(num_threads - 1); // 1
	std::vector<std::thread> threads(num_threads - 1);
	join_threads joiner(threads); //控制threads的生命周期
	Iterator block_start = first;
	for (unsigned long i = 0; i < (num_threads - 1); ++i)
	{
		Iterator block_end = block_start;
		std::advance(block_end, block_size);
		std::packaged_task<void(void)> task( // 2
			[=]()
		{
			std::for_each(block_start, block_end, f);
		});
		futures[i] = task.get_future();
		threads[i] = std::thread(std::move(task)); // 3
		block_start = block_end;
	}
	std::for_each(block_start, last, f);
	for (unsigned long i = 0; i < (num_threads - 1); ++i)
	{
		futures[i].get(); // 4
	}
}

template<typename Iterator, typename Func>
void async_parallel_for_each(Iterator first, Iterator last, Func f)
{
	unsigned long const length = std::distance(first, last);
	if (!length)
		return;
	unsigned long const min_per_thread = 25;
	if (length < (2 * min_per_thread))
	{
		std::for_each(first, last, f); // 1
	}
	else
	{
		Iterator const mid_point = first + length / 2;
		std::future<void> first_half = // 2
			std::async(&parallel_for_each<Iterator, Func>,
				first, mid_point, f);
		parallel_for_each(mid_point, last, f); // 3
		first_half.get(); // 4
	}
}

void Test(int i)
{
	std::cout << i << " ";
}

int main()
{
	std::vector<int> vn(2131);
	std::iota(vn.begin(), vn.end(), 1);
	parallel_for_each(vn.cbegin(), vn.cend(), Test);

	return EXIT_SUCCESS;
}