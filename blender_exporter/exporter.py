import bpy
import bgl

from mathutils import Vector, Matrix
import ozymandias as ozy
import importlib
importlib.reload(ozy)


class CustomRenderEngine(bpy.types.RenderEngine):
    bl_idname = 'ozymandias_renderer'
    bl_label = 'Ozymandias'
    bl_use_preview = True
    
    def render(self, scene):
        scale = scene.render.resolution_percentage / 100.0
        self.size_x = int(scene.render.resolution_x * scale)
        self.size_y = int(scene.render.resolution_y * scale)
        if scene.name == 'preview':
            self.render_preview(scene)
        else:
            import mathutils
            import copy

            def matrix_to_list(mat):
                return [mat[0][0],mat[1][0],mat[2][0],mat[3][0],
                        mat[0][1],mat[1][1],mat[2][1],mat[3][1],
                        mat[0][2],mat[1][2],mat[2][2],mat[3][2],
                        mat[0][3],mat[1][3],mat[2][3],mat[3][3]]

            filename = '/tmp/scene.ozy'
            file = open(filename,'wb')
           
            materials = []
            material_refs = [] #to keep track of which blender material corresponds to each ozy material

            ozy_scene = ozy.Scene()
            
            #TODO(Vidar): convenience functions for adding materials
            default_material = ozy_scene.add_material("test", [0.0,0.0,0.0])
            materials.append(default_material)
            material_refs.append(0)
            
            for mat in bpy.data.materials:
                #TODO(Vidar): Use blender colors
                emit = [0.0,0.0,0.0]
                col = [mat.diffuse_color.r,mat.diffuse_color.g,mat.diffuse_color.b]
                if mat.emit > 0.001: #TODO(Vidar): epsilon... :)
                    emit = [col[0]*mat.emit,col[1]*mat.emit,col[2]*mat.emit]
                    #col = [0.0,0.0,0.0]
                if mat.raytrace_mirror.use:
                    #col = [mat.mirror_color.r,mat.mirror_color.g,mat.mirror_color.b]
                    material = ozy_scene.add_material("metal",emit)
                            #mat.raytrace_mirror.fresnel,
                            #pow(100000.0, mat.raytrace_mirror.gloss_factor))
                else:
                    material = ozy_scene.add_material("test2",emit)
                materials.append(material)
                material_refs.append(mat)
            
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
                    print("len vert_uvs: {} len tri_uvs: {}".format(len(vert_uvs),
                            len(tri_uvs)))
                    if len(mesh.tessface_uv_textures) > 0:
                        ozy_scene.obj_set_uvs(id,vert_uvs)
                        ozy_scene.obj_set_tri_uvs(id,tri_uvs)
            
            cam_obj = scene.camera    
            cam_mat = copy.copy(obj.matrix_world.transposed())
            #NOTE(Vidar): Blender traces rays in -z direction, ozymandias in +z
            cam_mat[2] = - cam_mat[2]
            ozy_scene.set_camera(matrix_to_list(cam_mat),cam_obj.data.angle)
            
            self.render_scene(scene,ozy_scene)
            ozy_scene.destroy()
    
    def render_preview(self, scene):
        #TODO(Vidar): Implement...
        result = self.begin_result(0, 0, self.size_x, self.size_y)
        self.end_result(result)
        
    def render_scene(self, scene, ozy_scene):
        
        class Context:
            def __init__(self,filename,blender_result,ozy_result):
                self.filename = filename
                self.ozy_result = ozy_result
                self.blender_result = blender_result

        def func1(state,message,context):
            if(state == ozy.OZY_PROGRESS_RENDER_BEGIN):
                #print('begin render')
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
        shot.subsamples_per_thread = 30
        shot.enable_pass(ozy.PASS_FINAL)
        shot.enable_pass(ozy.PASS_NORMAL)
        shot.enable_pass(ozy.PASS_COLOR)
        shot.enable_pass(ozy.PASS_DEPTH)

        workers = ozy.Workers(8)
        ozy_result  = ozy.Result()

        blender_result = self.begin_result(0, 0, self.size_x, self.size_y,"")
        ozy.render(ozy_result,shot,ozy_scene,workers,func1,
                Context('/tmp/ozy',blender_result,ozy_result))

        ozy_result.destroy()
        workers.destroy()
        
    

# Register the RenderEngine
bpy.utils.register_class(CustomRenderEngine)

# RenderEngines also need to tell UI Panels that they are compatible
# Otherwise most of the UI will be empty when the engine is selected.
# In this example, we need to see the main render image button and
# the material preview panel.
from bl_ui import properties_render
properties_render.RENDER_PT_render.COMPAT_ENGINES.add('ozymandias_renderer')
properties_render.RENDER_PT_dimensions.COMPAT_ENGINES.add('ozymandias_renderer')
properties_render.RENDER_PT_output.COMPAT_ENGINES.add('ozymandias_renderer')
del properties_render

from bl_ui import properties_material
for member in dir(properties_material):
    print(member)
    subclass = getattr(properties_material, member)
    try:
        subclass.COMPAT_ENGINES.remove('ozymandias_renderer')
    except:
        pass
properties_material.MATERIAL_PT_preview.COMPAT_ENGINES.add('ozymandias_renderer')
properties_material.MATERIAL_PT_custom_props.COMPAT_ENGINES.add('ozymandias_renderer')
properties_material.MATERIAL_PT_diffuse.COMPAT_ENGINES.add('ozymandias_renderer')
properties_material.MATERIAL_PT_context_material.COMPAT_ENGINES.add('ozymandias_renderer')
properties_material.MATERIAL_PT_mirror.COMPAT_ENGINES.add('ozymandias_renderer')
properties_material.MATERIAL_PT_shading.COMPAT_ENGINES.add('ozymandias_renderer')
del properties_material

from bl_ui import properties_material

del properties_material

class MaterialButtonsPanel():
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "material"
    # COMPAT_ENGINES must be defined in each subclass, external engines can add themselves here

    @classmethod
    def poll(cls, context):
        mat = context.material
        rd = context.scene.render
        return mat and (rd.use_game_engine is False) and (rd.engine in cls.COMPAT_ENGINES)

class MATERIAL_PT_povray_mirrorIOR(MaterialButtonsPanel, bpy.types.Panel):
    bl_label = "IOR Mirror"
    COMPAT_ENGINES = {'ozymandias_renderer'}

    def draw_header(self, context):
        scene = context.material

        self.layout.prop(scene.pov, "mirror_use_IOR", text="")

    def draw(self, context):
        layout = self.layout

        mat = context.material
        layout.active = true

        if true:
            col = layout.column()
            col.alignment = 'CENTER'
            col.label(text="The current Raytrace ")
            col.label(text="Transparency IOR is: " + str(mat.raytrace_transparency.ior))
