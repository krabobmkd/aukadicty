#include "aukiffserializer.h"
#include "aukstring.h"
#include "aukobject.h"
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>
#include <stdio.h>

/*
 * IFF Serializer Implementation
 * Big-endian binary format using IFF chunks (Amiga standard)
 */

/* IFF Chunk IDs (FourCC) */
#define ID_FORM 0x464F524D  /* 'FORM' */
#define ID_AUPJ 0x4155504A  /* 'AUPJ' - Aukadicty Project */
#define ID_OBJ  0x4F424A20  /* 'OBJ ' - Object */
#define ID_INT  0x494E5420  /* 'INT ' - int */
#define ID_UINT 0x55494E54  /* 'UINT' - unsigned int */
#define ID_I64  0x49363420  /* 'I64 ' - long long */
#define ID_U64  0x55363420  /* 'U64 ' - unsigned long long */
#define ID_FIX  0x46495820  /* 'FIX ' - AukFixed */
#define ID_BOOL 0x424F4F4C  /* 'BOOL' - boolean */
#define ID_STR  0x53545220  /* 'STR ' - string */
#define ID_ARRY 0x41525259  /* 'ARRY' - array */

#define ID_MNME 0x4D4E4D45  /* 'MNME' - member name */
#define ID_TYPE 0x54595045  /* 'TYPE' - type name */


/* IFF Writer context */
typedef struct {
    BPTR file;                  /* AmigaDOS file handle */
   // long startPos;
    int error;                  /* Error flag */
} IFFWriterContext;

/* IFF Reader context */
typedef struct {
    BPTR file;                  /* AmigaDOS file handle */
    long startPos;                /* Start position of current chunk */
    long endPos;                /* End position of current chunk */
    int error;                  /* Error flag */
} IFFReaderContext;

/* ========== Big-Endian Helpers ========== */
#if defined(__AMIGA__)
#define BIGENDIAN
#endif

#ifndef BIGENDIAN
static unsigned long SwapLong(unsigned long val) {
    return ((val & 0xFF000000) >> 24) |
           ((val & 0x00FF0000) >> 8) |
           ((val & 0x0000FF00) << 8) |
           ((val & 0x000000FF) << 24);
}

static unsigned long long SwapLongLong(unsigned long long val) {
    return ((val & 0xFF00000000000000ULL) >> 56) |
           ((val & 0x00FF000000000000ULL) >> 40) |
           ((val & 0x0000FF0000000000ULL) >> 24) |
           ((val & 0x000000FF00000000ULL) >> 8) |
           ((val & 0x00000000FF000000ULL) << 8) |
           ((val & 0x0000000000FF0000ULL) << 24) |
           ((val & 0x000000000000FF00ULL) << 40) |
           ((val & 0x00000000000000FFULL) << 56);
}
#else
INLINE unsigned long SwapLong(unsigned long val) {
    return val;
}
INLINE unsigned long long SwapLongLong(unsigned long long val) {
    return val;
}
#endif


static void WriteBigEndianLong(BPTR file, unsigned long val) {
    unsigned char c[4];
    c[0]=val>>24;
    c[1]=val>>16;
    c[2]=val>>8;
    c[3]=val;
    Write(file, &c[0], 4);
}

static void WriteBigEndianLongLong(BPTR file, unsigned long long val) {
    unsigned char c[8];
    c[0]=val>>56;
    c[1]=val>>48;
    c[2]=val>>40;
    c[3]=val>>32;
    c[4]=val>>24;
    c[5]=val>>16;
    c[6]=val>>8;
    c[7]=val;
    Write(file, &c[0], 8);
}

static unsigned long ReadBigEndianLong(BPTR file) {
    unsigned char c[4];
    if (Read(file, &c[0], 4) == 4) {
        return
            (
        (((unsigned long)c[0])<<24)|
       (((unsigned long)c[1])<<16)|
       (((unsigned long)c[2])<<8)|
       ((unsigned long)c[3])
        );

    }
    return 0;
}

static unsigned long long ReadBigEndianLongLong(BPTR file) {
    unsigned char c[8];
    if (Read(file, &c[0], 8) == 8) {
        return
            (
        (((unsigned long long)c[0])<<56)|
       (((unsigned long long)c[1])<<48)|
       (((unsigned long long)c[2])<<40)|
       (((unsigned long long)c[3])<<32)|
       (((unsigned long long)c[4])<<24)|
       (((unsigned long long)c[5])<<16)|
       (((unsigned long long)c[6])<<8)|
       ((unsigned long long)c[7])
        );

    }
    return 0;
}

/* ========== IFF Writer Helpers ========== */

static void IFFWriter_BeginChunk(BPTR file, long*chunkstartpos, unsigned long chunkID) {

    *chunkstartpos = Seek(file, 0, OFFSET_CURRENT);
    /* Write chunk ID and placeholder size */
    WriteBigEndianLong(file, chunkID);

    WriteBigEndianLong(file, 0);  /* Size placeholder */

}

static void IFFWriter_EndChunk(BPTR file,long chunkStartPos) {
    long currentPos;
    unsigned long chunkSize;

    /* Get current position */
    currentPos = Seek(file, 0, OFFSET_CURRENT);

    /* Calculate chunk data size (exclude ID and size fields) */
    chunkSize = (unsigned long)(currentPos - chunkStartPos - 8);

    /* Go back and write actual size */
    Seek(file, chunkStartPos + 4, OFFSET_BEGINNING);
    WriteBigEndianLong(file, chunkSize);

    /* Return to end */
    Seek(file, currentPos, OFFSET_BEGINNING);

}

/* ========== IFF Writer Implementation ========== */

static void IFFWriter_WriteNamedValue(BPTR file, const char* name, unsigned long chunkID,
                                       const void* data, unsigned long dataSize) {
    unsigned long nameLen = strlen(name);

    /* Write chunk header */
    WriteBigEndianLong(file, chunkID);
    WriteBigEndianLong(file, nameLen + 1 + dataSize);  /* name + null + data */

    /* Write name */
    Write(file, name, nameLen + 1);  /* Include null terminator */

    /* Write data */
    if(dataSize>0) Write(file, data, dataSize);

    /* Pad if needed */
    // if ((nameLen + 1 + dataSize) & 1) {
    //     unsigned char pad = 0;
    //     Write(file, &pad, 1);
    // }
}
static void IFFWriter_WriteImNamedValue(BPTR file, unsigned long chunkID,
                                       const void* data, unsigned long dataSize) {

    /* Write chunk header */
    WriteBigEndianLong(file, chunkID);
    WriteBigEndianLong(file, dataSize);  /* name + null + data */

    /* Write data */
    if(dataSize>0) Write(file, data, dataSize);

}


static void IFFWriter_t_int(ISerializer* This, const char* name, int* value) {
    IFFWriterContext* ctx = (IFFWriterContext*)This->context;
    unsigned long val = SwapLong((unsigned long)*value);
    IFFWriter_WriteNamedValue(ctx->file, name, ID_INT, &val, 4);
}

static void IFFWriter_t_uint(ISerializer* This, const char* name, unsigned int* value) {
    IFFWriterContext* ctx = (IFFWriterContext*)This->context;
    unsigned long val = SwapLong((unsigned long)*value);
    IFFWriter_WriteNamedValue(ctx->file, name, ID_UINT, &val, 4);
}

static void IFFWriter_t_longlong(ISerializer* This, const char* name, long long* value) {
    IFFWriterContext* ctx = (IFFWriterContext*)This->context;
    unsigned long long val = SwapLongLong((unsigned long long)*value);
    IFFWriter_WriteNamedValue(ctx->file, name, ID_I64, &val, 8);
}

static void IFFWriter_t_ulonglong(ISerializer* This, const char* name, unsigned long long* value) {
    IFFWriterContext* ctx = (IFFWriterContext*)This->context;
    unsigned long long val = SwapLongLong(*value);
    IFFWriter_WriteNamedValue(ctx->file, name, ID_U64, &val, 8);
}

static void IFFWriter_t_fixed(ISerializer* This, const char* name, AukFixed* value) {
    IFFWriterContext* ctx = (IFFWriterContext*)This->context;
    unsigned long long val = SwapLongLong((unsigned long long)*value);
    IFFWriter_WriteNamedValue(ctx->file, name, ID_FIX, &val, 8);
}

static void IFFWriter_t_bool(ISerializer* This, const char* name, int* value) {
    IFFWriterContext* ctx = (IFFWriterContext*)This->context;
    unsigned long val = SwapLong((unsigned long)*value);
    IFFWriter_WriteNamedValue(ctx->file, name, ID_BOOL, &val, 4);
}

static void IFFWriter_t_string(ISerializer* This, const char* name, const char** value) {
    IFFWriterContext* ctx = (IFFWriterContext*)This->context;
    unsigned long len;

    if (*value) {
        len = strlen(*value);
        IFFWriter_WriteNamedValue(ctx->file, name, ID_STR, *value, len + 1);
    }
}

static void IFFWriter_t_string_mutable(ISerializer* This, const char* name, char** value) {
    IFFWriter_t_string(This, name, (const char**)value);
}

static void IFFWriter_t_object(ISerializer* This, const char* name, AukObjectPtr* object) {
    IFFWriterContext* ctx = (IFFWriterContext*)This->context;
    AukObject* obj = *object;
    long startpos;
    const char* typeName;

    if (!obj) {
        /* Write null object marker */
        IFFWriter_WriteNamedValue(ctx->file, name, ID_OBJ, NULL, 0);
        return;
    }

    /* Begin object chunk */
    IFFWriter_BeginChunk(ctx->file, &startpos, ID_OBJ);

    /* Write member tags */
    IFFWriter_WriteImNamedValue(ctx->file,ID_MNME,name, strlen(name) + 1);

    /* Write type name */
    typeName = obj->GetTypeName(obj);
    if (typeName) {
        IFFWriter_WriteImNamedValue(ctx->file, ID_TYPE, typeName, strlen(typeName) + 1);
    }

    /* Serialize object data */
    if (obj->Serialize) {
        obj->Serialize(obj, This, name);
    }

    /* End object chunk */
    IFFWriter_EndChunk(ctx->file, startpos);
}

static void IFFWriter_t_arrayobj(ISerializer* This, const char* name, AukArray** array,
    AukObjectNewFunc itemNewFunc, const char* (*itemGetTypeName)(AukObject*)) {
    IFFWriter_t_object(This, name, (AukObjectPtr*)array);
}

static void IFFWriter_t_int_array(ISerializer* This, const char* name, int** values, unsigned int* count) {
    IFFWriterContext* ctx = (IFFWriterContext*)This->context;
    unsigned long nameLen = strlen(name);
    unsigned long dataSize = *count * 4;
    unsigned int i;

    if (!*values || *count == 0) return;

    /* Write chunk header */
    WriteBigEndianLong(ctx->file, ID_ARRY);
    WriteBigEndianLong(ctx->file, nameLen + 1 + 4 + 4 + dataSize);  /* name + null + type + count + data */

    /* Write name */
    Write(ctx->file, name, nameLen + 1);

    /* Write type (INT) and count */
    WriteBigEndianLong(ctx->file, ID_INT);
    WriteBigEndianLong(ctx->file, *count);

    /* Write values */
    for (i = 0; i < *count; i++) {
        WriteBigEndianLong(ctx->file, (unsigned long)(*values)[i]);
    }

    /* Pad if needed */
    if ((nameLen + 1 + 4 + 4 + dataSize) & 1) {
        unsigned char pad = 0;
        Write(ctx->file, &pad, 1);
    }
}

static void IFFWriter_t_longlong_array(ISerializer* This, const char* name, long long** values, unsigned int* count) {
    IFFWriterContext* ctx = (IFFWriterContext*)This->context;
    unsigned long nameLen = strlen(name);
    unsigned long dataSize = *count * 8;
    unsigned int i;

    if (!*values || *count == 0) return;

    WriteBigEndianLong(ctx->file, ID_ARRY);
    WriteBigEndianLong(ctx->file, nameLen + 1 + 4 + 4 + dataSize);

    Write(ctx->file, name, nameLen + 1);
    WriteBigEndianLong(ctx->file, ID_I64);
    WriteBigEndianLong(ctx->file, *count);

    for (i = 0; i < *count; i++) {
        WriteBigEndianLongLong(ctx->file, (unsigned long long)(*values)[i]);
    }

    if ((nameLen + 1 + 4 + 4 + dataSize) & 1) {
        unsigned char pad = 0;
        Write(ctx->file, &pad, 1);
    }
}

static void IFFWriter_t_fixed_array(ISerializer* This, const char* name, AukFixed** values, unsigned int* count) {
    IFFWriterContext* ctx = (IFFWriterContext*)This->context;
    unsigned long nameLen = strlen(name);
    unsigned long dataSize = *count * 8;
    unsigned int i;

    if (!*values || *count == 0) return;

    WriteBigEndianLong(ctx->file, ID_ARRY);
    WriteBigEndianLong(ctx->file, nameLen + 1 + 4 + 4 + dataSize);

    Write(ctx->file, name, nameLen + 1);
    WriteBigEndianLong(ctx->file, ID_FIX);
    WriteBigEndianLong(ctx->file, *count);

    for (i = 0; i < *count; i++) {
        WriteBigEndianLongLong(ctx->file, (unsigned long long)(*values)[i]);
    }

    if ((nameLen + 1 + 4 + 4 + dataSize) & 1) {
        unsigned char pad = 0;
        Write(ctx->file, &pad, 1);
    }
}

static void IFFWriter_Destroy(ISerializer* This) {
    IFFWriterContext* ctx = (IFFWriterContext*)This->context;

    if (ctx) {
        /* Note: file is not closed here - caller owns it */
        FreeVec(ctx);
    }

    FreeVec(This);
}

ISerializer* AukIFFSerializer_CreateWriter(BPTR file) {
    ISerializer* ser = (ISerializer*)AllocVec(sizeof(ISerializer), MEMF_CLEAR);
    IFFWriterContext* ctx;

    if (!ser) return NULL;

    ctx = (IFFWriterContext*)AllocVec(sizeof(IFFWriterContext), MEMF_CLEAR);
    if (!ctx) {
        FreeVec(ser);
        return NULL;
    }

    ctx->file = file;
    ctx->error = 0;

    /* Write FORM header */
    WriteBigEndianLong(file, ID_FORM);
    WriteBigEndianLong(file, 0);  /* Size placeholder */
    WriteBigEndianLong(file, ID_AUPJ);  /* Aukadicty Project */

    /* Initialize serializer */
    ser->context = ctx;
    ser->_isReading = 0;
    ser->typeRegistry = NULL;

    /* Set function pointers */
    ser->t_int = IFFWriter_t_int;
    ser->t_uint = IFFWriter_t_uint;
    ser->t_longlong = IFFWriter_t_longlong;
    ser->t_ulonglong = IFFWriter_t_ulonglong;
    ser->t_fixed = IFFWriter_t_fixed;
    ser->t_bool = IFFWriter_t_bool;
    ser->t_string = IFFWriter_t_string;
    ser->t_string_mutable = IFFWriter_t_string_mutable;
    ser->t_object = IFFWriter_t_object;
    ser->t_arrayobj = IFFWriter_t_arrayobj;
    ser->t_int_array = IFFWriter_t_int_array;
    ser->t_longlong_array = IFFWriter_t_longlong_array;
    ser->t_fixed_array = IFFWriter_t_fixed_array;
    ser->Destroy = IFFWriter_Destroy;

    return ser;
}

int AukIFFSerializer_Finalize(ISerializer* ser) {
    IFFWriterContext* ctx;
    long currentPos;
    unsigned long formSize;

    if (!ser || ser->_isReading) return 0;

    ctx = (IFFWriterContext*)ser->context;
    if (!ctx) return 0;

    /* Get current position */
    currentPos = Seek(ctx->file, 0, OFFSET_CURRENT);

    /* Calculate FORM size (exclude FORM ID and size field itself) */
    formSize = (unsigned long)(currentPos - 8);

    /* Go back and write FORM size */
    Seek(ctx->file, 4, OFFSET_BEGINNING);
    WriteBigEndianLong(ctx->file, formSize);

    /* Return to end */
    Seek(ctx->file, currentPos, OFFSET_BEGINNING);

    return 1;
}

/* ========== IFF Reader Implementation ========== */

static int IFFReader_ReadChunkHeader(BPTR file, unsigned long* chunkID, unsigned long* chunkSize) {
    *chunkID = ReadBigEndianLong(file);
    *chunkSize = ReadBigEndianLong(file);

     unsigned long cid = *chunkID;
    printf("chunkID:%c%c%c%c\n",(int)(cid>>24),(int)(cid>>16),(int)(cid>>8),(int)(cid));
    printf("chunksize:%d\n",(int)*chunkSize);
    return 1;
}
// this ones only manage simpleton members that bcan be managed by copy.
static int IFFReader_FindChunk(IFFReaderContext* ctx, const char* name, unsigned long expectedID,
                                unsigned long* outSize, void* outData, unsigned long maxDataSize) {
    unsigned long chunkID, chunkSize;
    char chunkName[64];
    unsigned long nameLen;
    long pos = ctx->startPos;

    while (pos<ctx->endPos) {
        Seek(ctx->file, pos, OFFSET_BEGINNING);

        chunkID = ReadBigEndianLong(ctx->file);
        chunkSize = ReadBigEndianLong(ctx->file);

        if (chunkID != expectedID) {
            /* Skip this chunk */
            pos += 8+chunkSize;
            continue;
        }

        /* Read chunk name */
        nameLen = 0;
        while (nameLen < chunkSize && nameLen < 63) {
            if (Read(ctx->file, &chunkName[nameLen], 1) != 1) break;
            if (chunkName[nameLen] == 0) break;
            nameLen++;
        }
        chunkName[nameLen] = 0;

        if (AukString_Compare(chunkName, name) == 0) {
            /* Found it - read data */
            unsigned long dataSize = chunkSize - nameLen - 1;
            if (outData && dataSize <= maxDataSize) {
                Read(ctx->file, outData, dataSize);
            }
            if (outSize) *outSize = dataSize;

            /* Skip to next chunk */
            //Seek(file, pos + 8+chunkSize, OFFSET_BEGINNING);
            return 1;
        }

        /* Not the right name, skip */
        //Seek(file, chunkEnd, OFFSET_BEGINNING);
        pos += 8+chunkSize;
    }

    return 0;
}



static int IFFReader_PushContext(ISerializer* This, const char* name, unsigned long expectedID) {
    IFFReaderContext* ctx = (IFFReaderContext*)This->context;
    unsigned long chunkID, chunkSize;
    long pos=ctx->startPos;

    while(pos<ctx->endPos)
    {
        Seek(ctx->file, pos, OFFSET_BEGINNING);
        chunkID = ReadBigEndianLong(ctx->file);
        chunkSize = ReadBigEndianLong(ctx->file);
        if(chunkID == expectedID)
        {
            // just get immediate member name
            long mnamechunkID = ReadBigEndianLong(ctx->file);
            long mnamechunkSize = ReadBigEndianLong(ctx->file);
            if(mnamechunkID == ID_MNME)
            {
                char mname[64];
                long cpsize = mnamechunkSize;
                if(cpsize>64) cpsize=64;
                Read(ctx->file, &mname[0], cpsize);
                mname[63]=0;
                // we sure are on the right object.
                if(AukString_Compare(mname,name)==0)
                {
                    ctx->startPos = pos + 8 ;
                    ctx->endPos = pos + 8 + chunkSize;
                    return 1;
                }
            }

        }
        pos += 8+chunkSize;
    }

    return 0;
}

static void IFFReader_t_int(ISerializer* This, const char* name, int* value) {
    IFFReaderContext* ctx = (IFFReaderContext*)This->context;
    unsigned long val;
    unsigned long size;

    if (IFFReader_FindChunk(ctx, name, ID_INT, &size, &val, 4)) {
        *value = (int)SwapLong(val);
    }
}

static void IFFReader_t_uint(ISerializer* This, const char* name, unsigned int* value) {
    IFFReaderContext* ctx = (IFFReaderContext*)This->context;
    unsigned long val;
    unsigned long size;

    if (IFFReader_FindChunk(ctx, name, ID_UINT, &size, &val, 4)) {
        *value = SwapLong(val);
    }
}

static void IFFReader_t_longlong(ISerializer* This, const char* name, long long* value) {
    IFFReaderContext* ctx = (IFFReaderContext*)This->context;
    unsigned long long val;
    unsigned long size;

    if (IFFReader_FindChunk(ctx, name, ID_I64, &size, &val, 8)) {
        *value = (long long)SwapLongLong(val);
    }
}

static void IFFReader_t_ulonglong(ISerializer* This, const char* name, unsigned long long* value) {
    IFFReaderContext* ctx = (IFFReaderContext*)This->context;
    unsigned long long val;
    unsigned long size;

    if (IFFReader_FindChunk(ctx, name, ID_U64, &size, &val, 8)) {
        *value = SwapLongLong(val);
    }
}

static void IFFReader_t_fixed(ISerializer* This, const char* name, AukFixed* value) {
    IFFReaderContext* ctx = (IFFReaderContext*)This->context;
    unsigned long long val;
    unsigned long size;

    if (IFFReader_FindChunk(ctx, name, ID_FIX, &size, &val, 8)) {
        *value = (AukFixed)SwapLongLong(val);
    }
}

static void IFFReader_t_bool(ISerializer* This, const char* name, int* value) {
    IFFReaderContext* ctx = (IFFReaderContext*)This->context;
    unsigned long val;
    unsigned long size;

    if (IFFReader_FindChunk(ctx, name, ID_BOOL, &size, &val, 4)) {
        *value = (int)SwapLong(val);
    }
}

static void IFFReader_t_string(ISerializer* This, const char* name, const char** value) {
    /* Read-only string not supported in IFF reader - use t_string_mutable */
    (void)This;
    (void)name;
    *value = NULL;
}

static void IFFReader_t_string_mutable(ISerializer* This, const char* name, char** value) {
    IFFReaderContext* ctx = (IFFReaderContext*)This->context;
    char buffer[1024];
    unsigned long size;

    /* Free existing */
    if (*value) {
        AukString_Free(*value);
        *value = NULL;
    }

    if (IFFReader_FindChunk(ctx, name, ID_STR, &size, buffer, sizeof(buffer) - 1)) {
        buffer[size] = 0;
        *value = AukString_Duplicate(buffer);
    }
}

static void IFFReader_t_object(ISerializer* This, const char* name, AukObjectPtr* object) {
    IFFReaderContext* ctx = (IFFReaderContext*)This->context;
    char typeName[64];
    const TypeNameToContructor* reg;
    AukObject* newObj;
    long prevStart,prevEnd;
    int res;

    /* Release existing */
    if (*object) {
        AukObjectPtr_Release(object);
    }
    typeName[0]=0;

    /* Save position */
    prevStart = ctx->startPos;
    prevEnd = ctx->endPos;

    /* Read type name from object */
    res = IFFReader_PushContext(This, name,ID_OBJ);
    if(!res) return;


    // 2 first should be mname and type
    // ctx->endPos
    // inside ID_OBJ, 2 first should be ID_MNME and ID_TYPE.
    {
        long cur = ctx->startPos;
        long done=0;
        while( cur < ctx->endPos && done <1)
        {
            long chunkID = ReadBigEndianLong(ctx->file);
            long chunkSize = ReadBigEndianLong(ctx->file);
            if(chunkID == ID_TYPE)
            {
                long cpsize = chunkSize;
                if(cpsize>64) cpsize=64;
                Read(ctx->file, &typeName[0], cpsize);
                typeName[63]=0;
                done++;
            }
            cur += 8+ chunkSize;
            Seek(ctx->file, cur, OFFSET_BEGINNING);
        }
        if(done<1)
        {
            //IFFReader_PopContext(This);
            ctx->startPos = prevStart;
            ctx->endPos = prevEnd;
            return;
        }
    }

    /* Find constructor */
    if (This->typeRegistry)
    for (reg = This->typeRegistry; reg->typename != NULL; reg++) {
        if (AukString_Compare(reg->typename, typeName) == 0) {
            /* Create object */
            reg->NewConstructor(object);
            newObj = *object;

            if (newObj && newObj->Serialize) {
                newObj->Serialize(newObj, This, name);
            }
        }
    }
    //pop
    ctx->startPos = prevStart;
    ctx->endPos = prevEnd;
}

static void IFFReader_t_arrayobj(ISerializer* This, const char* name, AukArray** parray,
    AukObjectNewFunc itemNewFunc, const char* (*itemGetTypeName)(AukObject*)) {


    IFFReader_t_object(This, name, (AukObjectPtr*)parray);
    AukArray *array = *parray;
    if(array)
    {
        AukArray_SetType(array,itemNewFunc, itemGetTypeName);
    }
}

static void IFFReader_t_int_array(ISerializer* This, const char* name, int** values, unsigned int* count) {
    IFFReaderContext* ctx = (IFFReaderContext*)This->context;
    unsigned long chunkID, chunkSize;
    char chunkName[256];
    unsigned long nameLen, arrayType, arrayCount;
    unsigned int i;

    /* Free existing */
    if (*values) {
        FreeVec(*values);
        *values = NULL;
    }
    *count = 0;

    /* Find ARRY chunk */
    if (!IFFReader_ReadChunkHeader(ctx->file, &chunkID, &chunkSize) || chunkID != ID_ARRY) {
        return;
    }

    /* Read name */
    nameLen = 0;
    while (nameLen < 255) {
        if (Read(ctx->file, &chunkName[nameLen], 1) != 1) return;
        if (chunkName[nameLen] == 0) break;
        nameLen++;
    }
    chunkName[nameLen] = 0;

    if (AukString_Compare(chunkName, name) != 0) return;

    /* Read type and count */
    arrayType = ReadBigEndianLong(ctx->file);
    arrayCount = ReadBigEndianLong(ctx->file);

    if (arrayType != ID_INT || arrayCount == 0) return;

    /* Allocate and read */
    *values = (int*)AllocVec(arrayCount * sizeof(int), MEMF_CLEAR);
    if (!*values) return;

    for (i = 0; i < arrayCount; i++) {
        (*values)[i] = (int)ReadBigEndianLong(ctx->file);
    }
    *count = arrayCount;
}

static void IFFReader_t_longlong_array(ISerializer* This, const char* name, long long** values, unsigned int* count) {
    IFFReaderContext* ctx = (IFFReaderContext*)This->context;
    unsigned long chunkID, chunkSize;
    char chunkName[256];
    unsigned long nameLen, arrayType, arrayCount;
    unsigned int i;

    if (*values) {
        FreeVec(*values);
        *values = NULL;
    }
    *count = 0;

    if (!IFFReader_ReadChunkHeader(ctx->file, &chunkID, &chunkSize) || chunkID != ID_ARRY) {
        return;
    }

    nameLen = 0;
    while (nameLen < 255) {
        if (Read(ctx->file, &chunkName[nameLen], 1) != 1) return;
        if (chunkName[nameLen] == 0) break;
        nameLen++;
    }
    chunkName[nameLen] = 0;

    if (AukString_Compare(chunkName, name) != 0) return;

    arrayType = ReadBigEndianLong(ctx->file);
    arrayCount = ReadBigEndianLong(ctx->file);

    if (arrayType != ID_I64 || arrayCount == 0) return;

    *values = (long long*)AllocVec(arrayCount * sizeof(long long), MEMF_CLEAR);
    if (!*values) return;

    for (i = 0; i < arrayCount; i++) {
        (*values)[i] = (long long)ReadBigEndianLongLong(ctx->file);
    }
    *count = arrayCount;
}

static void IFFReader_t_fixed_array(ISerializer* This, const char* name, AukFixed** values, unsigned int* count) {
    IFFReaderContext* ctx = (IFFReaderContext*)This->context;
    unsigned long chunkID, chunkSize;
    char chunkName[256];
    unsigned long nameLen, arrayType, arrayCount;
    unsigned int i;

    if (*values) {
        FreeVec(*values);
        *values = NULL;
    }
    *count = 0;

    if (!IFFReader_ReadChunkHeader(ctx->file, &chunkID, &chunkSize) || chunkID != ID_ARRY) {
        return;
    }

    nameLen = 0;
    while (nameLen < 255) {
        if (Read(ctx->file, &chunkName[nameLen], 1) != 1) return;
        if (chunkName[nameLen] == 0) break;
        nameLen++;
    }
    chunkName[nameLen] = 0;

    if (AukString_Compare(chunkName, name) != 0) return;

    arrayType = ReadBigEndianLong(ctx->file);
    arrayCount = ReadBigEndianLong(ctx->file);

    if (arrayType != ID_FIX || arrayCount == 0) return;

    *values = (AukFixed*)AllocVec(arrayCount * sizeof(AukFixed), MEMF_CLEAR);
    if (!*values) return;

    for (i = 0; i < arrayCount; i++) {
        (*values)[i] = (AukFixed)ReadBigEndianLongLong(ctx->file);
    }
    *count = arrayCount;
}

static void IFFReader_Destroy(ISerializer* This) {
    IFFReaderContext* ctx = (IFFReaderContext*)This->context;

    if (ctx) {
        FreeVec(ctx);
    }

    FreeVec(This);
}

ISerializer* AukIFFSerializer_CreateReader(BPTR file, const TypeNameToContructor* typeRegistry) {
    ISerializer* ser = (ISerializer*)AllocVec(sizeof(ISerializer), MEMF_CLEAR);
    IFFReaderContext* ctx;
    unsigned long formID, formSize, formType;

    if (!ser) return NULL;

    ctx = (IFFReaderContext*)AllocVec(sizeof(IFFReaderContext), MEMF_CLEAR);
    if (!ctx) {
        FreeVec(ser);
        return NULL;
    }

    ctx->file = file;
    ctx->error = 0;

    /* Read and validate FORM header */
    formID = ReadBigEndianLong(file);
    formSize = ReadBigEndianLong(file);
    formType = ReadBigEndianLong(file);

    if (formID != ID_FORM || formType != ID_AUPJ) {
        FreeVec(ctx);
        FreeVec(ser);
        return NULL;
    }

    ctx->startPos = 12;
    ctx->endPos = 12+formSize;

    /* Initialize serializer */
    ser->context = ctx;
    ser->_isReading = 1;
    ser->typeRegistry = typeRegistry;

    /* Set function pointers */
    // ser->PushContext = IFFReader_PushContext;
    // ser->PopContext = IFFReader_PopContext;
    ser->t_int = IFFReader_t_int;
    ser->t_uint = IFFReader_t_uint;
    ser->t_longlong = IFFReader_t_longlong;
    ser->t_ulonglong = IFFReader_t_ulonglong;
    ser->t_fixed = IFFReader_t_fixed;
    ser->t_bool = IFFReader_t_bool;
    ser->t_string = IFFReader_t_string;
    ser->t_string_mutable = IFFReader_t_string_mutable;
    ser->t_object = IFFReader_t_object;
    ser->t_arrayobj = IFFReader_t_arrayobj;
    ser->t_int_array = IFFReader_t_int_array;
    ser->t_longlong_array = IFFReader_t_longlong_array;
    ser->t_fixed_array = IFFReader_t_fixed_array;
    ser->Destroy = IFFReader_Destroy;

    return ser;
}
