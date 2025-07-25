#include <chrono>
#include <iostream>
#include <random>
#include <vector>
#include <boost/unordered_map.hpp>

constexpr int NOPS = 1'000'000;
constexpr int ITERS = 1000;

template<class T>
__attribute__((noinline))
void doNotOptimizeAway(const T& v) {
  asm volatile("" :: "g"(v) : "memory");
}

int main() {
    std::mt19937_64 rng(42);
    std::uniform_int_distribution<uint64_t> dist_u64;
    std::uniform_int_distribution<size_t> idx_dist(0, NOPS - 1);
  
    std::vector<uint64_t> keys(NOPS), vals(NOPS);
    for (int i = 0; i < NOPS; ++i) {
        keys[i] = dist_u64(rng);
        vals[i] = dist_u64(rng);
    }
  
    std::vector<size_t> lookup_idx(NOPS), delete_idx(NOPS);
    for (int i = 0; i < NOPS; ++i) {
        lookup_idx[i] = idx_dist(rng);
        delete_idx[i] = idx_dist(rng);
    }
  
    boost::unordered_map<uint64_t, uint64_t> m;
    m.reserve(NOPS);
  
    {
      auto t0 = std::chrono::high_resolution_clock::now();
      for (int i = 0; i < NOPS; ++i) {
          m.emplace(keys[i], vals[i]);
      }
      auto t1 = std::chrono::high_resolution_clock::now();
      double avg = std::chrono::duration<double, std::nano>(t1 - t0)
                       .count() / NOPS;
      std::cout << "Insert avg: " << avg << " ns/op\n";
    }
  
    {
      auto t0 = std::chrono::high_resolution_clock::now();
      for (int i = 0; i < NOPS; ++i) {
          auto& v = m.find(keys[lookup_idx[i]])->second;
          doNotOptimizeAway(v);
      }
      auto t1 = std::chrono::high_resolution_clock::now();
      double avg = std::chrono::duration<double, std::nano>(t1 - t0)
                       .count() / NOPS;
      std::cout << "Lookup avg: " << avg << " ns/op\n";
    }
  
    {
      auto t0 = std::chrono::high_resolution_clock::now();
      uint64_t checksum = 0;
      for (int rep = 0; rep < ITERS; ++rep) {
          for (auto& p : m) {
              checksum += p.second;
          }
      }
      auto t1 = std::chrono::high_resolution_clock::now();
      double avg = std::chrono::duration<double, std::nano>(t1 - t0)
                       .count() / ITERS;
      std::cout << "Iterate avg: " << avg
                << " ns/op, checksum: " << checksum << "\n";
    }
  
    {
      auto t0 = std::chrono::high_resolution_clock::now();
      for (int i = 0; i < NOPS; ++i) {
          m.erase(keys[delete_idx[i]]);
      }
      auto t1 = std::chrono::high_resolution_clock::now();
      double avg = std::chrono::duration<double, std::nano>(t1 - t0)
                       .count() / NOPS;
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
    constexpr size_t pair_sz = sizeof(uint64_t) + sizeof(uint64_t);
    size_t kv_bytes = buckets * pair_sz;
    size_t total = ctrl_bytes + kv_bytes;
    size_t used = sz * pair_sz;
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
