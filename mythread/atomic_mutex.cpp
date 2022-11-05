#include <atomic>
#include <iostream>
#include <vector>
#include <thread>

//atomic实现互斥锁
class FoolMutex
{
private:
	std::atomic<bool> flag = false;
public:
	void lock()
	{
		while (flag.exchange(true, std::memory_order_relaxed));
		std::atomic_thread_fence(std::memory_order_acquire); //建立栅栏
	};
	void unlock()
	{
		std::atomic_thread_fence(std::memory_order_release);
		flag.store(false, std::memory_order_release);
	};
};

int a = 0;

int atomic_mutex()
{
	FoolMutex mutex;

	std::thread t1([&]() {
		mutex.lock();
		a += 1;
		mutex.unlock();
	});

	std::thread t2([&]() {
		mutex.lock();
		a += 2;
		mutex.unlock();
	});

	t1.join();

	std::cout << a << std::endl;

	t2.join();

	std::cout << a << std::endl;
	return 0;
}