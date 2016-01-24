#pragma once 

#ifdef __cplusplus 
extern "C" {
#endif
    #include "../../dynamic_array.h"
    #include "../../brdf_types.h"
    #include "../../shader.h"

    typedef struct
    {
        void *param; //TODO(Vidar): Use an union of the parameters instead?
        float color[3];
        float weight;
        enum BRDF_TYPE type;
    } OSL_Closure;

    DYNAMIC_ARRAY_DEF(OSL_Closure)

    typedef struct OSL_ShadingSystem OSL_ShadingSystem;
    typedef struct OSL_ThreadContext OSL_ThreadContext;
    typedef struct OSL_Parameter     OSL_Parameter;

    typedef struct{
        float P[3], dPdx[3], dPdy[3];        /**< Position */
        float dPdz[3];                       /**< z zeriv for volume shading */
        float I[3], dIdx[3], dIdy[3];        /**< Incident ray */
        float N[3];                          /**< Shading normal */
        float Ng[3];                         /**< True geometric normal */
        float u, dudx, dudy;                 /**< Surface parameter u */
        float v, dvdx, dvdy;                 /**< Surface parameter v */
        float dPdu[3], dPdv[3];              /**< Tangents on the surface */
        float Ps[3], dPsdx[3], dPsdy[3];     /**< Point being lit (valid only
                                               in light attenuation shaders */
        float surfacearea;                   /**< Total area of the object
                                               (defined by light shaders for
                                               energy normalization) */
        int raytype;                         /**< Bit field of ray type flags */
        int backfacing;                      /**< True if we want are shading
                                               the backside of the surface */
    } OSL_ShadeRec;
    OSL_Parameter *osl_new_float_parameter(const char *name, float f);
    OSL_Parameter *osl_new_int_parameter(const char *name, int i);
    OSL_Parameter *osl_new_color_parameter(const char *name, float *f);
    void osl_free_parameter(OSL_Parameter *param);
    void osl_compile_buffer (const char *filename, const char *shadername);
    void osl_info(const char *filename);
    OSL_ShadingSystem * osl_create_shading_system(const char **filenames,
            unsigned num_filenames, OSL_Parameter ***params,
            unsigned *num_params);
    void osl_delete_shading_system(OSL_ShadingSystem *shading_system);
    OzyShaderInfo osl_query(const char *filename);
    void osl_free_shader_info(OzyShaderInfo info);
    OSL_ThreadContext *osl_get_thread_context(OSL_ShadingSystem *shading_system);
    void osl_release_thread_context(OSL_ThreadContext *thread_context,
            OSL_ShadingSystem *shading_system);
    DynArr_OSL_Closure osl_shade(OSL_ShadingSystem *shading_system,
            OSL_ThreadContext *thread_context, OSL_ShadeRec *shade_rec,
            unsigned shader_id);
    void osl_free_closures(DynArr_OSL_Closure closures);
#ifdef __cplusplus 
}
#endif

