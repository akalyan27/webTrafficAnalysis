/*
The generic, thread-safe bounded buffer that decouples the 
I/O thread (Producer) from the CPU worker threads (Consumers).
*/

#pragma once 

#include <queue>
#include <mutex>
#include <condition_variable>

template <typename T>

class SafeQueue {
private:
    std::queue<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    bool m_stop = false;

public:
    void push(T item) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_stop) return;

        //m_queue.push(item)
        /* lesson
        we use std::move to avoid making an extra copy of the item
        -  the parameter would be a copy but std::move treats the item 
            as if it werea temporary object 
        - essentially says the object on the right is going to go away 
           so you can steal its internal pointers or memory so no need to copy
        */ 
        m_queue.push(std::move(item));
        m_condition.notify_one();
    }
    /**
     * @brief Pop an item from the queue
     * @param item Reference to store the popped value
     * @return True if an item was popped, false if the queue is stopped and empty
     */
    bool pop(T& item) {
        std::unique_lock<std::mutex> lock(m_mutex);

        //wait for queue to be non-empty or stop request
        m_condition.wait(lock, [this] 
            { return m_stop || !m_queue.empty(); }
        );

        if (m_stop && m_queue.empty()) return false;

        // store popped element in the reference parameter
        item = std::move(m_queue.front());
        m_queue.pop();
        return true
    }

    /**
     * signals queue to stop, wakes up all waiting threads
     * this is called by the producer thread once all input has been processed
    */
    void stop() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_stop = true;
        m_condition.notify_all();
    }
};