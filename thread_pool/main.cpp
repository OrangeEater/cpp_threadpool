#include <iostream>
#include <chrono>
#include <thread>
#include "threadpool.h"


//当前的线程池，1、无法实现对任意类型输入的接受，2、无法返回结果result
class MyTask :public Task
{
//public:
//    MyTask(int begin, int end)
//        :begin_(begin)
//        , end_(end)
//    {
//
//    }
    void run()
    {
        std::cout << "tid:" << std::this_thread::get_id()<< 
            "begin!"<<std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::cout << "tid:" << std::this_thread::get_id()<<
            "end!"<<std::endl;

    }
};

int main()
{
    ThreadPool pool;
    pool.start(4);
    pool.submitTask(std::make_shared<MyTask>());
    pool.submitTask(std::make_shared<MyTask>());
    pool.submitTask(std::make_shared<MyTask>());
    pool.submitTask(std::make_shared<MyTask>());
    pool.submitTask(std::make_shared<MyTask>());
    pool.submitTask(std::make_shared<MyTask>());
    pool.submitTask(std::make_shared<MyTask>());
    pool.submitTask(std::make_shared<MyTask>());
    pool.submitTask(std::make_shared<MyTask>());
    pool.submitTask(std::make_shared<MyTask>());
    pool.submitTask(std::make_shared<MyTask>());
    pool.submitTask(std::make_shared<MyTask>());
    pool.submitTask(std::make_shared<MyTask>());


    getchar();

}
