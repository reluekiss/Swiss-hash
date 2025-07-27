#include <boost/unordered_map.hpp>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <random>
#include <string_view>
#include <vector>

constexpr int NOPS = 3'000'000;
constexpr int STEPS = 10;

struct Entry {
    long data;
    long unsigned count;
};

template <class T>
__attribute__((noinline)) void doNotOptimizeAway(const T &v) {
    asm volatile("" ::"g"(v) : "memory");
}

int main() {
    std::mt19937_64 rng(42);
    std::uniform_int_distribution<uint64_t> dist_u64;
    std::uniform_int_distribution<size_t> idx_dist(0, NOPS - 1);
    std::uniform_int_distribution<int> op_dist(0, 2);
  
    std::vector<uint64_t> keys(NOPS), vals(NOPS);
    for (int i = 0; i < NOPS; ++i) {
        keys[i] = dist_u64(rng);
        vals[i] = dist_u64(rng);
    }
  
    std::vector<size_t> lookup_idx(NOPS), delete_idx(NOPS);
    std::vector<uint8_t> ops(NOPS);
    for (int i = 0; i < NOPS; ++i) {
        lookup_idx[i] = idx_dist(rng);
        delete_idx[i] = idx_dist(rng);
        ops[i] = op_dist(rng);
    }

    boost::unordered_map<uint64_t, uint64_t> m;
    m.reserve(NOPS);

    std::vector<Entry> times_ins(NOPS), times_lkp(NOPS), times_del(NOPS);

    int ins_c = 0, lkp_c = 0, del_c = 0;

    using clk = std::chrono::high_resolution_clock;
    for (int i = 0; i < NOPS; ++i) {
        switch (ops[i]) {
        case 0: {
            auto t0 = clk::now();
            m.emplace(keys[ins_c], vals[ins_c]);
            auto t1 = clk::now();
            if (i % 100 == 0) times_ins[ins_c++] = (Entry) { .data = std::chrono::duration<long, std::nano>(t1 - t0).count(), .count = m.size() };
            break;
        }
        case 1: {
            auto t0 = clk::now();
            auto it = m.find(keys[lookup_idx[i]]);
            if (it != m.end())
                doNotOptimizeAway(it->second);
            auto t1 = clk::now();
            if (i % 100 == 0) times_lkp[lkp_c++] = (Entry) { .data = std::chrono::duration<long, std::nano>(t1 - t0).count(), .count = m.size() };
            break;
        }
        case 2: {
            auto t0 = clk::now();
            m.erase(keys[delete_idx[i]]);
            auto t1 = clk::now();
            if (i % 100 == 0) times_del[del_c++] = (Entry) { .data = std::chrono::duration<long, std::nano>(t1 - t0).count(), .count = m.size() };
            break;
        }
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
