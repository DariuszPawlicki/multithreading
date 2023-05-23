#include <queue>
#include <mutex>
#include <memory>
#include <future>
#include <cassert>
#include <iostream>
#include <condition_variable>


template<typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() {}

    ThreadSafeQueue(const ThreadSafeQueue& rhs) {
        data = rhs.data;
    }

    ThreadSafeQueue& operator=(const ThreadSafeQueue& rhs) = delete;

    void push(T value) {
        auto new_value = std::make_shared<T>(std::move(value));
        std::lock_guard lock{ m };
        data.push(new_value);
        cond_var.notify_one();
    }

    std::shared_ptr<T> waitAndPop() {
        std::unique_lock lock{ m };
        cond_var.wait(lock, [&] { return !data.empty(); });

        auto popped = data.front();
        data.pop();

        return popped;
    }

    void waitAndPop(T& value) {
        std::unique_lock lock{ m };
        cond_var.wait(lock, [&] { return !data.empty(); });

        value = std::move(*data.front());
        data.pop();
    }

    std::shared_ptr<T> tryPop() {
        std::lock_guard lock{ m };
        if (data.empty()) {
            return std::shared_ptr<T>();
        }

        auto popped = data.front();
        data.pop();

        return popped;
    }

    bool tryPop(T& value) {
        std::lock_guard lock{ m };
        if (data.empty()) {
            return false;
        }

        value = std::move(*data.front());
        data.pop();

        return true;
    }

    bool empty() const {
        std::lock_guard lock{ m };
        return data.empty();
    }

private:
    mutable std::mutex m;
    std::queue<std::shared_ptr<T>> data;
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

        Node* const new_tail_ptr = new_tail.get();

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


void testConcurrentPushAndPopOnEmptyQueue() {
    ThreadSafeQueue<int> q;

    std::promise<void> go, push_ready, pop_ready;
    std::shared_future<void> ready{go.get_future()};
    std::future<void> push_done;
    std::future<int> pop_done;

    try {
        push_done = std::async(std::launch::async, [&q, ready, &push_ready]{
            push_ready.set_value();
            ready.wait();
            q.push(40);
        });

        pop_done = std::async(std::launch::async, [&q, ready, &pop_ready]{
           pop_ready.set_value();
           ready.wait();
           return *q.tryPop();
        });

        push_ready.get_future().wait();
        pop_ready.get_future().wait();
        go.set_value();
        push_done.get();
        assert(pop_done.get() == 40);
        assert(q.empty());
    }
    catch(...) {
        go.set_value();
        throw;
    }
}


int main() {
    std::promise<int> p;
    std::future<int> f{p.get_future()};

    p.set_value(5);
    f.wait();

    std::cout << f.get() << std::endl;
}