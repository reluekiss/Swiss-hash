#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "hash.h"

#include "xxhash3.h"

#define STEPS 10

#define DA_INIT_CAP 2048

#define da_append(da, ...)                                                     \
  do {                                                                          \
    if ((da)->count >= (da)->capacity) {                                        \
      (da)->capacity = (da)->capacity == 0                                     \
                        ? DA_INIT_CAP                                         \
                        : (da)->capacity * 2;                                 \
      (da)->items = realloc(                                                   \
        (da)->items,                                                           \
        (da)->capacity * sizeof(*(da)->items));                               \
      assert((da)->items != NULL && "Out of memory");                         \
    }                                                                           \
    (da)->items[(da)->count++] = (__VA_ARGS__);                                \
  } while (0)


typedef struct {
    char **items;
    uint64_t count, capacity;
} da_charp_t;

typedef struct {
    uint32_t *items;
    uint64_t count, capacity;
} da_val_t;

static inline uint32_t xorshift32(uint32_t *x) {
    uint32_t y = *x;
    y ^= y << 13;
    y ^= y >> 17;
    y ^= y <<  5;
    return *x = y;
}

char* random_string(uint64_t length) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    char* str = malloc(length + 1);
    for (uint64_t i = 0; i < length; i++)
        str[i] = charset[rand() % (sizeof charset - 1)];
    str[length] = '\0';
    return str;
}

sm_allocator_t newhash(sm_allocator_t a) {
    a.hash = XXH3_64bits;
    return a;
}
map(map1, char*, uint32_t, newhash(sm_mmap_allocator()));

static da_charp_t keys = {0};
static da_val_t vals = {0};

static void prepare_data(int nops) {
    for (int i = 0; i < nops; i++) {
        da_append(&keys, random_string(1024));
        da_append(&vals, i);
    }
}

static long ns_diff(const struct timespec *a,
                    const struct timespec *b) {
  return (b->tv_sec - a->tv_sec)*1000000000L
       + (b->tv_nsec - a->tv_nsec);
}

int main(int argc, char** argv) {
    int nops = argc > 1 ? atoi(argv[1]) : 1000000; 
    srand((unsigned)time(NULL));
    prepare_data(nops);
  
    uint64_t step = nops/STEPS;
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < nops; i++) {
        put(map1, keys.items[i], vals.items[i]);
        if ((i+1) % step == 0) {
            clock_gettime(CLOCK_MONOTONIC, &t1);
            long ns = ns_diff(&t0, &t1);
            printf("Insert avg @ %6u ops: %.2f ns/op\n", i+1, (double)ns/(i+1));
        }
    }
  
    clock_gettime(CLOCK_MONOTONIC, &t0);
    uint32_t state = 123456789;
    for (int i = 0; i < nops; i++) {
        uint32_t r = xorshift32(&state) & (nops - 1);
        uint32_t* c = get(map1, keys.items[r]);
        if ((i+1) % step == 0) {
            clock_gettime(CLOCK_MONOTONIC, &t1);
            long ns = ns_diff(&t0, &t1);
            printf("Lookup avg @ %6u ops: %.2f ns/op\n", i+1, (double)ns/(i+1));
            assert(*c == vals.items[r]);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &t0);
    int i;
    uint64_t checksum = 0;
    for (i = 0; i < nops; i++) {
        for_each(map1, key, val) {
            checksum += *val;
        }
        if ((i+1) % 1000 == 0) {
            break;
        }
        volatile uint64_t sink = checksum = 0;
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    long ns = ns_diff(&t0, &t1);
    printf("Iterate avg @ %6u ops: %.2f ns/op, checksum: %ld\n", i+1, ((double)ns/(i+1)), checksum);
  
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < nops; i++) {
      erase(map1, keys.items[i]);
        if ((i+1) % step == 0) {
            clock_gettime(CLOCK_MONOTONIC, &t1);
            long ns = ns_diff(&t0, &t1);
            printf("Deletion avg @ %6u ops: %.2f ns/op\n", i+1, (double)ns/(i+1));
        }
    }

    uint64_t ctrl_mem = map1->cap + 32;
    uint64_t keys_mem = map1->cap * sizeof(map1->keys);
    uint64_t vals_mem = map1->cap * sizeof(map1->vals[0]);
    uint64_t struct_mem = sizeof(*map1);
    uint64_t total_mem = struct_mem + ctrl_mem + keys_mem + vals_mem;
    double overhead = (double)(struct_mem + ctrl_mem) / total_mem * 100.0;

    printf("Memory usage:\n");
    printf("  SwissMap struct:  %zu bytes\n", struct_mem);
    printf("  control array:    %zu bytes\n", ctrl_mem);
    printf("  keys.items array: %zu bytes\n", keys_mem);
    printf("  vals.items array: %zu bytes\n", vals_mem);
    printf("  total:            %zu bytes\n", total_mem);
    printf("Overhead (struct+ctrl): %.2f%% of total\n", overhead);

    delete(map1);
    return 0;
}
