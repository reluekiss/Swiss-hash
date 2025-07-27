#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/unistd.h>
#include <sys/mman.h>
#include <time.h>

#include "../hash.h"
#include "../xxhash3.h"

#define ITERS 1000
#define STEPS 10

typedef struct {
    int num;
    char data[1024];
} my_val_t;

typedef struct {
    char data[1024];
} my_key_t;

typedef struct {
    my_key_t *items;
    uint64_t count, capacity;
} da_char_t;

typedef struct {
    my_val_t *items;
    uint64_t count, capacity;
} da_val_t;

static uint64_t xorshift64star_state = 88172645463325252ull;
uint64_t xor64_rand(void) {
    uint64_t x = xorshift64star_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    xorshift64star_state = x;
    return x * 2685821657736338717ull;
}

static void random_bytes(uint64_t length, char buf[length]) {
    static const char charset[] =
      "abcdefghijklmnopqrstuvwxyz"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "0123456789";
    for (uint64_t i = 0; i < length; i++) {
        buf[i] = charset[xor64_rand() % (sizeof charset - 1)];
    }
}

sm_allocator_t newhash(sm_allocator_t a) {
    a.hash = XXH3_64bits;
    return a;
}
map(map1, my_key_t, my_val_t, newhash(sm_mmap_allocator()));

static da_char_t keys = {0};
static da_val_t vals = {0};

static void prepare_data(int nops) {
  for (int i = 0; i < nops; i++) {
    my_key_t key;
    random_bytes(1024, key.data);
    keys.items[keys.count++] = key;

    my_val_t val;
    val.num = xor64_rand();
    random_bytes(1024, val.data);
    vals.items[vals.count++] = val;
  }
}

static long ns_diff(const struct timespec *a,
                    const struct timespec *b) {
  return (b->tv_sec - a->tv_sec)*1000000000L
       + (b->tv_nsec - a->tv_nsec);
}

int main(int argc, char **argv) {
    int nops = argc > 1 ? atoi(argv[1]) : 1000000;
    keys.items = malloc(sizeof(*keys.items) * nops);
    vals.items = malloc(sizeof(*vals.items) * nops);
    keys.capacity = vals.capacity = nops;
    prepare_data(nops);
  
    uint32_t *lookup = malloc(sizeof(*lookup) * nops);
    uint32_t *delete = malloc(sizeof(*delete) * nops);
    for (int i = 0; i < nops; i++) {
        lookup[i] = xor64_rand() % nops;
        delete[i] = xor64_rand() % nops;
    }

    typedef struct { long ins; long count; } entry;
    entry *times_ins = malloc(sizeof(*times_ins) * nops);
    entry *times_lkp = malloc(sizeof(*times_lkp) * nops);
    entry *times_del = malloc(sizeof(*times_del) * nops);

    int ins_c = 0, lkp_c = 0, del_c = 0;

    map1 = (map1_t *)sm_new(nops, sizeof(my_key_t), sizeof(my_val_t), newhash(sm_mmap_allocator()));
    struct timespec t0, t1;

    for (int i = 0; i < nops; ++i) {
        clock_gettime(CLOCK_MONOTONIC, &t0);
        put(map1, keys.items[i], vals.items[i]);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        if (i % 100 == 0) times_ins[ins_c++] = (entry) { .ins = ns_diff(&t0, &t1), .count = map1->size };
    }

    for (int i = 0; i < nops; ++i) {
        clock_gettime(CLOCK_MONOTONIC, &t0);
        (void)*get(map1, keys.items[lookup[i]]);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        if (i % 100 == 0) times_lkp[lkp_c++] = (entry) { .ins = ns_diff(&t0, &t1), .count = map1->size };
    }

    for (int i = 0; i < nops; ++i) {
        clock_gettime(CLOCK_MONOTONIC, &t0);
        erase(map1, keys.items[delete[i]]);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        if (i % 100 == 0) times_del[del_c++] = (entry) { .ins = ns_diff(&t0, &t1), .count = map1->size };
    }

    printf("operation,avg_ns,count\n");
    for (int i = 0; i < ins_c; i++) 
        printf("Insert,%lu,%lu\n", times_ins[i].ins, times_ins[i].count);
    for (int i = 0; i < lkp_c; i++)
        printf("Lookup,%lu,%lu\n", times_lkp[i].ins, times_lkp[i].count);
    for (int i = 0; i < del_c; i++)
        printf("Delete,%lu,%lu\n", times_del[i].ins, times_del[i].count);

    delete(map1);
    return 0;
}
