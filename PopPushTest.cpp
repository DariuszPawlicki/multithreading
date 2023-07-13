#include <queue>
#include <mutex>
#include <memory>
#include <future>
#include <cassert>
#include <iostream>
#include <gtest/gtest.h>


template<typename T>
class ConcurrentList {
public:
    ConcurrentList() = default;

    void push(const T& item){
        std::unique_ptr<Node> new_node = std::make_unique<Node>(item);

        std::lock_guard<std::mutex> lock{head.mutex};

        new_node->next = std::move(head.next);
        head.next = std::move(new_node);
    }

    template<typename Pred>
    std::shared_ptr<T> getItemIfExsits(Pred pred) {
        Node* current = &head;
        std::unique_lock<std::mutex> lock{head.mutex};

        while(Node* next = current->next.get()) {
            std::unique_lock<std::mutex> next_lock{next->mutex};
            lock.unlock();

            if(pred(*next->data)){
                return next->data;
            }

            current = next;
            lock = std::move(next_lock);
        }

        return std::shared_ptr<T>();
    }

    template<typename Pred>
    void removeIfExists(Pred pred) {
        Node* current = &head;
        std::unique_lock<std::mutex> lock{head.mutex};

        while(Node* next = current->next.get()) {
            std::unique_lock<std::mutex> next_lock{next->mutex};

            if(pred(*next->data)) {
                std::unique_ptr<Node> old_next = std::move(current->next);
                current->next = std::move(old_next->next);
                next_lock.unlock();
            }
            else {
                lock.unlock();
                current = next;
                lock = std::move(next_lock);
            }
        }
    }

    bool empty() const {
        return head.next == nullptr;
    }

private:
    struct Node{
        std::unique_ptr<Node> next;
        std::shared_ptr<T> data;
        std::mutex mutex;

        Node() : next() {}
        explicit Node(const T& value) :
                data(std::make_shared<T>(value)) {}
    };

    Node head;
};


void testConcurrentPushAndFindList() {
    std::promise<void> threads_ready;
    std::promise<void> push_thread_ready;
    std::promise<void> find_thread_ready;
    std::shared_future<void> semaphore{threads_ready.get_future()};
    std::future<void> push_done;
    std::future<std::string> search_done;
    const std::string val{"xD"};

    ConcurrentList<std::string> list;

    try {
        push_done = std::async(std::launch::async, [&list, &push_thread_ready, &val, semaphore]{
            push_thread_ready.set_value();
            semaphore.wait();
            list.push(val);
        });

        search_done = std::async(std::launch::async, [&list, &find_thread_ready, &val, semaphore]{
           find_thread_ready.set_value();
           semaphore.wait();
           return *list.getItemIfExsits([&val](const std::string& item) {
               return item == val;
           });
        });

        push_thread_ready.get_future().wait();
        find_thread_ready.get_future().wait();
        threads_ready.set_value();
        push_done.get();

        assert(search_done.get() == val);

        std::cout << "Test passed" << std::endl;
    }
    catch(const std::exception& e) {
        threads_ready.set_value();
        std::cout << e.what() << std::endl;
        std::cout << "Test failed" << std::endl;
    }
}


int main() {
//    std::promise<int> p;
//    std::future<int> f{p.get_future()};
//
//    p.set_value(5);
//    f.wait();
//    std::cout << f.get() << std::endl;
    testConcurrentPushAndFindList();
}