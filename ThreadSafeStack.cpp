#include <stack>
#include <mutex>
#include <memory>
#include <exception>
#include <condition_variable>


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


int main() {
	ThreadSafeStack<int> st;

	st.empty();

}