#include <stdint.h>
#include <stddef.h>
#include <string.h>

/* pick vector backend unless scalar is forced */
#ifndef XXH3_USE_SCALAR
# if defined(__AVX2__)
#   include <immintrin.h>
#   define XXH3_USE_AVX2
# elif defined(__SSE2__)
#   include <emmintrin.h>
#   define XXH3_USE_SSE2
# endif
#endif

/*— helpers —*/
static inline uint64_t read64(const void *p) {
    uint64_t v; memcpy(&v, p, 8); return v;
}
static inline uint64_t rotl64(uint64_t x, int r) {
    return (x << r) | (x >> (64 - r));
}

/*— default 192-byte secret —*/
static const uint8_t XXH3_kSecret[192] = {
    0xb8,0xfe,0x6c,0x39,0x23,0xa4,0x4b,0xbe,0x7c,0x01,0x81,0x2c,0xf7,0x21,0xad,
    0x1c,0xde,0xd4,0x6d,0xe9,0x83,0x90,0x97,0xdb,0x72,0x40,0xa4,0xa4,0xb7,0xb3,
    0x67,0x1f,0xcb,0x79,0xe6,0x4e,0xcc,0xc0,0xe5,0x78,0x82,0x5a,0xd0,0x7d,0xcc,
    0xff,0x72,0x21,0xb8,0x08,0x46,0x74,0xf7,0x43,0x24,0x8e,0xe0,0x35,0x90,0xe6,
    0x81,0x3a,0x26,0x4c,0x3c,0x28,0x52,0xbb,0x91,0xc3,0x00,0xcb,0x88,0xd0,0x65,
    0x8b,0x1b,0x53,0x2e,0xa3,0x71,0x64,0x48,0x97,0xa2,0x0d,0xf9,0x4e,0x38,0x19,
    0xef,0x46,0xa9,0xde,0xac,0xd8,0xa8,0xfa,0x76,0x3f,0xe3,0x9c,0x34,0x3f,0xf9,
    0xdc,0xbb,0xc7,0xc7,0x0b,0x4f,0x1d,0x8a,0x51,0xe0,0x4b,0xcd,0xb4,0x59,0x31,
    0xc8,0x9f,0x7e,0xc9,0xd9,0x78,0x73,0x64,0xea,0xc5,0xac,0x83,0x34,0xd3,0xeb,
    0xc3,0xc5,0x81,0xa0,0xff,0xfa,0x13,0x63,0xeb,0x17,0x0d,0xdd,0x51,0xb7,0xf0,
    0xda,0x49,0xd3,0x16,0x55,0x26,0x29,0xd4,0x68,0x9e,0x2b,0x16,0xbe,0x58,0x7d,
    0x47,0xa1,0xfc,0x8f,0xf8,0xb8,0xd1,0x7a,0xd0,0x31,0xce,0x45,0xcb,0x3a,0x8f,
    0x95,0x16,0x04,0x28,0xaf,0xd7,0xfb,0xca,0xbb,0x4b,0x40,0x7e
};

/*— stripe‐accumulator: 64‐byte -> eight 64‐bit lanes —*/
#if defined(XXH3_USE_AVX2)

static void accumulate512(uint64_t acc[8], const uint8_t *in, const uint8_t *sec) {
    __m256i *xacc    = (__m256i*)acc;
    const __m256i *xinp = (const __m256i*)in;
    const __m256i *xsec = (const __m256i*)sec;
    __m256i prime32 = _mm256_set1_epi32((int)0x9E3779B1U);

    for (int i = 0; i < 2; i++) {
        __m256i data = _mm256_loadu_si256(xinp + i);
        __m256i key  = _mm256_loadu_si256(xsec + i);
        __m256i dk   = _mm256_xor_si256(data, key);
        __m256i lo   = _mm256_shuffle_epi32(dk, _MM_SHUFFLE(0,3,0,1));
        __m256i prod = _mm256_mul_epu32(dk, lo);
        __m256i swap = _mm256_shuffle_epi32(data, _MM_SHUFFLE(1,0,3,2));
        __m256i sum  = _mm256_add_epi64(xacc[i], swap);
        xacc[i]      = _mm256_add_epi64(sum, prod);
    }
}

#elif defined(XXH3_USE_SSE2)

static void accumulate512(uint64_t acc[8], const uint8_t *in, const uint8_t *sec) {
    __m128i *xacc    = (__m128i*)acc;
    const __m128i *xinp = (const __m128i*)in;
    const __m128i *xsec = (const __m128i*)sec;
    __m128i prime32 = _mm_set1_epi32(0x9E3779B1U);

    for (int i = 0; i < 4; i++) {
        __m128i data = _mm_loadu_si128(xinp + i);
        __m128i key  = _mm_loadu_si128(xsec + i);
        __m128i dk   = _mm_xor_si128(data, key);
        __m128i lo   = _mm_shuffle_epi32(dk, _MM_SHUFFLE(0,3,0,1));
        __m128i prod = _mm_mul_epu32(dk, lo);
        __m128i swap = _mm_shuffle_epi32(data, _MM_SHUFFLE(1,0,3,2));
        __m128i sum  = _mm_add_epi64(xacc[i], swap);
        xacc[i]      = _mm_add_epi64(sum, prod);
    }
}

#else  /* scalar */

static void accumulate512(uint64_t acc[8], const uint8_t *in, const uint8_t *sec) {
    for (int i = 0; i < 8; i++) {
        uint64_t v = read64(in + i*8);
        uint64_t k = read64(sec + i*8);
        uint64_t x = v ^ k;
        /* low‐32 * high‐32 */
        uint64_t prod = (uint32_t)x * (uint32_t)(x >> 32);
        acc[i] += prod;
    }
}

#endif

/*— regime-1: 0–16 bytes —*/
static uint64_t xxh3_len_0to16(const uint8_t *p, uint64_t len) {
    if (len > 8) {
        uint64_t hi = read64(p + len - 8);
        uint64_t lo = read64(p);
        uint64_t h = lo ^ (hi + rotl64(hi, 32));
        h ^= h >> 47; h *= 0x165667919E3779F9ULL; h ^= h >> 47;
        return h;
    }
    if (len >= 4) {
        uint32_t a = *(const uint32_t*)p;
        uint32_t b = *(const uint32_t*)(p + len - 4);
        uint64_t h = ((uint64_t)a << 32) | b;
        h ^= rotl64(h, 13); h *= 0x9FB21C651E98DF25ULL; h ^= h >> 29;
        return h;
    }
    if (len) {
        uint8_t c1 = p[0], c2 = p[len >> 1], c3 = p[len - 1];
        uint32_t w = (c1) | (c2 << 8) | (c3 << 16) | (len << 24);
        uint64_t h = w ^ (read64(XXH3_kSecret) ^ read64(XXH3_kSecret+8));
        h ^= h >> 23; h *= 0x9FB21C651E98DF25ULL; h ^= h >> 47;
        return h;
    }
    return (read64(XXH3_kSecret) ^ read64(XXH3_kSecret+8))
         * 0x9FB21C651E98DF25ULL;
}

/*— regime-2: 17–128 bytes —*/
static uint64_t xxh3_len_17to128(const uint8_t *p, uint64_t len) {
    uint64_t acc = len * 0x9E3779B185EBCA87ULL;
    uint64_t n = (len - 1) / 16;
    for (uint64_t i = 0; i <= n; i++) {
        const uint8_t *c = p + 16*i;
        uint64_t lo = read64(c)     ^ (read64(XXH3_kSecret+16*i)     + acc);
        uint64_t hi = read64(c + 8) ^ (read64(XXH3_kSecret+16*i + 8) - acc);
        acc += rotl64(lo * hi, 31);
    }
    acc ^= acc >> 37; acc *= 0x165667919E3779F9ULL; acc ^= acc >> 32;
    return acc;
}

/*— regime-3: 129–240 bytes —*/
static uint64_t xxh3_len_129to240(const uint8_t *p, uint64_t len) {
    uint64_t acc = len * 0x9E3779B185EBCA87ULL;
    int rounds = (int)len / 32;
    for (int i = 0; i < 4; i++) {
        const uint8_t *a = p + 32*i;
        const uint8_t *b = a + 16;
        uint64_t lo = read64(a) ^ read64(XXH3_kSecret + 32*i);
        uint64_t hi = read64(b) ^ read64(XXH3_kSecret + 32*i + 8);
        acc += rotl64(lo * hi, 31);
    }
    acc ^= acc >> 41; acc *= 0x165667919E3779F9ULL; acc ^= acc >> 47;
    return acc;
}

/*— regime-4: >240 bytes —*/
static uint64_t xxh3_hashLong_64b(const uint8_t *p, uint64_t len) {
    uint64_t acc[8] = {
        0x9E3779B185EBCA87ULL,0xC2B2AE3D27D4EB4FULL,
        0x165667B19E3779F9ULL,0x85EBCA77C2B2AE63ULL,
        0x27D4EB2F165667C5ULL,0x9E3779B185EBCA87ULL,
        0xC2B2AE3D27D4EB4FULL,0x165667B19E3779F9ULL
    };
    uint64_t stripes = len / 64;
    for (uint64_t s = 0; s < stripes; s++) {
        accumulate512(acc, p + s*64, XXH3_kSecret);
    }
    uint64_t h = len * 0x9E3779B185EBCA87ULL;
    for (int i = 0; i < 8; i++) {
        h += acc[i] ^ read64(XXH3_kSecret + 64 + i*8);
        h = rotl64(h, 27) * 0x9E3779B185EBCA87ULL;
    }
    h ^= h >> 33; h *= 0xC2B2AE3D27D4EB4FULL;
    h ^= h >> 29; h *= 0x165667B19E3779F9ULL;
    h ^= h >> 32;
    return h;
}

uint64_t XXH3_64bits(const void *data, uint64_t len) {
    const uint8_t *p = (const uint8_t*)data;
    if (len <= 16)  return xxh3_len_0to16    (p,len);
    if (len <= 128) return xxh3_len_17to128  (p,len);
    if (len <= 240) return xxh3_len_129to240 (p,len);
    return xxh3_hashLong_64b(p,len);
}
