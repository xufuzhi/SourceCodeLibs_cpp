//
// Created by xfz on 2021/3/21.
//

#ifndef THREADPOOL_THREADPOOL_HPP
#define THREADPOOL_THREADPOOL_HPP

#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

class ThreadPool {
private:
    // 线程池最大线程数量
    const uint32_t MAX_THREAD_NUM = 256;

    void looprun()
    {
        std::function<void()> func; //定义基础函数类func
        bool if_success {false}; // 是否成功取出任务

        //判断线程池是否关闭，没有关闭，循环提取
        while (!this->m_shutdown)
        {
            {
                std::unique_lock<std::mutex> lock(this->m_conditional_mutex);   // 加锁
                //如果任务队列为空，阻塞当前线程
                if (this->tasks.empty()) {
                    this->m_conditional_lock.wait(lock); //等待条件变量通知，开启线程
                }
                if (this->m_shutdown)
                    return;
                //取出任务队列中的元素
                if (!tasks.empty()) {
                    func = std::move(this->tasks.front()); // 取一个 task
                    this->tasks.pop();
                    if_success = true;
                } else{
                    if_success = false;
                }
            }
            //如果成功取出，执行工作函数
            if (if_success) {
                func();
            }
        }
    }

    std::atomic<bool> m_shutdown; // 线程池是否关闭
    std::queue<std::function<void()>> tasks; // 任务队列, std::queue线程不安全，操作需要加锁
    std::vector<std::thread> threads; // 工作线程队列
    std::mutex m_conditional_mutex; // 线程休眠锁互斥变量
    std::condition_variable m_conditional_lock; // 线程环境锁，让线程可以处于休眠或者唤醒状态

public:
    //线程池构造函数
    ThreadPool(const int n_threads) : threads(std::vector<std::thread>(n_threads)), m_shutdown(false)
    {
        if (n_threads > MAX_THREAD_NUM) { std::cout << "警告： 线程池过大，建议重新构造线程池！" << std::endl; }

        for (int i = 0; i < threads.size(); ++i) {
            threads[i] = std::thread([this](){ this->looprun(); });     //分配工作线程
        }
    }

    ThreadPool(const ThreadPool &) = delete; //拷贝构造函数，并且取消默认父类构造函数
    ThreadPool(ThreadPool &&) = default; // 拷贝构造函数，允许右值引用

    ThreadPool & operator=(const ThreadPool &) = delete; // 赋值操作
    ThreadPool & operator=(ThreadPool &&) = delete; //赋值操作

    ~ThreadPool()
    {
        m_shutdown = true;
        m_conditional_lock.notify_all(); //通知 唤醒所有工作线程
        // Waits until threads finish their current task and shutdowns the pool
        for (int i = 0; i < threads.size(); ++i) {
            if(threads[i].joinable()) { //判断线程是否正在等待
                threads[i].join();  //将线程加入等待队列
            }
        }
    }

    /*! Submit a function to be executed asynchronously by the pool
     * 不支持直接接受类非静态成员函数。有三种方法可以实现接受类非静态成员函数：
     *      1. submit([&](){ dog.sayHello(); });
     *      2. submit(std::bind(&Dog::sayHello, &dog));
     *      3. submit(std::mem_fn(&Dog::sayHello), &dog);
     *
     * @tparam F
     * @tparam Args
     * @param f
     * @param args
     * @return
     */
    template<typename F, typename...Args>
    auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        // Create a function with bounded parameters ready to execute
        // 绑定函数和实参
        std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        // Encapsulate it into a shared ptr in order to be able to copy construct / assign
        auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);
        // Wrap packaged task into void function
        std::function<void()> wrapper_func = [task_ptr]() {
            (*task_ptr)();
        };

        {
            std::unique_lock<std::mutex> lock(m_conditional_mutex);   // 加锁
            tasks.emplace(wrapper_func);    // 压入队列
        }
        // 唤醒一个等待中的线程
        m_conditional_lock.notify_one();
        // 返回先前注册的任务指针
        return task_ptr->get_future();
    }

};

#endif //THREADPOOL_THREADPOOL_HPP
