#pragma once
#include <atomic>
#include <thread>
#include <vector>
#include <future>
#include "threadsafe_queue.h"
class join_threads //用来代替try catch，析构时自动调用joinable()
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

//可等待任务的线程池
class thread_pool
{
private:
	std::atomic_bool done;
	threadsafe_queue<function_wrapper> work_queue; //使用function_wrapper，而非使用std::function

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
	std::future<typename std::result_of<FunctionType()>::type> //1 返回一个std::future<>保存任务的返回值
		submit(FunctionType f)
	{
		//std::result_of<FunctionType()>::type 是FunctionType类型的引用实例(如，f)，并且没有参数
		typedef typename std::result_of<FunctionType()>::type result_type;//2

		std::packaged_task<result_type()> task(std::move(f)); //3 f是一个无参数的函数或是可调用对象
		std::future<result_type> res(task.get_future()); //4 从task中获取future
 		work_queue.push(std::move(task);//5将任务推送到任务队列中时，只能使用std::move()【std::packaged_task<>是不可拷贝的】
		return res;//6 
	}
};

