(cpp.root $)
// The config must contain one and only one cpp.root entry, which gives the path of generated c++ code. If the path is prefixed with $ then $ is replaced with the <project_root> given in the bennyhill command line

(cpp.exception Rococo::IException)

// sxy.types gives the location where custom types are generated from definitions found in this file. If the path is prefixed with $project$ then $project$ is replaced with the <project_root> given in the bennyhill command line
// if the path is prefixed with $cpp$ then $cpp$ is replaced with the cpp.root as specified above
// The first argument gives the sexy definition file, the second argument gives the types that are written into the cpp header
(type.files $project$../content/scripts/mhost/mhost.types.sxy $cpp$mhost.script.types.h)

// First define the fundamental types. The format of the command is (primitive <name> <sexy-name> <cpp-name>), which maps <name> found in the sxh file to the <sexy-name> in the generated sxy file, and <cpp-name> in the c++ files
// Arguments that are primitive are passed by value to the script system. If they are output arguments then the cpp side implements them as a return value for the first argument and ref values for the successive arguments.

// Note that namespaces are separated by . in all cases, and the bennyhill system maps cpp namespace separators . to ::

(primitive Int32 Int32 Rococo.int32)
(primitive Int64 Int64 Rococo.int64)
(primitive Float32 Float32 float)
(primitive Float64 Float64 double)
(primitive Bool Bool Rococo.boolean32)
(primitive Degrees Sys.Maths.Degrees Rococo.Degrees)
(primitive Radians Sys.Maths.Radians Rococo.Radians)
(primitive Seconds Sys.SI.Seconds Rococo.Seconds)
(primitive Metres Sys.SI.Metres Rococo.Metres)
(primitive Kilograms Sys.SI.Kilograms Rococo.Kilograms)
(primitive RGBAb Int32 Rococo.RGBAb)

(primitive IdTexture MHost.IdTexture Rococo.IdTexture)

(primitive Pointer Pointer uintptr_t)
(primitive IdMesh Int32 Rococo.ID_MESH)
(primitive IdEntity Int64 Rococo.ID_ENTITY)
(primitive MaterialId Float32 Rococo.Graphics.MaterialId)
(primitive MaterialCategory Int32 Rococo.Graphics.MaterialCategory)
(primitive GuiPopulator MHost.GuiPopulator MHost.GuiPopulator)
(primitive ELegacySoundShape Int32 Rococo.Audio.ELegacySoundShape)
(primitive IString Sys.Type.IString Rococo.fstring)
(primitive IStringBuilder Sys.Type.IStringBuilder Rococo.Strings.IStringPopulator)
(primitive IExpression Sys.Reflection.IExpression Rococo.Script.ISxyExpressionRef)

// (struct <name> <sexy-name> <cpp-name>)  maps <name> found in the sxh file to the <sexy-name> in the generated sxy file, and <cpp-name> in the c++ files
// Arguments that are struct are passed by reference in the script system. They may not be used as output arguments. It is assumed that the structures are defined elsewhere.

(struct Vec2i Sys.Maths.Vec2i Rococo.Vec2i)
(struct Vec2 Sys.Maths.Vec2 Rococo.Vec2)
(struct Vec3 Sys.Maths.Vec3 Rococo.Vec3)
(struct Vec4 Sys.Maths.Vec4 Rococo.Vec4)
(struct Quat Sys.Maths.Quat Rococo.Quat)
(struct Matrix4x4 Sys.Maths.Matrix4x4 Rococo.Matrix4x4)
(struct FPSAngles Sys.Maths.FPSAngles Rococo.FPSAngles)
(struct Vertex Rococo.ObjectVertex Rococo.ObjectVertex)
(struct VertexTriangle Rococo.VertexTriangle Rococo.VertexTriangle)
(struct Quad Sys.Maths.Quadf Quad)
(struct Triangle Sys.Maths.Triangle Rococo.Triangle)
(struct Triangle2d Sys.Maths.Triangle2d Rococo.Triangle2d)
(struct MaterialVertexData Rococo.MaterialVertexData Rococo.MaterialVertexData)
(struct AABB2d Rococo.AAB2d Rococo.AABB2d)
(struct QuadVertices Rococo.QuadVertices Rococo.QuadVertices)
(struct BitmapLocation MPlat.BitmapLocation Rococo.Graphics.Textures.BitmapLocation)
(struct GuiTriangle MPlat.GuiTriangle Rococo.Graphics.GuiTriangle)
(struct GuiQuad MPlat.GuiQuad Rococo.Graphics.GuiQuad)
(struct Rectf Sys.Maths.Rectf Rococo.GuiRectf)
(struct Recti Sys.Maths.Recti Rococo.GuiRect)
(struct KeyState MHost.OS.KeyState MHost.OS.KeyState)
(struct MouseEvent MHost.OS.MouseEvent Rococo.MouseEvent)
(struct KeyboardEvent MHost.OS.KeyboardEvent Rococo.MHostKeyboardEvent)
(struct RGBA RGBA Rococo.RGBA)
(struct FontDesc MHost.Graphics.FontDesc MHost.Graphics.FontDesc)
(struct WorldOrientation MHost.WorldOrientation MHost.WorldOrientation)
(struct CubeTextureDef MHost.CubeTextureDef MHost.Graphics.CubeTextureDef)

// (defstruct <name> <sexy-name> <cpp-name> (fields) )  maps <name> found in the sxh file to the <sexy-name> in the generated sxy file, and <cpp-name> in the c++ files. It creates new structures in the target specified in (cpp.types ...)
// (fields) is a sequence of s-expressions of the format (<type> <name>) where <type> is either a primitive or struct defined BEFORE the parent defstruct and <name> is a unique name for the variable.
// Field names must follow the naming rules for field variables in sexy, i.e, begin with a lowercase letter a-z and succeed with any sequence of alphanumerics.

(struct GuiEvent MHost.GuiTypes.GuiEvent MHost.GuiTypes.GuiEvent)

