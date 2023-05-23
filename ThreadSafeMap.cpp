#include <map>
#include <mutex>
#include <iostream>

template<typename Key, typename Value>
class ThreadSafeMap {
public:
    ThreadSafeMap() = default;

    Value operator[](const Key& key) {
        std::scoped_lock lk{mutex};
        return data[key];
    }

    Value at(const Key& key) {
        std::cout << "non-const" << std::endl;
        std::scoped_lock lk{mutex};
        return data.at(key);
    }

    const Value at(const Key& key) const {
        std::cout << "const" << std::endl;
        std::scoped_lock lk{mutex};
        return data.at(key);
    }

private:
    std::map<Key, Value> data;
    mutable std::mutex mutex;
};

template<typename T>
class TD;

int main() {
    const ThreadSafeMap<int, std::string> map;

   map[1] = "xd";

    const auto& v = map.at(1);
}