#pragma once
#include <assert.h>

#define DYNAMIC_ARRAY_DEF(t) typedef struct                                        \
{                                                                              \
    t *data;                                                                   \
    unsigned size,count;                                                            \
}DynArr_##t;                                                                

#define DYNAMIC_ARRAY_IMP(t)\
DynArr_##t da_create_##t(void);                                                        \
void da_destroy_##t(DynArr_##t *a);                                                    \
void da_push_##t(DynArr_##t *a, t elem);                                               \
t da_pop_##t(DynArr_##t *a);                                                           \
                                                                               \
DynArr_##t da_create_##t(void)                                                         \
{                                                                              \
    DynArr_##t ret = {};                                                           \
    return ret;                                                                \
}                                                                              \
                                                                               \
void da_destroy_##t(DynArr_##t *a)                                                     \
{                                                                              \
    free(a->data);                                                             \
    a->size = 0;                                                               \
    a->count = 0;                                                              \
    a->data = 0;                                                               \
}                                                                              \
                                                                               \
void da_push_##t(DynArr_##t *a, t elem)                                                \
{                                                                              \
    if(a->count + 1 > a->size){                                                \
        a->size = a->size > 0 ? a->size*2 : 8;                                 \
        a->data = (t*)realloc(a->data,a->size*sizeof(t));                          \
        assert(a->data != 0);                                                  \
    }                                                                          \
    a->data[a->count] = elem;                                                  \
    a->count++;                                                                \
}                                                                              \
                                                                               \
t da_pop_##t(DynArr_##t *a)                                                            \
{                                                                              \
    a->count--;                                                                \
    t ret = a->data[a->count];                                                 \
    if(a->count < a->size/2){                                                  \
        a->size /= 2;                                                          \
        a->data = (t*)realloc(a->data,a->size*sizeof(t));                          \
        assert(a->data != 0);                                                  \
    }                                                                          \
    return ret;                                                                \
}
