#pragma once
#include <memory>
#include <queue>
#include <condition_variable>

template<typename T>
class threadsafe_queue
{
private:
	mutable std::mutex mut;
	std::queue<T> data_queue;
	std::condition_variable data_cond;
public:
	threadsafe_queue()
	{}
	//void push(T new_value)
	//{
	//	std::lock_guard<std::mutex> lk(mut);
	//	data_queue.push(std::move(data));
	//	data_cond.notify_one(); // 1
	//}
	//改为使用智能指针
	void push(T new_value) 
	{
		//新的实例分配结束时，不会被锁在push()中
		std::shared_ptr<T> data(std::make_shared<T>(std::move(new_value))); //5
		std::lock_guard<std::mutex> lk(mut);
		data_queue.push(data);
		data_cond.notify_one();
	}

	void wait_and_pop(T& value) 
	{
		std::unique_lock<std::mutex> lk(mut);
		data_cond.wait(lk, [this] {return !data_queue.empty(); });
		//value = std::move(data_queue.front());
		//弹出函数会持有一个变量的引用，为了接收这个新值
		//必须对存储的指针进行解引用
		value = std::move(*data_queue.front()); //1
		data_queue.pop();
	}
	std::shared_ptr<T> wait_and_pop() 
	{
		std::unique_lock<std::mutex> lk(mut);
		data_cond.wait(lk, [this] {return !data_queue.empty(); }); 
		//这里加入make_shared<T>失败，其他线程会永世长眠，因为uniqe
			//std::shared_ptr<T> res(
			//	std::make_shared<T>(std::move(data_queue.front())));
		std::shared_ptr<T> res = data_queue.front(); //3
		data_queue.pop();
		return res;
	}
	bool try_pop(T& value)
	{
		std::lock_guard<std::mutex> lk(mut);
		if (data_queue.empty())
			return false;
		//value = std::move(data_queue.front());
		value = std::move(*data_queue.front()); //2
		data_queue.pop();
		return true;
	}
	std::shared_ptr<T> try_pop()
	{
		std::lock_guard<std::mutex> lk(mut);
		if (data_queue.empty())
			return std::shared_ptr<T>(); 
		//std::shared_ptr<T> res(
		//	std::make_shared<T>(std::move(data_queue.front())));
		std::shared_ptr<T> res = data_queue.front(); //4
		data_queue.pop();
		return res;
	}
	bool empty() const
	{
		std::lock_guard<std::mutex> lk(mut);
		return data_queue.empty();
	}
};