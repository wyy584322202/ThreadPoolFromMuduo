//#include <muduo/base/Exception.h>
#include <exception>
#include <boost/bind.hpp>
#include <assert.h>
#include "ThreadPool.h"
#include <iostream>

using namespace muduo;

ThreadPool::ThreadPool(const string& nameArg) 
   :mutex_(),
	notEmpty_(),
	notFull_(),
	name_(nameArg),
	maxQueueSize_(0),
	running_(false)
{}

ThreadPool::~ThreadPool()
{
	if (running_)
	{
		stop();
	}
}

void ThreadPool::start(int numThreads)
{
	assert(threads_.empty());
	running_ = true;
	threads_.reserve(numThreads);
	for (int i = 0; i < numThreads; ++i)
	{
		char id[32] = {0};
		snprintf(id, sizeof id, "%d", i+1);
		threads_.push_back(new boost::thread(boost::bind(&ThreadPool::runInThread, this)));
		//threads_.push_back(new muduo::Thread(
		//	boost::bind(&ThreadPool::runInThread, this), name_+id));
		//threads_[i].start();
	}
	if (numThreads == 0 && threadInitCallback_)
	{
		threadInitCallback_();
	}
}

void ThreadPool::stop()
{
	{
		boost::unique_lock<boost::mutex> lock(mutex_);
		running_ = false;
		notEmpty_.notify_all();
	}
	for_each(threads_.begin(),
		threads_.end(),
		boost::bind(&boost::thread::join, _1));
}

size_t ThreadPool::queueSize() const
{
	boost::unique_lock<boost::mutex> lock(mutex_);
	return queue_.size();
}

//将Task放入任务队列，等待线程去执行
void ThreadPool::run(const Task& task)
{
	//如果线程池中的线程数为0就在当前线程执行任务
	if (threads_.empty())
	{
		task();
	}
	else
	{
		boost::unique_lock<boost::mutex> lock(mutex_);
		while (isFull())
		{
			notFull_.wait(lock);
		}
		assert(!isFull());

		queue_.push_back(task);
		notEmpty_.notify_one();
	}
}

ThreadPool::Task ThreadPool::take()
{
	boost::unique_lock<boost::mutex> lock(mutex_);
	// always use a while-loop, due to spurious wakeup
	while (queue_.empty() && running_)
	{
		notEmpty_.wait(lock);
	}
	Task task;
	if (!queue_.empty())
	{
		task = queue_.front();
		queue_.pop_front();
		if (maxQueueSize_ > 0)
		{
			notFull_.notify_one();
		}
	}
	return task;
}

bool ThreadPool::isFull() const
{
	//mutex_.assertLocked();
	return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
}

void ThreadPool::runInThread()
{
	try
	{
		if (threadInitCallback_)
		{
			threadInitCallback_();
		}
		while (running_)
		{
			Task task(take());
			if (task)
			{
				task();
			}
		}
	}
	catch (const std::exception& ex)
	{
		LogWarn(0, "exception caught in ThreadPool:" << name_);
		LogWarn(0, "reason: " << ex.what());
		abort();
	}
	catch (...)
	{
		LogWarn(0, "unknown exception caught in ThreadPool " << name_);
		throw; // rethrow
	}
}