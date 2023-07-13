#include <atomic>
#include <memory>


template<typename T>
class LockFreeStack {
public:
    void push(const T& data) {
        Node* new_node = new Node(data);
        new_node->next = head.load();

        while(!head.compare_exchange_weak(new_node->next, new_node));
    }

    std::shared_ptr<T> pop() {
        ++popping_threads;
        Node* old_head = head.load();
        while(old_head && !head.compare_exchange_weak(old_head, old_head->next));
        std::shared_ptr<T> result;

        if(old_head) {
            result.swap(old_head->data);
        }

        tryReclaim(old_head);
        return result;
    }

private:
    struct Node {
        std::shared_ptr<T> data;
        Node* next;

        Node(const T& data) : data(std::make_shared<T>(data)) {}
    };

    static void deleteNodes(Node* nodes) {
        while(nodes) {
            Node* next = nodes->next;
            delete nodes;
            nodes = next;
        }
    }

    void tryReclaim(Node* old_head) {
        
    }

    std::atomic<Node*> head;
    std::atomic<unsigned int> popping_threads{0};
    std::atomic<Node*> to_delete;
};

int main() {

}