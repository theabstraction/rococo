# filename = "C:/Blender/2.83/scripts/addons/sexy.py"
# exec(compile(open(filename).read(), filename, 'exec'))

import time
import bpy
 
# write_vertex writes to 4 decimal places, which is 1/10th of a millimetre
# since we are not modelling micrometre or smaller phenomena, this should be enough for our purposes
def write_vertex (output, vertex, index):
    p = vertex.co
    n = vertex.normal
    output.write(" (%.2d %+4.4f %+4.4f %+4.4f\t%+4.4f %+4.4f %+4.4f\n" % (index, p.x, p.y, p.z, n.x, n.y, n.z))

    for g in vertex.groups:
        output.write("\t(%2d %4.4f)\n" % (g.group, g.weight))
   
    output.write(" )\n")

def export_vertices(output, object, mesh ):
    index = 0
    
    output.write("(vertices // %d\n" % len(mesh.vertices))
    for v in mesh.vertices:
        write_vertex(output, v, index)
        index = index + 1
        
    #mirror = object.modifiers["Mirror"]
    #    
    #if mirror != nil:
    #    if mirror.use_axis[0]:
    #        for v in mesh.vertices:
    #            write_vertex_mirrorX(output, v, index)
    #            index = index + 1
        
    output.write(")\n")
       
def export_polygons(output, object, mesh):
    output.write("(polygons // %d\n" % len(mesh.polygons))   
    index = 0
    
    for p in mesh.polygons:
        output.write("(%d %d " % (index, p.material_index))
        first = True

        for i, j in zip(p.vertices, p.loop_indices):
            uva = object.data.uv_layers.active
            if uva is None:
                if first:
                    first = False
                else:
                    output.write(" ")
                output.write("%3i" % i)
            else:
                if first:
                    first = False
                    output.write("\n")

                uv = uva.data[j].uv
                output.write(" (%3i %.4f %.4f)\n" % (i, uv.x, uv.y))
        output.write(")\n")  
        index = index + 1
    
    output.write(")")
    
def export_vec4(output, v, tabCount):
    i = 0
    while i < tabCount:
        output.write("\t")
        i += 1
        
    output.write("(%f %f %f %f)\n" % (v.x, v.y, v.z, v.w))
    
def export_transforms(output, object):
    output.write("(local-matrix\n")
    
    m = object.matrix_local
    
    export_vec4(output, m.row[0], 1)
    export_vec4(output, m.row[1], 1)
    export_vec4(output, m.row[2], 1)
    export_vec4(output, m.row[3], 1)
    
    output.write(")\n")
    
def export_mesh_elements(output, mesh, object):
    if len(mesh.polygons) > 0:
        output.write("(mesh \"%s\"\n" % object.name)
        export_vertices(output, object, mesh)
        export_polygons(output, object, mesh)
        export_materials(output, mesh.materials)
        export_transforms(output, object)
        output.write("(shaders (pixels \"no-texture\"))\n")
        output.write(") // %s\n" % object.name)

def export_mesh(output, object):
    if object.type == 'MESH' and object.name != "_mesh":
        meshIndex = bpy.data.meshes.find(object.name)
        mesh = 0
        if meshIndex != -1:
            mesh = bpy.data.meshes[meshIndex]
            export_mesh_elements(output, mesh, object)
        else:
            mesh = object.data
            export_mesh_elements(output, mesh, object)
            
def output_spaces(output, number):
    i = 0
    while i < number:
        output.write("\t")
        i += 1

def export_bone(output, bone, depth):
    output.write("\n")
    output_spaces(output, depth)
    output.write("(bone %d \"%s\"\n" % (bone.bone_group_index, bone.name))
    
    # q = bone.rotation_quaternion
    # output.write("(rot %4.4f %4.4f %4.4f %4.4f)" % (q.x, q.y, q.z, q.w))
    # 
    # s = bone.scale;
    # output.write("(scale %4.4f %4.4f %4.4f)" % (s.x, s.y, s.z))
    # 
    #  l = bone.location;
    # output.write("(loc %4.4f %4.4f %4.4f)" % (l.x, l.y, l.z))

    m = bone.matrix
   
    export_vec4(output, m.row[0], depth + 1)
    export_vec4(output, m.row[1], depth + 1)
    export_vec4(output, m.row[2], depth + 1)
    # export_vec4(output, m.row[3], depth)
    
    for child in bone.children:
        export_bone(output, child, depth + 1)
        
    output_spaces(output, depth)
        
    output.write(")")

def export_pose(output, armature):
    pose = armature.pose
    
    output.write("\n(pose /* (rot q.x q.y. q.z q.scalar) (scale Sx Sy Sz) (loc lx ly lz) */ ")
 
    for bone in pose.bones:
        if bone.name == "c_pos":
            export_bone(output, bone, 1)
    
    output.write(")\n")         
                    
def export_geometry(output, object):
    export_mesh(output,object)
    for child in object.children:
        export_geometry(output,child)
                    
    
def export_materials(output, mats):
    i = 0
    if (len(mats) > 0):
        output.write("\n(materials\n")
        for m in mats:
            d = m.diffuse_color
            s = m.specular_color
            output.write("\t(%d \"%s\"\n" % (i, m.name_full))
            output.write("\t\t(diffuse %.3f %.3f %.3f)\n" % (d[0], d[1], d[2]))
            output.write("\t\t(specular %.3f %.3f %.3f)\n" % (s[0], s[1], s[2]))
           
            if len(m.texture_paint_slots) > 0:
                j = 0;
                for t in m.texture_paint_slots:
                    output.write("\t\t(texture %d \"%s\")\n" % (j, t.filepath))
                    j = j + 1
            output.write("\t)\n")  
            i = i + 1
        output.write(")\n")
                

def export_object_to_file(object, filename):
    output = open(filename, 'w', encoding='utf-8') 
    output.write("(' #file.type mplat.rig (version 1 0 0 0))\n")

    t = time.time()
    timestring = time.strftime("%b %d %Y %H %M %S", time.gmtime(t))

    output.write("(' #file.created %s)\n" % timestring)
    output.write("(' #file.origin \"exported from blender %s with sexy.py\")\n\n" %  bpy.app.version_string)
    output.write("(' #mesh.format (mesh <name> (for-each-vertex-in-mesh: (#vertex.format)) (for-each-polygon-in-mesh: (#polygon.format))))\n")
    output.write("(' #vertex.format (Int32 index)(Float32 x)(Float32 y)(Float32 z)(Float32 nx)(Float32 ny)(Float32 nz)(for-each-group: (groupId weight)))\n")
    output.write("(' #polygon.format (Int32 index)(Int32 matId) (for-each-vertex-in-polygon: (Int32 vertexIndex)(Float32 u)(Float32 v)))\n\n")
    output.write("(' #include\n")
    output.write('\t"!scripts/mplat.sxh.sxy"\n')
    output.write('\t"!scripts/mplat.types.sxy"\n')
    output.write('\t"!scripts/types.sxy"\n')
    output.write(')\n\n')

    output.write("(using Sys.Reflection)\n\n")
    output.write("(function Main (Int32 id) -> (Int32 exitCode):\n")
    output.write("(IExpression rig = '(")

    output.write("\"%s\"\n" % object.name)
    export_geometry(output, object)
    
    if object.type == 'ARMATURE':
        export_pose(output, object)

    output.write("))\n")
    output.write("(Rococo.LoadRig rig)\n)\n(namespace EntryPoint)\n(alias Main EntryPoint.Main)\n")
    output.close()

bl_info = {
    "name": "Export to Sexy Script Rig File (.sxy)",
    "blender": (2, 80, 0),
    "location": "File > Export > Sexy Script Rig File (.sxy)",
    "description": "Export rig to sexy script file (.sxy)",
    "category": "Import-Export"  
}
def register():
    bpy.utils.register_class(SexyExporter)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export_button)
    
def unregister():
   bpy.utils.unregister_class(SexyExporter)
   bpy.types.TOPBAR_MT_file_export.remove(menu_func_export_button)
    
class SexyExporter(bpy.types.Operator):
    """Sexy Exporter"""      # Use this as a tooltip for menu items and buttons.
    bl_idname = "export.sxy"        # Unique identifier for buttons and menu items to reference.
    bl_label = "Export to Sexy Format"         # Display name in the interface.
    bl_options = {'REGISTER' }  # Enable undo for the operator.

def execute(self, context):        # execute() is called when running the operator.
    run();
    return {'FINISHED'}    

def menu_func_export_button(self, context):
    self.layout.operator(SexyExporter.bl_idname, text="Sexy Script Rig (.sxy)")
    
def run():
    export_object_to_file(bpy.context.active_object, 'C:\\work\\rococo\\blender\\export.sxy');
    
if __name__ == "__main__":
    run()
