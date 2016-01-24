#include "ozymandias_public.h"
#include "libs/osl/osl.h"
OzyShaderInfo ozy_get_shader_info(const char * filename)
{
    return osl_query(filename);
}
void ozy_free_shader_info(OzyShaderInfo info)
{
    osl_free_shader_info(info);
}
