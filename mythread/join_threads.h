#pragma once
#include <vector>
#include <thread>

class join_threads
{
public:
	join_threads(std::vector<std::thread> &threads)
		: m_threads(threads)
	{

	}

	~join_threads()
	{
		for (auto &t : m_threads)
		{
			if (t.joinable())
			{
				t.join();
			}
		}
	}
private:
	std::vector<std::thread> &m_threads;
};