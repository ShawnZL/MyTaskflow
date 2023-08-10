//
// Created by Shawn Zhao on 2023/8/10.
//
#include "Task.h"

namespace Sawn {
    Task::Task(std::function<void()> &&func, size_t id) {
        func_ = func;
        id_ = id;
    }

    size_t Task::getTaskNum() {
        size_t res = 0;
        for (auto& it : children_) {
            // 引用对象，不进行拷贝，高效
            res += it -> getTaskNum();
        }
        res += 1; // self
        return res;
    }

    void Task::addChild(Task * task) {
        children_.push_back(task);
        task -> parent = this;

        unfinished_children_num_ += 1;
    }

    // 先于任务
    void Task::precede(Task *task) {
        this -> successors_.push_back(task);
        task -> dependents_.push_back(this);
        task -> unfinished_dependents_num_ += 1;
    }

    // 后继任务
    void Task::succeed(Task *task) {
        task -> successors_.push_back(this);
        this -> dependents_.push_back(task);
        this -> unfinished_dependents_num_ += 1;
    }

    void Task::run() {
        func_();
    }
}

