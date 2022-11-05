#pragma once
#include <mutex>
#include <memory>
#include <atomic>
template<typename T>
class lock_free_stack
{
private:
	struct node
	{
		std::shared_ptr<T> data; // 1 ָ���ȡ����
		node* next;
		node(T const& data_) :
			data(std::make_shared<T>(data_)) // 2 ��std::shared_ptrָ���·��������T
		{}
	};
	std::atomic<node*> head;

private:
	std::atomic<unsigned> threads_in_pop; //1 ԭ�ӱ���
	void try_reclaim(node* old_head);
public:
	std::shared_ptr<T> pop()
	{
		++threads_in_pop; // 2 ������֮ǰ������ֵ��1
		node* old_head = head.load();
		while (old_head && !head.compare_exchange_weak(old_head, old_head->next));
		std::shared_ptr<T> res;
		if (old_head)
		{
			res.swap(old_head->data); //3 ����ɾ���Ľڵ�
		}
		try_reclaim(old_head); // 4 �ӽڵ���ֱ����ȡ���ݣ����ǿ���ָ�룬��������1
		return res;
	}
//�������ü����Ļ��ջ���
private:
	std::atomic<node*> to_be_deleted;
	static void delete_nodes(node* nodes)
	{
		while (nodes)
		{
			node* next = nodes->next;
			delete nodes;
			nodes = next;
		}
	}
	void try_reclaim(node* old_head)
	{
		if (threads_in_pop == 1) // 1 ���threas_in_pop==1�Ǿͱ�����ǰ�߳����ڶ�pop()���з���
			//���԰�ȫɾ��
		{
			node* nodes_to_delete = to_be_deleted.exchange(nullptr); // 2 ��������ɾ�����б�ԭ�Ӳ���exchange
			if (!--threads_in_pop) // 3 �Ƿ�ֻ��һ���̵߳���pop()��
			{
				delete_nodes(nodes_to_delete); // 4
			}
			else if (nodes_to_delete) // 5 ������Ϊ0�����սڵ㲻��ȫ
			{
				chain_pending_nodes(nodes_to_delete); // 6 ���ڵȴ�ɾ���б�
			}
			delete old_head; // 7
		}
		else
		{
			chain_pending_node(old_head); // 8 ���ڵȴ�ɾ���б�
			--threads_in_pop;
		}
	}
	void chain_pending_nodes(node* nodes)
	{
		node* last = nodes;
		while (node* const next = last->next) // 9 ��nextָ��ָ�������ĩβ
		{
			last = next;
		}
		chain_pending_nodes(nodes, last);
	}
	void chain_pending_nodes(node* first, node* last)
	{
		last->next = to_be_deleted; // 10 �����һ���ڵ��nextָ���滻Ϊ��ǰto_be_deletedָ��
		while (!to_be_deleted.compare_exchange_weak( // 11 ��ѭ������֤last->next����ȷ��
			last->next, first));
	}
	void chain_pending_node(node* n)
	{
		chain_pending_nodes(n, n); // 12 ��ӵ����ڵ���һ�����������
		//��Ϊ����Ҫ������ڵ���Ϊ��һ���ڵ㣬ͬʱҲ�����һ���ڵ�������
	}
};
