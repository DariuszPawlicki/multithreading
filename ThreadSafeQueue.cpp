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


template<typename T>
class FineGrainedQueue {
public:
	FineGrainedQueue() : head(std::make_unique<Node>()), tail(head.get()) {};
	FineGrainedQueue(const FineGrainedQueue& rhs) = delete;
	FineGrainedQueue& operator=(const FineGrainedQueue& rhs) = delete;

	void push(T new_value) {
		std::shared_ptr<T> new_data = std::make_shared<T>(std::move(new_value));

		std::unique_ptr<Node> new_tail = std::make_unique<Node>();

        Node* const  new_tail_ptr = new_tail.get();

		{
			std::lock_guard lock{ tail_mutex };

			tail->data = new_data;
			tail->next = std::move(new_tail);
			tail = new_tail_ptr;
		}

		cond_var.notify_one();
	}

	std::shared_ptr<T> tryPop() {
		std::unique_ptr<Node> old_head = tryPopHead(); // deletion outside lock
		return old_head->data ? old_head->data : std::shared_ptr<T>();
	}

	bool tryPop(T& value) {
		std::unique_ptr<Node> old_head = tryPopHead(value);
		return old_head != nullptr;
	}

	std::shared_ptr<T> waitAndPop() {
        const std::unique_ptr<Node> old_head = waitPopHead();
		return old_head->data;
	}

	void waitAndPop(T& value) {
        const std::unique_ptr<Node> old_head = waitPopHead(value);
	}

	bool empty() {
		std::lock_guard lock{ head_mutex };

		return head.get() == getTail();
	}

private:
	struct Node {
		std::shared_ptr<T> data;
		std::unique_ptr<Node> next;
	};

	std::unique_ptr<Node> tryPopHead() {
		std::lock_guard lock{ head_mutex };
		return (head.get() != getTail()) ? popHead() : std::unique_ptr<Node>();
	}

	std::unique_ptr<Node> tryPopHead(T& value) {
		std::lock_guard lock{ head_mutex };

		if (head.get() == getTail()) {
			return std::unique_ptr<Node>();
		}

		value = std::move(*(head->data));

		return popHead();
	}

	std::unique_ptr<Node> waitPopHead() {
		std::unique_lock<std::mutex> lock{ waitForData() };
		return popHead();
	}

	std::unique_ptr<Node> waitPopHead(T& value) {
		std::unique_lock<std::mutex> lock{ waitForData() };
		value = std::move(*head->data);

		return popHead();
	}

	std::unique_lock<std::mutex> waitForData() {
		std::unique_lock<std::mutex> lock{ head_mutex };

		cond_var.wait(lock, [this]() {
			return head.get() != getTail();
			});

		return std::move(lock);
	}

	std::unique_ptr<Node> popHead() {
		std::unique_ptr<Node> old_head = std::move(head);
		head = std::move(old_head->next);
		return old_head;
	}

	Node* getTail() {
		std::lock_guard lock{ tail_mutex };
		return tail;
	}

	std::mutex head_mutex;
	std::mutex tail_mutex;
	std::condition_variable cond_var;
	std::unique_ptr<Node> head;
	Node* tail;
};

int main() {
	/*ThreadSafeQueue<int> q;

	q.push(5);

	std::cout << *q.waitAndPop() << std::endl;*/
	FineGrainedQueue<int> q;

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