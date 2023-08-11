#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "ThreadPool.h"

template <typename T>
static T add(T x, T y) {
    return x + y;
}
template <typename T>
static T sub(T x, T y) {
    return x - y;
}
template <typename T>
static T mul(T x, T y) {
    return x * y;
}

int sum(int a, int b) { return add(a, b) + sub(a, b) + mul(a, b); }

int main()
{
    auto threadPool = ThreadPool(4);
    int a = 10;
    int b = 20;

    std::vector<std::future<int>> futures;
    auto add_fut = threadPool.enqueue([a, b]() { return add(a, b); });
    auto sub_fut = threadPool.enqueue([a, b]() { return sub(a, b); });
    auto mul_fut = threadPool.enqueue([a, b]() { return mul(a, b); });
    auto params_fut = threadPool.enqueue(sum, a, b);
    futures.push_back(std::move(add_fut));
    futures.push_back(std::move(sub_fut));
    futures.push_back(std::move(mul_fut));
    futures.push_back(std::move(params_fut));

    for (auto& future : futures) {
        try {
            int result = future.get();
            std::cout << "Result: " << result << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Exception occurred: " << e.what() << std::endl;
        }
    }

    return 0;
}