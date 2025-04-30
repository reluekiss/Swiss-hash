#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define CWISS_EXAMPLE_UNIFIED
#include "cwisstable.h"

/*────────────────────────────────────────────────────────────────
   your policy from before: copy, dtor, hash, eq
────────────────────────────────────────────────────────────────*/
static inline void kCStrPolicy_copy(void *dst, const void *src) {
    typedef struct { const char *k; float v; } entry_t;
    const entry_t *e = src;
    entry_t *d = dst;
    size_t len = strlen(e->k);
    d->k = malloc(len + 1);
    memcpy((char*)d->k, e->k, len + 1);
    d->v = e->v;
}
static inline void kCStrPolicy_dtor(void *val) {
    char *s = *(char**)val;
    free(s);
}
static inline size_t kCStrPolicy_hash(const void *val) {
    const char *s = *(const char *const*)val;
    size_t len = strlen(s);
    CWISS_FxHash_State st = 0;
    CWISS_FxHash_Write(&st, s, len);
    return st;
}
static inline bool kCStrPolicy_eq(const void *a, const void *b) {
    const char *as = *(const char *const*)a;
    const char *bs = *(const char *const*)b;
    return strcmp(as, bs) == 0;
}

CWISS_DECLARE_NODE_MAP_POLICY(kCStrPolicy, const char*, float,
    (obj_copy, kCStrPolicy_copy),
    (obj_dtor,  kCStrPolicy_dtor),
    (key_hash,  kCStrPolicy_hash),
    (key_eq,    kCStrPolicy_eq));

CWISS_DECLARE_HASHMAP_WITH(MyCStrMap, const char*, float, kCStrPolicy);

/*────────────────────────────────────────────────────────────────
   benchmark harness
────────────────────────────────────────────────────────────────*/
#define NOPS       1000000
#define BATCH_SIZE (NOPS/10)

static char *random_string(size_t length) {
    static const char charset[] =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789";
    char *str = malloc(length + 1);
    for (size_t i = 0; i < length; ++i)
        str[i] = charset[rand() % (sizeof charset - 1)];
    str[length] = '\0';
    return str;
}

#define BENCHMARK(LABEL, VAR, OP)                                            \
    do {                                                                     \
        printf("Starting " LABEL "s\n");                                     \
        clock_t total = 0, batch = 0;                                        \
        for (int i = 0; i < NOPS; ++i) {                                     \
            clock_t t0 = clock();                                           \
            OP;                                                              \
            clock_t t1 = clock();                                           \
            total += t1 - t0;                                                \
            batch += t1 - t0;                                                \
            if ((i + 1) % BATCH_SIZE == 0) {                                 \
                printf(                                                     \
                  LABEL " avg @ op %4d–%4d: %.3f ns\n",                     \
                  i - BATCH_SIZE + 1, i,                                     \
                  ((double)batch / CLOCKS_PER_SEC / BATCH_SIZE) * 1e9);     \
                batch = 0;                                                   \
            }                                                                \
        }                                                                    \
        printf("Average " LABEL " time per op: %.3f ns\n\n",                 \
               ((double)total / CLOCKS_PER_SEC / NOPS) * 1e9);               \
    } while (0)

int main(void) {
    srand((unsigned)time(NULL));

    MyCStrMap map = MyCStrMap_new(8);
    char **inserted_keys = malloc(NOPS * sizeof(char*));
    size_t inserted_count = 0;

    /*--------- INSERT ----------*/
    BENCHMARK("Insert", insert, {
        char *key = random_string(10);
        float  val = (float)rand() / RAND_MAX;
        MyCStrMap_Entry e = { 0 };
        e.key = key;
        e.val = val;
        MyCStrMap_insert(&map, &e);
        inserted_keys[inserted_count++] = key;
    });

    /*--------- LOOKUP ----------*/
    BENCHMARK("Lookup", lookup, {
        const char *const k =
            inserted_keys[rand() % inserted_count];
        (void)MyCStrMap_find(&map, &k);
    });

    /*--------- ITERATE ---------*/
    BENCHMARK("Iterate", iterate, {
        size_t checksum = 0;
        MyCStrMap_Iter it = MyCStrMap_iter(&map);
        for (MyCStrMap_Entry *p = MyCStrMap_Iter_get(&it);
             p != NULL;
             p = MyCStrMap_Iter_next(&it)) {
            checksum += (size_t)p->val;
        }
        volatile size_t sink = checksum;
        (void)sink;
        break;
    });

    /*--------- DELETE ----------*/
    BENCHMARK("Delete", delete, {
        size_t idx = rand() % inserted_count;
        const char *const k = inserted_keys[idx];
        MyCStrMap_erase(&map, &k);
        free((void*)k);
        inserted_keys[idx] = inserted_keys[--inserted_count];
    });

    printf("Final map size: %zu\n\n", MyCStrMap_size(&map));

    /* cleanup the rest of the keys */
    for (size_t i = 0; i < inserted_count; ++i)
        free(inserted_keys[i]);
    free(inserted_keys);

    MyCStrMap_destroy(&map);
    return 0;
}
