#pragma once
#include <stdint.h>

typedef struct {
    char *name;
    uint8_t basetype;     ///< C data type at the heart of our type
    uint8_t aggregate;    ///< What kind of AGGREGATE is it?
    uint8_t vecsemantics;
} OzyShaderParameter;

typedef struct {
    uint8_t valid;
    uint32_t num_params;
    OzyShaderParameter *params;
} OzyShaderInfo;

