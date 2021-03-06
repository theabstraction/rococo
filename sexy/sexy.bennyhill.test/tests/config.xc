// The config must contain one and only one cpp.root entry, which gives the path of generated c++ code. If the path is prefixed with $ then $ is replaced with the <project_root> given in the bennyhill command line
(cpp.root $generated\)

// sxy.types gives the location where custom types are generated from definitions found in this file. If the path is prefixed with $project$ then $project$ is replaced with the <project_root> given in the bennyhill command line
// if the path is prefixed with $cpp$ then $cpp$ is replaced with the cpp.root as specified above
// The first argument gives the sexy definition file, the second argument gives the types that are written into the cpp header
(type.files $project$types.sxy $cpp$types.h)

// First define the fundamental types. The format of the command is (primitive <name> <sexy-name> <cpp-name>), which maps <name> found in the sxh file to the <sexy-name> in the generated sxy file, and <cpp-name> in the c++ files
// Arguments that are primitive are passed by value to the script system. If they are output arguments then the cpp side implements them as a return value for the first argument and ref values for the successive arguments.

// Note that namespaces are separated by . in all cases, and the bennyhill system maps cpp namespace separators . to ::

(primitive Int32 Int32 Sys.int32)
(primitive Int64 Int64 Sys.int64)
(primitive Float32 Float32 float)
(primitive Float64 Float64 double)
(primitive Bool Bool boolean32)

// System defined strings. They are defined as structs to stop them being used as output values, which is not yet implemented.
(struct IString Sys.Type.IString fstring)
(struct IStringBuilder Sys.Type.IStringBuilder Rococo.IStringPopulator)

(primitive Pointer Pointer uintptr_t)

(cpp.exception Sys::IException)

// (struct <name> <sexy-name> <cpp-name>)  maps <name> found in the sxh file to the <sexy-name> in the generated sxy file, and <cpp-name> in the c++ files
// Arguments that are struct are passed by reference in the script system. They may not be used as output arguments. It is assumed that the structures are defined elsewhere.

(struct Vec2 Sys.Maths.Vec3 Sys.Vector2)
(struct Vec3 Sys.Maths.Vec3 Sys.Vector3)
(struct Vec4 Sys.Maths.Vec3 Sys.Vector4)

// (defstruct <name> <sexy-name> <cpp-name> (fields) )  maps <name> found in the sxh file to the <sexy-name> in the generated sxy file, and <cpp-name> in the c++ files. It creates new structures in the target specified in (cpp.types ...)
// (fields) is a sequence of s-expressions of the format (<type> <name>) where <type> is either a primitive or struct defined BEFORE the parent defstruct and <name> is a unique name for the variable.
// Field names must follow the naming rules for field variables in sexy, i.e, begin with a lowercase letter a-z and succeed with any sequence of alphanumerics.

(defstruct Sys.Type.GuiRect Sys.Type.GuiRect Sys.Type.GuiRect
	(Vec2 bottomLeft)
	(Vec2 span) 
)


(defstruct Sys.HTIGER Sys.HTIGER Sys.HTIGER
	(Pointer hTiger)
)
