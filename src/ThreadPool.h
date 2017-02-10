// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)


//wyy 2017/2/10
//基于原来的代码把线程类换掉，换成boost::thread，来在windows上使用


#ifndef MUDUO_BASE_THREADPOOL_H
#define MUDUO_BASE_THREADPOOL_H

#ifdef _MSC_VER  
#define snprintf sprintf_s  
#endif


#ifndef LogWarn
#define  LogWarn(show, msg) std::cout << "LogWarn: " << msg;
//#define LogTrace(show, msg) BOOST_LOG_SEV(Logger::getLog(), boost::log::trivial::trace) << THREAD_ID << msg; if (show){std::stringstream ss; ss << msg; Logger::display(ss.str());}
#endif

//#include <muduo/base/Condition.h>
//#include <muduo/base/Mutex.h>
//#include <muduo/base/Thread.h>
//#include <muduo/base/Types.h>
#include <boost/thread.hpp>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <stdio.h>
#include <deque>

using std::string;

namespace muduo
{

	class ThreadPool : boost::noncopyable
	{
	public:
		typedef boost::function<void ()> Task;

		/*explicit*/ ThreadPool(const string& nameArg = string("ThreadPool"));
		~ThreadPool();

		// Must be called before start(). 如果不设置的话默认为0，意思就是任务队列无上限，永远不会满，有安全问题
		void setMaxQueueSize(int maxSize) { maxQueueSize_ = maxSize; }
		void setThreadInitCallback(const Task& cb)
		{ threadInitCallback_ = cb; }

		void start(int numThreads);
		void stop();

		const string& name() const
		{ return name_; }

		size_t queueSize() const;

		// Could block if maxQueueSize > 0
		void run(const Task& f);

	private:
		bool isFull() const;
		void runInThread();
		Task take();

		mutable boost::mutex mutex_;
		boost::condition_variable notEmpty_;
		boost::condition_variable notFull_;
		string name_;
		Task threadInitCallback_;
		boost::ptr_vector<boost::thread> threads_;
		std::deque<Task> queue_;
		size_t maxQueueSize_;
		bool running_;
	};

}

#endif
