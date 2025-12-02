#include "aukfixed.h"

/*
 * Fixed-point arithmetic implementation
 * 32.32 format: 32 bits for integer part, 32 bits for fractional part
 */


int AukFixed_ToInt(AukFixed value) {
    /* Round to nearest integer */
    return (int)((value + AUK_FIXED_HALF) >> AUK_FIXED_SHIFT);
}

AukFixed AukFixed_FromFraction(int numerator, int denominator) {
    if (denominator == 0) {
        return 0;
    }
    return ((AukFixed)numerator << AUK_FIXED_SHIFT) / denominator;
}

AukFixed AukFixed_Add(AukFixed a, AukFixed b) {
    return a + b;
}

AukFixed AukFixed_Sub(AukFixed a, AukFixed b) {
    return a - b;
}

AukFixed AukFixed_Mul(AukFixed a, AukFixed b) {
    /* Multiply and shift back to maintain 32.32 format */
    return (a * b) >> AUK_FIXED_SHIFT;
}

AukFixed AukFixed_Div(AukFixed a, AukFixed b) {
    if (b == 0) {
        return 0;
    }
    /* Shift left before division to maintain 32.32 format */
    return (a << AUK_FIXED_SHIFT) / b;
}

int AukFixed_Compare(AukFixed a, AukFixed b) {
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

int AukFixed_IsZero(AukFixed value) {
    return value == 0;
}

int AukFixed_IsNegative(AukFixed value) {
    return value < 0;
}

AukFixed AukFixed_Abs(AukFixed value) {
    return value < 0 ? -value : value;
}

AukFixed AukFixed_Min(AukFixed a, AukFixed b) {
    return a < b ? a : b;
}

AukFixed AukFixed_Max(AukFixed a, AukFixed b) {
    return a > b ? a : b;
}

AukFixed AukFixed_Clamp(AukFixed value, AukFixed min, AukFixed max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

AukFixed AukFixed_Lerp(AukFixed a, AukFixed b, AukFixed t) {
    /* Linear interpolation: a + (b - a) * t */
    /* t should be in range [0, 1] (represented as fixed-point) */
    AukFixed diff = AukFixed_Sub(b, a);
    AukFixed scaled = AukFixed_Mul(diff, t);
    return AukFixed_Add(a, scaled);
}
