#include "threadpool.h"
#include <functional>
#include <thread>
#include <iostream>

const int TASK_MAX_THRESHHOLD = 4;
//线程池构造
ThreadPool::ThreadPool()
	:	initThreadSize_(0)
	,taskSize_(0)
	,taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD)
	,poolMode_(PoolMode::MODE_FIXED)
{ }

//线程池析构
ThreadPool::~ThreadPool()
{ }


//设置线程池的工作模式
void ThreadPool::setMode(PoolMode mode)
{
	poolMode_ = mode;
}

//设置task任务队列上线的阈值
void ThreadPool::setTaskQueMaxThreshHold(int threshhold)
{
	taskQueMaxThreshHold_ = threshhold;
}

//给线程池提交任务  用户借助该接口传入任务
void ThreadPool::submitTask(std::shared_ptr<Task>sp)
{
	//获取锁
	std::unique_lock<std::mutex>lock(taskQueMtx_);
	//线程通信  等待任务队列没有满，可接受任务 wait一直等 wait_for最多等多久 wait_until
	//有时压力大，需要非常久的时间上传任务，若上传等待的时间过久提示上传失败，返回
	if (!notFull_.wait_for(lock, std::chrono::seconds(1),
		[&]()->bool {return taskQue_.size() < (size_t)taskQueMaxThreshHold_;}))
	{
		//表示等待1秒后仍然没有执行，执行别的操作
		std::cerr << "task queuq is full, submit task fail." << std::endl;
		return;

	}
	//空余后放入任务
	taskQue_.emplace(sp);
	taskSize_++;//放入任务后任务数+1
	//新放入，队列不空notEmpty_变量
	notEmpty_.notify_all();//告知消费者任务队列有任务，可执行任务了
}

//开启线程池
void ThreadPool::start(int initThreadSize)
{
	//记录初始线程个数
	initThreadSize_ = initThreadSize;

	//创建线程对象
	for (int i = 0;i < initThreadSize_;i++) 
	{
		//创建thread线程对象的时候，吧县城函数给到线程对象
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this));
		threads_.emplace_back(std::move(ptr));
	}

	//启动所有线程  std::vector<Thread*> threads_;
	for (int i = 0;i < initThreadSize_;i++)
	{
		threads_[i]->start();
	}
}

//定义线程函数 从任务队列获取任务并完成
void ThreadPool::threadFunc()
{
	for (;;)
	{
		std::shared_ptr<Task>  task;
		{
		//先获取锁
		std::unique_lock<std::mutex>lock(taskQueMtx_);
		std::cout << "tid:" << std::this_thread::get_id() <<
			"尝试获取任务" << std::endl;
		//等待notempty通知
		notEmpty_.wait(lock, [&]()->bool {return taskQue_.size() > 0;});
		std::cout << "tid:" << std::this_thread::get_id() <<
			"任务获取成功" << std::endl;
		//在任务队列获取一个任务
		task = taskQue_.front();
		taskQue_.pop();
		taskSize_--;

		//如果任务队列还有任务，通知别的线程，可继续获取
		if (taskQue_.size() > 0)
		{
			notEmpty_.notify_all();
		}
		//取出一个任务通知,可以继续提交任务
		notFull_.notify_all();

		}//把锁释放掉,否则别的线程获取不到任务

		//执行这个任务
		if (task != nullptr) 
		{
			task->run();
			//执行完任务
		}


	}


	//std::cout << "begin threadFunc tid:" << std::this_thread::get_id()<< std::endl;
	//std::cout << "end threadFunc tid:" << std::this_thread::get_id()<< std::endl;

}
//////线程方法实现

//线程构造
Thread::Thread(ThreadFunc func)
	:func_(func)
{

}
// 线程析构
Thread::~Thread()
{

}
void Thread::start()
{
	//创建一个线程执行一个线程函数
	std::thread t(func_);
	t.detach();//设置分离线程

}