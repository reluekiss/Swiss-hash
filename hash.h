#ifndef SWISSMAP_H
#define SWISSMAP_H

#include <stddef.h>
#include <stdint.h>

/* opaque handle */
typedef struct SwissMap SwissMap;

/* create/free */
SwissMap *sm_new(size_t init_cap, size_t key_size, size_t val_size);
void      sm_free(SwissMap *m);

/* get-or-insert: returns pointer to value slot; *inserted=1 if new */
void *sm_get(SwissMap *m, const void *key, int *inserted);

/* delete: returns 0 on success, –1 if not found */
int   sm_delete(SwissMap *m, const void *key);

/* user-facing macros */
#define map(m, KeyT, ValT)                                   \
    typedef KeyT     m##_key_t;                              \
    typedef ValT     m##_val_t;                              \
    static SwissMap *m##_sm = NULL;                          \
    static inline void m##_init(void) {                      \
        m##_sm = sm_new(16, sizeof(KeyT), sizeof(ValT));     \
    }                                                        \
    static inline ValT *m##_acc(KeyT k) {                    \
        if (!m##_sm) m##_init();                             \
        int ins;                                             \
        return (ValT*)sm_get(m##_sm, &k, &ins);              \
    }                                                        \
    static inline int m##_erase(KeyT k) {                    \
        if (!m##_sm) return -1;                              \
        return sm_delete(m##_sm, &k);                        \
    }                                                        \
    static inline void m##_del(void) {                       \
        if (m##_sm) {                                        \
            sm_free(m##_sm);                                 \
            m##_sm = NULL;                                   \
        }                                                    \
    }

#define put(m, k)   (*m##_acc(k))
#define get(m, k)    m##_acc(k)
#define erase(m, k)  m##_erase(k)
#define delete(m)    m##_del()

#define for_each(m, k, v)                                                                 \
  for (size_t _i = 0; _i < m##_sm->cap; ++_i)                                             \
    if (m##_sm->ctrl[_i] != EMPTY && m##_sm->ctrl[_i] != DELETED)                         \
      for (m##_key_t k = *(m##_key_t*)((uint8_t*)m##_sm->keys + _i * m##_sm->key_size);   \
           k; k = (m##_key_t)0)                                                           \
        for (m##_val_t *v = (m##_val_t*)((uint8_t*)m##_sm->vals + _i * m##_sm->val_size);\
             v; v = NULL)

#endif // SWISSMAP_H

#ifdef SWISSMAP_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>

#define GROUP_SIZE 8
#define EMPTY      0x80u
#define DELETED    0xFEu

struct SwissMap {
    uint8_t *ctrl;     // [cap + GROUP_SIZE]
    void    *keys;     // contiguous key_size×cap
    void    *vals;     // contiguous val_size×cap
    size_t   cap, size;
    size_t   key_size, val_size;
};

/*—— FNV-1a 64-bit hash ——*/
static size_t map_hash(const void *data, size_t len) {
    const uint8_t *p = data;
    size_t h = 14695981039346656037ULL;
    for (size_t i = 0; i < len; i++) {
        h ^= p[i];
        h *= 1099511628211ULL;
    }
    return h;
}

static size_t next_pow2(size_t x) {
    if (x < 2) return 2;
    x--;
    for (int shift = 1; shift < (int)(8*sizeof(size_t)); shift <<= 1)
        x |= x >> shift;
    return x+1;
}

SwissMap *sm_new(size_t init_cap, size_t key_size, size_t val_size) {
    SwissMap *m = malloc(sizeof *m);
    m->cap      = next_pow2(init_cap);
    m->size     = 0;
    m->key_size = key_size;
    m->val_size = val_size;
    m->ctrl     = malloc(m->cap + GROUP_SIZE);
    memset(m->ctrl, EMPTY, m->cap + GROUP_SIZE);
    m->keys     = calloc(m->cap, key_size);
    m->vals     = calloc(m->cap, val_size);
    return m;
}

void sm_free(SwissMap *m) {
    free(m->ctrl);
    free(m->keys);
    free(m->vals);
    free(m);
}

static void sm_grow(SwissMap *m) {
    size_t old_cap    = m->cap;
    uint8_t *old_ctrl = m->ctrl;
    void    *old_keys = m->keys;
    void    *old_vals = m->vals;

    m->cap   *= 2;
    m->size   = 0;
    m->ctrl   = malloc(m->cap + GROUP_SIZE);
    memset(m->ctrl, EMPTY, m->cap + GROUP_SIZE);
    m->keys   = calloc(m->cap, m->key_size);
    m->vals   = calloc(m->cap, m->val_size);

    for (size_t i = 0; i < old_cap; i++) {
        uint8_t c = old_ctrl[i];
        if (c != EMPTY && c != DELETED) {
            void *k_src = (char*)old_keys + i*m->key_size;
            void *v_src = (char*)old_vals + i*m->val_size;
            size_t h    = map_hash(k_src, m->key_size);
            uint8_t h2  = ((uint8_t)(h >> 56)) & 0x7F;
            size_t idx  = h & (m->cap - 1);
            for (;;) {
                for (int j = 0; j < GROUP_SIZE; j++) {
                    size_t pos = (idx + j) & (m->cap - 1);
                    if (m->ctrl[pos] == EMPTY) {
                        m->ctrl[pos] = h2;
                        memcpy((char*)m->keys + pos*m->key_size, k_src, m->key_size);
                        memcpy((char*)m->vals + pos*m->val_size, v_src, m->val_size);
                        m->size++;
                        goto next;
                    }
                }
                idx = (idx + GROUP_SIZE) & (m->cap - 1);
            }
        }
    next:;
    }

    free(old_ctrl);
    free(old_keys);
    free(old_vals);
}

void *sm_get(SwissMap *m, const void *key, int *inserted) {
    if ((m->size + 1)*5 >= m->cap*4)
        sm_grow(m);

    size_t h    = map_hash(key, m->key_size);
    uint8_t h2  = ((uint8_t)(h >> 56)) & 0x7F;
    size_t idx  = h & (m->cap - 1);

    for (;;) {
        for (int j = 0; j < GROUP_SIZE; j++) {
            size_t pos = (idx + j) & (m->cap - 1);
            uint8_t c  = m->ctrl[pos];
            if (c == EMPTY || c == DELETED) {
                m->ctrl[pos] = h2;
                memcpy((char*)m->keys + pos*m->key_size, key, m->key_size);
                m->size++;
                *inserted = 1;
                return (char*)m->vals + pos*m->val_size;
            }
            if (c == h2 && memcmp((char*)m->keys + pos*m->key_size, key, m->key_size) == 0) {
                *inserted = 0;
                return (char*)m->vals + pos*m->val_size;
            }
        }
        idx = (idx + GROUP_SIZE) & (m->cap - 1);
    }
}

int sm_delete(SwissMap *m, const void *key) {
    size_t h    = map_hash(key, m->key_size);
    uint8_t h2  = ((uint8_t)(h >> 56)) & 0x7F;
    size_t idx  = h & (m->cap - 1);

    for (;;) {
        for (int j = 0; j < GROUP_SIZE; j++) {
            size_t pos = (idx + j) & (m->cap - 1);
            uint8_t c  = m->ctrl[pos];
            if (c == EMPTY) return -1;
            if (c == h2 && memcmp((char*)m->keys + pos*m->key_size, key, m->key_size) == 0) {
                m->ctrl[pos] = DELETED;
                m->size--;
                return 0;
            }
        }
        idx = (idx + GROUP_SIZE) & (m->cap - 1);
    }
}

#endif // SWISSMAP_IMPLEMENTATION
