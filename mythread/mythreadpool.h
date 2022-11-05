#pragma once
#include <atomic>
#include <thread>
#include <vector>
#include <future>
#include "threadsafe_queue.h"
class join_threads //��������try catch������ʱ�Զ�����joinable()
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

class thread_pool1
{
private:
	std::atomic_bool done; 
	threadsafe_queue<std::function<void()>> work_queue; 

	std::vector<std::thread> threads; 
	join_threads joiner; 
	void worker_thread()
	{
		while (!done)
		{
			std::function<void()> task;
			if (work_queue.try_pop(task)) 
			{
				task(); 
			}
			else
			{
				std::this_thread::yield(); 
			}
		}
	}
public:
	thread_pool1() :
		done(false), joiner(threads)
	{
		unsigned const thread_count = std::thread::hardware_concurrency(); 
		try
		{
			for (unsigned i = 0; i < thread_count; ++i)
			{
				threads.push_back(
					std::thread(&thread_pool1::worker_thread, this)); 
			}
		}
		catch (...)
		{
			done = true; 
			throw;
		}
	}
	~thread_pool1()
	{
		done = true; 
	}
	template<typename FunctionType>
	void submit(FunctionType f)
	{

		work_queue.push(std::function<void()>(f)); 
	}
};


class function_wrapper
{
private:
	struct impl_base {
		virtual void call() = 0;
		virtual ~impl_base() {}
	};
	std::unique_ptr<impl_base> impl;	template<typename F>	struct impl_type : impl_base
	{
		F f;
		impl_type(F&& f_) : f(std::move(f_)) {}
		void call() { f(); }
	};
public:
	template<typename F>
	function_wrapper(F&& f) :
		impl(new impl_type<F>(std::move(f)))
	{}

	void operator()() { impl->call(); }

	function_wrapper() = default;	function_wrapper(function_wrapper&& other) :
		impl(std::move(other.impl))
	{}
	function_wrapper& operator=(function_wrapper&& other)
	{
		impl = std::move(other.impl);
		return *this;
	}
	function_wrapper(const function_wrapper&) = delete;
	function_wrapper(function_wrapper&) = delete;
	function_wrapper& operator=(const function_wrapper&) = delete;
};

//�ɵȴ�������̳߳�
class thread_pool
{
private:
	std::atomic_bool done;
	threadsafe_queue<function_wrapper> work_queue; //ʹ��function_wrapper������ʹ��std::function

	void worker_thread()
	{
		while (!done)
		{
			function_wrapper task;
			if (work_queue.try_pop(task))
			{
				task();
			}
			else
			{
				std::this_thread::yield();
			}
		}
	}
public:
	thread_pool() {
	}
	~thread_pool()
	{
		done = true;
	}
	template<typename FunctionType>
	std::future<typename std::result_of<FunctionType()>::type> //1 ����һ��std::future<>��������ķ���ֵ
		submit(FunctionType f)
	{
		//std::result_of<FunctionType()>::type ��FunctionType���͵�����ʵ��(�磬f)������û�в���
		typedef typename std::result_of<FunctionType()>::type result_type;//2

		std::packaged_task<result_type()> task(std::move(f)); //3 f��һ���޲����ĺ������ǿɵ��ö���
		std::future<result_type> res(task.get_future()); //4 ��task�л�ȡfuture
 		work_queue.push(std::move(task);//5���������͵����������ʱ��ֻ��ʹ��std::move()��std::packaged_task<>�ǲ��ɿ����ġ�
		return res;//6 
	}
};

