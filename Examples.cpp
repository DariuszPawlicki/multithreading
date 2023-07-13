#include <list>
#include <queue>
#include <mutex>
#include <vector>
#include <ranges>
#include <utility>
#include <iostream>
#include <functional>
#include <shared_mutex>
#include <condition_variable>


template<typename Key, typename Value, typename Hash = std::hash<Key>>
class ConcurrentMap{
public:
    ConcurrentMap(unsigned int buckets_num = 10, const Hash& hash_func = Hash()) :
        buckets(buckets_num), hash_func(hash_func) {

        for(auto& bucket: buckets) {
            bucket = std::make_unique<Bucket>();
        }
    };

    Value valueFor(const Key& key, const Value& default_value = Value()) const {
        const Bucket& bucket{getBucket(key)};
        return bucket.valueFor(key, default_value);
    }

    void addOrUpdateItem(const Key& key, const Value& value) {
        Bucket& bucket{getBucket(key)};
        bucket.addOrUpdateItem(key, value);
    }


private:
    struct Bucket {
        using BucketItem = std::pair<Key, Value>;
        using BucketData = std::list<BucketItem>;
        using BucketIterator = typename BucketData::iterator;
        using BucketConstIterator = typename BucketData::const_iterator;

        BucketData data;
        mutable std::shared_mutex mutex;

        BucketConstIterator findEntryFor(const Key& key) const {
            return std::ranges::find_if(data, [&key](const BucketItem& item) {
               return item.first == key;
            });
        }

        BucketIterator findEntryFor(const Key& key) {
            return std::ranges::find_if(data, [&key](const BucketItem& item) {
                return item.first == key;
            });
        }

        Value valueFor(const Key& key, const Value& default_value) const {
            std::shared_lock lk {mutex};
            auto entry = findEntryFor(key);

            return (entry == data.end()) ? default_value : entry->second;
        }

        void addOrUpdateItem(const Key& key, const Value& value) {
            std::unique_lock lk{mutex};
            auto entry = findEntryFor(key);

            if(entry != data.end()) {
                entry->second = value;
            }
            else {
                data.emplace_back(key, value);
            }
        }

        void removeItem(const Key& key) {
            std::unique_lock lk{mutex};
            auto entry = findEntryFor(key);

            if(entry != data.end()) {
                data.erase(entry);
            }
        }
    };

    Bucket& getBucket(const Key& key) const {
        std::size_t bucket_id = hash_func(key) % buckets.size();
        return *buckets[bucket_id];
    }

    std::vector<std::unique_ptr<Bucket>> buckets;
    Hash hash_func;
};


int main() {
    ConcurrentMap<std::string, int> m;

    m.addOrUpdateItem("xd", 2);

    std::cout << m.valueFor("xd");
}