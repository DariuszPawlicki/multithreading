cmake_minimum_required(VERSION 3.16)

project(multithreading-exam)

if(WIN32)
    add_compile_options("/std:c++latest")
endif()

if(UNIX AND NOT APPLE)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

add_executable(joining-thread JoiningThread.cpp)

add_executable(deadlock Deadlock.cpp)

add_executable(data-race DataRace.cpp)

add_executable(spin-lock SpinLock.cpp)

add_executable(unique-lock UniqueLock.cpp)

add_executable(thread-safe-stack ThreadSafeStack.cpp)

add_executable(thread-safe-queue ThreadSafeQueue.cpp)

add_executable(thread-safe-map ThreadSafeMap.cpp)

add_executable(concurrent-map ConcurrentMap.cpp)

add_executable(concurrent-list ConcurrentList.cpp)

add_executable(pop-push-test PopPushTest.cpp)

add_executable(examples Examples.cpp)