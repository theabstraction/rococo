# Blender Model SXY exporter
# Copyright(c)2019-2020 Mark Anthony Taylor. All rights reserved.
# This software was tested on Blender 2.81

bl_info = {
    "name": "Export S-expression 1.0.1 Format (.model.sxy)",
	"description": "Exports the currently selected object as a Sexy Script file",
	"author": "Mark Anthony Taylor",
    "category": "Import-Export",
	"version": (1,1),
	"blender": (2, 81, 0),
	"location": "File > Export",
	"twitter": "@Shyreman",
	"website": "www.shyreman.com",
	"support": "TESTING"
}

import Blender
import bpy

def register():
	print('Sexy Exporter registered')

def unregister():
	print('Sexy Exporter unregistered')

def write_obj(filepath):
	out = file(filepath, 'w')
	sce = bpy.data.scenes.active
	ob = sce.objects.active
	mesh = ob.getData(mesh=1)
	
	out.write('(\' file.type = blender.export (version 1.0.1))')
	
	out.write(' (ISExpression s = \'\n\t(vertices\n')
	for vert in mesh.verts:
		out.write( '\t(%f %f %f)\n' % (vert.co.x, vert.co.y, vert.co.z) )
	
	out.write('\t)\n\t(faces\n')
	
	for face in mesh.faces:
		out.write('\t(')
		for vert in face.v:
			out.write('%i ' % (vert.index + 1))
		out.write(')\n')
	out.close()
Blender.Window.FileSelector(write_obj, "Export")