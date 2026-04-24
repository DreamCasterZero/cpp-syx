// ============================================================
// 条件变量的使用：生产者消费者模式
// ============================================================
// 核心三件套：mutex + condition_variable + bool 条件
// wait  → 检查条件，不满足则释放锁睡着，满足则继续执行
// notify_one → 随机唤醒一个等待线程
// notify_all → 唤醒所有等待线程
// ============================================================

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <chrono>

// ============================================================
// 【场景一】最基本的用法：单个 producer，单个 consumer
// ============================================================

std::mutex m1;
std::condition_variable cv1;
bool dataReady1 = false;

void producer1(){
    std::cout << "Producer 1 is producing..." << std::endl;
    std::this_thread::sleep_for(std::chrono::microseconds(500));
    {
        std::lock_guard<std::mutex> lock(m1);
        dataReady1 = true;
        std::cout << "Producer 1 has produced data." << std::endl;
    }
    cv1.notify_one();
}

void consumer1(){
    std::cout << "Consumer 1 is waiting for data..." << std::endl;
    std::unique_lock<std::mutex> lock(m1);
    cv1.wait(lock, []{return dataReady1;});
    // wait 内部：
    // 1. 检查 dataReady1，false 则释放锁睡着
    // 2. 被 notify 唤醒后重新加锁
    // 3. 再检查 dataReady1，true 则往下执行
    std::cout << "Consumer 1 has consumed data." << std::endl;
}

void scene1(){
    std::cout << "=== Scene 1: Single Producer, Single Consumer ===" << std::endl;
    std::thread t_producer(producer1);
    std::thread t_consumer(consumer1);
    t_producer.join();
    t_consumer.join();
    std::cout << std::endl;
}

// ============================================================
// 【场景二】单个 producer，多个 consumer，用 notify_all
// 广播通知：所有 consumer 都需要知道条件满足
// ============================================================

std::mutex m2;
std::condition_variable cv2;
bool initialized = false;

void producer2(){
    std::cout << "[producer] 系统初始化中...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    {
        std::lock_guard<std::mutex> lock(m2);
        initialized = true;
        std::cout << "[producer] 初始化完成，通知所有 consumer\n";
    }

    cv2.notify_all();
}

void consumer2(int id){
    std::cout << "[consumer" << id << "] 等待初始化完成...\n";
    std::unique_lock<std::mutex> lock(m2);
    cv2.wait(lock, []{return initialized;});
    std::cout << "[consumer" << id << "] 初始化完成，开始工作\n";
}

void scene2(){
    std::cout << "===== 场景二：单 producer，多 consumer，notify_all =====\n";
    std::thread t_producer(producer2);
    std::thread t_c1(consumer2, 1);
    std::thread t_c2(consumer2, 2);
    std::thread t_c3(consumer2, 3);

    t_producer.join();
    t_c1.join();
    t_c2.join();
    t_c3.join();
    std::cout << "\n";
}

// ============================================================
// 【场景三】任务队列：多个 producer，多个 consumer
// 用 notify_one，每次只唤醒一个 consumer 处理任务
// ============================================================

std::mutex m3;
std::condition_variable cv3;
std::queue<int> taskQueue;
bool done = false;   // 所有任务是否生产完毕

void producer3(int id, int taskCount){
    for(int i = 0; i < taskCount; i++){
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        {
            std::lock_guard<std::mutex> lock(m3);
            int task = id * 100 + i;
            taskQueue.push(task);
            std::cout << "[producer" << id << "] 生产任务: " << task << "\n";
        }
        cv3.notify_one();
    }
}

void consumer3(int id){
    while(true){
        std::unique_lock<std::mutex> lock(m3);
        cv3.wait(lock, []{return !taskQueue.empty() || done; });
        if(taskQueue.empty() && done) break;

        int task = taskQueue.front();
        taskQueue.pop();
        lock.unlock();

        // 处理任务（不需要持锁）
        std::cout << "[consumer" << id << "] 处理任务: " << task << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(150));  // 模拟处理耗时
    }
    std::cout << "[consumer" << id << "] 退出\n";
}

void scene3(){
    std::cout << "===== 场景三：多 producer 多 consumer，任务队列 =====\n";
    std::thread t_p1(producer3, 1, 3);   // producer1 生产3个任务
    std::thread t_p2(producer3, 2, 3);   // producer2 生产3个任务
 
    std::thread t_c1(consumer3, 1);
    std::thread t_c2(consumer3, 2);

    t_p1.join();
    t_p2.join();

    // 所有 producer 结束，标记完成
    {
        std::lock_guard<std::mutex> lock(m3);
        done = true;
    }

    cv3.notify_all();   // 唤醒所有 consumer，让它们检查 done 并退出

    t_c1.join();
    t_c2.join();
    std::cout << "\n";
}

// ============================================================
// 【核心总结】
// ============================================================
//
// 1. 条件变量三件套
//    mutex              → 保护条件变量的 bool 标志
//    condition_variable → 通知和等待机制
//    bool 条件          → 真正的条件，表示事件是否发生
//
// 2. wait 的行为
//    检查条件 → false 则释放锁睡着 → 被 notify 唤醒 → 重新加锁 → 再检查条件
//    必须传 lambda 防止虚假唤醒
//
// 3. consumer 用 unique_lock，producer 用 lock_guard
//    wait 内部需要解锁和重新加锁，所以必须用 unique_lock
//    producer 只需要简单加锁，lock_guard 就够了
//
// 4. 先解锁再 notify
//    producer 修改条件后先解锁，再 notify
//    让 consumer 被唤醒后能立刻拿到锁，不需要再等
//
// 5. notify_one vs notify_all
//    notify_one → 随机唤醒一个，生产一个任务唤醒一个 consumer 处理
//    notify_all → 唤醒所有，广播通知（如初始化完成、程序退出）
//
// ============================================================
 
int main()
{
    scene1();
    scene2();
    scene3();
    return 0;
}