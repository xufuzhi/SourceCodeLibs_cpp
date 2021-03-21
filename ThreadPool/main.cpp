#include <iostream>
#include "string"
#include <random>
#include "ThreadPool.hpp"

using namespace std;

// ȫ����
std::mutex  mtx;

// �����ؽ��
void print_int(int a)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    unique_lock<mutex> lock(mtx);   // ������ֹ��ӡ����
    cout << "a: " << a << endl;
}

// ֱ�ӷ��ؽ��
int test_return(int a, int b)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    return a + b;
}

// ͨ�����ú�return���ؽ��
int test_ref(int a, int& ref_1, int& ref_2)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    ref_1 = a + 100;
    ref_2 = a + 1000;
    return a + 10000;
}

int main()
{
    // ����3���̵߳��̳߳�
    ThreadPool pool(3);

    // �ύ10�������ؽ��������
    for (int i = 0; i < 10; i++) {
        pool.submit(print_int, i);
    }
    this_thread::sleep_for(chrono::seconds(5)); // �ȴ������������

    // �ύ��return�ĺ���
    auto future1 = pool.submit(test_return, 10, 20);
    // �ȴ�������ɣ���ȡ�����
    int res = future1.get();
    std::cout << "Last operation result is equals to�� " << res << std::endl;

    // ͨ�����ú�return���ؽ��
    int output_ref_1, output_ref_2;
    auto future2 = pool.submit(test_ref, 8, std::ref(output_ref_1),  std::ref(output_ref_2));
    // �ȴ��������
    int out = future2.get();
    std::cout << "output_ref_1: " << output_ref_1 << "    output_ref_2: " << output_ref_2 << "    output_return: " << out << std::endl;

    return 0;
}
