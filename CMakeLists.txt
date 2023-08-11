cmake_minimum_required(VERSION 3.21)
project(MyTaskflow)

set(CMAKE_CXX_STANDARD 23)

#add_executable(MyTaskflow main.cpp MsgQueue.h Task.h Task.cpp MsgQueue.cpp Executor.h Executor.cpp ThreadPool.h)
add_executable(MyTaskflow main.cpp MsgQueue.h Task.h Task.cpp MsgQueue.cpp ThreadPool.h)