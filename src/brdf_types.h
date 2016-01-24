#pragma once
enum BRDF_TYPE
{
    BRDF_TYPE_PHONG,
    BRDF_TYPE_LAMBERT,
    BRDF_TYPE_EMIT, // TODO(Vidar): start using this... ;)
    BRDF_TYPE_COUNT
};

typedef struct
{
    float ior, shininess;
} PhongParameters;
