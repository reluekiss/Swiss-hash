#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>
#include "flat_hash_map.hpp"  // ska::flat_hash_map

constexpr int NOPS = 1'000'000;
constexpr int ITERS = 1000;

template<class T>
__attribute__((noinline))
void doNotOptimizeAway(const T& v) {
  asm volatile("" :: "g"(v) : "memory");
}

struct Entry {
    long data;
    long unsigned count;
};

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

    ska::flat_hash_map<uint64_t, uint64_t> m;
    m.reserve(NOPS);

    std::vector<Entry> times_ins(NOPS), times_lkp(NOPS), times_del(NOPS);

    int ins_c = 0, lkp_c = 0, del_c = 0;

    {
      for (int i = 0; i < NOPS; ++i) {
            auto t0 = std::chrono::high_resolution_clock::now();
            m.emplace(keys[i], vals[i]);
            auto t1 = std::chrono::high_resolution_clock::now();
            if (i % 100 == 0) times_ins[ins_c++] = (Entry) { .data = std::chrono::duration<long, std::nano>(t1 - t0).count(), .count = m.size() };
      }
    }
  
    {
      for (int i = 0; i < NOPS; ++i) {
          auto t0 = std::chrono::high_resolution_clock::now();
          auto& v = m.find(keys[lookup_idx[i]])->second;
          doNotOptimizeAway(v);
          auto t1 = std::chrono::high_resolution_clock::now();
          if (i % 100 == 0) times_lkp[lkp_c++] = (Entry) { .data = std::chrono::duration<long, std::nano>(t1 - t0).count(), .count = m.size() };
      }
    }
  
    {
      for (int i = 0; i < NOPS; ++i) {
          auto t0 = std::chrono::high_resolution_clock::now();
          m.erase(keys[delete_idx[i]]);
          auto t1 = std::chrono::high_resolution_clock::now();
          if (i % 100 == 0) times_del[del_c++] = (Entry) { .data = std::chrono::duration<long, std::nano>(t1 - t0).count(), .count = m.size() };
      }
    }
  
    std::cout << "operation,avg_ns,count\n";
    for (int i = 0; i < ins_c; ++i)
        std::cout << "Insert," << times_ins[i].data << "," << times_ins[i].count << "\n";
    for (int i = 0; i < lkp_c; ++i)
        std::cout << "Lookup," << times_lkp[i].data << "," << times_lkp[i].count << "\n";
    for (int i = 0; i < del_c; ++i)
        std::cout << "Delete," << times_del[i].data << "," << times_del[i].count << "\n";

  return 0;
}
