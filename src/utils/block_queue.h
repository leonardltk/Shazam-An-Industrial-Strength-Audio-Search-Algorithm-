#pragma once
#include <brpc/server.h>
#include <unistd.h>
#include <condition_variable>
#include <list>
#include <mutex>
#include <thread>


template<typename T>
class BlockingQueue {
public:
    BlockingQueue(int max_size = 512) : _mutex(), _emptyCV(), _fullCV(), _queue(), _max_size(max_size) {}

    BlockingQueue(BlockingQueue&& rhs) {
        _queue = rhs._queue;
        _max_size = rhs._max_size;
        rhs._queue.clear();
    }

    BlockingQueue& operator=(BlockingQueue&& rhs) {
        _queue = rhs._queue;
        _max_size = rhs._max_size;
        rhs._queue.clear();
        return *this;
    }


    void put(const T& task) {
        std::unique_lock<std::mutex> lock(_mutex);
        _fullCV.wait(lock, [this] { return _queue.size() < _max_size; });
        assert(_queue.size() < _max_size);

        _queue.push_back(task);

        lock.unlock();
        _emptyCV.notify_all();
    }

    T get() {
        std::unique_lock<std::mutex> lock(_mutex);
        _emptyCV.wait(lock, [this] { return !_queue.empty(); });
        assert(!_queue.empty());

        T front(_queue.front());
        _queue.pop_front();

        lock.unlock();
        _fullCV.notify_all();

        return front;
    }

    T get_timeout(int timeout_ms) {
        if (timeout_ms > 0)
            return get_timeout_impl(timeout_ms);
        else
            return get();
    }

    T get_timeout_impl(int timeout_ms) {
        std::unique_lock<std::mutex> lock(_mutex);
        _emptyCV.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this] { return !_queue.empty(); });
        if (_queue.empty())
            return nullptr;

        T front(_queue.front());
        _queue.pop_front();

        lock.unlock();
        _fullCV.notify_all();

        return front;
    }

    size_t size() const {
        // std::lock_guard<std::mutex> lock(_mutex);
        return _queue.size();
    }

    void set_max_size(int size) { _max_size = size; }

private:
    BlockingQueue(const BlockingQueue& rhs);
    BlockingQueue& operator=(const BlockingQueue& rhs);

private:
    mutable std::mutex _mutex;
    std::condition_variable _emptyCV;
    std::condition_variable _fullCV;
    std::list<T> _queue;
    size_t _max_size;
};
