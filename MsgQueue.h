//
// Created by Shawn Zhao on 2023/8/10.
//

#ifndef MYTASKFLOW_MSGQUEUE_H
#define MYTASKFLOW_MSGQUEUE_H
#include <queue>
#include <mutex>
#include <condition_variable>
namespace Sawn {
    class Task;

    enum class TaskState { FINISH };

    struct Msg {
        Task* task = nullptr;
        TaskState taskState = TaskState::FINISH;
    };

    class MsgQueue {
    public:
        void push(const Msg& task);
        Msg pop();
        Msg front();
        bool empty();
        void wait();
        void stop_wait();
    private:
        std::condition_variable condition_;
        std::mutex mutex_;
        std::queue<Msg> que_;
        bool stop_ = false;
    };
}

#endif //MYTASKFLOW_MSGQUEUE_H
