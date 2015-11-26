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
    if(state == ozy.OZY_PROGRESS_RENDER_DONE):
        print('render done')
        #context.shot.save_to_file(context.filename)

shot  = ozy.Shot()
shot.width  = 512
shot.height = 512
shot.num_buckets_x = 4
shot.num_buckets_y = 4
shot.subsamples_per_thread = 10
shot.enable_pass(ozy.PASS_FINAL)

scene = ozy.Scene('/tmp/scene.ozy')
workers = ozy.Workers(8)
result  = ozy.Result()

ozy.render(result, shot, scene, workers, func1, Context(result,'/tmp/ozy'))

print('getting pass')
result.get_pass(ozy.PASS_FINAL)

result.destroy()
scene.destroy()
workers.destroy()
