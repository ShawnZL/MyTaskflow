//
// Created by Shawn Zhao on 2023/8/10.
//
#include "Executor.h"
#include "Task.h"
#include "ThreadPool.h"
#include <cassert>

namespace Sawn {
    Executor::Executor() {
        pool_ = new ThreadPool(std::thread::hardware_concurrency());
    }

    Executor::~Executor() {
        delete pool_;
    }

    void Executor::run(Task *taskf) {
        if (taskf == nullptr) return ;
        totalUnFinishedTaskNum_ = taskf -> getTaskNum();

        spawn(taskf);
        while (totalUnFinishedTaskNum_ > 0) {
            queue_.wait(); // 停止，等到队列非空才继续行动
//            if (queue_.empty()) {
//                assert(false);
//            }

            Msg msg = queue_.front();
            //在pop,之前，先把msg push到队列，从而保证队列始终非空
            if (msg.taskState == TaskState::FINISH) {
                totalUnFinishedTaskNum_ -= 1;
                if (totalUnFinishedTaskNum_ == 0) {
                    break; // 完成任务
                }
                for (Task* task: msg.task -> successors_) {
                    task -> unfinished_dependents_num_ -= 1;
                    if (task -> unfinished_dependents_num_ == 0)
                        spawn(task); // 执行这个任务
                }
                if (msg.task -> parent) {
                    msg.task -> parent -> unfinished_children_num_ -= 1;
                    if (msg.task -> parent -> unfinished_dependents_num_ == 0) {
                        Msg msg2;
                        msg2.task = msg.task;
                        notify(msg2);
                    }
                }
            }
            else {
                assert(totalUnFinishedTaskNum_ == 0);
            }
        }
        queue_.pop();
    }

    void Executor::spawn(Task* task) {
        //如果有子任务，先spawn子任务
        if (!task -> children_.empty()) {
            std::vector<Task*> srcs;
            for (auto it : task->children_) {
                if (it -> dependents_.empty()) {
                    srcs.push_back(it);
                }
            }
            for (auto& it : srcs) {
                spawn(it);
            }
        }
        else {
            auto ftr = pool_ -> enqueue([task, this]() {
                task->run();
                Msg msg;
                msg.task = task;
                notify(msg);
            });
        }
    }

    void Executor::notify(const Msg& msg) {
        queue_.push(msg);
    }

}
