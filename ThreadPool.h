//
// Created by Shawn Zhao on 2023/8/10.
//

#ifndef MYTASKFLOW_THREADPOOL_H
#define MYTASKFLOW_THREADPOOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

class ThreadPool {
public:
    ThreadPool(size_t);
    ~ThreadPool();

    // bug2
    template<typename F, typename ... Args>
    std::future<typename std::invoke_result<F, Args...>::type> enqueue(F&& f, Args&&... args);

private:
    std::vector<std::thread> workers;

    std::queue<std::function<void()>> tasks;

    std::mutex queue_mutex;
    std::condition_variable condtion;
    bool stop;
};

inline ThreadPool::ThreadPool(size_t threads) : stop(false) {
    for (size_t i = 0; i < threads; ++i) {
        workers.emplace_back([this] {
            for (;;) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    // 会在当前状态阻塞，知道条件成立，stop 线程池停止或者任务队列为空
                    this -> condtion.wait(lock,
                                        [this] { return this->stop || !this->tasks.empty(); });
                    if (this -> stop && this -> tasks.empty()) {
                        return;
                    }
                    // 选择任务队列第一，准备执行
                    task = std::move(this -> tasks.front());
                    this->tasks.pop();
                }
                task();
            }
        });
    }
}
/*
template<typename F, typename ... Args>
std::future<typename std::invoke_result<F, Args...>::type> ThreadPool::enqueue(F&& f, Args&&... args)
{
    using return_type = typename std::invoke_result<F, Args...>::type;
    std::promise<return_type> promise;
    std::future<return_type> future = promise.get_future();

    auto task = [f = std::forward<F>(f),
            tup = std::make_tuple(std::forward<Args>(args)...),
            promise = std::move(promise)] () mutable {
        try {
            if constexpr (!std::is_void_v<return_type>) {
                return_type result = std::apply(std::move(f), std::move(tup));
                promise.set_value(result);
            } else {
                std::apply(std::move(f), std::move(tup));
            }
        } catch (...) {
            promise.set_exception(std::current_exception());
        }
    };

    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        if (stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");
//        tasks.emplace(task);
        tasks.emplace(task);
    }
    condtion.notify_one();
    return future;
}
*/
/*
template<typename F, typename ... Args>
std::future<typename std::invoke_result<F, Args...>::type> ThreadPool::enqueue(F&& f, Args&&... args)
{
    using return_type = typename std::invoke_result<F, Args...>::type;
    auto task = std::make_shared< std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<return_type> res = task -> get_future();
    // 对的，这行代码在创建 `std::packaged_task` 后，使用 `get_future()` 函数获取一个与该任务相关联的 `std::future` 对象，
    // 以便稍后可以通过这个 `std::future` 对象来获取异步操作的结果。这是一种将任务和其结果关联起来的方式，方便管理异步操作的执行和结果获取。
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        // don't allow enqueueing after stopping the pool
        if(stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");

//        tasks.emplace([task](){ (*task)(); });
        // 有一个lambda表达式，返回值是task，无参数，执行*task任务，所以我这里想加上对于函数返回值的判断
        tasks.emplace([task]() {
            (*task)(); // 执行task
            if constexpr (!std::is_void_v<return_type>) { // 有返回值
                res.set_value(task -> get_return_value());
            }
        });
    }
    condtion.notify_one();
    return res;
}
*/

template<typename F, typename ... Args>
std::future<typename std::invoke_result<F, Args...>::type> ThreadPool::enqueue(F&& f, Args&&... args)
{
    using return_type = typename std::invoke_result<F, Args...>::type;
    auto task = std::make_shared< std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<return_type> res = task -> get_future();
    // 对的，这行代码在创建 `std::packaged_task` 后，使用 `get_future()` 函数获取一个与该任务相关联的 `std::future` 对象，
    // 以便稍后可以通过这个 `std::future` 对象来获取异步操作的结果。这是一种将任务和其结果关联起来的方式，方便管理异步操作的执行和结果获取。
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        // don't allow enqueueing after stopping the pool
        if(stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");

//        tasks.emplace([task](){ (*task)(); });
        // 有一个lambda表达式，返回值是task，无参数，执行*task任务，所以我这里想加上对于函数返回值的判断
        tasks.emplace([task](){ (*task)(); });
    }
    condtion.notify_one();
    return res;
}

inline ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condtion.notify_all();
    for(std::thread &worker: workers)
        worker.join();
}

#endif //MYTASKFLOW_THREADPOOL_H
