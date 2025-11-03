// Header for the consumer side, managing a fixed set of worker threads.
#pragma once

#include <vector>
#include <thread>
#include <atomic>
#include <functional>
#include <memory>
#include "SafeQueue.h"       // Includes the SafeQueue definition
#include "PlayerCommand.h"   // Includes the PlayerCommand definition

// 1. Define the specific Queue type used by the ThreadPool
using CommandQueue = SafeQueue<PlayerCommand>;

// 2. Define the signature for the worker function (the entire loop)
using WorkerTaskFunc = std::function<void(CommandQueue&)>;


/** The ThreadPool is a consumer of the SafeQueue<PlayerCommand>.\n * It manages a fixed number of worker threads that continuously\n * pull tasks from the queue (via the provided handler) and update\n * shared state.\n*/
class ThreadPool {
private:
    std::vector<std::thread> m_threads;

    // Reference to the thread-safe queue containing commands (tasks)
    CommandQueue& m_command_queue;

    // The user-provided function that each worker thread will execute
    WorkerTaskFunc m_worker_task_func;

    const size_t m_num_threads;
    std::atomic<bool> m_joined;

    /**
     * @brief The main function executed by each worker thread.
     * It simply calls the user-provided worker_task_func.
     */
    void worker_loop();

public:
    /**
     * @brief Constructor for the ThreadPool.
     * @param num_threads The number of worker threads to create.
     * @param command_queue_ref A reference to the CommandQueue containing the PlayerCommands.
     * @param worker_func The function containing the loop logic (e.g., worker_task from main.cpp).
     */
    ThreadPool(size_t num_threads, CommandQueue& command_queue_ref, WorkerTaskFunc worker_func);

    // Destructor ensures that any running threads are joined.
    ~ThreadPool();

    /**
     * @brief Creates and launches all worker threads, starting the consumption process.
     */
    void start();

    /**
     * @brief Blocks until all worker threads complete their execution.
     */
    void join();
};