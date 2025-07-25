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

static void random_bytes(uint64_t length, char buf[length]) {
    static const char charset[] =
      "abcdefghijklmnopqrstuvwxyz"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "0123456789";
    for (uint64_t i = 0; i < length; i++) {
        buf[i] = charset[rand() % (sizeof charset - 1)];
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
    val.num = rand();
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
    srand(42);
    keys.items = malloc(sizeof(*keys.items) * nops);
    vals.items = malloc(sizeof(*vals.items) * nops);
    keys.capacity = vals.capacity = nops;
    prepare_data(nops);
  
    uint32_t *lookup_idx = malloc(sizeof(*lookup_idx) * nops);
    uint32_t *delete_idx = malloc(sizeof(*delete_idx) * nops);
    for (int i = 0; i < nops; i++) {
        lookup_idx[i] = rand() % nops;
        delete_idx[i] = rand() % nops;
    }
  
    map1 = (map1_t *)sm_new(nops, sizeof(my_key_t), sizeof(my_val_t), newhash(sm_mmap_allocator()));
  
    struct timespec t0, t1;
    long ns;
  
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < nops; i++) {
        put(map1, keys.items[i], vals.items[i]);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    ns = ns_diff(&t0, &t1);
    printf("Insert avg: %.2f ns/op\n", (double)ns / nops);
  
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < nops; i++) {
      (void)*get(map1, keys.items[lookup_idx[i]]);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    ns = ns_diff(&t0, &t1);
    printf("Lookup avg: %.2f ns/op\n", (double)ns / nops);
  
    uint64_t checksum = 0;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int rep = 0; rep < ITERS; rep++) {
        for_each(map1, key, val) {
            checksum += val->num;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    ns = ns_diff(&t0, &t1);
    printf("Iterate avg: %.2f ns/op, checksum: %lu\n", (double)ns / ITERS, checksum);
  
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < nops; i++) {
        erase(map1, keys.items[delete_idx[i]]);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    ns = ns_diff(&t0, &t1);
    printf("Delete avg: %.2f ns/op\n", (double)ns / nops);

    uint64_t cap = map1->cap;
    uint64_t sz = map1->size;
    uint64_t grp_bytes;
#ifdef __AVX2__
    grp_bytes = 32;
#else
    grp_bytes = 8;
#endif
    uint64_t ctrl_bytes = cap + grp_bytes;
    uint64_t key_bytes = cap * sizeof(my_key_t);
    uint64_t val_bytes = cap * sizeof(my_val_t);
    uint64_t total = ctrl_bytes + key_bytes + val_bytes;
    uint64_t used = sz * (sizeof(my_key_t) + sizeof(my_val_t));
    uint64_t ovhd = total - used;
    double pct = 100.0 * ovhd / total;
    
    printf("SwissMap cap=%zu, size=%zu\n", cap, sz);
    printf(" mem: ctrl=%zu KB, keys=%zu KB, vals=%zu KB\n", ctrl_bytes / 1024,
           key_bytes / 1024, val_bytes / 1024);
    printf(" total=%zu KB, used=%zu KB, overhead=%zu KB (%.2f%%)\n", total / 1024,
           used / 1024, ovhd / 1024, pct);
  
    delete (map1);
    return 0;
}
