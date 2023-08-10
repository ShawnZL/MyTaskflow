//
// Created by Shawn Zhao on 2023/8/10.
//
#include "MsgQueue.h"

namespace Sawn {
    void MsgQueue::push(const Msg &task) {
        std::unique_lock<std::mutex> lock(mutex_);
        que_.push(task);
        // 唤醒等待使用相同资源的一个线程，_all是都唤醒
        condition_.notify_one();
    }

    Msg MsgQueue::front() {
        std::unique_lock<std::mutex> lock(mutex_);
        return que_.front();
    }

    Msg MsgQueue::pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        Msg msg = que_.front();
        que_.pop();
        return msg;
    }

    bool MsgQueue::empty() {
        std::unique_lock<std::mutex> lock(mutex_);
        return que_.empty();
    }

    void MsgQueue::wait() {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this]() {
            return (!que_.empty());
        });
    }

    void MsgQueue::stop_wait() {
        std::unique_lock<std::mutex> lock(mutex_);
        stop_ = true;
        condition_.notify_one();
    }
}
