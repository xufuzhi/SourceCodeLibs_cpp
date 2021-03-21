#include <iostream>
#include "string"
#include <random>
#include "ThreadPool.hpp"

using namespace std;

// 全局锁
std::mutex  mtx;

// 不返回结果
void print_int(int a)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    unique_lock<mutex> lock(mtx);   // 加锁防止打印错乱
    cout << "a: " << a << endl;
}

// 直接返回结果
int test_return(int a, int b)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    return a + b;
}

// 通过引用和return返回结果
int test_ref(int a, int& ref_1, int& ref_2)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    ref_1 = a + 100;
    ref_2 = a + 1000;
    return a + 10000;
}

int main()
{
    // 创建3个线程的线程池
    ThreadPool pool(3);

    // 提交10个不返回结果的任务
    for (int i = 0; i < 10; i++) {
        pool.submit(print_int, i);
    }
    this_thread::sleep_for(chrono::seconds(5)); // 等待所有任务完成

    // 提交带return的函数
    auto future1 = pool.submit(test_return, 10, 20);
    // 等待任务完成，并取出结果
    int res = future1.get();
    std::cout << "Last operation result is equals to： " << res << std::endl;

    // 通过引用和return返回结果
    int output_ref_1, output_ref_2;
    auto future2 = pool.submit(test_ref, 8, std::ref(output_ref_1),  std::ref(output_ref_2));
    // 等待任务完成
    int out = future2.get();
    std::cout << "output_ref_1: " << output_ref_1 << "    output_ref_2: " << output_ref_2 << "    output_return: " << out << std::endl;

    return 0;
}
