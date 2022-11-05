#pragma once
#include <string>
#include <future>
struct X
{
	void foo(int, std::string const&);
	std::string bar(std::string const&);
};
X x;
auto f1 = std::async(&X::foo, &x, 42, "hello"); // ����p->foo(42, "hello")��p��ָ��x��ָ��
auto f2 = std::async(&X::bar, x, "goodbye"); // ����tmpx.bar("goodbye")�� tmpx��x�Ŀ�������
struct Y
{
	double operator()(double);
};

Y y;
auto f3 = std::async(Y(), 3.141); // ����tmpy(3.141)��tmpyͨ��Y���ƶ����캯���õ�
auto f4 = std::async(std::ref(y), 2.718); // ����y(2.718)
X baz(X&);
//std::async(baz,std::ref(x)); // ����baz(x)
class move_only
{
public:
	move_only();
	move_only(move_only&&);
	move_only(move_only const&) = delete;
	move_only& operator=(move_only&&);
	move_only& operator=(move_only const&) = delete;
	void operator()();
};

auto f5 = std::async(move_only()); // ����tmp()��tmp��ͨ��std::move(move_only())����õ�

auto f6 = std::async(std::launch::async, Y(), 1.2); // �����߳���ִ��
auto f7 = std::async(std::launch::deferred, baz, std::ref(x)); // ��wait()��get()����ʱִ��
auto f8 = std::async(
	std::launch::deferred | std::launch::async,
	baz, std::ref(x)); // ʵ��ѡ��ִ�з�ʽ
auto f9 = std::async(baz, std::ref(x));
//f7.wait(); // �����ӳٺ���


template<std::string(std::vector<char>*, int)>
class packaged_task
{
public:
	template<typename Callable>
	explicit packaged_task(Callable&& f);
	std::future<std::string> get_future();
	void operator()(std::vector<char>*, int);
};

