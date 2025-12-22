/**
 * compilers.h - Compiler-specific macros and definitions for AmigaOS
 */
#ifndef COMPILERS_H
#define COMPILERS_H

/* Register macros for different compilers */
#ifdef __SASC
    #define REG(reg,arg) register __##reg arg
    #define ASM
    #define SAVEDS __saveds
#endif

#ifdef __GNUC__
    #define REG(reg,arg) arg __asm(#reg)
    #define ASM
    #define SAVEDS __attribute__((saveds))
#endif

#ifdef __VBCC__
    #define REG(reg,arg) __reg(#reg) arg
    #define ASM
    #define SAVEDS __saveds
#endif

/* Fallback for unknown compilers */
#ifndef REG
    #define REG(reg,arg) arg
    #define ASM
    #define SAVEDS
#endif

#endif /* COMPILERS_H */
