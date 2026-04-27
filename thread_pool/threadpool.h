#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <unordered_map>

//接受任意类型的输入
class Any
{
public:
	Any() = default;
	~Any() = default;
	Any(const Any&)=delete;
	Any& operator=(const Any&) = delete;
	Any(Any&&) = default;
	Any& operator=(Any&&) = default;


	//能够接受任意类型的输入
	template<typename T>
	Any(T data) : base_(std::make_unique<Derive<T>>(data))
	{ }

	//能够把any对象里存储的data取出
	template<typename T>
	T cast_()
	{
		//如何从base找到derive对象，取出变量
		//基类到派生指针
		Derive<T>* pd = dynamic_cast<Derive<T>*>(base_.get());
		if (pd == nullptr)
		{
			throw"type is unmatch";
		}
		return pd->data_;

	}
private:
	//积累类型
	class Base
	{
	public:
		virtual ~Base() = default;
	};
	//派生类类型
	template<typename T>
	class Derive : public Base
	{
	public:
		Derive(T data) : data_(data)
		{

		}
		T data_;
	};
private:
	std::unique_ptr<Base> base_;
};


//实现一个信号量类
class Semaphore
{
public:
	Semaphore(int limit=0) : resLimit_(limit)
	{ }
	~Semaphore() = default;
	void wait()
	{
		std::unique_lock<std::mutex> lock(mtx_);
		//等待有资源
		cond_.wait(lock, [&]()->bool {return resLimit_ > 0;});
		resLimit_--;
	}

	void post()
	{
		std::unique_lock<std::mutex> lock(mtx_);
		resLimit_++;
		cond_.notify_all();
	}
private:
	int resLimit_;
	std::mutex mtx_;
	std::condition_variable cond_;
};
//实现接受提交到任务完成后的返回值类型result

class Task;

class Result
{
public:
	Result(std::shared_ptr<Task> task, bool isValid=true);
	~Result() = default;
	//setval的方法，获取任务执行完的返回值
	void setVal(Any any);
	//get方法，用户调用获取task返回值
	Any get();

private:
	Any any_;//存储任务的返回值
	Semaphore sem_;//线程通信的信号量
	std::shared_ptr<Task> task_;//指向对应后去返回值的任务对象
	std::atomic_bool isValid_;
};
//线程类型
class Thread
{
public:
	//线程函数对象类型
	using ThreadFunc = std::function<void(int)>;
	//线程构造
	Thread(ThreadFunc func);
	// 线程析构
	~Thread();
	//启动线程
	void start();

	//获取线程id
	int getId() const;
private:
	ThreadFunc func_;
	static int generateId_;
	int threadId_;//保存线程id
};
/*
example:
ThreadPool pool;
pool.start(4);
class mytask:public task
{
public:
		void run(){//线程代码};
};
pool.submittask(std::make_shared<mytask>());


*/



//线程池支持的模式
enum class PoolMode
{
	MODE_FIXED,
	MODE_CACHED,
};
//任务抽象基类
class Task
{
public:
	Task();
	~Task()=default;

	void exec();
	void setResult(Result* res);
	//用户可自定义的任务类型，从task继承，重写run，实现自定义
	virtual Any run() = 0;
private:
	Result* result_;//task会在result之前结束
};
//线程池类型
class ThreadPool
{
public:
	//线程池构造
	ThreadPool();

	//线程池析构
	~ThreadPool();

	//设置线程池的工作模式
	void setMode(PoolMode mode);

	//设置初始的线程数量
	void setInitThreadSize(int size);

	//设置task任务队列上线的阈值
	void setTaskQueMaxThreshHold(int threshhold);

	//设置线程池cach模式线程的数量上线
	void setThreadSizeThreshHold(int threshhold);

	//给线程池提交任务
	Result submitTask(std::shared_ptr<Task>sp);

	//开启线程池
	void start(int initThreadSize = 4);

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

private:
	//定义线程函数
	void threadFunc(int threadid);

	//检查运行状态
	bool checkRunningState() const;

private:
	//std::vector<std::unique_ptr<Thread>> threads_;//线程列表
	std::unordered_map<int, std::unique_ptr<Thread>> threads_;//线程列表
	int initThreadSize_;//初始线程数量
	int threadSizeThreshHold_;//线程数量上限阈值
	std::atomic_int idleThreadSize_;//记录空闲线程的数量
	std::atomic_int curThreadSize_;//当前线程池线程的数量

	std::queue<std::shared_ptr<Task>> taskQue_;//任务队列
	std::atomic_int taskSize_;//任务的数量
	int taskQueMaxThreshHold_;//任务队列数量上线阈值

	std::mutex taskQueMtx_;//保证任务队列的线程安全
	std::condition_variable notFull_;//任务队列不满
	std::condition_variable notEmpty_;//任务队列不空
	std::condition_variable exitCond_;//等待线程资源全部回收

	PoolMode poolMode_;//当前线程池的工作模式
	//当前线程池的启动状态
	std::atomic_bool isPoolRunning_;


};



#endif // !THREADPOOL_H

