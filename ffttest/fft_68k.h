/*
 * fft_68k.h - Fixed-point FFT for Motorola 68000
 *
 * C89 compatible, uses 16-bit fixed-point Q15 format.
 * Optimized for 68000: minimizes multiplications, uses lookup tables.
 *
 * Usage:
 *   1. Call fft_init() once at startup
 *   2. Call fft_forward() to transform PCM -> frequency domain
 *   3. Call fft_inverse() to transform frequency domain -> PCM
 */

#ifndef FFT_68K_H
#define FFT_68K_H

/* FFT size - must be power of 2. 256 is good balance for 68000 */
#define FFT_SIZE      256
#define FFT_SIZE_LOG2 8

/* Fixed-point Q15 format: value = integer / 32768 */
/* Range: -1.0 to +0.99997 */
#define Q15_SHIFT     15
#define Q15_ONE       32767

/* Complex number in fixed-point Q15 */
typedef struct {
    short re;  /* real part, Q15 */
    short im;  /* imaginary part, Q15 */
} Complex16;

/*
 * Initialize FFT tables. Call once at startup.
 * Returns 0 on success, -1 on failure.
 */
int fft_init(void);

/*
 * Free FFT tables. Call at shutdown.
 */
void fft_cleanup(void);

/*
 * Forward FFT: time domain -> frequency domain
 *
 * input:  Array of FFT_SIZE 16-bit signed PCM samples
 * output: Array of FFT_SIZE complex frequency bins
 *
 * Note: Input samples should be pre-scaled to avoid overflow.
 *       Divide by FFT_SIZE or use samples in range [-16384, 16383]
 */
void fft_forward(const short *input, Complex16 *output);

/*
 * Inverse FFT: frequency domain -> time domain
 *
 * input:  Array of FFT_SIZE complex frequency bins
 * output: Array of FFT_SIZE 16-bit signed PCM samples
 */
void fft_inverse(const Complex16 *input, short *output);

/*
 * Apply Hanning window to reduce spectral leakage.
 * Call before fft_forward() for better frequency resolution.
 *
 * data: Array of FFT_SIZE samples, modified in place
 */
void fft_apply_window(short *data);

#endif /* FFT_68K_H */
