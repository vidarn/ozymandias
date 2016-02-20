import ozymandias as ozy

class Context:
    def __init__(self,result,filename):
        self.result = result
        self.filename = filename

def func1(state,message,context):
    if(state == ozy.OZY_PROGRESS_RENDER_BEGIN):
        print('begin render')
    if(state == ozy.OZY_PROGRESS_BUCKET_DONE):
        print('bucket ',message['bucket_id'],' done')
        #context.result.save_to_file("{}{}".format(context.filename,
            #str(message['bucket_id'])),"exr", ozy.OZY_COLORSPACE_LINEAR)
    if(state == ozy.OZY_PROGRESS_RENDER_DONE):
        print('render done')
        context.result.save_to_file(context.filename,"exr",
                ozy.OZY_COLORSPACE_LINEAR)

shot  = ozy.Shot()
shot.width  = 800
shot.height = 600
shot.bucket_resolution = 2
shot.subsamples_per_thread = 10
shot.enable_pass(ozy.PASS_FINAL)
shot.enable_pass(ozy.PASS_COLOR)
shot.enable_pass(ozy.PASS_DEPTH)
shot.enable_pass(ozy.PASS_NORMAL)


scene = ozy.Scene()
workers = ozy.Workers(8)
result  = ozy.Result()

material = scene.add_material("test")
obj = scene.add_object(3,1,0,1)
scene.obj_set_verts(obj,[-1.0,-1.0,20.0,
                          1.0,-1.0,20.0,
                          1.0, 1.0,20.0])
scene.obj_set_normals(obj,[0.0,0.0,-1.0])
scene.obj_set_tris(obj,[0,1,2])
scene.obj_set_tri_normals(obj,[0,0,0])
scene.obj_set_tri_materials(obj,[material])
scene.obj_set_transform(obj,[1.0,0.0,0.0,0.0,
                             0.0,1.0,0.0,0.0,
                             0.0,0.0,1.0,0.0,
                             0.0,0.0,0.0,1.0])
scene.set_camera([1.0,0.0,0.0,0.0,
                  0.0,1.0,0.0,0.0,
                  0.0,0.0,1.0,0.0,
                  0.0,0.0,0.0,1.0], 0.9)

info = ozy.ShaderInfo('test')
if info.is_valid():
    print('valid shader')
    print('num parameters:', info.num_params())
    param = info.get_param(0)
    print(param.get_basetype())
else:
    print('invalid shader')

ozy.render(result, shot, scene, workers, func1, Context(result,'/tmp/ozy'))

result.destroy()
scene.destroy()
workers.destroy()
