#include <iostream>
#include <chrono>
#include <thread>
#include "threadpool.h"


//当前的线程池，1、无法实现对任意类型输入的接受，2、无法返回结果result
class MyTask : public Task
{
public:
    MyTask(int begin,int end)
        :begin_(begin)
        ,end_(end)
    { }

    Any run()
    {

        std::cout << "tid:" << std::this_thread::get_id()<< 
            "begin!"<<std::endl;
        //std::this_thread::sleep_for(std::chrono::seconds(5));
        int sum = 0;
        for (int i = begin_;i <= end_;i++)
            sum += i;
        std::cout << "tid:" << std::this_thread::get_id()<<
            "end!"<<std::endl;

        return sum;

    }
private:
    int begin_;
    int end_;
};

int main()
{
    //threadpool析构后，如何把线程池相关的线程资源全部回收
    ThreadPool pool;

    //用户如何设置工作模式
    pool.setMode(PoolMode::MODE_CACHED);
    //设置完毕启动
    pool.start(4);
    Result res_1 = pool.submitTask(std::make_shared<MyTask>(1,1000));
    Result res_2 = pool.submitTask(std::make_shared<MyTask>(1001, 2000));
    Result res_3 = pool.submitTask(std::make_shared<MyTask>(2001, 3000));
    pool.submitTask(std::make_shared<MyTask>(3001, 4000));
    pool.submitTask(std::make_shared<MyTask>(3001, 4000));
    pool.submitTask(std::make_shared<MyTask>(3001, 4000));
    //随着task执行，task对象没了，依赖于task对象的result对象也没了
    int sum1 = res_1.get().cast_<int>();
    int sum2 = res_2.get().cast_<int>();
    int sum3 = res_3.get().cast_<int>();
    //master 线程分解任务，然后给各个slave线程去执行任务
    //等待各个slave线程完成任务返回结果
    //master得到所有任务的结果后，输出
    std::cout << (sum1 + sum2 + sum3) << std::endl;

    int sum=0;
    for (int i = 1;i <= 3000;i++)
        sum += i;
    std::cout << sum << std::endl;

    //pool.submitTask(std::make_shared<MyTask>());
    //pool.submitTask(std::make_shared<MyTask>());
    //pool.submitTask(std::make_shared<MyTask>());
    //pool.submitTask(std::make_shared<MyTask>());
    //pool.submitTask(std::make_shared<MyTask>());
    //pool.submitTask(std::make_shared<MyTask>());
    //pool.submitTask(std::make_shared<MyTask>());
    //pool.submitTask(std::make_shared<MyTask>());
    //pool.submitTask(std::make_shared<MyTask>());
    //pool.submitTask(std::make_shared<MyTask>());
    //pool.submitTask(std::make_shared<MyTask>());
    //pool.submitTask(std::make_shared<MyTask>());
    //pool.submitTask(std::make_shared<MyTask>());
    getchar();


}
