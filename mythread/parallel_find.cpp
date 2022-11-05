#include <numeric>
#include <atomic>
#include <thread>
#include <future>

template<typename Iterator, typename MatchType>
Iterator parallel_find(Iterator first, Iterator last, MatchType match)
{
	struct find_element // 1
	{
		void operator()(Iterator begin, Iterator end,
			MatchType match,
			std::promise<Iterator>* result,
			std::atomic<bool>* done_flag)		{
			try
			{
				for (; (begin != end) && !done_flag->load(); ++begin) // 2 ѭ��ͨ���ڸ������ݿ��е�Ԫ�أ����ÿһ���ϵı�ʶ
				{
					if (*begin == match)
					{
						result->set_value(begin); // 3���ƥ���Ԫ�ر��ҵ����ͽ����յĽ�����õ�promise�۵���
						done_flag->store(true); // 4 �����ڷ���ǰ��done_flag�ܽ�������
						return;
					}
				}
			}
			catch (...) // 5
			{
				try
				{
					result->set_exception(std::current_exception()); // 6 ����߳̾������ɹ��ľ����߷��ؽ��
					done_flag->store(true);
				}
				catch (...) // 7
				{
				}
			}
		}
	};	unsigned long const length = std::distance(first, last);
	if (!length)
		return last;
	unsigned long const min_per_thread = 25;
	unsigned long const max_threads =
		(length + min_per_thread - 1) / min_per_thread;
	unsigned long const hardware_threads =
		std::thread::hardware_concurrency();
	unsigned long const num_threads =
		std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
	unsigned long const block_size = length / num_threads;
	std::promise<Iterator> result; // 8 ֹͣ������promise
	std::atomic<bool> done_flag(false); // 9 ��ʶ
	std::vector<std::thread> threads(num_threads - 1);
	{ // 10
		join_threads joiner(threads);
		Iterator block_start = first;
		for (unsigned long i = 0; i < (num_threads - 1); ++i)
		{
			Iterator block_end = block_start;
			std::advance(block_end, block_size);
			threads[i] = std::thread(find_element(), // 11 �Է�Χ�ڵ�Ԫ�ز���
				block_start, block_end, match,
				&result, &done_flag);
			block_start = block_end;
		}
		//���̶߳�ʣ�µ�Ԫ�ؽ��в���
		find_element()(block_start, last, match, &result, &done_flag); // 12
	}
	if (!done_flag.load()) //13
	{
		return last; //û���ҵ��ͷ���βԪ��
	}
	return result.get_future().get(); // 14 �ҵ��Ϳ���ֱ�ӷ���
}

template<typename Iterator, typename MatchType> // 1
Iterator async_parallel_find_impl(Iterator first, Iterator last, MatchType match,
	std::atomic<bool>& done)
{
	try
	{
		unsigned long const length = std::distance(first, last);
		unsigned long const min_per_thread = 25; // 2 �õ����̴߳������ٵ�������
		if (length < (2 * min_per_thread)) // 3 ������ݿ��С�����ڷֳ�����
		{
			for (; (first != last) && !done.load(); ++first) // 4
			{
				if (*first == match)
				{
					done = true; // 5 ��ʶdone�ͻ��ڷ���ǰ��������
					return first;
				}
			}
			return last; // 6 û�ҵ��ͷ������һ��Ԫ��
		}
		else
		{
			Iterator const mid_point = first + (length / 2); // 7
			std::future<Iterator> async_result =
				std::async(&async_parallel_find_impl<Iterator, MatchType>, // 8
					mid_point, last, match, std::ref(done));//ʹ��ref��done�����õķ�ʽ����
			Iterator const direct_result =
				async_parallel_find_impl(first, mid_point, match, done); // 9
			return (direct_result == mid_point) ?
				async_result.get() : direct_result; // 10
		}
	}
	catch (...)
	{
		done = true; // 11
		throw;
	}
}
template<typename Iterator, typename MatchType>
Iterator async_parallel_find(Iterator first, Iterator last, MatchType match)
{
	std::atomic<bool> done(false);
	return async_parallel_find_impl(first, last, match, done); // 12
}