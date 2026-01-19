/*
 * fft_test.c - Test program for FFT round-trip
 *
 * Tests that: IFFT(FFT(signal)) ≈ signal
 */

#include <stdio.h>
#include "fft_68k.h"

/* Generate a simple test signal: sum of two sine waves */
static void generate_test_signal(short *buffer, int size)
{
    int i;
    /*
     * Create a signal with two frequency components:
     * - 4 cycles over the window (low frequency)
     * - 32 cycles over the window (higher frequency)
     *
     * For a 256-sample window, these correspond to bins 4 and 32.
     */
    for (i = 0; i < size; i++) {
        long sample = 0;

        /* Low frequency component: 4 cycles, amplitude 8000 */
        /* sin(2*pi*4*i/256) = sin(pi*i/32) */
        {
            long angle = (i * 4L * 256) / size;  /* 0-1023 for full cycle */
            long x = angle & 255;
            long s;

            /* Simple sine approximation */
            if (x < 64) {
                s = x * 512;
            } else if (x < 128) {
                s = (128 - x) * 512;
            } else if (x < 192) {
                s = -(x - 128) * 512;
            } else {
                s = -(256 - x) * 512;
            }
            s /= 32;  /* Scale to reasonable range */
            sample += s * 8;
        }

        /* Higher frequency component: 32 cycles, amplitude 4000 */
        {
            long angle = (i * 32L * 256) / size;
            long x = angle & 255;
            long s;

            if (x < 64) {
                s = x * 512;
            } else if (x < 128) {
                s = (128 - x) * 512;
            } else if (x < 192) {
                s = -(x - 128) * 512;
            } else {
                s = -(256 - x) * 512;
            }
            s /= 32;
            sample += s * 4;
        }

        if (sample > 16383) sample = 16383;
        if (sample < -16384) sample = -16384;
        buffer[i] = (short)sample;
    }
}

/* Calculate error between two signals */
static long calculate_error(const short *a, const short *b, int size)
{
    long total_error = 0;
    int i;
    for (i = 0; i < size; i++) {
        long diff = (long)a[i] - (long)b[i];
        if (diff < 0) diff = -diff;
        total_error += diff;
    }
    return total_error / size;  /* Average absolute error */
}

/* Print first few samples */
static void print_samples(const char *label, const short *data, int count)
{
    int i;
    printf("%s: ", label);
    for (i = 0; i < count && i < 8; i++) {
        printf("%6d ", data[i]);
    }
    printf("...\n");
}

/* Print magnitude of frequency bins */
static void print_spectrum(const Complex16 *freq, int size)
{
    int i;
    printf("Spectrum (first 64 bins):\n");
    for (i = 0; i < 64 && i < size / 2; i++) {
        /* Magnitude approximation: |z| ≈ max(|re|,|im|) + min(|re|,|im|)/2 */
        int re = freq[i].re;
        int im = freq[i].im;
        int mag;

        if (re < 0) re = -re;
        if (im < 0) im = -im;

        if (re > im) {
            mag = re + (im >> 1);
        } else {
            mag = im + (re >> 1);
        }

        if (mag > 100) {  /* Only print significant bins */
            printf("  Bin %2d: magnitude %d\n", i, mag);
        }
    }
}

int main(void)
{
    short original[FFT_SIZE];
    short reconstructed[FFT_SIZE];
    Complex16 frequency[FFT_SIZE];
    long error;

    printf("FFT Round-Trip Test\n");
    printf("FFT size: %d samples\n\n", FFT_SIZE);

    /* Initialize FFT tables */
    if (fft_init() != 0) {
        printf("Failed to initialize FFT tables!\n");
        return 1;
    }

    /* Generate test signal */
    generate_test_signal(original, FFT_SIZE);
    print_samples("Original", original, 8);

    /* Forward FFT */
    printf("\nPerforming forward FFT...\n");
    fft_forward(original, frequency);
    print_spectrum(frequency, FFT_SIZE);

    /* Inverse FFT */
    printf("\nPerforming inverse FFT...\n");
    fft_inverse(frequency, reconstructed);
    print_samples("Reconstructed", reconstructed, 8);

    /* Calculate reconstruction error */
    error = calculate_error(original, reconstructed, FFT_SIZE);
    printf("\nAverage reconstruction error: %ld (per sample)\n", error);

    if (error < 1000) {
        printf("SUCCESS: Signal reconstructed within acceptable error.\n");
    } else {
        printf("WARNING: High reconstruction error - may need adjustment.\n");
    }

    /* Cleanup */
    fft_cleanup();

    return 0;
}
