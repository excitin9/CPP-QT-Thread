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
	//��Ϊʹ������ָ��
	void push(T new_value) 
	{
		//�µ�ʵ���������ʱ�����ᱻ����push()��
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
		//�������������һ�����������ã�Ϊ�˽��������ֵ
		//����Դ洢��ָ����н�����
		value = std::move(*data_queue.front()); //1
		data_queue.pop();
	}
	std::shared_ptr<T> wait_and_pop() 
	{
		std::unique_lock<std::mutex> lk(mut);
		data_cond.wait(lk, [this] {return !data_queue.empty(); }); 
		//�������make_shared<T>ʧ�ܣ������̻߳��������ߣ���Ϊuniqe
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