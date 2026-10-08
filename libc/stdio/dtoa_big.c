/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Every significant digit of a binary64 value, for printf conversions that ask
 * for more digits than the 17 the dtoa engines produce (see dtoa.h).
 *
 * A finite double is m * 2^e. For e >= 0 that is the integer m * 2^e; for
 * e < 0 it is m * 5^-e / 10^-e, the integer m * 5^-e with the decimal point
 * -e places from the right. Either integer is at most 767 decimal digits long,
 * and is built here in base 10^9, so the digits are the exact decimal value of
 * the double. Rounding to the requested number of digits is done on those
 * digits, to nearest with ties to even, as the dtoa engines do.
 */

#include "local-stdio.h"

#ifdef FLOAT64

#define _NEED_IO_FLOAT64

#include "dtoa.h"

#ifdef _NEED_IO_FLOAT_ALL_DIGITS

#define BIG_BASE        1000000000u
#define BIG_BASE_DIGITS 9
/* limb < 10^9 < 2^30, so limb * mul + carry fits in 64 bits for mul <= 2^31. */
#define POW5_STEP 13 /* 5^13 < 2^31 */
#define POW2_STEP 31

#define DOUBLE_MANTISSA_BITS 52
#define DOUBLE_EXPONENT_BITS 11
#define DOUBLE_BIAS          1023

static const uint32_t pow10_9[BIG_BASE_DIGITS] = {
    1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000,
};

static const uint32_t pow5[POW5_STEP + 1] = {
    1, 5, 25, 125, 625, 3125, 15625, 78125, 390625, 1953125, 9765625, 48828125, 244140625,
    1220703125,
};

/* limb[0..n) *= mul (mul <= 2^31); returns the new limb count. */
static int
big_mul(uint32_t *limb, int n, uint32_t mul)
{
    uint64_t carry = 0;
    int      i;

    for (i = 0; i < n; i++) {
        uint64_t t = (uint64_t)limb[i] * mul + carry;
        carry = t / BIG_BASE;
        limb[i] = (uint32_t)(t - carry * BIG_BASE);
    }
    while (carry) {
        limb[n++] = (uint32_t)(carry % BIG_BASE);
        carry /= BIG_BASE;
    }
    return n;
}

/* Decimal digits in limb[0..n), with limb[n - 1] non-zero (or n == 1). */
static int
big_ndigits(const uint32_t *limb, int n)
{
    int      d = (n - 1) * BIG_BASE_DIGITS + 1;
    uint32_t top = limb[n - 1];

    while (top >= 10) {
        top /= 10;
        d++;
    }
    return d;
}

/* The digit worth 10^pos. */
static int
big_digit_at(const uint32_t *limb, int pos)
{
    return (int)(limb[pos / BIG_BASE_DIGITS] / pow10_9[pos % BIG_BASE_DIGITS] % 10);
}

char
__dtoa_big_digit(const struct dtoa_big *dtoa, int i)
{
    int pos = dtoa->ndigits - 1 - i;

    if (i < 0 || pos < 0)
        return '0';
    return (char)('0' + big_digit_at(dtoa->limb, pos));
}

/*
 * Same contract as __dtoa_engine: returns the number of significant digits
 * (read them with __dtoa_big_digit; any further digits are zero), and sets
 * dtoa->exp to the exponent of the first one. In fmode, round to max_decimals
 * digits right of the decimal point (max_decimals >= 0); otherwise to
 * max_digits significant digits.
 */
int
__dtoa_big_engine(uint64_t bits, struct dtoa_big *dtoa, int max_digits, bool fmode,
                  int max_decimals)
{
    uint64_t m = bits & ((1ull << DOUBLE_MANTISSA_BITS) - 1);
    int      ieee_exp = (int)((bits >> DOUBLE_MANTISSA_BITS) & ((1u << DOUBLE_EXPONENT_BITS) - 1));
    int      e2;
    int      scale; /* decimal places right of the point in the integer */
    int      n;
    int      ndig;
    int      exp;
    int      keep;

    dtoa->flags = (bits >> 63) ? DTOA_MINUS : 0;

    if (ieee_exp == (1 << DOUBLE_EXPONENT_BITS) - 1) {
        dtoa->flags |= m ? DTOA_NAN : DTOA_INF;
        return 0;
    }
    if (ieee_exp == 0 && m == 0) {
        dtoa->flags |= DTOA_ZERO;
        dtoa->limb[0] = 0;
        dtoa->ndigits = 1;
        dtoa->exp = 0;
        return 1;
    }

    if (ieee_exp == 0) {
        e2 = 1 - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS;
    } else {
        m |= 1ull << DOUBLE_MANTISSA_BITS;
        e2 = ieee_exp - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS;
    }
    while (!(m & 1)) {
        m >>= 1;
        e2++;
    }

    dtoa->limb[0] = (uint32_t)(m % BIG_BASE);
    dtoa->limb[1] = (uint32_t)(m / BIG_BASE % BIG_BASE);
    n = dtoa->limb[1] ? 2 : 1;

    if (e2 >= 0) {
        scale = 0;
        while (e2 > 0) {
            int step = e2 < POW2_STEP ? e2 : POW2_STEP;
            n = big_mul(dtoa->limb, n, (uint32_t)1 << step);
            e2 -= step;
        }
    } else {
        int e5 = -e2;
        scale = e5;
        while (e5 > 0) {
            int step = e5 < POW5_STEP ? e5 : POW5_STEP;
            n = big_mul(dtoa->limb, n, pow5[step]);
            e5 -= step;
        }
    }

    ndig = big_ndigits(dtoa->limb, n);
    exp = ndig - 1 - scale;

    keep = max_digits;
    if (fmode && max_decimals + exp + 1 < keep)
        keep = max_decimals + exp + 1;

    if (keep < ndig) {
        int  pos = ndig - 1 - keep; /* the first dropped digit */
        int  first;
        bool rest = false;
        bool up;
        int  i;

        if (keep < 0) {
            /* Less than half a unit in the last place: rounds to zero. */
            dtoa->limb[0] = 0;
            dtoa->ndigits = 1;
            dtoa->exp = exp;
            return 0;
        }

        first = big_digit_at(dtoa->limb, pos);
        if (dtoa->limb[pos / BIG_BASE_DIGITS] % pow10_9[pos % BIG_BASE_DIGITS])
            rest = true;
        for (i = 0; i < pos / BIG_BASE_DIGITS && !rest; i++)
            if (dtoa->limb[i])
                rest = true;
        up = first > 5
            || (first == 5 && (rest || (keep > 0 && (big_digit_at(dtoa->limb, pos + 1) & 1))));

        /* Clear the dropped digits. */
        for (i = 0; i < pos / BIG_BASE_DIGITS; i++)
            dtoa->limb[i] = 0;
        i = pos / BIG_BASE_DIGITS;
        dtoa->limb[i] -= dtoa->limb[i] % (pow10_9[pos % BIG_BASE_DIGITS] * 10u);

        if (up) {
            /* Add one in the last kept place, 10^(pos + 1). */
            int      p = pos + 1;
            uint32_t carry;

            i = p / BIG_BASE_DIGITS;
            while (n <= i)
                dtoa->limb[n++] = 0;
            carry = pow10_9[p % BIG_BASE_DIGITS];
            while (carry) {
                if (i == n)
                    dtoa->limb[n++] = 0;
                dtoa->limb[i] += carry;
                carry = dtoa->limb[i] >= BIG_BASE;
                if (carry)
                    dtoa->limb[i] -= BIG_BASE;
                i++;
            }
        }

        while (n > 1 && dtoa->limb[n - 1] == 0)
            n--;
        if (n == 1 && dtoa->limb[0] == 0) {
            /* keep == 0 and it rounded down. */
            dtoa->ndigits = 1;
            dtoa->exp = exp;
            return 0;
        }

        /* A carry out of the top digit makes the value a power of ten one
           place higher. */
        ndig = big_ndigits(dtoa->limb, n);
        exp = ndig - 1 - scale;
        keep = ndig - 1 - pos;
    } else {
        keep = ndig;
    }

    dtoa->ndigits = (int16_t)ndig;
    dtoa->exp = exp;
    return keep;
}

#endif /* _NEED_IO_FLOAT_ALL_DIGITS */

#endif /* FLOAT64 */
