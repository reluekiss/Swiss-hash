#ifndef SWISSMAP_H
#define SWISSMAP_H

#include <stddef.h>
#include <stdint.h>

typedef void*(*sm_alloc_fn)(void* ctx, uint64_t n);
typedef void(*sm_free_fn)(void* ctx, void* p);
typedef uint64_t(*sm_hash_fn)(const void *data, uint64_t len);

typedef struct {
    void* ctx;
    sm_alloc_fn alloc;
    sm_free_fn free;
    sm_hash_fn hash;
} sm_allocator_t;

sm_allocator_t sm_mmap_allocator(void);
void *sm_new(uint64_t init_cap, uint64_t key_size, uint64_t val_size, sm_allocator_t allocs);
void sm_free(void *m, sm_allocator_t allocs);
void *sm_find(void *m, const void *key, uint64_t key_size, uint64_t val_size);
void *sm_get(void *m, const void *key, int *inserted, uint64_t key_size, uint64_t val_size);
int sm_delete(void *m, const void *key, uint64_t key_size, uint64_t val_size);

#define map(m, key_t, val_t, allocs)                                 \
    typedef struct {                                                   \
        sm_allocator_t alloc;                         \
        uint8_t *ctrl;                                              \
        key_t  *keys;                                              \
        val_t  *vals;                                              \
        uint64_t cap, size;                                         \
        uint64_t lgcap;                                               \
    } m##_t;                                                           \
                                                                       \
    static m##_t *m = NULL;                                            \
    static inline void m##_init(void) {                                \
        m = (m##_t*)sm_new(1024, sizeof(key_t), sizeof(val_t), allocs);    \
    }                                                                  \
                                                                       \
    static inline val_t *m##_get(key_t k) {                            \
        if (!m) m##_init();                                              \
        return (val_t*)sm_find(m, &k, sizeof(key_t), sizeof(val_t)); \
    }                                                                  \
                                                                       \
    static inline int m##_put(key_t k, val_t v) {                      \
        if (!m) m##_init();                                              \
        int ins;                                                         \
        *(val_t*)sm_get(m, &k, &ins, sizeof(key_t), sizeof(val_t)) = v;  \
        return ins;                                                      \
    }                                                                  \
                                                                       \
    static inline int m##_erase(key_t k) {                             \
        if (!m) return -1;                                               \
        return sm_delete(m, &k, sizeof(key_t), sizeof(val_t));          \
    }                                                                  \
                                                                       \
    static inline void m##_del(void) {                                 \
        if (m) {                                                         \
          sm_free(m, m->alloc);                                          \
          m = NULL;                                                      \
        }                                                                \
    }

#define put(m, k, v) m##_put(k, v)
#define get(m, k)    m##_get(k)
#define erase(m, k)  m##_erase(k)
#define delete(m)    m##_del()

#define for_each(m, k, v)                                                    \
    for (uint8_t* _ctrl = (m)->ctrl, *_end = _ctrl + (m)->cap; _ctrl < _end; ++_ctrl)                                                           \
        if (!(*_ctrl & 0x80))                                                  \
            for (__typeof__(*(m)->vals)* v = (m)->vals + (_ctrl - (m)->ctrl); v; v = NULL) \
                for (__typeof__(*(m)->keys)* k = (m)->keys + (_ctrl - (m)->ctrl); k; k = NULL)

#define EMPTY      0x80u
#define DELETED    0xFEu

#endif // SWISSMAP_H
