%module ozymandias
%{
#include "../ozymandias_public_cpp.hpp"
    struct PythonProgressCallbackData{
        PyObject *func;
        PyObject *context;
    };
    static void python_progress_callback(OzyProgressState state, void *message,
            void *data);
%}

%rename(_render) render;

%include "stdint.i"
%include "../common.h"
%include "../ozymandias_public_cpp.hpp"

%rename(render) render_py;

typedef enum
{
    OZY_PROGRESS_RENDER_BEGIN,
    OZY_PROGRESS_BUCKET_DONE,
    OZY_PROGRESS_RENDER_DONE
} OzyProgressState;

typedef enum 
{
    PASS_FINAL,
    PASS_NORMAL,
    PASS_COLOR,
    PASS_DEPTH,
    //---
    PASS_COUNT
}OzyPass;

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

