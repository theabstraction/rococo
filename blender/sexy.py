# filename = "C:/Blender/2.83/sexy.py"
# >>> exec(compile(open(filename).read(), filename, 'exec'))

def print_vertex (vertex, index):
   p = vertex.co
   n = vertex.normal
   print(" (%d\t" % index, "%+6f" % p.x, "%+6f" % p.y, "%+6f" % p.z, "\t%+6f" % n.x, "%+6f" % n.y, " %+6f" % n.z, ")")

def export_object( object, mesh ):
    index = 0
    
    print("(vertices")
    for v in mesh.vertices:
        print_vertex(v, index)
        index = index + 1
        
    print(")")
       

    print("(polygons")   
    index = 0
    
    for p in mesh.polygons:
        print("(%d " % index, end='')
        print("(mat %d)" % p.material_index)
        
        first = True
        
        uva = object.data.uv_layers.active
        if uva is None:
            print("(")  
            for i in p.vertices:
                print("%i" % i, end='')        
        else:
            for i, j in zip(p.vertices, p.loop_indices):
                uva = object.data.uv_layers.active
                uv = uva.data[j].uv
                print(" (%i\t%f %f)" % (i, uv.x, uv.y))
            print(")")  
            index = index + 1
    
    print(")")
        

objects = bpy.context.scene.objects
for o in objects:
    if o.type == 'MESH':
        meshIndex = bpy.data.meshes.find(o.name)
        if meshIndex != -1:
            print("(mesh \"%s\"" % o.name)
            export_object(o, bpy.data.meshes[meshIndex])
            print(")")
            
mats = bpy.data.materials
for m in mats:
    if len(m.texture_paint_slots) >  0:
        for t in m.texture_paint_slots:
            print(t.name,'saved at',t.filepath)

