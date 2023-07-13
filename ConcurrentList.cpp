#include <mutex>
#include <memory>
#include <iostream>
#include <shared_mutex>


template<typename T>
class ConcurrentList {
public:
    ConcurrentList() = default;
    ConcurrentList(const ConcurrentList& rhs) = delete;
    ConcurrentList& operator=(const ConcurrentList& rhs) = delete;

    void push(const T& value){
        std::unique_ptr<T> new_node = std::make_unique<Node>(value);

        std::lock_guard<std::mutex> lock{head.mutex};

        new_node->next = std::move(head.next);
        head.next = std::move(new_node);
    }

    template<typename Function>
    void forEach(Function func) {
        Node* current = &head;
        std::unique_lock<std::mutex> lock{head.mutex};

        while(Node* next = current->next.get()){
            std::unique_lock<std::mutex> next_lock{next->mutex};
            lock.unlock();
            func(*next->data);
            current = next;
            lock = std::move(next_lock);
        }
    }

    template<typename Predicate>
    std::shared_ptr<T> getItemIfExsits(Predicate pred) {
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

    template<typename Predicate>
    void removeIfExists(Predicate pred) {
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

private:
    struct Node{
        std::mutex mutex;
        std::shared_ptr<T> data;
        std::unique_ptr<Node> next;

        Node() : next() {}
        explicit Node(const T& value) :
            data(std::make_shared<T>(value)) {}
    };

    Node head;
};

int main() {

}