import ozymandias as ozy

class Context:
    def __init__(self,shot,filename):
        self.shot = shot
        self.filename = filename

def func1(state,message,context):
    if(state == ozy.OZY_PROGRESS_RENDER_BEGIN):
        print('begin render')
    if(state == ozy.OZY_PROGRESS_BUCKET_DONE):
        print('bucket ',message['bucket_id'],' done')
    if(state == ozy.OZY_PROGRESS_RENDER_DONE):
        print('render done')
        context.shot.save_to_file(context.filename)

scene   = ozy.Scene('/tmp/scene.ozy')
shot    = ozy.Shot(512,512)
workers = ozy.Workers(8)

shot.render(scene,workers,func1,Context(shot,'/tmp/ozy'))
