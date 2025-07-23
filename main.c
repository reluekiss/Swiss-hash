#include <stdio.h>
#define SWISSMAP_IMPLEMENTATION
#include "hash.h"
#include <assert.h>

typedef struct {
    int a;
    float b;
    struct {
        int c;
        float d;
    } e;
} type;

map(map, char*, type, (sm_allocator_t){0});

int main() {
    type a = {
        .a = 1,
        .b = 1.0,
        .e = {
            .c = 1,
            .d = 1.0,
        },
    };
    char* b = "hi";

    put(map, b, a);
    type *c = get(map, b);
    assert(c->a == a.a);
    erase(map, b);
    type* d = get(map, b);
    assert(d->a != a.a);
    delete(map);
}
