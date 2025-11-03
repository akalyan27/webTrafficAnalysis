/*
Implementation of the worker loop where we fetch and execute 
tasks from the queue

threadPool.cpp
Implementation of the worker loop where we fetch and execute 
tasks from the queue using a user-provided handler.
*/

#include "ThreadPool.h"
#include <iostream>

// The provided main.cpp already includes the necessary dependencies:
// SafeQueue.h, PlayerCommand.h, ThreadPool.h, GameState.h

// Constructor
ThreadPool::ThreadPool(size_t num_threads, CommandQueue& command_queue_ref, WorkerTaskFunc worker_func)
    : m_command_queue(command_queue_ref), 
      m_worker_task_func(worker_func),
      m_num_threads(num_threads),
      m_joined(false) 
{
    // The threads are created and launched in the start() method.
}

// Destructor
ThreadPool::~ThreadPool() {
    // IMPORTANT: Destructor should NEVER run if join() was properly called
    // If this destructor runs and threads are still active, something went wrong
    // We can't safely join here because the queue might already be destroyed
    if (!m_joined.load()) {
        std::cerr << "[ThreadPool] ERROR: Destructor called without join()! This should not happen." << std::endl;
        // Don't try to join here - it's too late, objects may be destroyed
        // Just detach threads to avoid std::terminate (but this is a last resort)
        for (std::thread& worker : m_threads) {
            if (worker.joinable()) {
                worker.detach(); // Detach to avoid terminate, but this is bad practice
            }
        }
    }
}


/**
 * @brief The main function executed by each worker thread.
 * It simply calls the user-provided function, which contains the queue processing loop.
 */
void ThreadPool::worker_loop() {
    // Execute the user-defined task handler function, passing the command queue reference.
    // In this case, this handler contains the while (pop) loop from main.cpp.
    m_worker_task_func(m_command_queue);
}


/**
 * @brief Creates and launches all worker threads, starting the consumption process.
 */
void ThreadPool::start() {
    for (size_t i = 0; i < m_num_threads; ++i) {
        // Create a new thread and move it into the vector.
        // The worker_loop function is executed when the thread starts.
        m_threads.emplace_back(&ThreadPool::worker_loop, this);
    }
}


/**
 * @brief Blocks until all worker threads complete their execution.
 */
void ThreadPool::join() {
    // Atomically check if already joined and set to true
    if (m_joined.exchange(true)) {
        // Already joined, nothing to do
        return;
    }

    // Join all threads safely - wait for each one to fully exit
    for (std::thread& worker : m_threads) {
        if (worker.joinable()) {
            try {
                // This will block until the thread exits
                // At this point, the worker should have exited its loop and released all locks
                worker.join(); 
            } catch (const std::system_error& e) {
                // Log but don't crash if join fails
                std::cerr << "[ThreadPool] Warning: Failed to join thread: " << e.what() << std::endl;
                // If join fails, try to detach to avoid terminate
                if (worker.joinable()) {
                    worker.detach();
                }
            }
        }
    }
    
    // Clear the thread vector after all threads are joined
    // This ensures destructor won't try to join again
    m_threads.clear();
}