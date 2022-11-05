#pragma once
#include <thread>
#include <future>
class interrupt_flag
{
	std::atomic<bool> flag;
	std::condition_variable* thread_cond;
	std::condition_variable_any* thread_cond_any;
	std::mutex set_clear_mutex;
public:
	interrupt_flag():thread_cond(0),thread_cond_any(0){}
	void set() 
	{
		flag.store(true, std::memory_order_relaxed);
		std::lock_guard<std::mutex> lk(set_clear_mutex);
		if (thread_cond)
		{
			thread_cond->notify_all();
		}
		else if (thread_cond_any)
		{
			thread_cond_any->notify_all();
		}
	}
	template <typename Lockable>
	void wait(std::condition_variable_any& cv, Lockable& lk)
	{
		struct custom_lock
		{
			interrupt_flag* self;
			Lockable& lk;

			custom_lock(interrupt_flag* self_, std::condition_variable_any& cond,
				Lockable& lk_) :self(self_), lk(lk_)
			{
				self->set_clear_mutex.lock(); //1 自定义锁构造时，需要锁住内部set_clear_mutex
				self->thread_cond_any = &cond;//2 设置std::condition_variable_any
			}
			void unlock() //3
			{
				lk.unlock();
				self->set_clear_mutex.unlock();//允许线程可以尝试中断其他线程
			}

			void lock()
			{
				std::lock(self->set_clear_mutex, lk);// 要求锁住内部set_clear_mutex，并且锁住Lockable对象
			}

			~custom_lock()
			{
				self->thread_cond_any = 0;//5 在wait()调用时，custom_lock的析构函数中⑤清理thread_cond_any指针
				self->set_clear_mutex.unlock();
			}
		};
		custom_lock cl(this, cv, lk);
		interruption_point();
		cv.wait(cl);
		interruption_point();
	}

	bool is_set() const
	{
		return flag.load(std::memory_order_relaxed);
	}
	void set_condition_variable(std::condition_variable& cv)
	{
		std::lock_guard<std::mutex> lk(set_clear_mutex);
		thread_cond = &cv;
	}
	void clear_condition_variable()
	{
		std::lock_guard<std::mutex> lk(set_clear_mutex);
		thread_cond = 0;
	}
	struct clear_cv_on_destruct
	{
		~clear_cv_on_destruct()
		{
			this_thread_interrupt_flag.clear_condition_variable();
		}
	};
};

thread_local interrupt_flag this_thread_interrupt_flag; // 1
class interruptible_thread
{
	std::thread internal_thread;
	interrupt_flag* flag;
public:
	template<typename FunctionType>
	interruptible_thread(FunctionType f)
	{
		std::promise<interrupt_flag*> p; // 2 
		internal_thread = std::thread([f, &p] { // 3 线程将会持有f副本和本地promise变量(p)的引用
			p.set_value(&this_thread_interrupt_flag);
			f(); // 4 为的是让线程能够调用提供函数的副本
		});
		//调用线程会等待与其future相关的promise就绪，并且将结果存入到flag成员变量中
		flag = p.get_future().get(); // 5
	}
	void join();
	void detach();
	bool joinable() const;
	void interrupt()
	{
		if (flag)
		{
			flag->set(); // 6 设置一个中断标志
		}
	}
	void interruption_point()
	{
		if (this_thread_interrupt_flag.is_set())
		{
			//throw thread_interrupted();
		}
	}
	template<typename Predicate>
	void interruptible_wait(std::condition_variable& cv,
		std::unique_lock<std::mutex>& lk,Predicate pred)
	{
		
		interruption_point();
		this_thread_interrupt_flag.set_condition_variable(cv);
		interrupt_flag::clear_cv_on_destruct guard;
		while (!this_thread_interrupt_flag.is_set() && !pred())
		{
			cv.wait_for(lk, std::chrono::milliseconds(1));
		}
		interruption_point();
	}
	template<typename T>
	void interruptible_wait(std::future<T>& uf)
	{
		while (!this_thread_interrupt_flag.is_set())
		{
			if (uf.wait_for(lk, std::chrono::microseconds(1)) == 
					std::future_status::ready)
				break;
		}
		interruption_point();
	}
};