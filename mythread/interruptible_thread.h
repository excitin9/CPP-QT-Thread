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
				self->set_clear_mutex.lock(); //1 �Զ���������ʱ����Ҫ��ס�ڲ�set_clear_mutex
				self->thread_cond_any = &cond;//2 ����std::condition_variable_any
			}
			void unlock() //3
			{
				lk.unlock();
				self->set_clear_mutex.unlock();//�����߳̿��Գ����ж������߳�
			}

			void lock()
			{
				std::lock(self->set_clear_mutex, lk);// Ҫ����ס�ڲ�set_clear_mutex��������סLockable����
			}

			~custom_lock()
			{
				self->thread_cond_any = 0;//5 ��wait()����ʱ��custom_lock�����������Т�����thread_cond_anyָ��
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
		internal_thread = std::thread([f, &p] { // 3 �߳̽������f�����ͱ���promise����(p)������
			p.set_value(&this_thread_interrupt_flag);
			f(); // 4 Ϊ�������߳��ܹ������ṩ�����ĸ���
		});
		//�����̻߳�ȴ�����future��ص�promise���������ҽ�������뵽flag��Ա������
		flag = p.get_future().get(); // 5
	}
	void join();
	void detach();
	bool joinable() const;
	void interrupt()
	{
		if (flag)
		{
			flag->set(); // 6 ����һ���жϱ�־
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