%module ozymandias
%{
#include "../shader.h"
#include "../ozymandias_public.h"
#include "../ozymandias_public_cpp.hpp"
#include "../scene.h"
    struct PythonProgressCallbackData{
        PyObject *func;
        PyObject *context;
    };
    static void python_progress_callback(OzyProgressState state, void *message,
            void *data);
%}

%rename(_render) render;

%typemap(in) Matrix4 {
    if (!PyList_Check($input)) {
        PyErr_SetString(PyExc_ValueError, "Expecting a list");
        return NULL;
    }
    for(u32 i=0;i<16;i++){
        PyObject *s = PyList_GetItem($input,i);
        if (!PyFloat_Check(s)) {
            PyErr_SetString(PyExc_ValueError, "List items must be float");
            return NULL;
        }
        $1.m[i] = PyFloat_AsDouble(s);
    }
}
%typemap(in) Vec3 {
    if (!PyList_Check($input)) {
        PyErr_SetString(PyExc_ValueError, "Expecting a list");
        return NULL;
    }
    for(u32 i=0;i<3;i++){
        PyObject *s = PyList_GetItem($input,i);
        if (!PyFloat_Check(s)) {
            PyErr_SetString(PyExc_ValueError, "List items must be float");
            return NULL;
        }
        $1.v[i] = PyFloat_AsDouble(s);
    }
}

%include "stdint.i"
%include "../common.h"
#include "../shader.h"
%include "../ozymandias_public.h"
%include "../ozymandias_public_cpp.hpp"

typedef struct{float v[4];}  Vec3;
typedef struct{float m[9];}  Matrix3;
typedef struct{float m[16];} Matrix4;

%rename(render) render_py;

%{
    static void python_progress_callback(OzyProgressState state, void *message,
            void *data)
    {
        PythonProgressCallbackData *pdata = (PythonProgressCallbackData*)data;
        PyObject *arglist;
        PyObject *ret;

        switch(state){
            case OZY_PROGRESS_RENDER_BEGIN:
            case OZY_PROGRESS_RENDER_DONE:
                arglist = Py_BuildValue("IsO",state,NULL,pdata->context); 
                break;
            case OZY_PROGRESS_BUCKET_DONE:
                {
                    OzyProgressBucketDoneMessage *msg
                        = (OzyProgressBucketDoneMessage*)message;
                    arglist = Py_BuildValue("I{s:I,s:I,s:I}O",state,
                            "bucket_id",  msg->bucket_id,
                            "num_buckets",msg->num_buckets,
                            "num_done",   msg->num_done,
                            pdata->context); 
                    break;
                }
            default:
                break;
        }
        ret = PyEval_CallObject(pdata->func,arglist);
        Py_DECREF(arglist);
        Py_XDECREF(ret);
    }
%}

void render_py(ozymandias::Result result, ozymandias::Shot shot,
        ozymandias::Scene scene, ozymandias::Workers workers,
        PyObject *func, PyObject *context);
%{
    void render_py(ozymandias::Result result, ozymandias::Shot shot,
            ozymandias::Scene scene, ozymandias::Workers workers,
            PyObject *func, PyObject *context)
    {
        PythonProgressCallbackData data;
        data.func = func;
        data.context = context;
        ozy_render(result.result, (OzyShot*)&shot, scene.scene,
                workers.workers, python_progress_callback,&data);
    }
%}


%extend ozymandias::Scene{
#define SET_OBJ_VEC3(func, data_size) \
    PyObject *obj_##func(u32 obj, PyObject *list) {\
        if (!PyList_Check(list)) { \
            PyErr_SetString(PyExc_ValueError, "Expecting a list"); \
            return NULL; \
        } \
        if(obj >= $self->scene->objects.count){ \
            PyErr_SetString(PyExc_ValueError, "Invalid object"); \
            return NULL; \
        } \
        u32 len = PyList_Size(list); \
        if(len != $self->scene->objects.data[obj].##data_size*3){ \
            PyErr_SetString(PyExc_ValueError, "List is wrong length"); \
            return NULL; \
        } \
        float *data = (float *) malloc(len*sizeof(Vec3)); \
        u32 a = 0; \
        for(u32 i = 0; i < len; i++) { \
            PyObject *s = PyList_GetItem(list,i); \
            if (!PyFloat_Check(s)) { \
                free(data); \
                PyErr_SetString(PyExc_ValueError, "List items must be float"); \
                return NULL; \
            } \
            data[a] = PyFloat_AsDouble(s); \
            a += i%3 == 2 ? 2 : 1; \
        } \
        ozy_scene_obj_##func($self->scene,obj,(Vec3*)data); \
        free(data); \
        return Py_None; \
    }

#define SET_OBJ_FLOAT(func, data_size) \
    PyObject *obj_##func(u32 obj, PyObject *list) {\
        if (!PyList_Check(list)) { \
            PyErr_SetString(PyExc_ValueError, "Expecting a list"); \
            return NULL; \
        } \
        if(obj >= $self->scene->objects.count){ \
            PyErr_SetString(PyExc_ValueError, "Invalid object"); \
            return NULL; \
        } \
        u32 len = PyList_Size(list); \
        if(len != $self->scene->objects.data[obj].##data_size){ \
            PyErr_SetString(PyExc_ValueError, "List is wrong length"); \
            return NULL; \
        } \
        float *data = (float *) malloc(len*sizeof(float)); \
        for(u32 i = 0; i < len; i++) { \
            PyObject *s = PyList_GetItem(list,i); \
            if (!PyFloat_Check(s)) { \
                free(data); \
                PyErr_SetString(PyExc_ValueError, "List items must be float"); \
                return NULL; \
            } \
            data[i] = PyFloat_AsDouble(s); \
        } \
        ozy_scene_obj_##func($self->scene,obj,data); \
        free(data); \
        return Py_None; \
    }

#define SET_OBJ_U32(func, data_size) \
    PyObject *obj_##func(u32 obj, PyObject *list) {\
        if (!PyList_Check(list)) { \
            PyErr_SetString(PyExc_ValueError, "Expecting a list"); \
            return NULL; \
        } \
        if(obj >= $self->scene->objects.count){ \
            PyErr_SetString(PyExc_ValueError, "Invalid object"); \
            return NULL; \
        } \
        u32 len = PyList_Size(list); \
        if(len != $self->scene->objects.data[obj].##data_size){ \
            PyErr_SetString(PyExc_ValueError, "List is wrong length"); \
            return NULL; \
        } \
        u32 *data = (u32 *) malloc(len*sizeof(u32)); \
        for(u32 i = 0; i < len; i++) { \
            PyObject *s = PyList_GetItem(list,i); \
            if (!PyInt_Check(s)) { \
                free(data); \
                PyErr_SetString(PyExc_ValueError, "List items must be integer"); \
                return NULL; \
            } \
            data[i] = PyInt_AsLong(s); \
        } \
        ozy_scene_obj_##func($self->scene,obj,data); \
        free(data); \
        return Py_None; \
    }

    SET_OBJ_VEC3(set_verts,num_verts)
    SET_OBJ_VEC3(set_normals,num_normals)
    SET_OBJ_FLOAT(set_uvs,num_uvs*2)
    SET_OBJ_U32(set_tris,num_tris*3)
    SET_OBJ_U32(set_tri_normals,num_tris*3)
    SET_OBJ_U32(set_tri_uvs,num_tris*3)
    SET_OBJ_U32(set_tri_materials,num_tris)
};

%extend ozymandias::Result{
    //TODO(Vidar): Unify these functions, they are almost identical...
    PyObject *get_pass(OzyPass pass)
    {
        int w = $self->get_width();
        int h = $self->get_height();
        int c = ozy_pass_channels[pass];
        float *buffer = (float*)malloc(sizeof(float) * w * h * c);
        $self->get_pass(pass,buffer);
        PyObject *ret = PyList_New(w*h);
        for(int y=0;y<h;y++){
            for(int x=0;x<w;x++){
                PyObject *list = PyList_New(c);
                for(int j=0;j<c;j++){
                    PyList_SET_ITEM(list,j,PyFloat_FromDouble(buffer[(x+y*w)*c+j]));
                }
                PyList_SET_ITEM(ret,(x+(h-y-1)*w),list);
            }
        }
        free(buffer);
        return ret;
    }

    PyObject *get_bucket(OzyPass pass, int bucket_id)
    {
        int w = $self->get_bucket_width(bucket_id);
        int h = $self->get_bucket_height(bucket_id);
        int c = ozy_pass_channels[pass];
        float *buffer = (float*)malloc(sizeof(float) * w * h * c);
        $self->get_bucket(pass,bucket_id,buffer);
        PyObject *ret = PyList_New(w*h);
        for(int y=0;y<h;y++){
            for(int x=0;x<w;x++){
                PyObject *list = PyList_New(c);
                for(int j=0;j<c;j++){
                    PyList_SET_ITEM(list,j,PyFloat_FromDouble(buffer[(x+y*w)*c+j]));
                }
                PyList_SET_ITEM(ret,(x+(h-y-1)*w),list);
            }
        }
        free(buffer);
        return ret;
    }
};

