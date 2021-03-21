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
    // �̳߳�����߳�����
    const uint32_t MAX_THREAD_NUM = 256;

    void looprun()
    {
        std::function<void()> func; //�������������func
        bool if_success {false}; // �Ƿ�ɹ�ȡ������

        //�ж��̳߳��Ƿ�رգ�û�йرգ�ѭ����ȡ
        while (!this->m_shutdown)
        {
            {
                std::unique_lock<std::mutex> lock(this->m_conditional_mutex);   // ����
                //����������Ϊ�գ�������ǰ�߳�
                if (this->tasks.empty()) {
                    this->m_conditional_lock.wait(lock); //�ȴ���������֪ͨ�������߳�
                }
                if (this->m_shutdown)
                    return;
                //ȡ����������е�Ԫ��
                if (!tasks.empty()) {
                    func = std::move(this->tasks.front()); // ȡһ�� task
                    this->tasks.pop();
                    if_success = true;
                } else{
                    if_success = false;
                }
            }
            //����ɹ�ȡ����ִ�й�������
            if (if_success) {
                func();
            }
        }
    }

    std::atomic<bool> m_shutdown; // �̳߳��Ƿ�ر�
    std::queue<std::function<void()>> tasks; // �������, std::queue�̲߳���ȫ��������Ҫ����
    std::vector<std::thread> threads; // �����̶߳���
    std::mutex m_conditional_mutex; // �߳��������������
    std::condition_variable m_conditional_lock; // �̻߳����������߳̿��Դ������߻��߻���״̬

public:
    //�̳߳ع��캯��
    ThreadPool(const int n_threads) : threads(std::vector<std::thread>(n_threads)), m_shutdown(false)
    {
        if (n_threads > MAX_THREAD_NUM) { std::cout << "���棺 �̳߳ع��󣬽������¹����̳߳أ�" << std::endl; }

        for (int i = 0; i < threads.size(); ++i) {
            threads[i] = std::thread([this](){ this->looprun(); });     //���乤���߳�
        }
    }

    ThreadPool(const ThreadPool &) = delete; //�������캯��������ȡ��Ĭ�ϸ��๹�캯��
    ThreadPool(ThreadPool &&) = default; // �������캯����������ֵ����

    ThreadPool & operator=(const ThreadPool &) = delete; // ��ֵ����
    ThreadPool & operator=(ThreadPool &&) = delete; //��ֵ����

    ~ThreadPool()
    {
        m_shutdown = true;
        m_conditional_lock.notify_all(); //֪ͨ �������й����߳�
        // Waits until threads finish their current task and shutdowns the pool
        for (int i = 0; i < threads.size(); ++i) {
            if(threads[i].joinable()) { //�ж��߳��Ƿ����ڵȴ�
                threads[i].join();  //���̼߳���ȴ�����
            }
        }
    }

    /*! Submit a function to be executed asynchronously by the pool
     * ��֧��ֱ�ӽ�����Ǿ�̬��Ա�����������ַ�������ʵ�ֽ�����Ǿ�̬��Ա������
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
        // �󶨺�����ʵ��
        std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        // Encapsulate it into a shared ptr in order to be able to copy construct / assign
        auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);
        // Wrap packaged task into void function
        std::function<void()> wrapper_func = [task_ptr]() {
            (*task_ptr)();
        };

        {
            std::unique_lock<std::mutex> lock(m_conditional_mutex);   // ����
            tasks.emplace(wrapper_func);    // ѹ�����
        }
        // ����һ���ȴ��е��߳�
        m_conditional_lock.notify_one();
        // ������ǰע�������ָ��
        return task_ptr->get_future();
    }

};

#endif //THREADPOOL_THREADPOOL_HPP
