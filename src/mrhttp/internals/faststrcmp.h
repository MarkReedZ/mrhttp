
static const unsigned char lct[] __attribute__((aligned(64))) = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
        0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
        0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
        0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
        0x78, 0x79, 0x7a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
        0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
        0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
        0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
        0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
        0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
        0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
        0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
        0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
        0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
        0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
        0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
        0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
        0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
        0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
        0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
        0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
        0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
        0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
        0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
        0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

static struct {
        __m128i A128;
        __m128i a128;
        __m128i D128;
        __m128i CASE128;

        __m256i A256;
        __m256i a256;
        __m256i D256;
        __m256i CASE256;
} __C;

static inline unsigned int
__stricmp_avx2_2lc(const char *s0, const char *s1)
{       
        __m256i v0 = _mm256_lddqu_si256((void *)s0);
        __m256i v1 = _mm256_lddqu_si256((void *)s1);
        
        __m256i sub = _mm256_sub_epi8(v0, __C.A256);
        __m256i cmp_r = _mm256_cmpgt_epi8(__C.D256, sub);
        __m256i lc = _mm256_and_si256(cmp_r, __C.CASE256);
        __m256i vl = _mm256_or_si256(v0, lc);
        
        __m256i eq = _mm256_cmpeq_epi8(vl, v1);
        
        return ~_mm256_movemask_epi8(eq);
}

static inline unsigned int
__stricmp_avx2_2lc_64(const char *s0, const char *s1)
{       
        __m256i v00 = _mm256_lddqu_si256((void *)s0);
        __m256i v01 = _mm256_lddqu_si256((void *)(s0 + 32));
        __m256i v10 = _mm256_lddqu_si256((void *)s1);
        __m256i v11 = _mm256_lddqu_si256((void *)(s1 + 32));
        
        __m256i sub00 = _mm256_sub_epi8(v00, __C.A256);
        __m256i sub01 = _mm256_sub_epi8(v01, __C.A256);
        __m256i cmp_r00 = _mm256_cmpgt_epi8(__C.D256, sub00);
        __m256i cmp_r01 = _mm256_cmpgt_epi8(__C.D256, sub01);
        
        __m256i lc00 = _mm256_and_si256(cmp_r00, __C.CASE256);
        __m256i lc01 = _mm256_and_si256(cmp_r01, __C.CASE256);
        
        __m256i vl00 = _mm256_or_si256(v00, lc00);
        __m256i vl01 = _mm256_or_si256(v01, lc01);
        
        __m256i eq0 = _mm256_cmpeq_epi8(vl00, v10);
        __m256i eq1 = _mm256_cmpeq_epi8(vl01, v11);
        
        return ~(_mm256_movemask_epi8(eq0) & _mm256_movemask_epi8(eq1));
}

static inline unsigned int
__stricmp_avx2_2lc_128(const char *s0, const char *s1)
{
        __m256i v00 = _mm256_lddqu_si256((void *)s0);
        __m256i v01 = _mm256_lddqu_si256((void *)(s0 + 32));
        __m256i v02 = _mm256_lddqu_si256((void *)(s0 + 64));
        __m256i v03 = _mm256_lddqu_si256((void *)(s0 + 96));
        __m256i v10 = _mm256_lddqu_si256((void *)s1);
        __m256i v11 = _mm256_lddqu_si256((void *)(s1 + 32));
        __m256i v12 = _mm256_lddqu_si256((void *)(s1 + 64));
        __m256i v13 = _mm256_lddqu_si256((void *)(s1 + 96));

        __m256i sub00 = _mm256_sub_epi8(v00, __C.A256);
        __m256i sub01 = _mm256_sub_epi8(v01, __C.A256);
        __m256i sub02 = _mm256_sub_epi8(v02, __C.A256);
        __m256i sub03 = _mm256_sub_epi8(v03, __C.A256);

        __m256i cmp_r00 = _mm256_cmpgt_epi8(__C.D256, sub00);
        __m256i cmp_r01 = _mm256_cmpgt_epi8(__C.D256, sub01);
        __m256i cmp_r02 = _mm256_cmpgt_epi8(__C.D256, sub02);
        __m256i cmp_r03 = _mm256_cmpgt_epi8(__C.D256, sub03);

        __m256i lc00 = _mm256_and_si256(cmp_r00, __C.CASE256);
        __m256i lc01 = _mm256_and_si256(cmp_r01, __C.CASE256);
        __m256i lc02 = _mm256_and_si256(cmp_r02, __C.CASE256);
        __m256i lc03 = _mm256_and_si256(cmp_r03, __C.CASE256);

        __m256i vl00 = _mm256_or_si256(v00, lc00);
        __m256i vl01 = _mm256_or_si256(v01, lc01);
        __m256i vl02 = _mm256_or_si256(v02, lc02);
        __m256i vl03 = _mm256_or_si256(v03, lc03);

        __m256i eq0 = _mm256_cmpeq_epi8(vl00, v10);
        __m256i eq1 = _mm256_cmpeq_epi8(vl01, v11);
        __m256i eq2 = _mm256_cmpeq_epi8(vl02, v12);
        __m256i eq3 = _mm256_cmpeq_epi8(vl03, v13);

        return ~(_mm256_movemask_epi8(eq0) & _mm256_movemask_epi8(eq1)
                 & _mm256_movemask_epi8(eq2) & _mm256_movemask_epi8(eq3));
}


static inline int
__stricmp_avx2_2lc_tail(const char *s1, const char *s2, size_t len)
{
        __m128i sub, cmp_r, lc, vl, eq;
        __m128d v0, v1;

        if (len >= 16) {
                int r;
                __m128i v0 = _mm_lddqu_si128((void *)s1);
                __m128i v1 = _mm_lddqu_si128((void *)s2);
                sub = _mm_sub_epi8(v0, __C.A128);
                cmp_r = _mm_cmpgt_epi8(__C.D128, sub);
                lc = _mm_and_si128(cmp_r, __C.CASE128);
                vl = _mm_or_si128(v0, lc);
                eq = _mm_cmpeq_epi8(vl, v1);
                r = _mm_movemask_epi8(eq) ^ 0xffff;
                if (len == 16 || r)
                        return r;
                s1 += len - 16;
                s2 += len - 16;
                len = 16;
        }

        // 8 to 16 bytes.  Ignore the uninit since we loadl and h
#pragma GCC diagnostic ignored "-Wuninitialized"
        v0 = _mm_loadh_pd(v0, (double *)s1);
#pragma GCC diagnostic ignored "-Wuninitialized"
        v1 = _mm_loadh_pd(v1, (double *)s2);
        v0 = _mm_loadl_pd(v0, (double *)(s1 + len - 8));
        v1 = _mm_loadl_pd(v1, (double *)(s2 + len - 8));

        sub = _mm_sub_epi8((__m128i)v0, __C.A128);
        cmp_r = _mm_cmpgt_epi8(__C.D128, (__m128i)sub);
        lc = _mm_and_si128(cmp_r, __C.CASE128);
        vl = _mm_or_si128((__m128i)v0, lc);
        eq = _mm_cmpeq_epi8(vl, (__m128i)v1);

        return _mm_movemask_epi8(eq) ^ 0xffff;
}

// Returns 0 if equal 1 otherwise
int fast_compare(const char *s1, const char *s2, size_t len)
{
        size_t i = 0;
        int c = 0;

        switch (len) {
        case 0:
                return 0;
        case 8:
                c |= lct[(unsigned char)s1[7]] ^ s2[7];
        case 7:
                c |= lct[(unsigned char)s1[6]] ^ s2[6];
        case 6:
                c |= lct[(unsigned char)s1[5]] ^ s2[5];
        case 5:
                c |= lct[(unsigned char)s1[4]] ^ s2[4];
        case 4:
                c |= lct[(unsigned char)s1[3]] ^ s2[3];
        case 3:
                c |= lct[(unsigned char)s1[2]] ^ s2[2];
        case 2:
                c |= lct[(unsigned char)s1[1]] ^ s2[1];
        case 1:
                c |= lct[(unsigned char)s1[0]] ^ s2[0];
                return c;
        }
        if (likely(len < 32))
                return __stricmp_avx2_2lc_tail(s1, s2, len);

        for ( ; unlikely(i + 128 <= len); i += 128)
                if (__stricmp_avx2_2lc_128(s1 + i, s2 + i))
                        return 1;
        if (unlikely(i + 64 <= len)) {
                if (__stricmp_avx2_2lc_64(s1 + i, s2 + i))
                        return 1;
                i += 64;
        }
        if (unlikely(i + 32 <= len)) {
                if (__stricmp_avx2_2lc(s1 + i, s2 + i))
                        return 1;
                i += 32;
        }
        if (i == len)
                return 0;
        len -= i;
        if (len < 8) {
                i -= 8 - len;
                len = 8;
        }

        return __stricmp_avx2_2lc_tail(s1 + i, s2 + i, len);
}
