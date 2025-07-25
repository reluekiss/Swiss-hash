#include <boost/unordered_map.hpp>
#include <chrono>
#include <cstring>
#include <iostream>
#include <random>
#include <string_view>
#include <vector>

constexpr int NOPS = 1'000'000;
constexpr int ITERS = 1000;

struct Value {
  int num;
  char str[1024];
};
struct Key {
  char buf[1024];
  bool operator==(Key const &o) const noexcept {
    return std::memcmp(buf, o.buf, sizeof(buf)) == 0;
  }
};
struct KeyHash {
  size_t operator()(Key const &k) const noexcept {
    return std::hash<std::string_view>{}(
        std::string_view(k.buf, sizeof(k.buf)));
  }
};

template <class T>
__attribute__((noinline)) void doNotOptimizeAway(const T &v) {
  asm volatile("" ::"g"(v) : "memory");
}

static void random_bytes(char *buf, size_t len, std::mt19937_64 &rng) {
  static const char charset[] = "abcdefghijklmnopqrstuvwxyz"
                                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                "0123456789";
  std::uniform_int_distribution<size_t> dist(0, sizeof(charset) - 2);
  for (size_t i = 0; i < len; ++i)
    buf[i] = charset[dist(rng)];
}

int main() {
    std::mt19937_64 rng(42);
    std::uniform_int_distribution<size_t> idx_dist(0, NOPS - 1);

    std::vector<Key> keys(NOPS);
    std::vector<Value> vals(NOPS);
    for (int i = 0; i < NOPS; i++) {
        random_bytes(keys[i].buf, sizeof(keys[i].buf), rng);
        vals[i].num = int(std::uniform_int_distribution<int>()(rng));
        random_bytes(vals[i].str, sizeof(vals[i].str) - 1, rng);
        vals[i].str[sizeof(vals[i].str) - 1] = '\0';
    }

    std::vector<size_t> lookup_idx(NOPS), delete_idx(NOPS);
    for (int i = 0; i < NOPS; i++) {
        lookup_idx[i] = idx_dist(rng);
        delete_idx[i] = idx_dist(rng);
    }

    boost::unordered_map<Key, Value, KeyHash> m;
    m.reserve(NOPS);

    {
      auto t0 = std::chrono::high_resolution_clock::now();
      for (int i = 0; i < NOPS; i++) {
          m.emplace(keys[i], vals[i]);
      }
      auto t1 = std::chrono::high_resolution_clock::now();
      double avg = std::chrono::duration<double, std::nano>(t1 - t0).count() / NOPS;
      std::cout << "Insert avg: " << avg << " ns/op\n";
    }

    {
      auto t0 = std::chrono::high_resolution_clock::now();
      for (int i = 0; i < NOPS; i++) {
          auto &v = m.find(keys[lookup_idx[i]])->second;
          doNotOptimizeAway(v);
      }
      auto t1 = std::chrono::high_resolution_clock::now();
      double avg = std::chrono::duration<double, std::nano>(t1 - t0).count() / NOPS;
      std::cout << "Lookup avg: " << avg << " ns/op\n";
    }

    {
      auto t0 = std::chrono::high_resolution_clock::now();
      size_t checksum = 0;
      for (int rep = 0; rep < ITERS; rep++) {
          for (auto &p : m) {
              checksum += p.second.num;
          }
      }
      auto t1 = std::chrono::high_resolution_clock::now();
      double avg = std::chrono::duration<double, std::nano>(t1 - t0).count() / ITERS;
      std::cout << "Iterate avg: " << avg << " ns/op, checksum: " << checksum << "\n";
    }

    {
      auto t0 = std::chrono::high_resolution_clock::now();
      for (int i = 0; i < NOPS; i++) {
          m.erase(keys[delete_idx[i]]);
      }
      auto t1 = std::chrono::high_resolution_clock::now();
      double avg = std::chrono::duration<double, std::nano>(t1 - t0).count() / NOPS;
      std::cout << "Delete avg: " << avg << " ns/op\n";
    }
    size_t buckets = m.bucket_count();
    size_t sz = m.size();
#ifdef __AVX2__
    constexpr size_t grp = 16;
#else
    constexpr size_t grp = 8;
#endif
    size_t ctrl_bytes = buckets + grp;
    size_t kv_bytes = buckets * (sizeof(Key) + sizeof(Value));
    size_t total = ctrl_bytes + kv_bytes;
    size_t used = sz * (sizeof(Key) + sizeof(Value));
    size_t ovhd = total - used;
    double pct = 100.0 * ovhd / total;
    
    std::cout << "boost::unordered_map buckets=" << buckets << ", size=" << sz << "\n";
    std::cout << " mem: ctrl=" << ctrl_bytes / 1024 << " KB, "
              << "kv=" << kv_bytes / 1024 << " KB\n";
    std::cout << " total=" << total / 1024 << " KB, "
              << "used=" << used / 1024 << " KB, "
              << "overhead=" << ovhd / 1024 << " KB (" << pct << "%)\n";

    return 0;
}
