#include <vector>
#include <atomic>
#include <iostream>
#include <algorithm>
#include <execution>


int main() {
    // unsigned int count{0};
    std::atomic<unsigned int> count{0};

    std::vector<int> v(100000000);
    std::for_each(std::execution::par, std::begin(v), std::end(v), [&count](auto& item){
        item += count.fetch_add(1);
    });

    for(const auto& item : v) {
        if(item % 1000000 == 0){
            std::cout << item << " ";
        }
    }
}