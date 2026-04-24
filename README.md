# cpp-syx

三、future 和 promise：一次性通知

四、packaged_task：把任务和 future 绑定

五、async 是最简洁的方式

六、时间相关的等待

必须掌握：
  atomic 基本用法，默认 seq_cst 保证正确性
  memory_order_relaxed 用在计数器场景
  memory_order_acquire/release 用在生产者消费者场景

了解即可：
  memory_order_consume（实际很少用）
  memory_order_acq_rel
  底层硬件细节
  
  
atomic<T> 的基本用法
memory_order 三个档位
happens-before 关系
CAS（compare_exchange）
atomic_thread_fence（栅栏）
