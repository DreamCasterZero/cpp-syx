// ============================================================
// C++ 并发编程：future 和 promise
// ============================================================
// 核心结论：
//   promise  → 生产者持有，负责设置结果
//   future   → 消费者持有，负责获取结果
//   两者配套使用，实现一次性结果传递
//   比条件变量代码少、语义清晰、能自动传递异常
// ============================================================

#include <iostream>
#include <thread>
#include <future>
#include <stdexcept>
#include <chrono>
#include <vector>

// ============================================================
// 【Part 1】基本用法：promise 设置结果，future 获取结果
// ============================================================

void part1_basic(){
    std::promise<int> p;
    std::future<int> fut = p.get_future(); // 和 promise 关联

    // 生产者线程：做计算，设置结果
    std::thread producer([&p]{
        std::cout << "[producer] 开始计算...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        p.set_value(42);
        std::cout << "[producer] 结果已设置\n";
    });

    // 主线程：等待结果
    std::cout << "[main] 等待结果...\n";
    int result = fut.get();  // 阻塞，直到 promise 设置了值
    std::cout << "[main] 拿到结果: " << result << "\n";
    producer.join();
    std::cout << "\n";
}

// ============================================================
// 【Part 2】异常传递：promise 可以把异常传给 future
// ============================================================

void part2_exception(){
    std::promise<int> p;
    auto fut = p.get_future();
    std::thread t([&p]{
        try{
            throw std::runtime_error("计算出错了");
        }catch(...){
            p.set_exception(std::current_exception());
        }
    });

    try{
        int result = fut.get();
    } catch (const std::exception& e){
        std::cout << "捕获异常: " << e.what() << "\n";
    }
    t.join();
    std::cout << "\n";
}

// ============================================================
// 【Part 3】future 只能 get 一次
//           需要多次获取用 shared_future
// ============================================================

void part3_shared_future(){
    std::promise<int> p;
    auto fut = p.get_future();
    std::shared_future<int> sf = fut.share();  // 转成 shared_future

    p.set_value(42);
    // shared_future 可以多次 get
    std::cout << "第一次 get: " << sf.get() << "\n";
    std::cout << "第二次 get: " << sf.get() << "\n";
    std::cout << "第三次 get: " << sf.get() << "\n";
 
    // 普通 future 只能 get 一次
    // fut2.get();   // 第二次报错！
 
    std::cout << "\n";
}

// ============================================================
// 【Part 4】future 的三种状态，用 wait_for 检查
// ============================================================

void part4_future_status(){
    auto fut = std::async(std::launch::async, []{
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        return 42;
    });
    while(true){
        auto status = fut.wait_for(std::chrono::microseconds(100));
        if(status == std::future_status::ready){
            std::cout << "任务完成，结果: " << fut.get() << "\n";
            break;
        }else if(status == std::future_status::timeout){
            std::cout << "任务还没完成，继续等待...\n";
        }else if (status == std::future_status::deferred){
            std::cout << "任务是 deferred 模式，还没开始\n";
            break;
        }
    }
    std::cout << "\n";
}

// ============================================================
// 【Part 5】promise 不设置值就销毁，future.get() 抛异常
// ============================================================

void part5_broken_promise(){
    std::future<int> fut;
    {
        std::promise<int> p;
        fut = p.get_future();
        // p 离开作用域销毁，没有 set_value
    }
    try {
        fut.get();   // 抛出 std::future_error: broken promise
    } catch (const std::future_error& e) {
        std::cout << "捕获异常: " << e.what() << "\n";
    }
 
    std::cout << "\n";
}

// ============================================================
// 【Part 6】packaged_task：把函数包装成异步任务
//           比 promise 更简洁，自动 set_value
// ============================================================

void part6_packaged_task(){
    std::packaged_task<int(int,int)> task([](int a, int b){
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return a + b;
    });
    auto fut = task.get_future();
     // 在另一个线程执行，执行完自动 set_value
    std::thread t(std::move(task), 10, 20);
    t.join();
    std::cout << "计算结果: " << fut.get() << "\n";  // 30
    std::cout << "\n";
}

// ============================================================
// 【Part 7】并行计算多个结果，promise vs async 对比
// ============================================================

int compute(int x) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    return x * x;
}


void part7_parallel_compute(){
    // 方式一：promise 手动控制
    std::promise<int> p1, p2, p3;
    auto f1 = p1.get_future();
    auto f2 = p2.get_future();
    auto f3 = p3.get_future();
    std::thread t1([&p1]{ p1.set_value(compute(3)); });
    std::thread t2([&p2]{ p2.set_value(compute(4)); });
    std::thread t3([&p3]{ p3.set_value(compute(5)); });
 
    std::cout << "promise 结果: "
              << f1.get() + f2.get() + f3.get() << "\n";  // 9+16+25=50
 
    t1.join(); t2.join(); t3.join();

    // 方式二：async 更简洁（推荐）
    auto af1 = std::async(std::launch::async, compute, 3);
    auto af2 = std::async(std::launch::async, compute, 4);
    auto af3 = std::async(std::launch::async, compute, 5);
    std::cout << "async 结果: "
              << af1.get() + af2.get() + af3.get() << "\n";  // 50
 
    std::cout << "\n";
}

 
// ============================================================
// 【核心总结】
// ============================================================
//
// 1. promise 和 future 是一对
//    promise  → 生产者，set_value 设置结果
//    future   → 消费者，get() 获取结果，阻塞直到结果就绪
//
// 2. 和条件变量的区别
//    条件变量 → 重复通知，生产者消费者模式
//    future   → 一次性结果传递，代码更简洁
//
// 3. 异常传递
//    p.set_exception(std::current_exception()) 把异常传给 future
//    fut.get() 时异常被抛出，可以正常 catch
//
// 4. future 只能 get 一次
//    需要多次获取用 shared_future = fut.share()
//
// 5. future 三种状态
//    ready    → 结果已就绪
//    timeout  → 超时，任务还没完成
//    deferred → deferred 模式，还没开始执行
//    用 wait_for 检查，不阻塞主线程
//
// 6. broken promise
//    promise 没有 set_value 就销毁，future.get() 抛 future_error
//    保证 promise 一定会 set_value 或 set_exception
//
// 7. 三者关系
//    promise       → 最底层，手动控制结果设置
//    packaged_task → 中层，把函数包装成异步任务，自动 set_value
//    async         → 最高层，一行搞定，90% 场景用它
//
// ============================================================
 
int main()
{
    part1_basic();
    part2_exception();
    part3_shared_future();
    part4_future_status();
    part5_broken_promise();
    part6_packaged_task();
    part7_parallel_compute();
    return 0;
}