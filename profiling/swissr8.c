#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/unistd.h>
#include <time.h>

#include "../hash.h"
#include "../xxhash3.h"

#define NOPS 3000000

static uint64_t xorshift64star_state = 88172645463325252ull;
uint64_t xor64_rand(void) {
    uint64_t x = xorshift64star_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    xorshift64star_state = x;
    return x * 2685821657736338717ull;
}

sm_allocator_t newhash(sm_allocator_t a) {
    a.hash = XXH3_64bits;
    return a;
}
map(map1, uint64_t, uint64_t, newhash(sm_mmap_allocator()));

static long ns_diff(const struct timespec *a,
                    const struct timespec *b) {
    return (b->tv_sec - a->tv_sec)*1000000000L
         + (b->tv_nsec - a->tv_nsec);
}

int main(int argc, char **argv) {
    int nops = argc > 1 ? atoi(argv[1]) : NOPS;
    uint64_t* keys = malloc(sizeof(uint64_t) * nops);
    uint64_t* vals = malloc(sizeof(uint64_t) * nops);
    uint64_t* lookup = malloc(sizeof(uint32_t) * nops);
    uint64_t* delidx = malloc(sizeof(uint32_t) * nops);
    uint8_t *ops = malloc(nops);

    for (int i = 0; i < nops; ++i) {
        keys[i] = xor64_rand();
        vals[i] = xor64_rand();
        lookup[i] = xor64_rand() % nops;
        delidx[i] = xor64_rand() % nops;
        ops[i] = xor64_rand() % 3;
    }

    typedef struct { long ins; long count; } entry;
    entry *times_ins = malloc(sizeof(*times_ins) * nops);
    entry *times_lkp = malloc(sizeof(*times_lkp) * nops);
    entry *times_del = malloc(sizeof(*times_del) * nops);

    int ins_c = 0, lkp_c = 0, del_c = 0;

    map1 = (map1_t *)sm_new(nops, sizeof(uint64_t), sizeof(uint64_t), newhash(sm_mmap_allocator()));
    struct timespec t0, t1;

    for (int i = 0; i < nops; i++) {
        switch (ops[i]) {
          case 0:
            clock_gettime(CLOCK_MONOTONIC, &t0);
            put(map1, keys[ins_c], vals[ins_c]);
            clock_gettime(CLOCK_MONOTONIC, &t1);
            if (i % 100 == 0) times_ins[ins_c++] = (entry) { .ins = ns_diff(&t0, &t1), .count = map1->size };
            break;
          case 1:
            clock_gettime(CLOCK_MONOTONIC, &t0);
            {
                uint64_t *v = get(map1, keys[lookup[i]]);
            }
            clock_gettime(CLOCK_MONOTONIC, &t1);
            if (i % 100 == 0) times_lkp[lkp_c++] = (entry) { .ins = ns_diff(&t0, &t1), .count = map1->size };
            break;
          case 2:
            clock_gettime(CLOCK_MONOTONIC, &t0);
            erase(map1, keys[delidx[i]]);
            clock_gettime(CLOCK_MONOTONIC, &t1);
            if (i % 100 == 0) times_del[del_c++] = (entry) { .ins = ns_diff(&t0, &t1), .count = map1->size };
            break;
        }
    }

    printf("operation,avg_ns,count\n");
    for (int i = 0; i < ins_c; i++) 
        printf("Insert,%lu,%lu\n", times_ins[i].ins, times_ins[i].count);
    for (int i = 0; i < lkp_c; i++)
        printf("Lookup,%lu,%lu\n", times_lkp[i].ins, times_lkp[i].count);
    for (int i = 0; i < del_c; i++)
        printf("Delete,%lu,%lu\n", times_del[i].ins, times_del[i].count);

    return 0;
}
