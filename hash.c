#include "hash.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <immintrin.h>
#include <sys/mman.h>

#ifndef NULL
#define NULL (void*)0
#endif

#ifdef __AVX2__
#  define GROUP_SIZE 32
#else
#  define GROUP_SIZE 8
#endif

typedef struct {
    sm_allocator_t alloc;
    uint8_t *ctrl;
    void *keys;
    void *vals;
    uint64_t cap, size;
    uint64_t lgcap;
} swiss_map_generic_t;

static uint64_t fnv1a(const void *data, uint64_t len) {
    const uint8_t *p = data;
    uint64_t h = 14695981039346656037ULL;
    for (uint64_t i = 0; i < len; i++) {
        h ^= p[i];
        h *= 1099511628211ULL;
    }
    return h;
}

static uint64_t next_pow2(uint64_t x) {
    if (x < 2)
        return 2;
    x--;
    for (int shift = 1; shift < (int)(8 * sizeof(uint64_t)); shift <<= 1)
        x |= x >> shift;
    return x + 1;
}

static inline uint64_t index_for(uint64_t h, uint64_t lgcap) {
    return (h * 11400714819323198485ull) >> (64 - lgcap);
}

static void *mmap_alloc(void *ctx, uint64_t n) {
    (void)ctx;
    uint64_t pagesz = (uint64_t)sysconf(_SC_PAGESIZE);
    uint64_t hdr = sizeof(uint64_t);
    uint64_t total = n + hdr;
    uint64_t region = (total + pagesz - 1) & ~(pagesz - 1);
    void *p = mmap(NULL, region, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) return NULL;
    *(uint64_t*)p = region;
    return (char*)p + hdr;
}

static void mmap_free(void *ctx, void *ptr) {
    (void)ctx;
    if (!ptr) return;
    char *base = (char*)ptr - sizeof(uint64_t);
    uint64_t region = *(uint64_t*)base;
    munmap(base, region);
}

sm_allocator_t sm_mmap_allocator(void) {
    sm_allocator_t a;
    a.ctx = NULL;
    a.alloc = mmap_alloc;
    a.free = mmap_free;
    a.hash = fnv1a;
    return a;
}

static void* sm_alloc(void* ctx, uint64_t n) {
    (void)ctx;
    return malloc(n);
}
static void sm_unalloc(void* ctx, void* p) {
    (void)ctx;
    free(p);
}

void *sm_new(uint64_t init_cap, uint64_t key_size, uint64_t val_size, sm_allocator_t allocs) {
    if (allocs.alloc == NULL || allocs.free == NULL) {
        allocs.ctx = NULL;
        allocs.alloc = sm_alloc;
        allocs.free = sm_unalloc;
    }
    if (allocs.hash == NULL) 
        allocs.hash = fnv1a;

    swiss_map_generic_t *m = allocs.alloc(allocs.ctx, sizeof(*m));
    m->alloc = allocs;
    m->cap = next_pow2(init_cap);
    m->lgcap = __builtin_ctzll(m->cap);
    m->size = 0;
    m->ctrl = allocs.alloc(allocs.ctx, m->cap + GROUP_SIZE);
    memset(m->ctrl, EMPTY, m->cap + GROUP_SIZE);
    m->keys = allocs.alloc(allocs.ctx, m->cap * key_size);
    m->vals = allocs.alloc(allocs.ctx, m->cap * val_size);
    return m;
}

void sm_free(void *map, sm_allocator_t allocs) {
    swiss_map_generic_t *m = (swiss_map_generic_t*)map;
    allocs.free(allocs.ctx, m->ctrl);
    allocs.free(allocs.ctx, m->keys);
    allocs.free(allocs.ctx, m->vals);
    allocs.free(allocs.ctx, m);
}

#ifdef __AVX2__
static inline uint32_t match(uint8_t h, const uint8_t *ctrl) {
    __m256i group = _mm256_loadu_si256((const __m256i*)ctrl);
    __m256i cmp = _mm256_cmpeq_epi8(_mm256_set1_epi8(h), group);
    return _mm256_movemask_epi8(cmp);
}
#else
static inline uint32_t match(uint8_t h, const uint8_t *ctrl) {
    __m128i group = _mm_loadu_si128((const __m128i*)ctrl);
    __m128i cmp = _mm_cmpeq_epi8(_mm_set1_epi8(h), group);
    return _mm_movemask_epi8(cmp);
}
#endif

static void sm_grow(swiss_map_generic_t *m, uint64_t key_size, uint64_t val_size) {
    uint64_t old_cap = m->cap;
    uint8_t *old_ctrl = m->ctrl;
    void *old_keys = m->keys;
    void *old_vals = m->vals;

    m->cap *= 2;
    m->lgcap = __builtin_ctzll(m->cap);
    m->size = 0;
    m->ctrl = m->alloc.alloc(m->alloc.ctx, m->cap + GROUP_SIZE);
    memset(m->ctrl, EMPTY, m->cap + GROUP_SIZE);
    m->keys = m->alloc.alloc(m->alloc.ctx, m->cap * key_size);
    // memset(m->keys, 0, m->cap * key_size);
    m->vals = m->alloc.alloc(m->alloc.ctx, m->cap * val_size);
    // memset(m->vals, 0, m->cap * val_size);
    for (uint64_t i = 0; i < old_cap; i++) {
        uint8_t c = old_ctrl[i];
        if (c != EMPTY && c != DELETED) {
            void *k_src = (char*)old_keys + i * key_size;
            void *v_src = (char*)old_vals + i * val_size;
            uint64_t h  = m->alloc.hash(k_src, key_size);
            uint8_t h2 = ((uint8_t)(h >> 56)) & 0x7F;
            uint64_t idx = index_for(h, m->lgcap);
            for (;;idx = (idx + GROUP_SIZE) & (m->cap - 1)) {
                uint32_t mask = match(EMPTY, &m->ctrl[idx]);
                if (mask) {
                    int j = __builtin_ctz(mask);
                    uint64_t pos = (idx + j) & (m->cap - 1);
                    m->ctrl[pos] = h2;
                    memcpy((char*)m->keys + pos * key_size, k_src, key_size);
                    memcpy((char*)m->vals + pos * val_size, v_src, val_size);
                    m->size++;
                    break;
                }
            }
        }
    }
    m->alloc.free(m->alloc.ctx, old_ctrl);
    m->alloc.free(m->alloc.ctx, old_keys);
    m->alloc.free(m->alloc.ctx, old_vals);
}

void *sm_get(void *map, const void *key, int *inserted, uint64_t key_size, uint64_t val_size) {
    swiss_map_generic_t *m = (swiss_map_generic_t*)map;
    if ((m->size + 1) * 5 >= m->cap * 4)
        sm_grow(m, key_size, val_size);

    uint64_t h = m->alloc.hash(key, key_size);
    uint8_t h2 = ((uint8_t)(h >> 56)) & 0x7F;
    uint64_t idx = index_for(h, m->lgcap); 
    for (;; idx = (idx + GROUP_SIZE) & (m->cap-1)) {
        uint8_t *ctrl = m->ctrl + idx;
        __builtin_prefetch(ctrl + GROUP_SIZE, 0, 1);
        uint32_t mask = match(h2, ctrl) | match(EMPTY, ctrl) | match(DELETED, ctrl);
        while (mask) {
            int j = __builtin_ctz(mask);
            uint64_t pos = (idx + j) & (m->cap-1);
            mask &= mask - 1;
  
            if (ctrl[j] == h2) {
                if (!memcmp((char*)m->keys + pos*key_size, key, key_size)) {
                    *inserted = 0;
                    return (char*)m->vals + pos*val_size;
                }
            } else {
                m->ctrl[pos] = h2;
                memcpy((char*)m->keys + pos*key_size, key, key_size);
                m->size++;
                *inserted = 1;
                return (char*)m->vals + pos*val_size;
            }
        }
    }
}

int sm_delete(void *map, const void *key, uint64_t key_size, uint64_t val_size) {
    swiss_map_generic_t *m = (swiss_map_generic_t*)map;
    uint64_t h = m->alloc.hash(key, key_size);
    uint8_t h2 = ((uint8_t)(h >> 56)) & 0x7F;
    uint64_t idx = index_for(h, m->lgcap);
    for (;;) {
        uint32_t match_mask = match(h2, &m->ctrl[idx]);
        while (match_mask) {
            int j = __builtin_ctz(match_mask);
            uint64_t pos = (idx + j) & (m->cap - 1);
            if (memcmp((char*)m->keys + pos*key_size, key, key_size) == 0) {
                m->ctrl[pos] = DELETED;
                // This memset slows down deletions by about ~20% but it means
                // that you cannot get values that are marked as deleted.
                // Feel free to comment this out but be aware of funky behaviour.
                // memset((char *)m->vals + pos * val_size, 0, val_size);
                m->size--;
                return 0;
            }
            match_mask &= match_mask - 1;
        }
        if (match(EMPTY, &m->ctrl[idx])) return -1;
        idx = (idx + GROUP_SIZE) & (m->cap - 1);
    }
}
