#include <stack>
#include <mutex>
#include <memory>
#include <exception>


struct EmptyStackException : std::exception {};


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
		std::lock_guard lock{ m };
		data.push(std::move(item));
	}

	std::shared_ptr<T> pop() {
		std::lock_guard lock{ m };
		if (data.empty()) {
			throw EmptyStackException();
		}

		std::shared_ptr<T> popped = std::make_shared<T>(std::move(data.top()));
		data.pop();

		return popped;
	}

	void pop(T& value) {
		std::lock_guard lock{ m };

		if (data.empty()) {
			throw EmptyStackException();
		}

		value = std::move(data.top());
		data.pop();
	}

	bool empty() const {
		std::lock_guard lock{ m };
		return data.empty();
	}

private:
	std::stack<T> data;
	mutable std::mutex m;
};


int main() {
	ThreadSafeStack<int> st;

	st.empty();

}