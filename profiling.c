#include <stdio.h>
#include <time.h>

#define SWISSMAP_IMPLEMENTATION
#include "hash.h"

#define NOPS 100000

/*—— Profiling ——*/
typedef struct {
    int   num;
    char *string;
} my_type_t;

char* random_string(size_t length) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    char* str = malloc(length + 1);
    for (size_t i = 0; i < length; i++)
        str[i] = charset[rand() % (sizeof charset - 1)];
    str[length] = '\0';
    return str;
}

map(map1, char*, my_type_t);

#define BENCHMARK(LABEL, VAR, CODE)                                    \
    do {                                                               \
        printf("Starting " LABEL "s\n");                               \
        clock_t diff = 0, batch_diff = 0;                              \
        for (int op = 0; op < NOPS; op++) {                            \
            clock_t start = clock(); CODE; clock_t end = clock();      \
            diff       += end - start;                                 \
            batch_diff += end - start;                                 \
            if ((op+1) % batch_size == 0) {                            \
                printf(                                                \
                  LABEL " avg @ size %zu (ops %4d–%4d): %.3f ns\n",    \
                  map1_sm->size,                                       \
                  op-(batch_size-1), op,                               \
                  ((double)batch_diff / CLOCKS_PER_SEC / batch_size)*1e9 \
                );                                                     \
                batch_diff = 0;                                        \
            }                                                          \
        }                                                              \
        double avg_##VAR = ((double)diff / CLOCKS_PER_SEC / NOPS) * 1e9; \
        printf("Average " LABEL " time per op: %.6f ns\n",             \
               avg_##VAR);                                            \
    } while (0)

int main(void) {
    srand((unsigned)time(NULL));
    char *inserted_keys[NOPS];
    size_t inserted_count = 0;
    int batch_size = NOPS/10;

    BENCHMARK("Insert", insert, {
        char *key   = random_string(10);
        my_type_t value = {0};
        value.num = rand();
        value.string = random_string(15);
        put(map1, key) = value;
        inserted_keys[inserted_count++] = key;
    });
    
    BENCHMARK("Lookup", lookup, {
        char *key = inserted_keys[rand() % inserted_count];
        (void)*get(map1, key);
    });

     BENCHMARK("Iterate", iterate, {
        size_t checksum = 0;
        for_each(map1, k, v) {
          (void)k;
          checksum += v->num;
        }
        size_t sink = checksum;
        (void)sink;
    });   

    BENCHMARK("Delete", delete, {
        size_t idx = rand() % inserted_count;
        char *key  = inserted_keys[idx];
        if (!erase(map1, key))
            inserted_keys[idx] = inserted_keys[--inserted_count];
    });

    SwissMap *m = map1_sm;
    size_t ctrl_mem   = m->cap + GROUP_SIZE;
    size_t keys_mem   = m->cap * m->key_size;
    size_t vals_mem   = m->cap * m->val_size;
    size_t struct_mem = sizeof(*m);
    size_t total_mem  = struct_mem + ctrl_mem + keys_mem + vals_mem;
    double overhead   = (double)(struct_mem + ctrl_mem) / total_mem * 100.0;

    printf("Memory usage:\n");
    printf("  SwissMap struct:       %zu bytes\n", struct_mem);
    printf("  control array:         %zu bytes\n", ctrl_mem);
    printf("  keys array:            %zu bytes\n", keys_mem);
    printf("  vals array:            %zu bytes\n", vals_mem);
    printf("  total:                 %zu bytes\n", total_mem);
    printf("Overhead (struct+ctrl): %.2f%% of total\n", overhead);

    delete(map1);
    return 0;
}
