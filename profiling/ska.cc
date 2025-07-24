#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include "flat_hash_map.hpp"  // ska::flat_hash_map

struct Value {
  int   num;
  char  str[117];  // 116 chars + '\0'
};

static std::string random_string(size_t len, std::mt19937_64 &rng) {
  static const char charset[] =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789";
  std::uniform_int_distribution<size_t> dist(0, sizeof(charset) - 2);
  std::string s;
  s.reserve(len);
  while (len--) {
    s += charset[dist(rng)];
  }
  return s;
}

static void random_cstr(char *buf, size_t len, std::mt19937_64 &rng) {
  static const char charset[] =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789";
  std::uniform_int_distribution<size_t> dist(0, sizeof(charset) - 2);
  for (size_t i = 0; i < len; ++i) {
    buf[i] = charset[dist(rng)];
  }
  buf[len] = '\0';
}
template<class T>
__attribute__((noinline))
void doNotOptimizeAway(const T &value) {
  asm volatile("" : : "g"(value) : "memory");
}

int main() {
  constexpr int NOPS = 1'000'000;
  std::mt19937_64 rng(std::random_device{}());

  std::vector<std::string> keys; keys.reserve(NOPS);
  std::vector<Value>       vals; vals.reserve(NOPS);

  for (int i = 0; i < NOPS; ++i) {
    keys.push_back(random_string(10, rng));
    Value v;
    v.num = i;
    random_cstr(v.str, 116, rng);
    vals.push_back(v);
  }

  std::uniform_int_distribution<size_t> idx_dist(0, NOPS - 1);
  std::vector<size_t> lookup_idx(NOPS), delete_idx(NOPS);
  for (int i = 0; i < NOPS; ++i) {
    lookup_idx[i] = idx_dist(rng);
    delete_idx[i] = idx_dist(rng);
  }

  ska::flat_hash_map<std::string, Value> m;

  // Insert
  {
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < NOPS; ++i) {
      m.emplace(std::move(keys[i]), std::move(vals[i]));
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    double avg =
      std::chrono::duration<double, std::nano>(t1 - t0).count() / NOPS;
    std::cout << "Insert avg: " << avg << " ns/op\n";
  }

  // Lookup
  {
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < NOPS; ++i) {
      volatile auto &v = m.find(keys[lookup_idx[i]])->second;
      (void)v;
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    double avg = std::chrono::duration<double, std::nano>(t1 - t0).count() / NOPS;
    std::cout << "Lookup avg: " << avg << " ns/op\n";
  }

  // Iterate
  {
    auto t0 = std::chrono::high_resolution_clock::now();
    size_t checksum = 0;
    for (int i = 0; i < NOPS; ++i) {
        for (auto &p : m) {
          checksum += p.second.num;
        }
        if (i % 1000 == 0) break;
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    double avg = std::chrono::duration<double, std::nano>(t1 - t0).count() / 1000;
    std::cout << "Iterate avg: " << avg << " ns/op, checksum: " << checksum << "\n";
  }

  // Delete
  {
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < NOPS; ++i) {
      m.erase(keys[delete_idx[i]]);
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    double avg =
      std::chrono::duration<double, std::nano>(t1 - t0).count() / NOPS;
    std::cout << "Delete avg: " << avg << " ns/op\n";
  }

  return 0;
}
