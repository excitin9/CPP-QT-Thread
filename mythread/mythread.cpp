/*使用原子操作来控制线程顺序，实现了一个类似于互斥锁这么一个概念*/
#include <thread>
#include <atomic>
#include <iostream>
#include <Windows.h>
using namespace std;

int g_value(0);
atomic<bool> g_data_ready(false);

void read_thread()
{
	while (true)
	{
		while (!g_data_ready.load());//用于控制进入
		cout << g_value << endl;
		g_data_ready = false;
	}
}

void write_thread()
{
	while (true)
	{
		while (g_data_ready.load());//用于控制进入
		Sleep(1000);
		g_value++;
		g_data_ready = true;
	}
}

//int main()
//{
//	thread th(read_thread);
//	th.detach();
//	thread th2(write_thread);
//	th2.detach();
//	char a;
//	while (cin >> a);
//	return 0;
//}