#include <future>
#include <vector>
#include <numeric>
#include <iostream>


template<typename Iterator, typename Func>
void parallelForEach(Iterator first, Iterator last, Func f) {
    unsigned const long length = std::distance(first, last);

    if(!length){
        return;
    }

    unsigned const int min_per_thread{6};

    if(length < (2 * min_per_thread)) {
        std::for_each(first, last, f);
    }
    else {
        const Iterator mid_point = first + length / 2;
        std::future<void> first_half = std::async(&parallelForEach<Iterator, Func>, first, mid_point, f);
        parallelForEach(mid_point, last, f);
        first_half.get();
    }
}

int main() {
    std::vector<int> v{1,7,23,2,7,7,90,0,3,234,65,-22,
                       1,7,23,2,7,7,90,0,3,234,65,-22};
    parallelForEach(v.begin(), v.end(), [](int& item) {
        item *= 2;
    });
    std::cout << "";
}