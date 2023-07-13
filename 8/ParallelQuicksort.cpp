#include <list>
#include <stack>
#include <future>
#include <vector>
#include <memory>
#include <iostream>


template<typename T>
class ThreadSafeStack {
public:
    ThreadSafeStack() {};
    ThreadSafeStack(const ThreadSafeStack& rhs) {
        std::lock_guard lock{ m };
        data = rhs.data;
    }

    ThreadSafeStack& operator=(const ThreadSafeStack& rhs) = delete;

    void push(T item) {
        std::shared_ptr<T> item_ptr = std::make_shared<T>(item);
        std::lock_guard lock{ m };
        data.push(item_ptr);
        cond_var.notify_one();
    }

    bool tryPop(T& value) {
        std::scoped_lock lock{ m };
        if (data.empty()) {
            return false;
        }

        value = std::move(data.top());
        data.pop();

        return true;
    }

    std::shared_ptr<T> tryPop() {
        std::scoped_lock lock{ m };
        if (data.empty()) {
            return std::shared_ptr<T>();
        }

        std::shared_ptr<T> popped{data.top()};
        data.pop();

        return popped;
    }

    void waitAndPop(T& value) {
        std::unique_lock lk{m};
        cond_var.wait(lk, [&]{
            return !data.empty();
        });

        value = *data.top();
        data.pop();
    }

    std::shared_ptr<T> waitAndPop() {
        std::unique_lock lk{m};
        cond_var.wait(lk, [&]{
            return !data.empty();
        });

        std::shared_ptr<T> popped{data.top()};
        data.pop();

        return popped;
    }


    bool empty() const {
        std::lock_guard lock{ m };
        return data.empty();
    }

private:
    std::stack<std::shared_ptr<T>> data;
    mutable std::mutex m;
    std::condition_variable cond_var;
};


template<typename T>
struct Sorter {
    struct ChunkToSort {
        std::list<T> data;
        std::promise<std::list<T>> promise;

        ChunkToSort() = default;

        ChunkToSort(const ChunkToSort& other) {
            data = other.data;
        }
    };

    ThreadSafeStack<ChunkToSort> chunks;
    std::vector<std::thread> threads;
    const unsigned int max_threads_count;
    std::atomic<bool> end_of_data;

    Sorter() :
        max_threads_count(std::thread::hardware_concurrency() - 1),
        end_of_data(false) {}

    ~Sorter() {
        end_of_data = true;
        auto m = std::make_shared<ChunkToSort>();
        for(auto& t : threads) {
            t.join();
        }
    }

    std::list<T> doSort(std::list<T>& chunk_data) {
        if (chunk_data.empty()) {
            return chunk_data;
        }

        std::list<T> result;
        result.splice(result.begin(), chunk_data, chunk_data.begin());

        const T& partition_value = *result.begin();

        typename std::list<T>::iterator divide_point = std::partition(chunk_data.begin(), chunk_data.end(),
                                                                      [&](const T& value) { return value < partition_value; });

        ChunkToSort new_lower_chunk;
        new_lower_chunk.data.splice(new_lower_chunk.data.end(), chunk_data,
                                    chunk_data.begin(),
                                    divide_point);

        std::future<std::list<T>> new_lower = new_lower_chunk.promise.get_future();

        chunks.push(std::move(new_lower_chunk));

        if (threads.size() < max_threads_count) {
            threads.push_back(std::thread{&Sorter<T>::sortThread, this});
        }

        std::list<T> new_higher{doSort(chunk_data)};
        result.splice(result.end(), new_higher);

        while(new_lower.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
            trySortChunk();
        }

        result.splice(result.begin(), new_lower.get());
        return result;
    }

    void sortChunk(std::shared_ptr<ChunkToSort>& chunk) {
        chunk->promise.set_value(doSort(chunk->data));
    }

    void trySortChunk() {
        std::shared_ptr<ChunkToSort> chunk = chunks.tryPop();

        if(chunk) {
            sortChunk(chunk);
        }
    }

    void sortThread() {
        while(!end_of_data) {
            trySortChunk();
            std::this_thread::yield();
        }
    }
};

template<typename T>
std::list<T> parallelQuickSort(std::list<T> data) {
    if(data.empty()) {
        return data;
    }

    Sorter<T> sorter;
    return sorter.doSort(data);
}

int main() {
    auto sorted = parallelQuickSort<int>({9, -1, 2, 41, 30});

    for(const auto& s : sorted) {
        std::cout << s << std::endl;
    }
    return 0;
}