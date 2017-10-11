#include "cor_uuid.h"

#include <fcntl.h>
#include <unistd.h>
#include <x86intrin.h>

int
cor_uuid_init(cor_uuid_seed_t *seed)
{
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd == -1) {
        return cor_error;
    }
    int rc = read(fd, seed, sizeof(cor_uuid_seed_t));
    close(fd);
    return rc == sizeof(cor_uuid_seed_t) ? cor_ok : cor_error;
}

/*
 * Used xoroshiro128 (http://xoroshiro.di.unimi.it/)
 */
void
cor_uuid_generate(cor_uuid_seed_t *seed, cor_uuid_t *uuid)
{
    __m128i s0 = _mm_loadu_si128((__m128i *) seed);
    __m128i s1 = _mm_loadu_si128((__m128i *) seed + 1);
    /*  result = s0 + s1  */
    _mm_store_si128((__m128i *) uuid, _mm_add_epi64(s0, s1));
    /*  s1 ^= s0  */
    s1 = _mm_xor_si128(s0, s1);
    /*  s[0] = ((s0 << 55) | (s0 >> 9)) ^ s1 ^ (s1 << 14)  */
    _mm_store_si128((__m128i *) seed, 
        _mm_xor_si128(
            _mm_xor_si128(
                _mm_or_si128(
                    _mm_slli_epi64(s0, 55),
                    _mm_srli_epi64(s0, 9)
                ),
                s1
            ),
            _mm_slli_epi64(s1, 14)
        )
    );
    /*  s[1] = (s1 << 36) | (s1 >> 28) */
    _mm_store_si128((__m128i *) seed + 1, 
        _mm_or_si128(
            _mm_slli_epi64(s1, 36),
            _mm_srli_epi64(s1, 28)
        )
    );
}

void
cor_uuid_parse(const char *p, cor_uuid_t *uuid)
{
    __m128i c_0 = _mm_set1_epi8('0');
    __m128i c_10 = _mm_set1_epi8(10);
    __m128i c_a = _mm_set1_epi8('a' - '0' - 10);
    /*  first 16 bytes  */
    __m128i src = _mm_loadu_si128((const __m128i *) p);
    /*  substract '0' from all bytes  */
    __m128i d = _mm_sub_epi8(src, c_0);
    /*  create mask where result bytes greater than 10 so its a-f symbols  */
    __m128i m = _mm_cmpgt_epi8(d, c_10);
    /*  apply mask to left only needed bytes  */
    __m128i af = _mm_and_si128(c_a, m);
    /**/
    d = _mm_sub_epi8(d, af);
    int8_t *u1 = (int8_t *) &d;
    /**/
    uint8_t *u = (uint8_t *) uuid;
    *u++ = (u1[0] << 4) | u1[1];
    *u++ = (u1[2] << 4) | u1[3];
    *u++ = (u1[4] << 4) | u1[5];
    *u++ = (u1[6] << 4) | u1[7];
    *u++ = (u1[9] << 4) | u1[10];
    *u++ = (u1[11] << 4) | u1[12];
    *u++ = (u1[14] << 4) | u1[15];
    /*  next 16 bytes  */
    p += 16;
    src = _mm_loadu_si128((const __m128i *) p);
    d = _mm_sub_epi8(src, c_0);
    m = _mm_cmpgt_epi8(d, c_10);
    af = _mm_and_si128(c_a, m);
    d = _mm_sub_epi8(d, af);
    u1 = (int8_t *) &d;
    /**/
    *u++ = (u1[0] << 4) | u1[1];
    *u++ = (u1[3] << 4) | u1[4];
    *u++ = (u1[5] << 4) | u1[6];
    *u++ = (u1[8] << 4) | u1[9];
    *u++ = (u1[10] << 4) | u1[11];
    *u++ = (u1[12] << 4) | u1[13];
    *u++ = (u1[14] << 4) | u1[15];
    /*  last 4 bytes  */
    p += 4;
    src = _mm_loadu_si128((const __m128i *) p);
    d = _mm_sub_epi8(src, c_0);
    m = _mm_cmpgt_epi8(d, c_10);
    af = _mm_and_si128(c_a, m);
    d = _mm_sub_epi8(d, af);
    u1 = (int8_t *) &d;
    /**/
    *u++ = (u1[12] << 4) | u1[13];
    *u++ = (u1[14] << 4) | u1[15];
}

void
cor_uuid_unparse(cor_uuid_t *uuid, char *p)
{
    static const char *hex = "0123456789abcdef";
    uint8_t *u = (uint8_t *) uuid;

#define PRINT() \
    *p++ = hex[*u >> 4]; \
    *p++ = hex[*u & 0x0f]; \
    u++;

    PRINT();
    PRINT();
    PRINT();
    PRINT();
    *p++ = '-';
    PRINT();
    PRINT();
    *p++ = '-';
    PRINT();
    PRINT();
    *p++ = '-';
    PRINT();
    PRINT();
    *p++ = '-';
    PRINT();
    PRINT();
    PRINT();
    PRINT();
    PRINT();
    PRINT();

#undef PRINT
}
