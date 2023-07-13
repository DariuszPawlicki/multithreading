#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <vector>
#include <memory>
#include <functional>
#include <condition_variable>


template<typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;
    ThreadSafeQueue(const ThreadSafeQueue& other) {
        std::scoped_lock lk{other.mutex};
        data = other.data;
    }

    void push(T item) {
        std::shared_ptr<T> item_ptr = std::make_shared<T>(item);
        std::scoped_lock lk{mutex};
        data.push(item_ptr);
        cond_var.notify_one();
    }

    bool tryPop(T& value) {
        std::scoped_lock lk{mutex};

        if(data.empty()) {
            return false;
        }

        value = std::move(*data.front());
        data.pop();

        return true;
    }

    std::shared_ptr<T> tryPop() {
        std::scoped_lock lk{mutex};

        if(data.empty()) {
            return std::shared_ptr<T>();
        }

        std::shared_ptr<T> item = data.front();
        data.pop();
        return item;
    }

    void waitAndPop(T& value) {
        std::unique_lock lk{mutex};
        cond_var.wait(lk, [&]{
            return !data.empty();
        });

        value = std::move(*data.front());
        data.pop();
    }

    std::shared_ptr<T> waitAndPop() {
        std::unique_lock lk{mutex};
        cond_var.wait(lk, [&]{
            return !data.empty();
        });

        std::shared_ptr<T> item = data.front();
        data.pop();

        return item;
    }

    bool empty() const {
        std::scoped_lock lk{mutex};
        return data.empty();
    }

private:
    std::queue<std::shared_ptr<T>> data;
    mutable std::mutex mutex;
    std::condition_variable cond_var;
};


class ThreadsJoiner {
public:
    ThreadsJoiner(std::vector<std::thread>& worker_threads) : worker_threads(worker_threads) {}

    ~ThreadsJoiner() {
        for(auto& thread : worker_threads) {
            if(thread.joinable()) {
                thread.join();
            }
        }
    }

private:
    std::vector<std::thread>& worker_threads;
};

class ThreadPool {
public:
    ThreadPool() : done(false), joiner(worker_threads) {
        unsigned int threads_count = std::thread::hardware_concurrency();
        try {
            for(unsigned int i = 0; i < threads_count; ++i) {
                worker_threads.push_back(std::thread(&ThreadPool::workerThread, this));
            }
        }
        catch(...) {
            done.store(true);
            throw;
        }
    }

    ~ThreadPool() {
        done.store(true);
    }

    template<typename Function>
    void addTask(Function f) {
        tasks.push(std::function<void()>(f));
    }

private:
    void workerThread() {
        while(!done.load()) {
            std::function<void()> task;

            if(tasks.tryPop(task)) {
                task();
            }
            else {
                std::this_thread::yield();
            }
        }
    }

    std::atomic<bool> done;
    ThreadSafeQueue<std::function<void()>> tasks;
    std::vector<std::thread> worker_threads;
    ThreadsJoiner joiner;
};

int main() {

}