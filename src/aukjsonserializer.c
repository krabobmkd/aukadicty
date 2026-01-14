#include "aukjsonserializer.h"
#include "aukstring.h"
#include "aukobject.h"
#include "aukscalararray.h"
#include <proto/exec.h>
#include <string.h>

/*
 * JSON Serializer Implementation
 * Implements ISerializer interface using cJSON library
 */

/* Context stack node for nested object serialization */
typedef struct sContextStackNode {
    cJSON* context;
    struct sContextStackNode* next;
} ContextStackNode;

/* JSON Writer context */
typedef struct {
    cJSON* root;               /* Root JSON object */
    ContextStackNode* stack;   /* Stack of cJSON contexts for nesting */
    cJSON* current;            /* Current context (top of stack) */

    /* Remember list for tracking serialized objects */
    AukObject** rememberList;  /* Dynamic array of object pointers */
    unsigned int rememberCount; /* Number of objects in remember list */
    unsigned int rememberCapacity; /* Allocated capacity */
} JsonWriterContext;

/* JSON Reader context */
typedef struct {
    cJSON* root;               /* Root JSON object */
    ContextStackNode* stack;   /* Stack of cJSON contexts for nesting */
    cJSON* current;            /* Current context (top of stack) */

    /* Remember list for tracking deserialized objects */
    AukObject** rememberList;  /* Dynamic array of object pointers */
    unsigned int rememberCount; /* Number of objects in remember list */
    unsigned int rememberCapacity; /* Allocated capacity */
} JsonReaderContext;

/* ========== Helper Functions ========== */

static void PushContextStack(ContextStackNode** stack, cJSON** current, cJSON* newContext) {
    ContextStackNode* node = (ContextStackNode*)AllocVec(sizeof(ContextStackNode), MEMF_CLEAR);
    if (node) {
        node->context = *current;
        node->next = *stack;
        *stack = node;
        *current = newContext;
    }
}

static void PopContextStack(ContextStackNode** stack, cJSON** current) {
    ContextStackNode* node = *stack;
    if (node) {
        *current = node->context;
        *stack = node->next;
        FreeVec(node);
    }
}

/* ========== JSON Writer Implementation ========== */

/* Find object in remember list, return index or -1 if not found */
static int JsonWriter_FindInRememberList(JsonWriterContext* ctx, AukObject* obj) {
    unsigned int i;
    for (i = 0; i < ctx->rememberCount; i++) {
        if (ctx->rememberList[i] == obj) {
            return (int)i;
        }
    }
    return -1;
}

/* Add object to remember list, return index */
static int JsonWriter_AddToRememberList(JsonWriterContext* ctx, AukObject* obj) {
    /* Grow array if needed */
    if (ctx->rememberCount >= ctx->rememberCapacity) {
        unsigned int newCapacity = ctx->rememberCapacity == 0 ? 16 : ctx->rememberCapacity * 2;
        AukObject** newList = (AukObject**)AllocVec(newCapacity * sizeof(AukObject*), MEMF_CLEAR);
        if (!newList) {
            return -1;
        }

        /* Copy existing entries */
        if (ctx->rememberList) {
            memcpy(newList, ctx->rememberList, ctx->rememberCount * sizeof(AukObject*));
            FreeVec(ctx->rememberList);
        }

        ctx->rememberList = newList;
        ctx->rememberCapacity = newCapacity;
    }

    /* Add object to list */
    ctx->rememberList[ctx->rememberCount] = obj;
    return (int)(ctx->rememberCount++);
}

//static void JsonWriter_PushContext(ISerializer* This, const char* name) {
//    JsonWriterContext* ctx = (JsonWriterContext*)This->context;
//    cJSON* newObj = cJSON_CreateObject();

//    if (newObj) {
//        /* Add the new object to current context */
//        cJSON_AddItemToObject(ctx->current, name, newObj);

//        /* Push current to stack and make newObj current */
//        PushContextStack(&ctx->stack, &ctx->current, newObj);
//    }
//}

//static void JsonWriter_PopContext(ISerializer* This) {
//    JsonWriterContext* ctx = (JsonWriterContext*)This->context;
//    PopContextStack(&ctx->stack, &ctx->current);
//}

static void JsonWriter_t_int(ISerializer* This, const char* name, int* value) {
    JsonWriterContext* ctx = (JsonWriterContext*)This->context;
    cJSON_AddNumberToObjectInt(ctx->current, name, *value);
}

static void JsonWriter_t_uint(ISerializer* This, const char* name, unsigned int* value) {
    JsonWriterContext* ctx = (JsonWriterContext*)This->context;
    cJSON_AddNumberToObjectInt(ctx->current, name, (int)*value);
}

static void JsonWriter_t_longlong(ISerializer* This, const char* name, long long* value) {
    JsonWriterContext* ctx = (JsonWriterContext*)This->context;
    cJSON_AddNumberToObjectInt(ctx->current, name, (int)*value);
}

static void JsonWriter_t_ulonglong(ISerializer* This, const char* name, unsigned long long* value) {
    JsonWriterContext* ctx = (JsonWriterContext*)This->context;
    cJSON_AddNumberToObjectInt(ctx->current, name, (int)*value);
}

static void JsonWriter_t_fixed(ISerializer* This, const char* name, AukFixed* value) {
    JsonWriterContext* ctx = (JsonWriterContext*)This->context;
    cJSON_AddNumberToObjectFixed(ctx->current, name, *value);
}

static void JsonWriter_t_bool(ISerializer* This, const char* name, int* value) {
    JsonWriterContext* ctx = (JsonWriterContext*)This->context;
    cJSON_AddBoolToObject(ctx->current, name, *value);
}

static void JsonWriter_t_string(ISerializer* This, const char* name, const char** value) {
    JsonWriterContext* ctx = (JsonWriterContext*)This->context;
    if (*value) {
        cJSON_AddStringToObject(ctx->current, name, *value);
    } else {
        cJSON_AddNullToObject(ctx->current, name);
    }
}

static void JsonWriter_t_string_mutable(ISerializer* This, const char* name, char** value) {
    JsonWriterContext* ctx = (JsonWriterContext*)This->context;
    if (*value) {
        cJSON_AddStringToObject(ctx->current, name, *value);
    } else {
        cJSON_AddNullToObject(ctx->current, name);
    }
}

static void JsonWriter_t_object(ISerializer* This, const char* name, AukObjectPtr* object) {
    JsonWriterContext* ctx = (JsonWriterContext*)This->context;
    AukObject* obj = *object;
    cJSON* objNode;
    const char* typeName;
    int objIndex;

    if (!obj) {
        cJSON_AddNullToObject(ctx->current, name);
        return;
    }

    /* Check if object was already serialized */
    objIndex = JsonWriter_FindInRememberList(ctx, obj);
    if (objIndex >= 0) {
        /* Object already serialized - write reference object */
        objNode = cJSON_CreateObject();
        if (!objNode) return;

        cJSON_AddStringToObject(objNode, "__ref", "reference");
        cJSON_AddNumberToObjectInt(objNode, "__index", objIndex);
        cJSON_AddItemToObject(ctx->current, name, objNode);
        return;
    }

    /* First time seeing this object - add to remember list */
    JsonWriter_AddToRememberList(ctx, obj);

    /* Create object node */
    objNode = cJSON_CreateObject();
    if (!objNode) return;

    /* Add type name for deserialization */
    typeName = obj->GetTypeName(obj);
    if (typeName) {
        cJSON_AddStringToObject(objNode, "__type", typeName);
    }

    /* Add to current context */
    cJSON_AddItemToObject(ctx->current, name, objNode);

    /* Push context and serialize object */
    PushContextStack(&ctx->stack, &ctx->current, objNode);

    /* Call object's Serialize method */
    if (obj->Serialize) {
        obj->Serialize(obj, This, name);
    }

    /* Pop context */
    PopContextStack(&ctx->stack, &ctx->current);
}

static void JsonWriter_t_arrayobj(ISerializer* This, const char* name, AukArray** array,
    AukObjectNewFunc itemNewFunc, const char* (*itemGetTypeName)(AukObject*)) {
    /* Delegate to t_object since AukArray handles its own serialization */
    JsonWriter_t_object(This, name, (AukObjectPtr*)array);
}

static void JsonWriter_t_scalararray(ISerializer* This, const char* name, AukScalarArray** array) {
    JsonWriterContext* ctx = (JsonWriterContext*)This->context;
    AukScalarArray* arr = *array;
    cJSON* sarrObj;
    cJSON* shapeArray;
    cJSON* dataArray;
    unsigned int scalarSize;
    unsigned int i;
    short j;
    char* dataChar;
    short* dataShort;
    int* dataInt;
    long long* dataLongLong;

    if (!arr) {
        cJSON_AddNullToObject(ctx->current, name);
        return;
    }

    /* Create object with metadata and data */
    sarrObj = cJSON_CreateObject();
    if (!sarrObj) return;

    /* Add scalar type */
    cJSON_AddNumberToObjectInt(sarrObj, "scalarType", (int)arr->scalarType);

    /* Add ndim */
    cJSON_AddNumberToObjectInt(sarrObj, "ndim", arr->ndim);

    /* Add shape */
    shapeArray = cJSON_CreateArray();
    if (shapeArray) {
        for (j = 0; j < arr->ndim; j++) {
            cJSON_AddItemToArray(shapeArray, cJSON_CreateNumberInt((int)arr->shape[j]));
        }
        cJSON_AddItemToObject(sarrObj, "shape", shapeArray);
    }

    /* Add data as flat array */
    dataArray = cJSON_CreateArray();
    if (dataArray) {
        scalarSize = AukScalarArray_GetScalarSize(arr->scalarType);

        if (scalarSize == 1) {
            dataChar = (char*)arr->data;
            for (i = 0; i < arr->totalElements; i++) {
                cJSON_AddItemToArray(dataArray, cJSON_CreateNumberInt((int)dataChar[i]));
            }
        } else if (scalarSize == 2) {
            dataShort = (short*)arr->data;
            for (i = 0; i < arr->totalElements; i++) {
                cJSON_AddItemToArray(dataArray, cJSON_CreateNumberInt((int)dataShort[i]));
            }
        } else if (scalarSize == 4) {
            dataInt = (int*)arr->data;
            for (i = 0; i < arr->totalElements; i++) {
                cJSON_AddItemToArray(dataArray, cJSON_CreateNumberInt(dataInt[i]));
            }
        } else if (scalarSize == 8) {
            dataLongLong = (long long*)arr->data;
            for (i = 0; i < arr->totalElements; i++) {
                cJSON_AddItemToArray(dataArray, cJSON_CreateNumberInt((int)dataLongLong[i]));
            }
        }

        cJSON_AddItemToObject(sarrObj, "data", dataArray);
    }

    cJSON_AddItemToObject(ctx->current, name, sarrObj);
}

static void JsonWriter_Destroy(ISerializer* This) {
    JsonWriterContext* ctx = (JsonWriterContext*)This->context;

    if (ctx) {
        /* Clean up stack */
        while (ctx->stack) {
            PopContextStack(&ctx->stack, &ctx->current);
        }

        /* Free remember list */
        if (ctx->rememberList) {
            FreeVec(ctx->rememberList);
        }

        /* Delete root (this deletes entire tree) */
        if (ctx->root) {
            cJSON_Delete(ctx->root);
        }

        FreeVec(ctx);
    }

    FreeVec(This);
}

ISerializer* AukJsonSerializer_CreateWriter(void) {
    ISerializer* ser = (ISerializer*)AllocVec(sizeof(ISerializer), MEMF_CLEAR);
    JsonWriterContext* ctx;

    if (!ser) return NULL;

    ctx = (JsonWriterContext*)AllocVec(sizeof(JsonWriterContext), MEMF_CLEAR);
    if (!ctx) {
        FreeVec(ser);
        return NULL;
    }

    /* Create root object */
    ctx->root = cJSON_CreateObject();
    if (!ctx->root) {
        FreeVec(ctx);
        FreeVec(ser);
        return NULL;
    }

    ctx->current = ctx->root;
    ctx->stack = NULL;
    ctx->rememberList = NULL;
    ctx->rememberCount = 0;
    ctx->rememberCapacity = 0;

    /* Initialize serializer */
    ser->context = ctx;
    ser->_isReading = 0;
    ser->typeRegistry = NULL;

    /* Set function pointers */
    ser->t_int = JsonWriter_t_int;
    ser->t_uint = JsonWriter_t_uint;
    ser->t_longlong = JsonWriter_t_longlong;
    ser->t_ulonglong = JsonWriter_t_ulonglong;
    ser->t_fixed = JsonWriter_t_fixed;
    ser->t_bool = JsonWriter_t_bool;
    ser->t_string = JsonWriter_t_string;
    ser->t_string_mutable = JsonWriter_t_string_mutable;
    ser->t_object = JsonWriter_t_object;
    ser->t_arrayobj = JsonWriter_t_arrayobj;
    ser->t_scalararray = JsonWriter_t_scalararray;
    ser->Destroy = JsonWriter_Destroy;

    return ser;
}

char* AukJsonSerializer_GetString(ISerializer* ser) {
    JsonWriterContext* ctx;
    char* jsonStr;
    char* result;
    unsigned long len;

    if (!ser || ser->_isReading) return NULL;

    ctx = (JsonWriterContext*)ser->context;
    if (!ctx || !ctx->root) return NULL;

    /* Get JSON string from cJSON */
    jsonStr = cJSON_Print(ctx->root);
    if (!jsonStr) return NULL;

    /* Copy to AllocVec memory */
    len = strlen(jsonStr);
    result = (char*)AllocVec(len + 1, MEMF_CLEAR);
    if (result) {
        memcpy(result, jsonStr, len + 1);
    }

    /* Free cJSON allocated string */
    FreeVec(jsonStr);

    return result;
}

cJSON* AukJsonSerializer_GetRoot(ISerializer* ser) {
    JsonWriterContext* ctx;

    if (!ser || ser->_isReading) return NULL;

    ctx = (JsonWriterContext*)ser->context;
    return ctx ? ctx->root : NULL;
}

/* ========== JSON Reader Implementation ========== */

/* Add object to remember list, return index */
static int JsonReader_AddToRememberList(JsonReaderContext* ctx, AukObject* obj) {
    /* Grow array if needed */
    if (ctx->rememberCount >= ctx->rememberCapacity) {
        unsigned int newCapacity = ctx->rememberCapacity == 0 ? 16 : ctx->rememberCapacity * 2;
        AukObject** newList = (AukObject**)AllocVec(newCapacity * sizeof(AukObject*), MEMF_CLEAR);
        if (!newList) {
            return -1;
        }

        /* Copy existing entries */
        if (ctx->rememberList) {
            memcpy(newList, ctx->rememberList, ctx->rememberCount * sizeof(AukObject*));
            FreeVec(ctx->rememberList);
        }

        ctx->rememberList = newList;
        ctx->rememberCapacity = newCapacity;
    }

    /* Add object to list */
    ctx->rememberList[ctx->rememberCount] = obj;
    return (int)(ctx->rememberCount++);
}

/* Get object from remember list by index */
static AukObject* JsonReader_GetFromRememberList(JsonReaderContext* ctx, unsigned int index) {
    if (index >= ctx->rememberCount) {
        return NULL;
    }
    return ctx->rememberList[index];
}

static void JsonReader_PushContext(ISerializer* This, const char* name) {
    JsonReaderContext* ctx = (JsonReaderContext*)This->context;
    cJSON* newObj = cJSON_GetObjectItem(ctx->current, name);

    if (newObj && cJSON_IsObject(newObj)) {
        /* Push current to stack and make newObj current */
        PushContextStack(&ctx->stack, &ctx->current, newObj);
    }
}

static void JsonReader_PopContext(ISerializer* This) {
    JsonReaderContext* ctx = (JsonReaderContext*)This->context;
    PopContextStack(&ctx->stack, &ctx->current);
}

static void JsonReader_t_int(ISerializer* This, const char* name, int* value) {
    JsonReaderContext* ctx = (JsonReaderContext*)This->context;
    cJSON* item = cJSON_GetObjectItem(ctx->current, name);

    if (item && cJSON_IsNumber(item)) {
        *value = item->valueint;
    }
}

static void JsonReader_t_uint(ISerializer* This, const char* name, unsigned int* value) {
    JsonReaderContext* ctx = (JsonReaderContext*)This->context;
    cJSON* item = cJSON_GetObjectItem(ctx->current, name);

    if (item && cJSON_IsNumber(item)) {
        *value = (unsigned int)item->valueint;
    }
}

static void JsonReader_t_longlong(ISerializer* This, const char* name, long long* value) {
    JsonReaderContext* ctx = (JsonReaderContext*)This->context;
    cJSON* item = cJSON_GetObjectItem(ctx->current, name);

    if (item && cJSON_IsNumber(item)) {
        *value = (long long)item->valueint;
    }
}

static void JsonReader_t_ulonglong(ISerializer* This, const char* name, unsigned long long* value) {
    JsonReaderContext* ctx = (JsonReaderContext*)This->context;
    cJSON* item = cJSON_GetObjectItem(ctx->current, name);

    if (item && cJSON_IsNumber(item)) {
        *value = (unsigned long long)item->valueint;
    }
}

static void JsonReader_t_fixed(ISerializer* This, const char* name, AukFixed* value) {
    JsonReaderContext* ctx = (JsonReaderContext*)This->context;
    cJSON* item = cJSON_GetObjectItem(ctx->current, name);

    if (item) {
        *value = cJSON_GetNumberFixed(item);
    }
}

static void JsonReader_t_bool(ISerializer* This, const char* name, int* value) {
    JsonReaderContext* ctx = (JsonReaderContext*)This->context;
    cJSON* item = cJSON_GetObjectItem(ctx->current, name);

    if (item && cJSON_IsBool(item)) {
        *value = cJSON_IsTrue(item) ? 1 : 0;
    }
}

static void JsonReader_t_string(ISerializer* This, const char* name, const char** value) {
    JsonReaderContext* ctx = (JsonReaderContext*)This->context;
    cJSON* item = cJSON_GetObjectItem(ctx->current, name);

    if (item && cJSON_IsString(item)) {
        *value = item->valuestring;  /* Points to cJSON internal string */
    } else {
        *value = NULL;
    }
}

static void JsonReader_t_string_mutable(ISerializer* This, const char* name, char** value) {
    JsonReaderContext* ctx = (JsonReaderContext*)This->context;
    cJSON* item = cJSON_GetObjectItem(ctx->current, name);

    /* Free existing string */
    if (*value) {
        AukString_Free(*value);
        *value = NULL;
    }

    if (item && cJSON_IsString(item)) {
        *value = AukString_Duplicate(item->valuestring);
    }
}

static void JsonReader_t_object(ISerializer* This, const char* name, AukObjectPtr* object) {
    JsonReaderContext* ctx = (JsonReaderContext*)This->context;
    cJSON* objNode = cJSON_GetObjectItem(ctx->current, name);
    cJSON* typeItem;
    cJSON* refItem;
    cJSON* indexItem;
    const char* typeName;
    const TypeNameToContructor* reg;
    AukObject* newObj;
    unsigned int refIndex;
    AukObject* refObj;

    /* Release existing object */
    if (*object) {
        AukObjectPtr_Release(object);
    }

    if (!objNode || cJSON_IsNull(objNode)) {
        *object = NULL;
        return;
    }

    if (!cJSON_IsObject(objNode)) {
        return;
    }

    /* Check if this is a reference */
    refItem = cJSON_GetObjectItem(objNode, "__ref");
    if (refItem && cJSON_IsString(refItem)) {
        /* This is a reference - get the index */
        indexItem = cJSON_GetObjectItem(objNode, "__index");
        if (indexItem && cJSON_IsNumber(indexItem)) {
            refIndex = (unsigned int)indexItem->valueint;
            refObj = JsonReader_GetFromRememberList(ctx, refIndex);

            if (refObj) {
                /* Retain reference to existing object */
                AukObjectPtr_Set(object, refObj);
            }
        }
        return;
    }

    /* Get type name */
    typeItem = cJSON_GetObjectItem(objNode, "__type");
    if (!typeItem || !cJSON_IsString(typeItem)) {
        return;
    }
    typeName = typeItem->valuestring;

    /* Find constructor in type registry */
    if (!This->typeRegistry) {
        return;
    }

    for (reg = This->typeRegistry; reg->typename != NULL; reg++) {
        if (AukString_Compare(reg->typename, typeName) == 0) {
            /* Create object using constructor */
            reg->NewConstructor(object);
            newObj = *object;

            if (newObj) {
                /* Add to remember list before serializing (for potential circular refs) */
                JsonReader_AddToRememberList(ctx, newObj);

                /* Push context and deserialize */
                PushContextStack(&ctx->stack, &ctx->current, objNode);

                if (newObj->Serialize) {
                    newObj->Serialize(newObj, This, name);
                }

                PopContextStack(&ctx->stack, &ctx->current);
            }
            return;
        }
    }
}

static void JsonReader_t_arrayobj(ISerializer* This, const char* name, AukArray** array,
    AukObjectNewFunc itemNewFunc, const char* (*itemGetTypeName)(AukObject*)) {
    /* Delegate to t_object since AukArray handles its own serialization */
    JsonReader_t_object(This, name, (AukObjectPtr*)array);
}

static void JsonReader_t_scalararray(ISerializer* This, const char* name, AukScalarArray** array) {
    JsonReaderContext* ctx = (JsonReaderContext*)This->context;
    cJSON* sarrObj = cJSON_GetObjectItem(ctx->current, name);
    cJSON* scalarTypeItem;
    cJSON* ndimItem;
    cJSON* shapeArray;
    cJSON* dataArray;
    cJSON* item;
    AukScalarType scalarType;
    short ndim;
    unsigned int* shape = NULL;
    AukScalarArray* newArray = NULL;
    unsigned int scalarSize;
    unsigned int i;
    short j;
    int arraySize;

    /* Delete existing array */
    if (*array) {
        AukScalarArray_Delete(*array);
        *array = NULL;
    }

    if (!sarrObj || cJSON_IsNull(sarrObj)) {
        *array = NULL;
        return;
    }

    if (!cJSON_IsObject(sarrObj)) {
        return;
    }

    /* Read scalar type */
    scalarTypeItem = cJSON_GetObjectItem(sarrObj, "scalarType");
    if (!scalarTypeItem || !cJSON_IsNumber(scalarTypeItem)) {
        return;
    }
    scalarType = (AukScalarType)scalarTypeItem->valueint;

    /* Read ndim */
    ndimItem = cJSON_GetObjectItem(sarrObj, "ndim");
    if (!ndimItem || !cJSON_IsNumber(ndimItem)) {
        return;
    }
    ndim = (short)ndimItem->valueint;

    if (ndim <= 0) return;

    /* Read shape */
    shapeArray = cJSON_GetObjectItem(sarrObj, "shape");
    if (!shapeArray || !cJSON_IsArray(shapeArray)) {
        return;
    }

    arraySize = cJSON_GetArraySize(shapeArray);
    if (arraySize != ndim) return;

    shape = (unsigned int*)AllocVec(ndim * sizeof(unsigned int), MEMF_CLEAR);
    if (!shape) return;

    j = 0;
    cJSON_ArrayForEach(item, shapeArray) {
        if (cJSON_IsNumber(item) && j < ndim) {
            shape[j++] = (unsigned int)item->valueint;
        }
    }

    /* Create scalar array */
    newArray = AukScalarArray_New(scalarType, ndim, shape);
    FreeVec(shape);

    if (!newArray) return;

    /* Read data */
    dataArray = cJSON_GetObjectItem(sarrObj, "data");
    if (!dataArray || !cJSON_IsArray(dataArray)) {
        AukScalarArray_Delete(newArray);
        return;
    }

    scalarSize = AukScalarArray_GetScalarSize(scalarType);

    i = 0;
    cJSON_ArrayForEach(item, dataArray) {
        if (cJSON_IsNumber(item) && i < newArray->totalElements) {
            if (scalarSize == 1) {
                ((char*)newArray->data)[i] = (char)item->valueint;
            } else if (scalarSize == 2) {
                ((short*)newArray->data)[i] = (short)item->valueint;
            } else if (scalarSize == 4) {
                ((int*)newArray->data)[i] = item->valueint;
            } else if (scalarSize == 8) {
                ((long long*)newArray->data)[i] = (long long)item->valueint;
            }
            i++;
        }
    }

    *array = newArray;
}

static void JsonReader_Destroy(ISerializer* This) {
    JsonReaderContext* ctx = (JsonReaderContext*)This->context;

    if (ctx) {
        /* Clean up stack */
        while (ctx->stack) {
            PopContextStack(&ctx->stack, &ctx->current);
        }

        /* Free remember list (objects are managed by reference counting) */
        if (ctx->rememberList) {
            FreeVec(ctx->rememberList);
        }

        /* Delete root (this deletes entire tree) */
        if (ctx->root) {
            cJSON_Delete(ctx->root);
        }

        FreeVec(ctx);
    }

    FreeVec(This);
}

ISerializer* AukJsonSerializer_CreateReader(const char* jsonString, const TypeNameToContructor* typeRegistry) {
    ISerializer* ser = (ISerializer*)AllocVec(sizeof(ISerializer), MEMF_CLEAR);
    JsonReaderContext* ctx;

    if (!ser) return NULL;

    ctx = (JsonReaderContext*)AllocVec(sizeof(JsonReaderContext), MEMF_CLEAR);
    if (!ctx) {
        FreeVec(ser);
        return NULL;
    }

    /* Parse JSON string */
    ctx->root = cJSON_Parse(jsonString);
    if (!ctx->root) {
        FreeVec(ctx);
        FreeVec(ser);
        return NULL;
    }

    ctx->current = ctx->root;
    ctx->stack = NULL;
    ctx->rememberList = NULL;
    ctx->rememberCount = 0;
    ctx->rememberCapacity = 0;

    /* Initialize serializer */
    ser->context = ctx;
    ser->_isReading = 1;
    ser->typeRegistry = typeRegistry;

    /* Set function pointers */
    ser->t_int = JsonReader_t_int;
    ser->t_uint = JsonReader_t_uint;
    ser->t_longlong = JsonReader_t_longlong;
    ser->t_ulonglong = JsonReader_t_ulonglong;
    ser->t_fixed = JsonReader_t_fixed;
    ser->t_bool = JsonReader_t_bool;
    ser->t_string = JsonReader_t_string;
    ser->t_string_mutable = JsonReader_t_string_mutable;
    ser->t_object = JsonReader_t_object;
    ser->t_arrayobj = JsonReader_t_arrayobj;
    ser->t_scalararray = JsonReader_t_scalararray;
    ser->Destroy = JsonReader_Destroy;

    return ser;
}
