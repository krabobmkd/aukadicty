#ifndef AUKSTRING_H
#define AUKSTRING_H

/*
 * String management library for Aukadicty
 * Uses AllocVec/FreeVec for all string operations
 * No standard string class available in C99
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Allocate and copy a string */
char* AukString_Duplicate(const char* str);

/* Allocate and concatenate two strings */
char* AukString_Concat(const char* str1, const char* str2);

/* Allocate and concatenate three strings */
char* AukString_Concat3(const char* str1, const char* str2, const char* str3);

/* Free a string allocated by AukString functions */
void AukString_Free(char* str);

/* Get length of string */
unsigned long AukString_Length(const char* str);

/* Compare two strings (returns 0 if equal) */
int AukString_Compare(const char* str1, const char* str2);

/* Check if string starts with prefix */
int AukString_StartsWith(const char* str, const char* prefix);

/* Check if string ends with suffix */
int AukString_EndsWith(const char* str, const char* suffix);

/* Find substring in string (returns pointer or NULL) */
char* AukString_Find(const char* str, const char* substr);

/* Convert absolute path to relative path from base directory */
char* AukString_MakeRelativePath(const char* base, const char* path);

/* Convert relative path to absolute path from base directory */
char* AukString_MakeAbsolutePath(const char* base, const char* relative);

#ifdef __cplusplus
}
#endif

#endif /* AUKSTRING_H */
