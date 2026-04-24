// ============================================================
// C++ 并发编程：packaged_task 和 async
// ============================================================
// 核心结论：
//   packaged_task → 把函数包装成任务，控制执行时机，适合线程池
//   async         → 一行搞定异步任务，最简洁，适合一次性计算
//   核心区别：async 立刻执行，packaged_task 可以存起来以后执行
// ============================================================

#include <iostream>
#include <thread>
#include <future>
#include <queue>
#include <vector>
#include <chrono>
#include <functional>

// ============================================================
// 【Part 1】packaged_task 基本用法
// 把函数包装成任务，自动管理 promise/future
// ============================================================

int compute(int x) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    return x * x;
}

void part1_packaged_task_basic(){
    // 没有 packaged_task，手动用 promise
    {
        std::promise<int> p;
        auto fut = p.get_future();
        std::thread t([&p]{ p.set_value(compute(5)); });  // 手动 set_value
        std::cout << "promise 结果: " << fut.get() << "\n";
        t.join();
    }

    // 用 packaged_task，自动管理
    {
        std::packaged_task<int(int)> task(compute);  // 把函数包进去
        auto fut = task.get_future();
        std::thread t(std::move(task), 5);           // 执行时自动 set_value
        std::cout << "packaged_task 结果: " << fut.get() << "\n";
        t.join();
    }
    std::cout << "\n";
}

// ============================================================
// 【Part 2】packaged_task 的三个特点
// 1. 可以控制执行时机
// 2. 只能执行一次
// 3. 不能拷贝只能移动
// ============================================================

void part2_packaged_task_features(){
    // 特点1：可以控制执行时机
    std::packaged_task<int(int)> task1(compute);
    auto fut1 = task1.get_future();
    // 方式一：在当前线程执行
    task1(5);
    std::cout << "当前线程执行结果: " << fut1.get() << "\n";
    // 方式二：在另一个线程执行
    std::packaged_task<int(int)> task2(compute);
    auto fut2 = task2.get_future();
    std::thread t(std::move(task2), 6);
    t.join();
    std::cout << "另一个线程执行结果: " << fut2.get() << "\n";

    // 特点2：只能执行一次
    // task1(5);   // 报错！已经执行过了

    // 特点3：不能拷贝只能移动
    std::packaged_task<int(int)> task3(compute);
    // auto task4 = task3;             // 报错！不能拷贝
    auto task4 = std::move(task3);    // OK，移动
 
    std::cout << "\n";
}

// ============================================================
// 【Part 3】async 基本用法
// 一行搞定异步任务，最简洁
// ============================================================

void part3_async_basic(){
    // 强制新线程执行
    auto fut1 = std::async(std::launch::async, compute, 5);
    std::cout << "async 结果: " << fut1.get() << "\n";
    // 延迟执行，调用 get() 时才在当前线程执行
    auto fut2 = std::async(std::launch::deferred, compute, 6);
    std::cout << "deferred 结果: " << fut2.get() << "\n";  // 这里才执行
    // 并行计算多个结果
    auto f1 = std::async(std::launch::async, compute, 3);
    auto f2 = std::async(std::launch::async, compute, 4);
    auto f3 = std::async(std::launch::async, compute, 5);
    std::cout << "并行结果: " << f1.get() + f2.get() + f3.get() << "\n";  // 9+16+25=50
 
    std::cout << "\n";
}

// ============================================================
// 【Part 4】packaged_task 用在线程池：可以存起来以后执行
// async 做不到，因为 async 立刻执行
// ============================================================

// 简单线程池
class SimpleThreadPool{
private:
    std::vector<std::thread> workers_;
    std::queue<std::packaged_task<int()>> tasks_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stopped_ = false;
public:
    SimpleThreadPool(int threadCount){
        for(int i = 0; i < threadCount; i++){
            workers_.emplace_back([this]{
                while(true){
                    std::packaged_task<int()> task;
                    {
                        std::unique_lock<std::mutex> lock(mutex_);
                        cv_.wait(lock, [this]{
                            return !tasks_.empty() || stopped_;
                        });
                        if(stopped_ && tasks_.empty()) break;
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    task();
                }
            });
        }
    }
    // 提交任务，返回 future
    std::future<int> submit(std::function<int()> func){
        std::packaged_task<int()> task(func);
        auto fut = task.get_future();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            tasks_.push(std::move(task));
        }
        cv_.notify_one();
        return fut;
    }
    ~SimpleThreadPool(){
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopped_ = true;
        }
        cv_.notify_all();
        for(auto& t : workers_) t.join();
    }
};

void part4_thread_pool(){
    SimpleThreadPool pool(2);  // 2个工作线程
    // 提交任务，拿到 future
    auto f1 = pool.submit([]{ return compute(3); });
    auto f2 = pool.submit([]{ return compute(4); });
    auto f3 = pool.submit([]{ return compute(5); });
 
    // 等待结果
    std::cout << "任务1结果: " << f1.get() << "\n";  // 9
    std::cout << "任务2结果: " << f2.get() << "\n";  // 16
    std::cout << "任务3结果: " << f3.get() << "\n";  // 25
 
    std::cout << "\n";
}

// ============================================================
// 【Part 5】三者对比：promise vs packaged_task vs async
// ============================================================

void part5_comparison(){
    // promise：手动控制，最底层
    // 适合：结果由复杂逻辑决定，不是简单的函数返回值
    {
        std::promise<int> p;
        auto fut = p.get_future();
        std::thread t([&p]{
            int result = 0;
            for (int i = 0; i < 10; i++) result += i;  // 复杂逻辑
            if (result > 40) p.set_value(result);
            else p.set_value(-1);
        });
        std::cout << "promise 结果: " << fut.get() << "\n";
        t.join();
    }

    // packaged_task：把函数包装成任务，控制执行时机
    // 适合：线程池、任务调度
    {
        std::packaged_task<int(int)> task(compute);
        auto fut = task.get_future();
        std::thread t(std::move(task), 7);
        std::cout << "packaged_task 结果: " << fut.get() << "\n";
        t.join();
    }
    // async：最简洁，一行搞定
    // 适合：简单的一次性异步计算
    {
        auto fut = std::async(std::launch::async, compute, 8);
        std::cout << "async 结果: " << fut.get() << "\n";
    }
 
    std::cout << "\n";
}

// ============================================================
// 【核心总结】
// ============================================================
//
// 1. packaged_task
//    把函数和 promise 打包在一起
//    可以控制执行时机：现在执行、以后执行、放进队列
//    只能执行一次，不能拷贝只能移动
//    适合线程池和任务调度系统
//
// 2. async
//    最高层封装，一行搞定异步任务
//    两种策略：launch::async 立刻新线程，launch::deferred 延迟执行
//    适合简单的一次性异步计算
//
// 3. 核心区别
//    async  → 立刻执行，没法存起来以后执行
//    packaged_task → 可以存进队列，等待调度执行
//
// 4. 实际开发怎么选
//    90% 场景         → async，最简洁
//    线程池、任务调度  → packaged_task
//    精确控制结果设置  → promise
//
// ============================================================
 
int main()
{
    part1_packaged_task_basic();
    part2_packaged_task_features();
    part3_async_basic();
    part4_thread_pool();
    part5_comparison();
    return 0;
}