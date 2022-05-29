#include <algorithm>
#include <cstdlib>
#include <future>
#include <map>
#include <numeric>
#include <random>
#include <string>
#include <vector>
#include <mutex>

#include "log_duration.h"

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Bucket {
        std::mutex value_mutex;
        std::map<Key, Value> data;
    };

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;

        Access(const Key& key, Bucket& bucket)
            : guard(bucket.value_mutex),
              ref_to_value(bucket.data[key]) {
        }
    };

    explicit ConcurrentMap(size_t bucket_count)
        : bucket_(bucket_count) {
    }

    Access operator[](const Key& key) {
        auto& bucket = bucket_[static_cast<uint64_t>(key) % bucket_.size()];
        return {key, bucket};
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;

        for (auto& [value_mutex, data] : bucket_) {
            std::lock_guard g(value_mutex);
            result.insert(data.begin(), data.end());
        }
        return result;
    }

private:
    std::vector<Bucket> bucket_;
};
