#include "threadpool.h"
#include <functional>
#include <thread>
#include <iostream>
#include <chrono>

const int TASK_MAX_THRESHHOLD = 10;
const int THREAD_MAX_THRESHHOLD = 200;
const int THREAD_MAX_IDLE_TIME = 60;//单位秒s
//线程池构造
ThreadPool::ThreadPool()
	:initThreadSize_(0)
	,taskSize_(0)
	,idleThreadSize_(0)
	,curThreadSize_(0)
	,taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD)
	,threadSizeThreshHold_(THREAD_MAX_THRESHHOLD)
	,poolMode_(PoolMode::MODE_FIXED)
	,isPoolRunning_(false)
{ }

//线程池析构
ThreadPool::~ThreadPool()
{
	isPoolRunning_ = false;
	notEmpty_.notify_all();
	//等待线程池的线程返回，结束，状态：阻塞、正在执行任务中
	std::unique_lock<std::mutex> lock(taskQueMtx_);
	exitCond_.wait(lock, [&]()->bool {return threads_.size() == 0;});
}


//设置线程池的工作模式
void ThreadPool::setMode(PoolMode mode)
{
	if (checkRunningState())
		return;
	poolMode_ = mode;
}

//设置task任务队列上线的阈值
void ThreadPool::setTaskQueMaxThreshHold(int threshhold)
{
	if (checkRunningState())
		return;
	taskQueMaxThreshHold_ = threshhold;
}


void ThreadPool::setThreadSizeThreshHold(int threshhold)
{
	if (checkRunningState())
		return;
	if (poolMode_ == PoolMode::MODE_CACHED)
	{
	threadSizeThreshHold_ = threshhold;
	}

}

//给线程池提交任务  用户借助该接口传入任务
Result ThreadPool::submitTask(std::shared_ptr<Task>sp)
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
		return Result(sp,false);

	}
	//空余后放入任务
	taskQue_.emplace(sp);
	taskSize_++;//放入任务后任务数+1
	//新放入，队列不空notEmpty_变量
	notEmpty_.notify_all();//告知消费者任务队列有任务，可执行任务了

	//根据当前的情况，任务的数量和空闲线程的数量判断是否需要创建新的线程出来
	if (poolMode_ == PoolMode::MODE_CACHED && taskSize_ > idleThreadSize_ &&
		curThreadSize_ < threadSizeThreshHold_)
	{

		std::cout << ">>>create new thread" <<std::endl;
		//创建新的线程对象
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));
		//启动线程
		threads_[threadId]->start();
		//threads_.emplace_back(std::move(ptr));
		//修改线程个数相关的变量
		curThreadSize_++;
		idleThreadSize_++;
	}
	//返回任务的result对象
	return Result(sp);
}

//开启线程池
void ThreadPool::start(int initThreadSize)
{
	//设置运行状态
	isPoolRunning_ = true;
	//记录初始线程个数
	initThreadSize_ = initThreadSize;
	curThreadSize_ = initThreadSize;

	//创建线程对象
	for (int i = 0;i < initThreadSize_;i++) 
	{
		//创建thread线程对象的时候，吧县城函数给到线程对象
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this,std::placeholders::_1));
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));
		//threads_.emplace_back(std::move(ptr));
	}

	//启动所有线程  std::vector<Thread*> threads_;
	for (int i = 0;i < initThreadSize_;i++)
	{
		threads_[i]->start();
		idleThreadSize_++;
	}
}

//定义线程函数 从任务队列获取任务并完成
void ThreadPool::threadFunc(int threadid)
{
	auto lastTime = std::chrono::high_resolution_clock().now();

	while(isPoolRunning_)
	{
		std::shared_ptr<Task>  task;
		{
		//先获取锁
		std::unique_lock<std::mutex>lock(taskQueMtx_);
		std::cout << "tid:" << std::this_thread::get_id() <<
			"尝试获取任务" << std::endl;

		//在线程空闲时间过长时需要回收如何实现

			//每秒返回一次  如何区分超时和有任务待执行返回
			while (taskQue_.size() == 0)
			{
				if (poolMode_ == PoolMode::MODE_CACHED)
				{				
					if (std::cv_status::timeout ==
					notEmpty_.wait_for(lock, std::chrono::seconds(1)))
				{
					auto now =std::chrono::high_resolution_clock().now();
					auto dur = std::chrono::duration_cast<
						std::chrono::seconds>(now - lastTime);
					if (dur.count() >= THREAD_MAX_IDLE_TIME
						&& curThreadSize_> initThreadSize_)
					{
						//回收线程
						//记录线程数量的相关变量的值修改
						//把线程对象从线程列表容器中删除  线程函数和线程对象如何一一对应
						threads_.erase(threadid);
						curThreadSize_--;
						idleThreadSize_--;
						std::cout << "threadsid:" << std::this_thread::get_id() << "exit!"
							<< std::endl;
						return;

					}
				}
			}
			else
			{	//等待notempty通知
			notEmpty_.wait(lock);
			}
				//线程池结束回收资源
				if (!isPoolRunning_)
				{
					threads_.erase(threadid);
					std::cout << "threadsid:" << std::this_thread::get_id() << "exit!"
						<< std::endl;
					exitCond_.notify_all();
					return;
				}
		 }
		

		idleThreadSize_--;

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
			task->exec();
			//执行完任务
		}
		idleThreadSize_++;
		lastTime = std::chrono::high_resolution_clock().now();//更新线程执行完的时间
	}
	threads_.erase(threadid);
	std::cout << "threadsid:" << std::this_thread::get_id() << "exit!"
		<< std::endl;
	exitCond_.notify_all();

	//std::cout << "begin threadFunc tid:" << std::this_thread::get_id()<< std::endl;
	//std::cout << "end threadFunc tid:" << std::this_thread::get_id()<< std::endl;

}

bool ThreadPool::checkRunningState() const
{
	return isPoolRunning_;

}

//////线程方法实现

int Thread::generateId_=0;

//线程构造
Thread::Thread(ThreadFunc func)
	:func_(func)
	, threadId_(generateId_++)
{

}
// 线程析构
Thread::~Thread()
{

}
void Thread::start()
{
	//创建一个线程执行一个线程函数
	std::thread t(func_,threadId_);
	t.detach();//设置分离线程

}

int Thread::getId() const
{
	return threadId_;
}
//task实现方法
Task::Task()
	:result_(nullptr)
{ }




void Task::exec()
{
	if(result_!=nullptr)
	{
	result_->setVal(run());//多态调用
	}
}

void Task::setResult(Result* res)
{
	result_ = res;
}


//result方法的实现
Result::Result(std::shared_ptr<Task> task, bool isValid )
	:isValid_(isValid)
	,task_(task)
{ 
	task_->setResult(this);
}
Any Result::get()
{
	if (!isValid_)
	{
		return "";
	}
	sem_.wait();//task如果没有执行完，会阻塞用户线程
	return std::move(any_);
}

void Result::setVal(Any any)//由谁调用
{
	//存储task返回值
	this->any_ = std::move(any);
	sem_.post();//以获取的任务返回值，增加信号量资源

}
;