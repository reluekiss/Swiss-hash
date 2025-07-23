#include <boost/unordered_map.hpp>
#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <vector>

constexpr int NOPS = 100000000;

std::string random_string(size_t len) {
    static const char charset[] =
      "abcdefghijklmnopqrstuvwxyz"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "0123456789";
    static thread_local std::mt19937_64 rng{std::random_device{}()};
    static thread_local std::uniform_int_distribution<size_t> dist(0, sizeof(charset)-2);
    std::string s;
    s.reserve(len);
    while (len--) s += charset[dist(rng)];
    return s;
}

#define BENCHMARK(NAME, CODE)                                            \
  do {                                                                   \
    auto t0 = std::chrono::high_resolution_clock::now();                 \
    for (int i = 0; i < NOPS; ++i) { CODE; }                             \
    auto t1 = std::chrono::high_resolution_clock::now();                 \
    double avg = std::chrono::duration<double, std::nano>(t1 - t0).count() / NOPS; \
    std::cout << NAME << " avg: " << avg << " ns\n";                     \
  } while (0)

struct Value { int num; std::string str; };

int main() {
    boost::unordered_map<std::string, Value> m;
    std::vector<std::string> keys;
    keys.reserve(NOPS);

    for (int i = 0; i < NOPS; ++i)
        keys.push_back(random_string(10));

    BENCHMARK("Insert", {
        m.emplace(keys[i], Value{ i, random_string(15) });
    });

    BENCHMARK("Lookup", {
        volatile auto& v = m[keys[i % keys.size()]];
        (void)v;
    });

    BENCHMARK("Iterate", {
        size_t checksum = 0;
        for (auto& p : m) {
            checksum += p.second.num;
        }
        volatile size_t sink = checksum;
    });

    BENCHMARK("Delete", {
        m.erase(keys[i % keys.size()]);
    });

    return 0;
}
