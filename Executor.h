//
// Created by Shawn Zhao on 2023/8/10.
//

#ifndef MYTASKFLOW_EXECUTOR_H
#define MYTASKFLOW_EXECUTOR_H
#include <thread>
#include "MsgQueue.h"

class ThreadPool;
namespace Sawn {
    class Task;
    class Executor {
    public:
        Executor();
        ~Executor();
        void run(Task* taskf);
        void spawn(Task* task);
        void notify(const Msg& msg);
    protected:
        size_t totalUnFinishedTaskNum_ = 0;
        std::condition_variable condition_;
        ThreadPool* pool_ = nullptr;
        MsgQueue queue_; // 保护Msg队列，记录任务与完成状态

    };
}
#endif //MYTASKFLOW_EXECUTOR_H
