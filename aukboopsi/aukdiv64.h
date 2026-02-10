/*
 * aukdiv64.h - 64-bit signed division routines
 *
 * Use aukdiv64_020.s for 68020+ (uses divs.l/divu.l)
 * Use aukdiv64_000.s for 68000 (shift-subtract only)
 */

#ifndef AUKDIV64_H
#define AUKDIV64_H

#include <exec/types.h>

/*
 * AukDiv64by64 - Divide a 64-bit signed number by a 64-bit signed number
 *
 * Parameters:
 *   quotient_hi - Pointer to store high 32 bits of quotient
 *   quotient_lo - Pointer to store low 32 bits of quotient
 *   dividend_hi - High 32 bits of dividend
 *   dividend_lo - Low 32 bits of dividend
 *   divisor_hi  - High 32 bits of divisor
 *   divisor_lo  - Low 32 bits of divisor
 *
 * Notes:
 *   - Division by zero returns 0
 *   - Optimized path when divisor upper 32 bits are zero
 *   - 68000 version has extra optimization for 16-bit divisors
 */
void AukDiv64by64(LONG *quotient_hi, LONG *quotient_lo,
                  LONG dividend_hi, LONG dividend_lo,
                  LONG divisor_hi, LONG divisor_lo);

/* Convenience macro for fixed-point 32.32 division */
#define AukDivFixed64(qhi, qlo, dhi, dlo, vhi, vlo) \
    AukDiv64by64((qhi), (qlo), (dhi), (dlo), (vhi), (vlo))

#endif /* AUKDIV64_H */
