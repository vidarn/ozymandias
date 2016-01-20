#include <OSL/oslcomp.h>
#include <OSL/oslquery.h>
#include <OSL/oslexec.h>
#include <OSL/oslclosure.h>
//#include <OSL/genclosure.h>
#include "simplerend.h"
#include "../../exceptions.h"

using namespace OSL;
#include "osl.h"
#include <cstdlib>
#include <cstring>


DYNAMIC_ARRAY_IMP(OSL_Closure);

extern "C"{
    struct OSL_ShadingSystem
    {
        SimpleRenderer *rend;
        ErrorHandler *errhandler;
        ShadingSystem *shadingsys;
        ShaderGroupRef *shadergroups;
        unsigned num_shadergroups;
    };

    //TODO(Vidar):This could be more compact...
    struct OSL_Parameter
    {
        TypeDesc t;
        char* name;
        union{
            float   float_param[3];
            int     int_param;
        };
        ustring string_param;
    };

    struct OSL_ThreadContext
    {
        OSL::PerThreadInfo *thread_info;
        ShadingContext *ctx;
    };

    OSL_Parameter *osl_new_float_parameter(const char *name, float f)
    {
        OSL_Parameter* ret = (OSL_Parameter*)calloc(1,sizeof(OSL_Parameter));
        ret->name = strdup(name);
        ret->t = TypeDesc::TypeFloat;
        ret->float_param[0] = f;
        return ret;
    } 

    OSL_Parameter *osl_new_int_parameter(const char *name, int i)
    {
        OSL_Parameter* ret = (OSL_Parameter*)calloc(1,sizeof(OSL_Parameter));
        ret->name = strdup(name);
        ret->t = TypeDesc::TypeInt;
        ret->int_param = i;
        return ret;
    } 

    OSL_Parameter *osl_new_color_parameter(const char *name, float *f)
    {
        OSL_Parameter* ret = (OSL_Parameter*)calloc(1,sizeof(OSL_Parameter));
        ret->name = strdup(name);
        ret->t = TypeDesc::TypeColor;
        ret->float_param[0] = f[0];
        ret->float_param[1] = f[1];
        ret->float_param[2] = f[2];
        return ret;
    } 

    void osl_free_parameter(OSL_Parameter *param)
    {
        free(param->name);
        free(param);
    }

    void osl_compile_buffer (const char *filename, const char *shadername)
    {
        std::string osobuffer;
        OSLCompiler compiler;
        std::vector<string_view> options;
        options.push_back("-I/home/vidarn/code/ozymandias/OSL/");

        if (! compiler.compile(filename,options)) {
            std::cerr << "Could not compile \"" << shadername << "\"\n";
        } else {
            std::cout << "\"" << shadername << "\" compiled successfully\n";
        }
    }
    
    void osl_info(const char *filename)
    {
        OSLQuery g;
        g.open (filename, "");
        std::string e = g.geterror();
        if (! e.empty()) {
            std::cout << "ERROR opening shader \"" << filename << "\" (" << e << ")\n";
            return;
        }
        std::cout << g.shadertype() << " \"" << g.shadername() << "\"\n";

        for (size_t i = 0;  i < g.nparams();  ++i) {
            const OSLQuery::Parameter *p = g.getparam (i);
            if (!p)
                break;
            std::string typestring;
            if (p->isstruct)
                typestring = "struct " + p->structname.string();
            else
                typestring = p->type.c_str();
            std::cout << "    \"" << p->name << "\" \""
                      << (p->isoutput ? "output " : "") << typestring << "\"\n";
        }
    }

    OSL_ShadingSystem * osl_create_shading_system(const char **filenames,
            unsigned num_filenames, OSL_Parameter ***params,
            unsigned *num_params)
    {
        SimpleRenderer *rend = new SimpleRenderer();
        ErrorHandler *errhandler = new ErrorHandler();
        ShadingSystem *shadingsys = new ShadingSystem (rend, NULL, errhandler);
        register_closures(shadingsys);
        ShaderGroupRef *shadergroups = new ShaderGroupRef[num_filenames];
        for(unsigned i = 0;i < num_filenames; i++){
            shadergroups[i] = shadingsys->ShaderGroupBegin ();
            shadingsys->attribute ("lockgeom", 1);
            for(unsigned ii = 0;ii < num_params[i]; ii++){
                const OSL_Parameter *param = params[i][ii];
                if(param->t == TypeDesc::TypeString){
                    shadingsys->Parameter(param->name,param->t,
                            &param->string_param);
                }else{
                    shadingsys->Parameter(param->name,param->t,
                            param->float_param);
                }
            }
            shadingsys->Shader ("surface", filenames[i], NULL);
            //TODO(Vidar): Here's were we connect the shader to others in the
            // network...
            // ... 
            shadingsys->ShaderGroupEnd ();
        }
        OSL_ShadingSystem *shading_system = new OSL_ShadingSystem();
        shading_system->errhandler = errhandler;
        shading_system->rend = rend;
        shading_system->shadingsys = shadingsys;
        shading_system->shadergroups = shadergroups;
        shading_system->num_shadergroups = num_filenames;
        return shading_system;
    }

    void osl_delete_shading_system(OSL_ShadingSystem *shading_system)
    {
        for(unsigned i = 0;i < shading_system->num_shadergroups; i++){
            shading_system->shadergroups[i].reset();
        }
        delete shading_system->shadingsys;
        delete shading_system->rend;
        delete shading_system->errhandler;
        delete shading_system;
    }

    OzyShaderInfo osl_query(const char *filename)
    {
        OzyShaderInfo info = {};
        OSLQuery q;
        if(q.open(filename)){
            info.valid = 1;
            info.num_params = q.nparams();
            info.params = new OzyShaderParameter[info.num_params];
            for(int i=0;i<info.num_params;i++){
                const OSL::OSLQuery::Parameter *osl_param = q.getparam(i);
                OzyShaderParameter* param = info.params + i;
                param->basetype     = osl_param->type.basetype;
                param->aggregate    = osl_param->type.aggregate;
                param->vecsemantics = osl_param->type.vecsemantics;
                size_t len = osl_param->name.length();
                param->name = new char[len+1];
                for(size_t ii=0;ii<len;ii++){
                    param->name[ii] = osl_param->name.c_str()[ii];
                }
                param->name[len] = 0;
            }
        }
        return info;
    }

    void osl_free_shader_info(OzyShaderInfo info)
    {
        for(unsigned i =0;i<info.num_params;i++){
            delete[] info.params[i].name;
        }
        delete[] info.params;
    }

    OSL_ThreadContext *osl_get_thread_context(OSL_ShadingSystem *shading_system)
    {
        OSL_ThreadContext *thread_context = new OSL_ThreadContext();
        thread_context->thread_info =
            shading_system->shadingsys->create_thread_info();
        thread_context->ctx =
            shading_system->shadingsys->get_context(thread_context->thread_info);
        return thread_context;
    }

    void osl_release_thread_context(OSL_ThreadContext *thread_context,
            OSL_ShadingSystem *shading_system)
    {
        shading_system->shadingsys->release_context (thread_context->ctx);
        shading_system->shadingsys->destroy_thread_info(
                thread_context->thread_info);
        delete thread_context;
    }

    static void handle_closure_color(const ClosureColor* closure,
            float attenuation[3], DynArr_OSL_Closure* closures)
    {
        switch(closure->type){
            case ClosureColor::MUL: {
                //std::cout << "  Mul\n";
                const ClosureMul* mul = (const ClosureMul*) closure;
                float a[3] = {attenuation[0] * mul->weight[0],
                              attenuation[1] * mul->weight[1],
                              attenuation[2] * mul->weight[2]};
                handle_closure_color(mul->closure,a,closures);
                break;
            }
            case ClosureColor::ADD: {
                //std::cout << "  Add\n";
                const ClosureAdd* add = (const ClosureAdd*) closure;
                handle_closure_color(add->closureA,attenuation,closures);
                handle_closure_color(add->closureB,attenuation,closures);
                break;
            }
            case ClosureColor::COMPONENT: {
                const ClosureComponent* comp = (const ClosureComponent*) closure;
                OSL_Closure closure;
                closure.color[0] = attenuation[0] * comp->w[0];
                closure.color[1] = attenuation[1] * comp->w[1];
                closure.color[2] = attenuation[2] * comp->w[2];
                closure.param = 0;
                closure.weight = closure.color[0] + closure.color[1]
                    + closure.color[2];
                switch(comp->id){
                    case EMISSION_ID:
                        closure.type = BRDF_TYPE_EMIT;
                        break;
                    case BACKGROUND_ID:
                        //std::cout << "Background\n";
                        break;
                    case DIFFUSE_ID:
                        closure.type = BRDF_TYPE_LAMBERT;
                        break;
                    case PHONG_ID:
                        {
                            closure.type = BRDF_TYPE_PHONG;
                            PhongParameters *params = (PhongParameters*)
                                malloc(sizeof(PhongParameters));
                            params->ior =  1000.f;
                            params->shininess =  1000.f;
                            closure.param = params;
                        }
                        break;
                    default:
                        //std::cout << "Invalid BRDF\n";
                        break;
                }
                da_push_OSL_Closure(closures,closure);
                break;
            }
        }
    }

    DynArr_OSL_Closure osl_shade(OSL_ShadingSystem *shading_system,
            OSL_ThreadContext *thread_context, OSL_ShadeRec *shade_rec,
            unsigned shader_id)
    {
        DISABLE_FPE; //NOTE(Vidar): Embree throws various FPE's
        DynArr_OSL_Closure ret = da_create_OSL_Closure();
        //TODO(Vidar): Proper transformations
        Matrix44 M (1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);
        shading_system->rend->camera_params (M, ustring("perspective"), 90.0f,
                            0.1f, 1000.0f, 800, 800);

        OSL::Matrix44 Mshad;  // "shader" space to "common" space matrix
        OSL::Matrix44 Mobj;   // "object" space to "common" space matrix
        Mshad.makeIdentity ();
        Mshad.translate (OSL::Vec3 (1.0, 0.0, 0.0));
        Mshad.rotate (OSL::Vec3 (0.0, 0.0, M_PI_4));
        Mobj.makeIdentity ();
        Mobj.translate (OSL::Vec3 (0.0, 1.0, 0.0));
        Mobj.rotate (OSL::Vec3 (0.0, 0.0, M_PI_2));

        ShaderGlobals sg;
        memset(&sg, 0, sizeof(ShaderGlobals));
        sg.renderstate = &sg;
        sg.shader2common = OSL::TransformationPtr (&Mshad);
        sg.object2common = OSL::TransformationPtr (&Mobj);

        sg.raytype     = shade_rec->raytype;
        sg.u           = shade_rec->u;
        sg.v           = shade_rec->v;
        sg.dudx        = shade_rec->dudx;
        sg.dvdy        = shade_rec->dvdy;
        sg.surfacearea = shade_rec->surfacearea;
#define COPY_VEC3(param) memcpy(&sg.param, shade_rec->param, 3*sizeof(float))
        COPY_VEC3(P);
        COPY_VEC3(dPdx);
        COPY_VEC3(dPdy);
        COPY_VEC3(dPdz);
        COPY_VEC3(dPdu);
        COPY_VEC3(dPdv);
        COPY_VEC3(I);
        COPY_VEC3(dIdx);
        COPY_VEC3(dIdy);
        COPY_VEC3(N);
        COPY_VEC3(Ng);

        if(shading_system->shadingsys->execute (*thread_context->ctx,
                    *shading_system->shadergroups[shader_id], sg)){
            const ClosureColor* closure = sg.Ci;
            if(!closure){
                //std::cout << "No closure!\n";
            }else{
                float attenuation[3] = {1.f,1.f,1.f};
                handle_closure_color(closure,attenuation,&ret);
            }
        }
        ENABLE_FPE;
        return ret;
    }

    void osl_free_closures(DynArr_OSL_Closure closures)
    {
        for(unsigned i = 0;i < closures.count;i ++){
            if(closures.data[i].param != 0){
                free(closures.data[i].param);
            }
        }
        da_destroy_OSL_Closure(&closures);
    }
}
