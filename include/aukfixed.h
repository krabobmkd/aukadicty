#ifndef AUKFIXED_H
#define AUKFIXED_H

/*
 * Fixed-point 64-bit arithmetic for Aukadicty
 * No float or double allowed - use fixed-point for real numbers
 * Format: 32.32 (32 bits integer, 32 bits fractional)
 */

#ifdef __cplusplus
extern "C" {
#endif
#include "compilers.h"

/* Fixed-point type: 32.32 format */
typedef long long AukFixed;

/* Constants */
#define AUK_FIXED_SHIFT 32
#define AUK_FIXED_ONE ((AukFixed)1 << AUK_FIXED_SHIFT)
#define AUK_FIXED_HALF ((AukFixed)1 << (AUK_FIXED_SHIFT - 1))

/* Conversion functions */
INLINE  AukFixed AukFixed_FromDouble(double d) { return (AukFixed)(d * (double)(1LL<<AUK_FIXED_SHIFT)); }

INLINE AukFixed AukFixed_FromInt(long value) {  return (AukFixed)value << AUK_FIXED_SHIFT; }

long AukFixed_ToInt(AukFixed value);
AukFixed AukFixed_FromFraction(long numerator, long denominator);

/* Arithmetic operations */
AukFixed AukFixed_Add(AukFixed a, AukFixed b);
AukFixed AukFixed_Sub(AukFixed a, AukFixed b);
AukFixed AukFixed_Mul(AukFixed a, AukFixed b);
AukFixed AukFixed_Div(AukFixed a, AukFixed b);

/* Comparison */
int AukFixed_Compare(AukFixed a, AukFixed b); /* Returns -1, 0, or 1 */
int AukFixed_IsZero(AukFixed value);
int AukFixed_IsNegative(AukFixed value);

/* Utility functions */
AukFixed AukFixed_Abs(AukFixed value);
AukFixed AukFixed_Min(AukFixed a, AukFixed b);
AukFixed AukFixed_Max(AukFixed a, AukFixed b);
AukFixed AukFixed_Clamp(AukFixed value, AukFixed min, AukFixed max);

/* Linear interpolation */
AukFixed AukFixed_Lerp(AukFixed a, AukFixed b, AukFixed t);

#ifdef __cplusplus
}
#endif

#endif /* AUKFIXED_H */
