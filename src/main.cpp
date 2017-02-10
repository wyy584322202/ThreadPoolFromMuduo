

#include <boost/bind.hpp>
#include <stdio.h>
#include "ThreadPool.h"
#include <boost/chrono.hpp>

void print()
{
	//printf("tid=%d\n", boost::this_thread::get_id());
	std::cout << "tid=" << boost::this_thread::get_id() << "\n";
}

void printString(const std::string& str)
{
	//��������е�����Ҫ��֤�̰߳�ȫ printf���̰߳�ȫ��std::cout����
	//std::cout << str << "\n";
	printf("%s\n", str.c_str());
	//boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
}

void test(int maxSize)
{
	std::cout << "Test ThreadPool with max queue size = " << maxSize << "\n";
	muduo::ThreadPool pool("MainThreadPool");
	pool.setMaxQueueSize(maxSize);
	pool.start(5);

	std::cout << "Adding" << "\n";
	pool.run(print);
	pool.run(print);
	for (int i = 0; i < 100; ++i)
	{
		char buf[32];
		snprintf(buf, sizeof buf, "task %d", i);
		pool.run(boost::bind(printString, std::string(buf)));
	}
	std::cout << "Done" << "\n";
}

int main()
{
	test(0);
	test(1);
	test(5);
	test(10);
	test(50);
}
