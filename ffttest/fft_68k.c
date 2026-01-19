/*
 * fft_68k.c - Fixed-point FFT for Motorola 68000
 *
 * Radix-2 Cooley-Tukey decimation-in-time algorithm.
 * Uses Q15 fixed-point arithmetic with precomputed twiddle factors.
 *
 * The 68000 MULS instruction does 16x16->32 in ~70 cycles.
 * We minimize multiplications by using lookup tables for sin/cos.
 * Additions and shifts are fast (4-8 cycles).
 */

#include "fft_68k.h"

/* For AmigaOS memory allocation */
#ifdef AMIGA
#include <exec/types.h>
#include <clib/exec_protos.h>
#define ALLOC(size)  AllocVec((size), 0)
#define FREE(ptr)    FreeVec(ptr)
#else
/* For testing on other platforms */
#include <stdlib.h>
#define ALLOC(size)  malloc(size)
#define FREE(ptr)    free(ptr)
#endif

/* Precomputed tables */
static short *sin_table = 0;    /* Sine table, FFT_SIZE entries */
static short *window_table = 0; /* Hanning window, FFT_SIZE entries */
static unsigned short *bit_reverse = 0; /* Bit-reversal permutation */

/*
 * Fixed-point multiply: (a * b) >> 15
 * Input: two Q15 values
 * Output: Q15 result
 *
 * On 68000: MULS gives 32-bit result, then ASR/LSR to shift
 */
static short q15_mul(short a, short b)
{
    long result;
    result = (long)a * (long)b;
    /* Add rounding bias before shift */
    result += (1L << 14);
    result >>= Q15_SHIFT;
    return (short)result;
}

/*
 * Bit-reverse an index for decimation-in-time FFT
 */
static unsigned short reverse_bits(unsigned short x, int bits)
{
    unsigned short result = 0;
    int i;
    for (i = 0; i < bits; i++) {
        result = (result << 1) | (x & 1);
        x >>= 1;
    }
    return result;
}

/*
 * Precomputed sine table for first quadrant (65 entries for 256-point FFT).
 * sin(2*pi*k/256) * 32767 for k = 0 to 64.
 * Computed externally with high precision.
 */
static const short sin_quarter[65] = {
        0,   804,  1608,  2410,  3212,  4011,  4808,  5602,
     6393,  7179,  7962,  8739,  9512, 10278, 11039, 11793,
    12539, 13279, 14010, 14732, 15446, 16151, 16846, 17530,
    18204, 18868, 19519, 20159, 20787, 21403, 22005, 22594,
    23170, 23731, 24279, 24811, 25329, 25832, 26319, 26790,
    27245, 27683, 28105, 28510, 28898, 29268, 29621, 29956,
    30273, 30571, 30852, 31113, 31356, 31580, 31785, 31971,
    32137, 32285, 32412, 32521, 32609, 32678, 32728, 32757,
    32767
};

/*
 * Build full sine table from quarter-wave data.
 */
static void compute_sin_table(void)
{
    int i;
    int quarter = FFT_SIZE / 4;

    /* First quadrant: 0 to pi/2 */
    for (i = 0; i <= quarter; i++) {
        sin_table[i] = sin_quarter[i];
    }

    /* Second quadrant: pi/2 to pi (mirror of first) */
    for (i = 1; i < quarter; i++) {
        sin_table[quarter + i] = sin_quarter[quarter - i];
    }

    /* Third and fourth quadrants: negate first half */
    for (i = 0; i < FFT_SIZE / 2; i++) {
        sin_table[FFT_SIZE / 2 + i] = -sin_table[i];
    }
}

/*
 * Get sine value from table
 */
static short get_sin(int index)
{
    return sin_table[index & (FFT_SIZE - 1)];
}

/*
 * Get cosine value from table: cos(x) = sin(x + pi/2)
 */
static short get_cos(int index)
{
    return sin_table[(index + FFT_SIZE / 4) & (FFT_SIZE - 1)];
}

/*
 * Compute Hanning window table
 * w(n) = 0.5 - 0.5*cos(2*pi*n/N)
 *      = 0.5 * (1 - cos(2*pi*n/N))
 *      = sin²(pi*n/N)
 */
static void compute_window_table(void)
{
    int i;
    for (i = 0; i < FFT_SIZE; i++) {
        /* sin²(pi*i/N) in Q15 */
        int half_index = (i * FFT_SIZE / 2) / FFT_SIZE;
        short s = get_sin(half_index);
        /* s² >> 15 to keep Q15 format */
        window_table[i] = q15_mul(s, s);
    }
}

/*
 * Compute bit-reversal permutation table
 */
static void compute_bit_reverse_table(void)
{
    int i;
    for (i = 0; i < FFT_SIZE; i++) {
        bit_reverse[i] = reverse_bits((unsigned short)i, FFT_SIZE_LOG2);
    }
}

int fft_init(void)
{
    /* Allocate tables */
    sin_table = (short *)ALLOC(FFT_SIZE * sizeof(short));
    if (!sin_table) return -1;

    window_table = (short *)ALLOC(FFT_SIZE * sizeof(short));
    if (!window_table) {
        FREE(sin_table);
        sin_table = 0;
        return -1;
    }

    bit_reverse = (unsigned short *)ALLOC(FFT_SIZE * sizeof(unsigned short));
    if (!bit_reverse) {
        FREE(sin_table);
        FREE(window_table);
        sin_table = 0;
        window_table = 0;
        return -1;
    }

    /* Precompute tables */
    compute_sin_table();
    compute_window_table();
    compute_bit_reverse_table();

    return 0;
}

void fft_cleanup(void)
{
    if (sin_table) {
        FREE(sin_table);
        sin_table = 0;
    }
    if (window_table) {
        FREE(window_table);
        window_table = 0;
    }
    if (bit_reverse) {
        FREE(bit_reverse);
        bit_reverse = 0;
    }
}

void fft_apply_window(short *data)
{
    int i;
    for (i = 0; i < FFT_SIZE; i++) {
        data[i] = q15_mul(data[i], window_table[i]);
    }
}

/*
 * Forward FFT: Cooley-Tukey radix-2 decimation-in-time
 *
 * This is the classic "butterfly" algorithm.
 * Each stage halves the number of groups and doubles the group size.
 *
 * Butterfly operation:
 *   temp = W * odd
 *   even' = even + temp
 *   odd'  = even - temp
 *
 * Where W = e^(-j*2*pi*k/N) = cos(2*pi*k/N) - j*sin(2*pi*k/N)
 */
void fft_forward(const short *input, Complex16 *output)
{
    int i, j;
    int stage, group, pair;
    int group_size, half_size;
    int twiddle_step;

    /* Bit-reversal permutation: copy input to output in bit-reversed order */
    /* No input scaling - we scale in butterflies instead */
    for (i = 0; i < FFT_SIZE; i++) {
        j = bit_reverse[i];
        output[j].re = input[i];
        output[j].im = 0;
    }

    /* FFT butterfly stages */
    for (stage = 0; stage < FFT_SIZE_LOG2; stage++) {
        group_size = 1 << (stage + 1);   /* 2, 4, 8, 16, ... */
        half_size = group_size >> 1;      /* 1, 2, 4, 8, ... */
        twiddle_step = FFT_SIZE >> (stage + 1);

        /* Process each group */
        for (group = 0; group < FFT_SIZE; group += group_size) {
            /* Process each butterfly in the group */
            for (pair = 0; pair < half_size; pair++) {
                int idx_even = group + pair;
                int idx_odd = idx_even + half_size;
                int twiddle_idx = pair * twiddle_step;

                /* Get twiddle factor: W = cos - j*sin */
                short w_re = get_cos(twiddle_idx);
                short w_im = -get_sin(twiddle_idx);  /* Negative for forward FFT */

                /* Get odd element */
                long odd_re = output[idx_odd].re;
                long odd_im = output[idx_odd].im;

                /* Complex multiply: temp = W * odd (using 32-bit intermediates) */
                long temp_re = q15_mul(w_re, (short)odd_re) - q15_mul(w_im, (short)odd_im);
                long temp_im = q15_mul(w_re, (short)odd_im) + q15_mul(w_im, (short)odd_re);

                /* Get even element */
                long even_re = output[idx_even].re;
                long even_im = output[idx_even].im;

                /* Butterfly with scaling by 1/2 to prevent overflow */
                /* This distributes the 1/N scaling across all stages */
                output[idx_even].re = (short)((even_re + temp_re) >> 1);
                output[idx_even].im = (short)((even_im + temp_im) >> 1);
                output[idx_odd].re = (short)((even_re - temp_re) >> 1);
                output[idx_odd].im = (short)((even_im - temp_im) >> 1);
            }
        }
    }
}

/*
 * Inverse FFT: Same as forward but with conjugate twiddle factors
 * and scaling by 1/N at the end.
 *
 * Actually, for IFFT: use +j instead of -j in twiddle factors,
 * then divide result by N.
 */
void fft_inverse(const Complex16 *input, short *output)
{
    int i, j;
    int stage, group, pair;
    int group_size, half_size;
    int twiddle_step;

    /* Working buffer - use static to avoid stack allocation on 68000 */
    static Complex16 work[FFT_SIZE];

    /* Bit-reversal permutation */
    for (i = 0; i < FFT_SIZE; i++) {
        j = bit_reverse[i];
        work[j].re = input[i].re;
        work[j].im = input[i].im;
    }

    /* IFFT butterfly stages */
    for (stage = 0; stage < FFT_SIZE_LOG2; stage++) {
        group_size = 1 << (stage + 1);
        half_size = group_size >> 1;
        twiddle_step = FFT_SIZE >> (stage + 1);

        for (group = 0; group < FFT_SIZE; group += group_size) {
            for (pair = 0; pair < half_size; pair++) {
                int idx_even = group + pair;
                int idx_odd = idx_even + half_size;
                int twiddle_idx = pair * twiddle_step;

                /* Conjugate twiddle factor: W* = cos + j*sin */
                short w_re = get_cos(twiddle_idx);
                short w_im = get_sin(twiddle_idx);  /* Positive for inverse FFT */

                long odd_re = work[idx_odd].re;
                long odd_im = work[idx_odd].im;

                long temp_re = q15_mul(w_re, (short)odd_re) - q15_mul(w_im, (short)odd_im);
                long temp_im = q15_mul(w_re, (short)odd_im) + q15_mul(w_im, (short)odd_re);

                long even_re = work[idx_even].re;
                long even_im = work[idx_even].im;

                /* Scale by 1/2 to prevent overflow, matching forward FFT */
                work[idx_even].re = (short)((even_re + temp_re) >> 1);
                work[idx_even].im = (short)((even_im + temp_im) >> 1);
                work[idx_odd].re = (short)((even_re - temp_re) >> 1);
                work[idx_odd].im = (short)((even_im - temp_im) >> 1);
            }
        }
    }

    /* Copy real part to output, scaling up to compensate */
    /* Total scaling: forward 1/N, inverse 1/N, so scale up by N */
    for (i = 0; i < FFT_SIZE; i++) {
        long temp = (long)work[i].re << FFT_SIZE_LOG2;
        if (temp > 32767L) temp = 32767L;
        if (temp < -32768L) temp = -32768L;
        output[i] = (short)temp;
    }
}
