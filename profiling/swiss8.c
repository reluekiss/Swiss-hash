#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include "../hash.h"
#include "../xxhash3.h"

#define NOPS  1000000
#define ITERS 1000

sm_allocator_t newhash(sm_allocator_t a) {
  a.hash = XXH3_64bits;
  return a;
}

map(map1, uint64_t, uint64_t, newhash(sm_mmap_allocator()));

static long ns_diff(const struct timespec* a,
                    const struct timespec* b) {
  return (b->tv_sec - a->tv_sec) * 1000000000L +
         (b->tv_nsec - a->tv_nsec);
}

int main(int argc, char** argv) {
    int nops = argc > 1 ? atoi(argv[1]) : NOPS;
    uint64_t* keys = malloc(sizeof(uint64_t) * nops);
    uint64_t* vals = malloc(sizeof(uint64_t) * nops);
    uint32_t* lookup = malloc(sizeof(uint32_t) * nops);
    uint32_t* delidx = malloc(sizeof(uint32_t) * nops);

    for (int i = 0; i < nops; ++i) {
        keys[i] = rand();
        vals[i] = rand();
        lookup[i] = rand() % nops;
        delidx[i] = rand() % nops;
    }

    struct timespec t0, t1;
    long ns;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < nops; ++i) {
        put(map1, keys[i], vals[i]);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    ns = ns_diff(&t0, &t1);
    printf("Insert avg: %.2f ns/op\n", (double)ns / nops);

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < nops; ++i) {
        (void)*get(map1, keys[lookup[i]]);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    ns = ns_diff(&t0, &t1);
    printf("Lookup avg: %.2f ns/op\n", (double)ns / nops);

    uint64_t checksum = 0;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int rep = 0; rep < ITERS; ++rep) {
        for_each(map1, key, val) {
            checksum += *val;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    ns = ns_diff(&t0, &t1);
    printf("Iterate avg: %.2f ns/op, checksum: %lu\n",
           (double)ns / ITERS, checksum);

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < nops; ++i) {
        erase(map1, keys[delidx[i]]);
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
    uint64_t key_bytes = cap * sizeof(map1->keys[0]);
    uint64_t val_bytes = cap * sizeof(map1->vals[0]);
    uint64_t total = ctrl_bytes + key_bytes + val_bytes;
    uint64_t used = sz * (sizeof(map1->keys[0]) + sizeof(map1->vals[0]));
    uint64_t ovhd = total - used;
    double pct = 100.0 * ovhd / total;
    
    printf("SwissMap cap=%zu, size=%zu\n", cap, sz);
    printf(" mem: ctrl=%zu KB, keys=%zu KB, vals=%zu KB\n", ctrl_bytes / 1024,
           key_bytes / 1024, val_bytes / 1024);
    printf(" total=%zu KB, used=%zu KB, overhead=%zu KB (%.2f%%)\n", total / 1024,
           used / 1024, ovhd / 1024, pct);

  delete(map1);
  return 0;
}
