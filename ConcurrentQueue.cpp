#include <queue>
#include <mutex>
#include <iostream>
#include <condition_variable>


template<typename T>
class ConcurrentQueue {
public:
    ConcurrentQueue() : head(std::make_unique<Node>()), tail(head.get()) {}

    void push(T item) {
        std::shared_ptr<T> item_ptr = std::make_shared<T>(item);
        std::unique_ptr<Node> new_tail = std::make_unique<Node>();

        Node* new_tail_ptr = new_tail.get();

        {
            std::scoped_lock lk{tail_mutex};

            tail->data = item_ptr;
            tail->next = std::move(new_tail);
            tail = new_tail_ptr;
        }

        cond_var.notify_one();
    }

    bool tryPop(T& value) {
        std::unique_ptr<Node> old_head = tryPopHead(value);
        return old_head != nullptr;
    }

    std::shared_ptr<T> tryPop() {
        std::unique_ptr<Node> old_head = tryPopHead();
        return old_head->data ? old_head->data : std::shared_ptr<T>();
    }

    void waitAndPop(T& value) {
        std::unique_ptr<Node> old_head{waitAndPopHead(value)};
    }

    std::shared_ptr<T> waitAndPop() {
        std::unique_ptr<Node> old_head{waitAndPopHead()};
        return old_head->data;
    }

private:
    struct Node {
        std::shared_ptr<T> data;
        std::unique_ptr<Node> next;
    };

    std::unique_ptr<Node> tryPopHead(T& value) {
        std::scoped_lock lk{head_mutex};

        if (head.get() == tail) {
            return std::unique_ptr<T>();
        }

        std::unique_ptr<Node> old_head = popHead();
        value = std::move(*old_head->data);
        return old_head;
    }

    std::unique_ptr<Node> tryPopHead() {
        std::scoped_lock lk{head_mutex};

        if (head.get() == tail) {
            return std::unique_ptr<Node>();
        }

        std::unique_ptr<Node> old_head = popHead();
        head = std::move(old_head->next);

        return old_head;
    }

    std::unique_ptr<Node> waitAndPopHead(T& value) {
        std::unique_lock lk{waitForData()};
        std::unique_ptr<Node> old_head{popHead()};

        value = std::move(*(old_head->data));

        return std::move(old_head);
    }

    std::unique_ptr<Node> waitAndPopHead() {
        std::unique_lock lk{waitForData()};
        return std::move(popHead());
    }

    std::unique_lock<std::mutex> waitForData() {
        std::unique_lock lk {head_mutex};
        cond_var.wait(lk, [&]{
            return head.get() != tail;
        });

        return std::move(lk);
    }

    std::unique_ptr<Node> popHead() {
        std::unique_ptr<Node> old_head = std::move(head);
        head = std::move(old_head->next);
        return old_head;
    }

    Node* getTail() const {
        std::scoped_lock lk{tail_mutex};
        return tail;
    }

    std::mutex head_mutex;
    std::mutex tail_mutex;
    std::unique_ptr<Node> head;
    Node* tail;
    std::condition_variable cond_var;
};

int main() {
    ConcurrentQueue<std::string> q;

    q.push("xd");

    std::cout << *q.tryPop() << std::endl;
}