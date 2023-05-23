#include <queue>
#include <mutex>
#include <memory>
#include <iostream>
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


int main() {
	ThreadSafeQueue<int> q;

	std::thread t1{ [&] {
		q.push(5);
		q.push(10);
	} };

	std::cout << *q.waitAndPop() << std::endl;
	std::cout << *q.waitAndPop() << std::endl;

	t1.join();

	q.push(20);
	q.push(30);

	int val;

	q.tryPop(val);

	std::cout << val << std::endl;
	std::cout << *q.tryPop() << std::endl;
}