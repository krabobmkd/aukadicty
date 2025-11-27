#include "aukstring.h"
#include <proto/exec.h>
#include <string.h>

/*
 * String management implementation
 * All strings are allocated with AllocVec and must be freed with AukString_Free
 */

char* AukString_Duplicate(const char* str) {
    char* newstr;
    unsigned long len;

    if (!str) {
        return NULL;
    }

    len = strlen(str);
    newstr = (char*)AllocVec(len + 1, MEMF_CLEAR);
    if (newstr) {
        strcpy(newstr, str);
    }

    return newstr;
}

char* AukString_Concat(const char* str1, const char* str2) {
    char* result;
    unsigned long len1, len2;

    if (!str1 || !str2) {
        return NULL;
    }

    len1 = strlen(str1);
    len2 = strlen(str2);

    result = (char*)AllocVec(len1 + len2 + 1, MEMF_CLEAR);
    if (result) {
        strcpy(result, str1);
        strcat(result, str2);
    }

    return result;
}

char* AukString_Concat3(const char* str1, const char* str2, const char* str3) {
    char* result;
    unsigned long len1, len2, len3;

    if (!str1 || !str2 || !str3) {
        return NULL;
    }

    len1 = strlen(str1);
    len2 = strlen(str2);
    len3 = strlen(str3);

    result = (char*)AllocVec(len1 + len2 + len3 + 1, MEMF_CLEAR);
    if (result) {
        strcpy(result, str1);
        strcat(result, str2);
        strcat(result, str3);
    }

    return result;
}

// void AukString_Free(char* str) {
//     if (str) {
//         FreeVec(str);
//     }
// }

unsigned long AukString_Length(const char* str) {
    return str ? strlen(str) : 0;
}

int AukString_Compare(const char* str1, const char* str2) {
    if (!str1 || !str2) {
        return (str1 == str2) ? 0 : -1;
    }
    return strcmp(str1, str2);
}

int AukString_StartsWith(const char* str, const char* prefix) {
    unsigned long str_len, prefix_len;

    if (!str || !prefix) {
        return 0;
    }

    str_len = strlen(str);
    prefix_len = strlen(prefix);

    if (prefix_len > str_len) {
        return 0;
    }

    return memcmp(str, prefix, prefix_len) == 0;
}

int AukString_EndsWith(const char* str, const char* suffix) {
    unsigned long str_len, suffix_len;

    if (!str || !suffix) {
        return 0;
    }

    str_len = strlen(str);
    suffix_len = strlen(suffix);

    if (suffix_len > str_len) {
        return 0;
    }

    return memcmp(str + str_len - suffix_len, suffix, suffix_len) == 0;
}

char* AukString_Find(const char* str, const char* substr) {
    if (!str || !substr) {
        return NULL;
    }
    return strstr(str, substr);
}

char* AukString_MakeRelativePath(const char* base, const char* path) {
    const char* common;
    unsigned long base_len, path_len, common_len;

    if (!base || !path) {
        return NULL;
    }

    base_len = strlen(base);
    path_len = strlen(path);

    /* Find common prefix */
    common = path;
    common_len = 0;

    while (*base && *path && *base == *path) {
        if (*base == '/' || *base == ':') {
            common = path + 1;
            common_len = (unsigned long)(common - path);
        }
        base++;
        path++;
    }

    /* If path starts with base, return the remainder */
    if (common_len > 0 && path_len >= common_len) {
        return AukString_Duplicate(path);
    }

    /* Otherwise return the full path */
    return AukString_Duplicate(path);
}

char* AukString_MakeAbsolutePath(const char* base, const char* relative) {
    /* Check if relative is already absolute (contains ':' for device) */
    if (!base || !relative) {
        return NULL;
    }

    if (AukString_Find(relative, ":")) {
        return AukString_Duplicate(relative);
    }

    /* Concatenate base and relative with '/' separator if needed */
    if (AukString_EndsWith(base, "/") || AukString_EndsWith(base, ":")) {
        return AukString_Concat(base, relative);
    } else {
        return AukString_Concat3(base, "/", relative);
    }
}
