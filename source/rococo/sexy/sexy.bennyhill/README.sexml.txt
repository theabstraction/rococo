BennyHill SEXML config files
============================

BennyHill reads two inputs:
	the generator file, which declares the functions, enums and interfaces to generate  - X.sxh (unchanged)
	the config file, which maps sxh type names to sexy and C++ types                   - config.xc.sexml (legacy: config.xc)

	bennyhill.exe <C++-project-root> <sxy-content-root> <sxh-file> <config-fullpath>

The config front end is chosen by the extension of <config-fullpath>. A file ending .sexml is parsed with the rococo.sexml library
(bennyhill.config.sexml.inl). Any other file is parsed as a legacy xc file (bennyhill.config.inl). Both front ends call the same config
builders in bennyhill.config.inl, so config.xc.sexml and the config.xc it was converted from give byte-identical generated code.
Generator files stay in the sxh format and are parsed exactly as before.

A config file is named <stem>.xc.sexml (api.config becomes api.config.sexml). The .xc keeps the familiar name and marks the file as a
BennyHill config.

Converting files
----------------
build/tools/xc2sexml is a C# (.NET 8) console tool that converts .xc/.config to .xc.sexml/.config.sexml:

	dotnet run --project build\tools\xc2sexml -c Release -- source\rococo\rococo.mhost\code-gen\config.xc
	dotnet run --project build\tools\xc2sexml -c Release -- source\rococo                 (every *.xc file below the directory)
	dotnet run --project build\tools\xc2sexml -c Release -- --check source\rococo source\rococo\sexy\sexy.bennyhill\api.generator\api.config
	                                                                                      (fails if a .sexml file is missing or out of date)

Before writing, the tool builds a model of the xc input (every item, in order, with its values and the token tree of every defstruct field)
and an independent model from the generated SEXML, and refuses to write if they differ.

Comments are kept as // comments on the lines before the directive they precede (a /* */ comment stays a block comment). Comments that
describe the xc syntax, such as (primitive <name> <sexy-name> <cpp-name>), are rewritten to describe the SEXML directive. A header comment
pointing to this file is added. BennyHill does not copy config comments into generated code, in xc or in SEXML.

SEXML in one paragraph
----------------------
A directive is (<Name> <attribute>... [: <sub-directive>...]). Config directives have attributes only.
	(Name value)          an atomic or "string literal" value
	(' Name (...))        a raw attribute: the s-expression is taken verbatim, which is how defstruct fields are written
Attribute names are unique within a directive.

Config file (config.xc.sexml)
=============================
Each directive corresponds to one xc config item. Directives are processed in order, except DefStruct, which is processed after all
other directives, as in xc.

(CppRoot (Path <cpp-root-directory>))                                     xc: (cpp.root ...)        required, once. $ = the project root
(CppException (Type <cpp-exception-type>))                                xc: (cpp.exception ...)   once
(TypeFiles (SexyTypes <sexy-types-path>) (CppTypes <cpp-types-path>))     xc: (type.files ...)      once, after CppRoot.
	Paths may begin $project$ (project root), $content$ (content root, SexyTypes only; for CppTypes it maps to the C++ root) or $cpp$ (C++ root)
(Primitive (Name <sxh-type>) (SexyType <sexy-type>) (CppType <cpp-type>)) xc: (primitive ...)  passed by value; may be an output
(Struct (Name <sxh-type>) (SexyType <sexy-type>) (CppType <cpp-type>))    xc: (struct ...)     passed by reference; defined elsewhere
(Interface (Name <sxh-type>) (SexyType <sexy-type>) (CppType <cpp-type>)) xc: (interface ...)
(DefStruct (Name <sxh-type>) (SexyType <sexy-type>) (CppType <cpp-type>) (' Fields (
	(<field-type> <field-name>)
	...
)))                                                                       xc: (defstruct ...)  generated into the TypeFiles

Before and after
----------------
xc (rococo.audio/config.xc, abridged):
	(cpp.root $)
	(cpp.exception Rococo::IException)
	(type.files $content$/audio_types.sxy $cpp$rococo.audio.types.h)
	(primitive Int32 Int32 int32)
	(struct Vertex Rococo.ObjectVertex ObjectVertex)
	(defstruct AudioSource3D Rococo.Audio.AudioSource3D Rococo.Audio.AudioSource3D
		(Int32 priority)
		(Float32 volume)
		(Vec3 position)
		(Vec3 dopplerVelocity)
		(Float32 msDelay)
	)

sexml (rococo.audio/config.xc.sexml, abridged):
	(CppRoot (Path $))
	(CppException (Type Rococo::IException))
	(TypeFiles (SexyTypes $content$/audio_types.sxy) (CppTypes $cpp$rococo.audio.types.h))
	(Primitive (Name Int32) (SexyType Int32) (CppType int32))
	(Struct (Name Vertex) (SexyType Rococo.ObjectVertex) (CppType ObjectVertex))
	(DefStruct (Name AudioSource3D) (SexyType Rococo.Audio.AudioSource3D) (CppType Rococo.Audio.AudioSource3D) (' Fields (
		(Int32 priority)
		(Float32 volume)
		(Vec3 position)
		(Vec3 dopplerVelocity)
		(Float32 msDelay)
	)))

Errors
------
SEXML syntax errors, unknown directives, unknown or duplicated attributes, a missing required attribute, sub-directives, a Fields
attribute that is not a raw compound, an empty Fields list and non-atomic DefStruct names are errors. The checks of the xc front end
(CppRoot once and first, TypeFiles once, duplicate types, type name rules, bool rejected as a primitive and so on) apply unchanged.
Every error names the config file and gives the line and position of the offending expression.
