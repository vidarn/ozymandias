import bpy
import bgl

from mathutils import Vector, Matrix

from ctypes import *

class CustomRenderEngine(bpy.types.RenderEngine):
    # These three members are used by blender to set up the
    # RenderEngine; define its internal name, visible name and capabilities.
    bl_idname = 'ozymandias_renderer'
    bl_label = 'Ozymandias'
    bl_use_preview = True
    
    # This is the only method called by blender, in this example
    # we use it to detect preview rendering and call the implementation
    # in another method.
    def render(self, scene):

        scale = scene.render.resolution_percentage / 100.0
        self.size_x = int(scene.render.resolution_x * scale)
        self.size_y = int(scene.render.resolution_y * scale)
        if scene.name == 'preview':
            self.render_preview(scene)
        else:
            import mathutils
            import copy

            class OzyScene(Structure):
                _fields_ = [("num_tris", c_int),
                            ("num_verts", c_int),
                            ("num_mats", c_int)]
              
            class OzyVec3(Structure):
                _fields_ = [("v", c_float *3)]
                def from_vec3(self,vec):
                    self.v[0] = vec[0]
                    self.v[1] = vec[1]
                    self.v[2] = vec[2]
                def vec3(self,x,y,z):
                    self.v[0] = x
                    self.v[1] = y
                    self.v[2] = z
                def mul(self,f):
                    self.v[0] *= f
                    self.v[1] *= f
                    self.v[2] *= f
                
            class OzyMatrix3(Structure):
                _fields_ = [("m", c_float *9)]
                
            class OzyMaterial(Structure):
                _fields_ = [("color", OzyVec3),
                            ("emit",  OzyVec3),
                            ("brdf", c_int)]
            class OzyLambertParameters(Structure):
                _fields_ = []
            class OzyPhongParameters(Structure):
                _fields_ = [("ior", c_float),
                            ("shininess", c_float)]
            
            filename = '/tmp/scene.ozy'
            file = open(filename,'wb')
           
            tris = []
            tris_material = []
            verts_co = []
            verts_normal = []
            
            materials = []
            material_refs = [] #to keep track of which blender material corresponds to each ozy material
            material_params = []
            
            #TODO(Vidar): convenience functions for adding materials
            default_material = OzyMaterial()
            default_material.color.vec3(0.6,0.6,0.6)
            default_material.brdf = 0
            params = OzyLambertParameters()
            materials.append(default_material)
            material_refs.append(0)
            material_params.append(params)
            
            for mat in bpy.data.materials:
                material = OzyMaterial()
                params = 0
                if mat.raytrace_mirror.use:
                    material.color.from_vec3(mat.mirror_color)
                    material.brdf = 1
                    params = OzyPhongParameters()
                    params.ior = mat.raytrace_mirror.fresnel
                    if(params.ior < 0.001): # Mirror-like at ior = 0
                        params.ior = 200.0
                    params.shininess = pow(100000.0, mat.raytrace_mirror.gloss_factor)
                else:
                    material.color.from_vec3(mat.diffuse_color)
                    material.brdf = 0
                    params = OzyLambertParameters()
                if mat.emit > 0.001: #TODO(Vidar): add epsilon... :)
                    material.emit = material.color
                    material.emit.mul(mat.emit)
                    material.color.vec3(0.0,0.0,0.0)
                materials.append(material)
                material_refs.append(mat)
                material_params.append(params)
            
            for obj in scene.objects:
                if obj.type == 'MESH':
                    obj_matrix = mathutils.Matrix(obj.matrix_world)
                    obj_matrix_normal = obj_matrix.inverted().transposed().normalized()
                    mesh = obj.data
                    mesh.update (calc_tessface=True)
                    faces = mesh.tessfaces
                    vert_offset = len(verts_co)
                    for vert in mesh.vertices:
                        #TODO(Vidar): Do we want to transform the normal too?
                        verts_co.append(obj_matrix * vert.co)
                        verts_normal.append(obj_matrix_normal * vert.normal)
                    for face in faces:
                        def find_material(face,obj,material_refs):
                            if len(obj.material_slots) > 0:
                                mat = obj.material_slots[face.material_index].material
                                for i in range(len(material_refs)):
                                    if material_refs[i] == mat:
                                        return i
                            return 0
                        f_verts = face.vertices
                        tris_material.append(find_material(face,obj,material_refs))
                        tris.append(f_verts[0]+vert_offset)
                        tris.append(f_verts[1]+vert_offset)
                        tris.append(f_verts[2]+vert_offset)
                        if(len(f_verts) > 3):
                            tris_material.append(find_material(face,obj,material_refs))
                            tris.append(f_verts[2]+vert_offset)
                            tris.append(f_verts[3]+vert_offset)
                            tris.append(f_verts[0]+vert_offset)
            
            num_tris = int(len(tris)/3)
            num_verts = len(verts_co)
            s = OzyScene(num_tris = num_tris, num_verts = num_verts, num_mats = len(materials))
            t = (c_int *(num_tris*3))()
            v = (OzyVec3*num_verts  )()
            n = (OzyVec3*num_verts  )()
            m = (c_int  *num_tris   )()
            for i in range(num_tris):
                for j in range(3):
                    t[i*3+j] = tris[i*3+j]
                m[i] = tris_material[i]
            for i in range(num_verts):
                for j in range(3):
                    v[i].v[j] = verts_co[i][j]
                    n[i].v[j] = verts_normal[i][j]
                    
            cam_obj = scene.camera    
            c_matrix = OzyMatrix3()
            c_pos = OzyVec3()
            c_fov = c_float(cam_obj.data.angle)
            for i in range(3):
                c_pos.v[i] = cam_obj.location[i]
                for j in range(3):
                    c_matrix.m[i+j*3] = cam_obj.matrix_world[i][j]
            
            
            # Geometry
            file.write(s)
            file.write(t)
            file.write(v)
            file.write(n)
            file.write(m)
            
            # Camera
            file.write(c_pos)
            file.write(c_matrix)
            file.write(c_fov)
            
            # Materials
            for i,material in enumerate(materials):
                file.write(material)
                file.write(material_params[i])
            
            file.close()
    
            #ozylib = cdll.LoadLibrary('libozymandias.so')
            #a = ozylib.test(b'hej')
            #scene = ozylib.read_scene_file(b'/tmp/scene.ozy')
            #print('scene loaded')
            #print(scene)
            #ozylib.free_scene(scene)
            #ozy_scene = ozylib.read_scene_file(c_char_p(b'test'))
    
            self.render_scene(scene)
    
    # In this example, we fill the preview renders with a flat green color.
    def render_preview(self, scene):
        pixel_count = self.size_x * self.size_y

        # The framebuffer is defined as a list of pixels, each pixel
        # itself being a list of R,G,B,A values
        green_rect = [[0.0, 1.0, 0.0, 1.0]] * pixel_count

        # Here we write the pixel values to the RenderResult
        result = self.begin_result(0, 0, self.size_x, self.size_y)
        layer = result.layers[0]
        layer.rect = green_rect
        self.end_result(result)
        


    # In this example, we fill the full renders with a flat blue color.
    def render_scene(self, scene):
        pixel_count = self.size_x * self.size_y
        
        from subprocess import call
        retcode = call(['./ozymandias',''], cwd='/home/vidarn/code/ozymandias/')
        print('render done')
        
        def load_ozy_image(filename,rect,components):
            f = open(filename, "rb")
            array = (c_float * (pixel_count*components))()
            f.readinto(array)
            f.close()
            for y in range(self.size_y):
                for x in range(self.size_x):
                    for j in range(components):
                        rect[x + (self.size_y-y-1)*self.size_x][j] = array[(x + y*self.size_x)*components+j]

        print('creating images')
        # The framebuffer is defined as a list of pixels, each pixel
        # itself being a list of R,G,B,A values
        rect       = [[0.0, 0.0, 1.0, 1.0] for i in range(pixel_count)]
        rect_norm  = [[0.0]*3 for i in range(pixel_count)]
        rect_col   = [[1.0]*4 for i in range(pixel_count)]
        rect_depth = [[0.0] for i in range(pixel_count)]
        
        print('loading images')
        if retcode == 0:
            load_ozy_image("/tmp/ozy_final",rect,3)
            load_ozy_image("/tmp/ozy_normal",rect_norm,3)
            load_ozy_image("/tmp/ozy_color",rect_col,3)
            load_ozy_image("/tmp/ozy_depth",rect_depth,1)
    
        print('registering results')
        # Here we write the pixel values to the RenderResult
        result = self.begin_result(0, 0, self.size_x, self.size_y,"")
        
        # TODO(Vidar) will this work using EXR?
        #result.load_from_file('/tmp/out.png')
        layer = result.layers[0]
        for p in layer.passes: 
            if p.type == 'NORMAL':
                p.rect = rect_norm
            if p.type == 'COLOR':
                p.rect = rect_col
            if p.type == 'Z':
                p.rect = rect_depth
        layer.rect = rect
        
        self.end_result(result)
    

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