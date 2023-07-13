#include <map>
#include <list>
#include <mutex>
#include <thread>
#include <ranges>
#include <vector>
#include <utility>
#include <iostream>
#include <shared_mutex>


template<typename Key, typename Value, typename Hash = std::hash<Key>>
class ConcurrentMap {
public:
    ConcurrentMap(unsigned int num_buckets = 19, const Hash& hasher = Hash()) :
            buckets(num_buckets), hasher(hasher) {

        for (auto& bucket: buckets) {
            bucket = std::make_unique<Bucket>();
        }
    };

    ConcurrentMap(const ConcurrentMap& rhs) = delete;
    ConcurrentMap& operator=(const ConcurrentMap& rhs) = delete;

    Value valueFor(const Key& key, const Value& default_value = Value()) const {
        Bucket& bucket{getBucket(key)};
        return bucket.valueFor(key, default_value);
    }

    void addOrUpdateItem(const Key& key, const Value& value) {
        Bucket& bucket{getBucket(key)};
        bucket.addOrUpdateItem(key, value);
    }

    void removeItem(const Key& key) {
        Bucket& bucket{getBucket(key)};
        bucket.removeItem(key);
    }

    std::map<Key, Value> getMap() {
        std::vector<std::unique_lock<std::shared_mutex>> locks;

        for (const auto& bucket: buckets) {
            locks.emplace_back(bucket->mutex);
        }

        std::map<Key, Value> ret_map;

        for (auto& bucket: buckets) {
            for (auto& bucket_iterator: bucket->data) {
                ret_map.insert(bucket_iterator);
            }
        }

        return ret_map;
    }

private:
    class Bucket {
    public:
        using BucketValue = std::pair<Key, Value>;
        using BucketData = std::list<BucketValue>;
        using BucketIterator = typename BucketData::iterator;
        using BucketConstIterator = typename BucketData::const_iterator;

        BucketData data;
        mutable std::shared_mutex mutex;

        friend class ConcurrentMap<Key, Value, Hash>;

        BucketConstIterator findEntryFor(const Key& key) const {
            return std::ranges::find_if(data, [&key](const BucketValue& item) {
                return key == item.first;
            });
        }

        BucketIterator findEntryFor(const Key& key) {
            return std::ranges::find_if(data, [&key](const BucketValue& item){
               return key == item.first;
            });
        }

    public:
        Value valueFor(const Key& key, const Value& default_value) const {
            std::shared_lock lock{mutex};

            const BucketConstIterator entry{findEntryFor(key)};
            return (entry == data.end()) ? default_value : entry->second;
        }

        void addOrUpdateItem(const Key& key, const Value& value) {
            std::unique_lock lock{mutex};

            const BucketIterator entry{findEntryFor(key)};

            if (entry != data.end()) {
                entry->second = value;
            }
            else {
                data.emplace_back(key, value);
            }
        }

        void removeItem(const Key& key) {
            std::unique_lock lock{mutex};

            const BucketIterator entry{findEntryFor(key)};

            if (entry != data.end()) {
                data.erase(entry);
            }
        }
    };

    [[nodiscard]] Bucket& getBucket(const Key& key) const {
        const std::size_t bucket_index = hasher(key) % buckets.size();
        return *buckets[bucket_index];
    }

    std::vector<std::unique_ptr<Bucket>> buckets;
    Hash hasher;
};


int main() {
    ConcurrentMap<int, std::string> map;

    map.addOrUpdateItem(1, "shit");
    map.addOrUpdateItem(2, "abcd");

    std::cout << map.valueFor(1) << std::endl;

    for (auto& [key, value]: map.getMap()) {
        std::cout << key << ": " << value << std::endl;
    }

    map.removeItem(1);

    std::cout << map.valueFor(1) << std::endl;

    return 0;
}