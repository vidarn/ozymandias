import bpy
import bgl

import os
from mathutils import Vector, Matrix, Color
import ozymandias as ozy
import importlib
importlib.reload(ozy)
import copy


class OzyContext:
    def __init__(self,filename,blender_result,ozy_result):
        self.filename = filename
        self.ozy_result = ozy_result
        self.blender_result = blender_result

def get_shader_name(o):
    if o.shader_filename == '':
        return 'ozy_default'
    else:
        return bpy.path.abspath(o.shader_filename[:-4])

class CustomRenderEngine(bpy.types.RenderEngine):
    bl_idname = 'ozymandias_renderer'
    bl_label = 'Ozymandias'
    bl_use_preview = True
    
    def render(self, scene):
        scale = scene.render.resolution_percentage / 100.0
        self.size_x = int(scene.render.resolution_x * scale)
        self.size_y = int(scene.render.resolution_y * scale)

        def matrix_to_list(mat):
            return [mat[0][0],mat[1][0],mat[2][0],mat[3][0],
                    mat[0][1],mat[1][1],mat[2][1],mat[3][1],
                    mat[0][2],mat[1][2],mat[2][2],mat[3][2],
                    mat[0][3],mat[1][3],mat[2][3],mat[3][3]]

        materials = []
        material_refs = [] #to keep track of which blender material corresponds to each ozy material

        ozy_scene = ozy.Scene()
        
        default_material = ozy_scene.add_material("ozy_default", [0.0,0.0,0.0])
        materials.append(default_material)
        material_refs.append(0)
        
        for mat in bpy.data.materials:
            #TODO(Vidar): Use blender colors
            emit = [0.0,0.0,0.0]
            col = [mat.diffuse_color.r,mat.diffuse_color.g,mat.diffuse_color.b]
            if mat.emit > 0.001: #TODO(Vidar): Let the shader handle this...
                emit = [col[0]*mat.emit,col[1]*mat.emit,col[2]*mat.emit]
            material = ozy_scene.add_material(get_shader_name(mat.ozymandias),emit)
            for p in mat.ozymandias.float_props:
                ozy_scene.material_set_float_param(material,p.name,p.value)
            for p in mat.ozymandias.color_props:
                c = p.value
                ozy_scene.material_set_color_param(material,p.name,[c[0],c[1],c[2]])
            materials.append(material)
            material_refs.append(mat)

        if scene.name == 'preview':
            self.render_preview(scene,ozy_scene)
        else:
            for obj in scene.objects:
                if obj.type == 'MESH':
                    mesh = obj.data
                    mesh.calc_normals_split()
                    mesh.update (calc_tessface=True)
                    faces = mesh.tessfaces
                    tris = []
                    tri_materials = []
                    tri_normals = []
                    tri_uvs = []
                    verts_co = []
                    vert_normals = []
                    vert_uvs = []
            
                    #TODO(Vidar): Proper normals
                    for vert in mesh.vertices:
                        verts_co.append(vert.co[0])
                        verts_co.append(vert.co[1])
                        verts_co.append(vert.co[2])
                    for i,face in enumerate(faces):
                        def find_material(face,obj,material_refs):
                            if len(obj.material_slots) > 0:
                                mat = obj.material_slots[face.material_index].material
                                for i in range(len(material_refs)):
                                    if material_refs[i] == mat:
                                        return materials[i]
                            return 0
                        f_verts = face.vertices
                        tri_materials.append(find_material(face,obj,material_refs))
                        tris.append(f_verts[0])
                        tris.append(f_verts[1])
                        tris.append(f_verts[2])
                        def add_normal(f,vert_normals,tri_normals,i):
                            vert_normals.append(f.split_normals[i][0])
                            vert_normals.append(f.split_normals[i][1])
                            vert_normals.append(f.split_normals[i][2])
                            tri_normals.append(len(tri_normals))
                        def add_uv(f,i,vert_uvs,tri_uvs,mesh):
                            vert_uvs.append(mesh.tessface_uv_textures[0].data[i].uv[f][0])
                            vert_uvs.append(mesh.tessface_uv_textures[0].data[i].uv[f][1])
                            tri_uvs.append(len(tri_uvs))
                        add_normal(face,vert_normals,tri_normals,0)
                        add_normal(face,vert_normals,tri_normals,1)
                        add_normal(face,vert_normals,tri_normals,2)
                        if len(mesh.tessface_uv_textures) > 0:
                            add_uv(0,i,vert_uvs,tri_uvs,mesh)
                            add_uv(1,i,vert_uvs,tri_uvs,mesh)
                            add_uv(2,i,vert_uvs,tri_uvs,mesh)
                        if(len(f_verts) > 3):
                            tri_materials.append(find_material(face,obj,material_refs))
                            tris.append(f_verts[2])
                            tris.append(f_verts[3])
                            tris.append(f_verts[0])
                            add_normal(face,vert_normals,tri_normals,2)
                            add_normal(face,vert_normals,tri_normals,3)
                            add_normal(face,vert_normals,tri_normals,0)
                            if len(mesh.tessface_uv_textures) > 0:
                                add_uv(2,i,vert_uvs,tri_uvs,mesh)
                                add_uv(3,i,vert_uvs,tri_uvs,mesh)
                                add_uv(0,i,vert_uvs,tri_uvs,mesh)
                    num_tris    = int(len(tris)/3)
                    num_verts   = int(len(verts_co)/3)
                    num_uvs     = int(len(vert_uvs)/2)
                    num_normals = int(len(vert_normals)/3)
                    id = ozy_scene.add_object(num_verts,num_normals,num_uvs,num_tris)
                    ozy_scene.obj_set_transform(id,matrix_to_list(obj.matrix_world.transposed()))
                    ozy_scene.obj_set_verts(id,verts_co)
                    ozy_scene.obj_set_normals(id,vert_normals)
                    ozy_scene.obj_set_tris(id,tris)
                    ozy_scene.obj_set_tri_materials(id,tri_materials)
                    ozy_scene.obj_set_tri_normals(id,tri_normals)
                    if len(mesh.tessface_uv_textures) > 0:
                        ozy_scene.obj_set_uvs(id,vert_uvs)
                        ozy_scene.obj_set_tri_uvs(id,tri_uvs)
            
            cam_obj = scene.camera    
            cam_mat = copy.copy(cam_obj.matrix_world.transposed())
            #NOTE(Vidar): Blender traces rays in -z direction, ozymandias in +z
            cam_mat[2] = - cam_mat[2]
            ozy_scene.set_camera(matrix_to_list(cam_mat),cam_obj.data.angle)
            self.render_scene(scene,ozy_scene)
            
            ozy_scene.destroy()

    
    def render_preview(self, scene, ozy_scene):
        def render_callback(state,message,context):
            if(state == ozy.OZY_PROGRESS_RENDER_DONE):
                rect = context.ozy_result.get_pass(ozy.PASS_FINAL)
                layer = context.blender_result.layers[0]
                layer.passes[0].rect = rect
                self.end_result(context.blender_result)
                print('preview render done!')

        shot  = ozy.Shot()
        shot.width  = self.size_x
        shot.height = self.size_y
        shot.bucket_resolution = 2
        shot.subsamples_per_thread = 4
        shot.enable_pass(ozy.PASS_FINAL)

        workers = ozy.Workers(8)
        ozy_result  = ozy.Result()

        blender_result = self.begin_result(0, 0, self.size_x, self.size_y,"")
        ozy.render(ozy_result,shot,ozy_scene,workers,render_callback,
                OzyContext('/tmp/ozy',blender_result,ozy_result))

        ozy_result.destroy()
        workers.destroy()
        
    def render_scene(self, scene, ozy_scene):
        def render_callback(state,message,context):
            if(state == ozy.OZY_PROGRESS_RENDER_BEGIN):
                pass
            if(state == ozy.OZY_PROGRESS_BUCKET_DONE):
                num_buckets = context.ozy_result.get_num_buckets_x()*context.ozy_result.get_num_buckets_y()
                progress = context.ozy_result.get_num_completed_buckets()/num_buckets
                self.update_progress(progress)
                if(progress < 1.0):
                    pass
                    context.ozy_result.save_to_file("/tmp/ozy_out","exr",ozy.OZY_COLORSPACE_LINEAR)
                    context.blender_result.load_from_file("/tmp/ozy_out.exr")
                    self.update_result(context.blender_result)
            if(state == ozy.OZY_PROGRESS_RENDER_DONE):
                context.ozy_result.save_to_file("/tmp/ozy_out","exr",ozy.OZY_COLORSPACE_LINEAR)
                context.blender_result.load_from_file("/tmp/ozy_out.exr")
                self.end_result(context.blender_result)

        shot  = ozy.Shot()
        shot.width  = self.size_x
        shot.height = self.size_y
        shot.bucket_resolution = 3
        shot.subsamples_per_thread = scene.ozymandias.subsamples
        shot.enable_pass(ozy.PASS_FINAL)
        shot.enable_pass(ozy.PASS_NORMAL)
        #shot.enable_pass(ozy.PASS_COLOR)
        shot.enable_pass(ozy.PASS_DEPTH)

        workers = ozy.Workers(8)
        ozy_result  = ozy.Result()

        blender_result = self.begin_result(0, 0, self.size_x, self.size_y,"")
        ozy.render(ozy_result,shot,ozy_scene,workers,render_callback,
                OzyContext('/tmp/ozy',blender_result,ozy_result))

        ozy_result.destroy()
        workers.destroy()
        
    

# Register the RenderEngine
bpy.utils.register_class(CustomRenderEngine)

def enable_all(props):
    for member in dir(props):
        subclass = getattr(props, member)
        try:
            print(subclass)
            subclass.COMPAT_ENGINES.add('ozymandias_renderer')
        except:
            pass

import bl_ui
enable_all(bl_ui.properties_data_mesh)
enable_all(bl_ui.properties_data_camera)
enable_all(bl_ui.properties_scene)
enable_all(bl_ui.properties_render_layer)
enable_all(bl_ui.properties_texture)
enable_all(bl_ui.properties_particle)
#enable_all(bl_ui.properties_world)

bl_ui.properties_material.MATERIAL_PT_context_material.COMPAT_ENGINES.add('ozymandias_renderer')
bl_ui.properties_material.MATERIAL_PT_preview.COMPAT_ENGINES.add('ozymandias_renderer')
bl_ui.properties_material.MATERIAL_PT_custom_props.COMPAT_ENGINES.add('ozymandias_renderer')

bl_ui.properties_render.RENDER_PT_render.COMPAT_ENGINES.add('ozymandias_renderer')
bl_ui.properties_render.RENDER_PT_dimensions.COMPAT_ENGINES.add('ozymandias_renderer')
bl_ui.properties_render.RENDER_PT_output.COMPAT_ENGINES.add('ozymandias_renderer')

def update_shader(self,context):
    shader_filename = get_shader_name(self)
    info = ozy.ShaderInfo(shader_filename)
    old_color_props = {}
    old_float_props = {}
    for prop in self.color_props:
        old_color_props[prop.name] = copy.copy(prop.value)
    for prop in self.float_props:
        old_float_props[prop.name] = prop.value
    self.color_props.clear()
    self.float_props.clear()
    if info.is_valid():
        for i in range(info.num_params()):
            param = info.get_param(i)
            n = param.get_name()
            if param.get_vecsemantics() == ozy.COLOR:
                p = self.color_props.add()
                p.name = n
                p.value = old_color_props.get(p.name,[0.3,0.3,0.3])
            elif param.get_aggregate() == ozy.SCALAR and param.get_basetype() == ozy.FLOAT:
                p = self.float_props.add()
                p.name = n
                p.value = old_float_props.get(p.name,0.0)
            else:
                print(param.get_aggregate(), param.get_basetype(), ozy.SCALAR, ozy.FLOAT)

class OzyColorParam(bpy.types.PropertyGroup):
    value = bpy.props.FloatVectorProperty(subtype = 'COLOR', min = 0.0, max = 1.0)
    name  = bpy.props.StringProperty()
bpy.utils.register_class(OzyColorParam)

class OzyFloatParam(bpy.types.PropertyGroup):
    value = bpy.props.FloatProperty()
    name  = bpy.props.StringProperty()
bpy.utils.register_class(OzyFloatParam)


class OzyMaterialSettings(bpy.types.PropertyGroup):
    shader_filename = bpy.props.StringProperty(name = 'Shader', subtype = 'FILE_PATH', update=update_shader)
    color_props = bpy.props.CollectionProperty(type=OzyColorParam)
    float_props = bpy.props.CollectionProperty(type=OzyFloatParam)

bpy.utils.register_class(OzyMaterialSettings)
bpy.types.Material.ozymandias = \
    bpy.props.PointerProperty(type=OzyMaterialSettings)

class OzyRenderSettings(bpy.types.PropertyGroup):
    subsamples = bpy.props.IntProperty(name = 'Subsamples', min=1, default=30)

bpy.utils.register_class(OzyRenderSettings)
bpy.types.Scene.ozymandias = \
    bpy.props.PointerProperty(type=OzyRenderSettings)


from bl_ui.properties_material import MaterialButtonsPanel

class MATERIAL_PT_ozy_shader(MaterialButtonsPanel, bpy.types.Panel):
    bl_label = "Shader"
    COMPAT_ENGINES = {'ozymandias_renderer'}

    def draw_header(self, context):
        mat = context.material

    def draw(self, context):
        layout = self.layout

        mat = context.material
        layout.active = True

        col = layout.column()
        col.alignment = 'CENTER'
        col.label(text='Shader:')
        col.prop(mat.ozymandias, 'shader_filename', text='')
        for p in mat.ozymandias.color_props:
            col.prop(p,'value',text=p.name)
        for p in mat.ozymandias.float_props:
            col.prop(p,'value',text=p.name)
bpy.utils.register_class(MATERIAL_PT_ozy_shader)

from bl_ui.properties_render import RenderButtonsPanel

class RENDER_PT_ozy_settings(RenderButtonsPanel, bpy.types.Panel):
    bl_label = "Ozymandias"
    COMPAT_ENGINES = {'ozymandias_renderer'}

    def draw(self, context):
        layout = self.layout

        scene = context.scene
        layout.active = True

        col = layout.column()
        col.alignment = 'CENTER'
        col.prop(scene.ozymandias, 'subsamples')

bpy.utils.register_class(RENDER_PT_ozy_settings)
