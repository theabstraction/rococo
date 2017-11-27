(cpp.root $)
// The config must contain one and only one cpp.root entry, which gives the path of generated c++ code. If the path is prefixed with $ then $ is replaced with the <project_root> given in the bennyhill command line

(cpp.exception Rococo::IException)

// sxy.types gives the location where custom types are generated from definitions found in this file. If the path is prefixed with $project$ then $project$ is replaced with the <project_root> given in the bennyhill command line
// if the path is prefixed with $cpp$ then $cpp$ is replaced with the cpp.root as specified above
// The first argument gives the sexy definition file, the second argument gives the types that are written into the cpp header
(type.files $project$../content/scripts/types.sxy $cpp$rococo.script.types.h)

// First define the fundamental types. The format of the command is (primitive <name> <sexy-name> <cpp-name>), which maps <name> found in the sxh file to the <sexy-name> in the generated sxy file, and <cpp-name> in the c++ files
// Arguments that are primitive are passed by value to the script system. If they are output arguments then the cpp side implements them as a return value for the first argument and ref values for the successive arguments.

// Note that namespaces are separated by . in all cases, and the bennyhill system maps cpp namespace separators . to ::

(primitive Int32 Int32 int32)
(primitive Int64 Int64 int64)
(primitive Float32 Float32 float)
(primitive Float64 Float64 double)
(primitive Bool Bool boolean32)
(primitive Degrees Sys.Maths.Degrees Degrees)
(primitive Radians Sys.Maths.Radians Radians)
(primitive Seconds Sys.SI.Seconds Seconds)
(primitive Metres Sys.SI.Metres Metres)
(primitive Kilograms Sys.SI.Kilograms Kilograms)
(primitive RGBAb Int32 RGBAb)

(primitive Pointer Pointer uintptr_t)
(primitive IdMesh Int32 ID_MESH)
(primitive IdEntity Int64 ID_ENTITY)

// (struct <name> <sexy-name> <cpp-name>)  maps <name> found in the sxh file to the <sexy-name> in the generated sxy file, and <cpp-name> in the c++ files
// Arguments that are struct are passed by reference in the script system. They may not be used as output arguments. It is assumed that the structures are defined elsewhere.

(struct Vec2i Sys.Maths.Vec2i Vec2i)
(struct Vec2 Sys.Maths.Vec2 Vec2)
(struct Vec3 Sys.Maths.Vec3 Vec3)
(struct Vec4 Sys.Maths.Vec4 Vec4)
(struct Quat Sys.Maths.Quat Quat)
(struct Rect Sys.Maths.Recti GuiRect)
(struct Rectf Sys.Maths.Rectf GuiRectf)
(struct Matrix4x4 Sys.Maths.Matrix4x4 Matrix4x4)
(struct IString Sys.Type.IString fstring)
(struct IStringBuilder Sys.Type.IStringBuilder IStringPopulator)
(struct FPSAngles Sys.Maths.FPSAngles FPSAngles)
(struct Quad Sys.Maths.Quadf Quad)
(struct Triangle Sys.Maths.Triangle Triangle)
(struct Triangle2d Sys.Maths.Triangle2d Triangle2d)
(struct RGBA Sys.Maths.Vec4 RGBA)

(primitive MaterialId Float32 MaterialId)
(primitive MaterialCategory Int32 Rococo.Graphics.MaterialCategory)

// (defstruct <name> <sexy-name> <cpp-name> (fields) )  maps <name> found in the sxh file to the <sexy-name> in the generated sxy file, and <cpp-name> in the c++ files. It creates new structures in the target specified in (cpp.types ...)
// (fields) is a sequence of s-expressions of the format (<type> <name>) where <type> is either a primitive or struct defined BEFORE the parent defstruct and <name> is a unique name for the variable.
// Field names must follow the naming rules for field variables in sexy, i.e, begin with a lowercase letter a-z and succeed with any sequence of alphanumerics.

(defstruct MaterialVertexData Rococo.MaterialVertexData MaterialVertexData
	(RGBAb colour)
	(MaterialId id)
	(Float32 gloss)
)

(defstruct Vertex Rococo.ObjectVertex ObjectVertex
	(Vec3 position)
	(Vec3 normal)
	(Vec2 uv)
	(MaterialVertexData mat)
)

(defstruct LightSpec Rococo.LightSpec LightSpec
	(Vec3 position)
	(Vec3 direction)
	(Degrees fov)
	(RGBA diffuse)
	(RGBA ambience)
	(Degrees cutoffAngle)
	(Float32 cutoffPower)
	(Float32 attenuation)
)

(defstruct QuadColours Rococo.QuadColours QuadColours
	(RGBAb a)
	(RGBAb b)
	(RGBAb c)
	(RGBAb d)
)

(defstruct QuadVertices Rococo.QuadVertices QuadVertices
	(Quad positions)
	(Rectf uv)
	(Quad normals)
	(QuadColours colours)
)

(defstruct VertexTriangle Rococo.VertexTriangle VertexTriangle
	(Vertex a)
	(Vertex b)
	(Vertex c)
)

(defstruct AABB2d Rococo.AAB2d AABB2d
	(Float32 left)
	(Float32 bottom)
	(Float32 right)
	(Float32 top)
)

(defstruct FlameDef Rococo.FlameDef FlameDef
	(Metres minStartParticleSize)
	(Metres maxStartParticleSize)
	(Metres minEndParticleSize)
	(Metres maxEndParticleSize)
	(Int32 particleCount)
	(Seconds minLifeSpan)
	(Seconds maxLifeSpan)
	(Float32 initialVelocityRange)
	(Float32 initialSpawnPosRange)
	(Float32 jetSpeed)
	(Metres attractorHeight)
	(Metres attractorMaxRange)
	(Metres attractorMinRange)
	(Metres attractorSpawnPosRange)
	(Seconds attractorAIduration)
	(Float32 attractorResetProbability)
	(Float32 attractorDriftFactor)
	(Float32 attractorPerturbFactor)
	(Float32 attractorForce)
)