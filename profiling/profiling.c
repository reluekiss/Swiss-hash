#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../hash.h"

#include "../xxhash3.h"

#define NOPS 100000000
#define STEPS 10

#define DA_INIT_CAP 16

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
    int num;
    char *string;
} my_type_t;

typedef struct {
    char **items;
    size_t count, capacity;
} da_charp_t;

typedef struct {
    my_type_t *items;
    size_t count, capacity;
} da_val_t;

char* random_string(size_t length) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    char* str = malloc(length + 1);
    for (size_t i = 0; i < length; i++)
        str[i] = charset[rand() % (sizeof charset - 1)];
    str[length] = '\0';
    return str;
}

sm_allocator_t newhash(sm_allocator_t a) {
    a.hash = XXH3_64bits;
    return a;
}
map(map1, char*, my_type_t, sm_mmap_allocator());

static da_charp_t keys = {0};
static da_val_t vals = {0};

static void prepare_data(void) {
    for (int i = 0; i < NOPS; i++) {
        da_append(&keys, random_string(10));
        da_append(&vals, (my_type_t){
            .num    = rand(),
            .string = random_string(15),
        });
    }
}

static long ns_diff(const struct timespec *a,
                    const struct timespec *b) {
  return (b->tv_sec - a->tv_sec)*1000000000L
       + (b->tv_nsec - a->tv_nsec);
}

int main(void) {
    srand((unsigned)time(NULL));
    prepare_data();
  
    size_t step = NOPS/STEPS;
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < NOPS; i++) {
        put(map1, keys.items[i], vals.items[i]);
        if ((i+1) % step == 0) {
            clock_gettime(CLOCK_MONOTONIC, &t1);
            long ns = ns_diff(&t0, &t1);
            printf("Insert avg @ %6zu ops: %.2f ns/op\n", i+1, (double)ns/(i+1));
        }
    }
  
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < NOPS; i++) {
      (void)*get(map1, keys.items[rand()%NOPS]);
        if ((i+1) % step == 0) {
            clock_gettime(CLOCK_MONOTONIC, &t1);
            long ns = ns_diff(&t0, &t1);
            printf("Lookup avg @ %6zu ops: %.2f ns/op\n", i+1, (double)ns/(i+1));
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < NOPS; i++) {
        size_t checksum = 0;
        for_each(map1, i) {
            checksum += map1->vals[i].num;
        }
       volatile size_t sink = checksum;
        if ((i+1) % step == 0) {
            clock_gettime(CLOCK_MONOTONIC, &t1);
            long ns = ns_diff(&t0, &t1);
            printf("Iterate avg @ %6zu ops: %.2f ns/op\n", i+1, (double)ns/(i+1));
        }
        break;
    }
  
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < NOPS; i++) {
      erase(map1, keys.items[i]);
        if ((i+1) % step == 0) {
            clock_gettime(CLOCK_MONOTONIC, &t1);
            long ns = ns_diff(&t0, &t1);
            printf("Deletion avg @ %6zu ops: %.2f ns/op\n", i+1, (double)ns/(i+1));
        }
    }

    size_t ctrl_mem = map1->cap + 32;
    size_t keys_mem = map1->cap * sizeof(map1->keys);
    size_t vals_mem = map1->cap * sizeof(map1->vals);
    size_t struct_mem = sizeof(*map1);
    size_t total_mem = struct_mem + ctrl_mem + keys_mem + vals_mem;
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
