#pragma once
template<typename T>
class threadsafe_list
{
	struct node // 1
	{
		std::mutex m;
		std::shared_ptr<T> data;
		std::unique_ptr<node> next;
		node() : // 2 其next指针②指向的是NULL
			next()
		{}
		node(T const& value) : // 3 
			data(std::make_shared<T>(value))
		{}
	};
	node head;
public:
	threadsafe_list()
	{}
	~threadsafe_list()
	{
		remove_if([](node const&) {return true; });
	}
	threadsafe_list(threadsafe_list const& other) = delete;
	threadsafe_list& operator=(threadsafe_list const& other) = delete;
	void push_front(T const& value)
	{
		std::unique_ptr<node> new_node(new node(value)); // 4 在堆上分配内存
		std::lock_guard<std::mutex> lk(head.m);
		new_node->next = std::move(head.next); // 5
		head.next = std::move(new_node); // 6
	}
	template<typename Function>
	void for_each(Function f) // 7 这个操作需要对队列中的每个元素执行Function(函数指针)
	{
		node* current = &head;
		std::unique_lock<std::mutex> lk(head.m); // 8 需要锁住head及节点⑧的互斥量
		while (node* const next = current->next.get()) // 9 安全的获取指向下一个节点的指针
		{
			std::unique_lock<std::mutex> next_lk(next->m); // 10为了继续对数据进行处理，就需要对指向的节点进行上锁⑩。
			lk.unlock(); // 11 当你已经锁住了那个节点，就可以对上一个节点进行释放了⑪
			f(*next->data); // 12 调用指定函数⑫
			current = next; 
			lk = std::move(next_lk); // 13 并且将所有权从next_lk移动移动到lk
		}
	}
	template<typename Predicate>
	std::shared_ptr<T> find_first_if(Predicate p) // 14 与for_each不同是，find_first_if的p(*next->data)会返回bool
	{
		node* current = &head;
		std::unique_lock<std::mutex> lk(head.m);
		while (node* const next = current->next.get())
		{
			std::unique_lock<std::mutex> next_lk(next->m);
			lk.unlock();
			if (p(*next->data)) // 15
			{
				return next->data; // 16 当条件匹配，只需要返回找到的数据
			}
			current = next;
			lk = std::move(next_lk);
		}
		return std::shared_ptr<T>();
	}
	template<typename Predicate>
	void remove_if(Predicate p) // 17
	{
		node* current = &head;
		std::unique_lock<std::mutex> lk(head.m);
		while (node* const next = current->next.get())
		{
			std::unique_lock<std::mutex> next_lk(next->m);
			if (p(*next->data)) // 18
			{
				std::unique_ptr<node> old_next = std::move(current->next);
				current->next = std::move(next->next); //19 更新current->next
				next_lk.unlock();
			} // 20 当 std::unique_ptr<node> 的移动超出链表范围
			else
			{
				lk.unlock(); // 21 当函数(谓词)返回false，那么移动的操作就和之前一样了
				current = next;
				lk = std::move(next_lk);
			}
		}
	}
};