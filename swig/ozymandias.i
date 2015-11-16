%module ozymandias
%{
#include "../ozymandias_public_cpp.h"
    struct PythonProgressCallbackData{
        PyObject *func;
        PyObject *context;
    };
    static void python_progress_callback(OzyProgressState state, void *message,
            void *data);
%}

%include "../ozymandias_public_cpp.h"

typedef enum
{
    OZY_PROGRESS_RENDER_BEGIN,
    OZY_PROGRESS_BUCKET_DONE,
    OZY_PROGRESS_RENDER_DONE
} OzyProgressState;

%{
    static void python_progress_callback(OzyProgressState state, void *message,
            void *data)
    {
        PythonProgressCallbackData *pdata = (PythonProgressCallbackData*)data;
        PyObject *arglist;
        PyObject *result;

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
        result = PyEval_CallObject(pdata->func,arglist);
        Py_DECREF(arglist);
        Py_XDECREF(result);
    }
%}
%extend ozymandias::Shot{
    void render(Scene scene, Workers workers, PyObject *func , PyObject *context)
    {
        PythonProgressCallbackData data;
        data.func = func;
        data.context = context;
        $self->render(scene, workers, python_progress_callback, &data);
    }
};

