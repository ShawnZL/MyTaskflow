//
// Created by Shawn Zhao on 2023/8/10.
//

#ifndef MYTASKFLOW_TASK_H
#define MYTASKFLOW_TASK_H
#include <vector>
#include <functional>

namespace Sawn {
    class Executor;
    enum class ETaskType { TASK, TASK_FLOW };

    class Task {
    public:
        // 获取没有完成的任务
        size_t getTaskNum();
        union {
            Task* parent = nullptr;
        };
        size_t id_ = 0;
        ETaskType parentType_ = ETaskType::TASK;
        Executor* executor_;
        std::function<void()> func_;

        std::vector<Task*> children_;
        int unfinished_children_num_ = 0;

        std::vector<Task*> successors_;
        std::vector<Task*> dependents_;

        // 先前的任务没有完成
        int unfinished_dependents_num_ = 0;


    public:
        Task() {}
        // std::function<void()>&& func 可以调用任何对象，右值引用
        Task(std::function<void()>&& func, size_t id = 0);

        void addChild(Task*);

        void precede(Task* task);

        void succeed(Task* task);

        void run();

    };
}

#endif //MYTASKFLOW_TASK_H
