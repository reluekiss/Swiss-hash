#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#define NOPS 100000

typedef struct { int num; char *string; } my_type_t;
typedef struct { char *key; my_type_t value; } kv_t;

char* random_string(size_t length) {
    static const char charset[] =
      "abcdefghijklmnopqrstuvwxyz"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "0123456789";
    char *str = malloc(length + 1);
    for (size_t i = 0; i < length; i++)
        str[i] = charset[rand() % (sizeof charset - 1)];
    str[length] = '\0';
    return str;
}

#define BENCHMARK(LABEL, VAR, ...)                                         \
    do {                                                                   \
        printf("Starting " LABEL "s\n");                                   \
        clock_t diff = 0, batch_diff = 0;                                  \
        for (int op = 0; op < NOPS; ++op) {                                \
            clock_t start = clock();                                       \
            __VA_ARGS__;                                                   \
            clock_t end   = clock();                                       \
            diff       += end - start;                                     \
            batch_diff += end - start;                                     \
            if ((op+1) % batch_size == 0) {                                \
                printf(                                                    \
                  LABEL " avg @ size %zd (ops %4d–%4d): %.3f ns\n",        \
                  shlen(map),                                             \
                  op-(batch_size-1), op,                                   \
                  ((double)batch_diff / CLOCKS_PER_SEC / batch_size)*1e9   \
                );                                                         \
                batch_diff = 0;                                            \
            }                                                              \
        }                                                                  \
        double avg_##VAR = ((double)diff / CLOCKS_PER_SEC / NOPS)*1e9;     \
        printf("Average " LABEL " time per op: %.6f ns\n\n", avg_##VAR);   \
    } while (0)

int main(void) {
    srand((unsigned)time(NULL));

    kv_t *map = NULL;
    char *inserted_keys[NOPS];
    size_t inserted_count = 0;
    int batch_size = NOPS/10;

    BENCHMARK("Insert", insert,
        char *key = random_string(10);
        my_type_t v = { .num = rand(), .string = random_string(15) };
        shput(map, key, v);
        inserted_keys[inserted_count++] = key;
    );

    BENCHMARK("Lookup", lookup,
        const char *key = inserted_keys[rand() % inserted_count];
        volatile my_type_t v = shget(map, key);
        (void)v;
    );

    BENCHMARK("Iterate", iterate,
        size_t checksum = 0;
        for (ptrdiff_t i = 0; i < shlen(map); ++i)
            checksum += map[i].value.num;
        volatile size_t sink = checksum;
        (void)sink;
    );

    BENCHMARK("Delete", delete,
        size_t idx = rand() % inserted_count;
        const char *key = inserted_keys[idx];
        shdel(map, key);
        inserted_keys[idx] = inserted_keys[--inserted_count];
    );

    shfree(map);
    return 0;
}
