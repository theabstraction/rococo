/*
	Sexy Scripting Language - Copright(c)2013. Mark Anthony Taylor. All rights reserved.

	https://github.com/theabstraction/rococo

	Email: mark.anthony.taylor@gmail.com

	The Sexy copyright holder is Mark Anthony Taylor, the English author of 'Lazy Bloke Ghosts of Parliament', 'Lazy Bloke in the 35th Century', 'King of the Republic', 'Origin of the Species' 
	

	1. This software is open-source. It can be freely copied, distributed, compiled and the compilation executed.
	
	1.a Modification of the software is not allowed where such modifcations fail to include this exact comment header in each source file, or alter, reduce or extend the Sexy language.
	The purpose of this limitation is to prevent different incompatible versions of the Sexy language emerging. 

	1.b You are allowed to fix bugs and implement performance improvements providing you inform Sexy's copyright owner via email. Such changes may then be incorporated in
	later versions of Sexy without compromising Sexy's copyright license.
	
	2. You are not permitted to copyright derivative versions of the source code. You are free to compile the code into binary libraries and include the binaries in a commercial application. 

	3. THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW. EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM “AS IS” WITHOUT
	WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK AS TO THE QUALITY
	AND PERFORMANCE OF THE PROGRAM IS WITH YOU. SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.

	4. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MODIFIES AND/OR CONVEYS THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR
	DAMAGES, INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR DATA BEING RENDERED
	INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF
	SUCH DAMAGES.

	5. Any program that distributes this software in source or binary form must include an obvious credit to the the language, its copyright holder, and the language homepage. This must be the case in its
	principal credit screen and its principal readme file.
*/

#pragma once

#ifndef sexy_compiler_public_h
# define sexy_compiler_public_h

#include <sexy.types.h>

#ifndef SEXYUTIL_API
# define SEXYUTIL_API ROCOCO_API_IMPORT
#endif

namespace Rococo { namespace VM
{
	struct IVirtualMachine;
	struct IAssembler;
	struct IProgramMemory;
}} // Rococo::VM

namespace Rococo::Sex
{
	struct ISExpression;
}

namespace Rococo::Script
{
	struct NativeSecurityHandler;
}

namespace Rococo::Components
{
	struct IComponentBase;
	struct IComponentLife;
}

namespace Rococo {
	namespace Compiler
	{
		struct IFunction;
		struct IFunctionSet;
		struct IFunctionCode;
		struct IModule;
		struct INamespace;
		struct IPublicProgramObject;
		struct IStructure;
		struct IStructureSet;
		struct IMember;
		struct IInterface;
		struct IArchetype;

		struct FunctionPrototype;
		struct StructurePrototype;

		struct FunctionPrototype
		{
			cstr Name;
			bool IsMethod;
			FunctionPrototype(cstr _name, bool _isMethod) : Name(_name), IsMethod(_isMethod) {}
		};

		bool IsDerivedFrom(const IInterface& sub, const IInterface& super);

		struct ProgramInitParameters
		{
			size_t MaxProgramBytes;
			const wchar_t* NativeSourcePath;
			bool addCoroutineLib = false; // Set to true to use sexy.nativelib.coroutine.dll
			bool useDebugLibs = false; // Set to true to use the sexy.nativelib.debug.<xxx>.dlls 
			bool addIO = false; // Set to true to use Sys.IO.sxy and the Sexy IO library

			enum { ONE_KILOBYTE = 1024 };
			ProgramInitParameters() : MaxProgramBytes(1024 * ONE_KILOBYTE), NativeSourcePath(nullptr) {}
			ProgramInitParameters(size_t _maxProgBytes, const wchar_t* _nativeSourcePath = nullptr) : MaxProgramBytes(_maxProgBytes), NativeSourcePath(_nativeSourcePath) {}
		};

		class NameString
		{
		private:
			cstr s;
			NameString(cstr _s) : s(_s) {}
		public:
			static NameString From(cstr s) { return NameString(s); }
			static NameString From(sexstring s) { return NameString(s->Buffer); }
			cstr c_str() const { return s; }
		};

		class TypeString
		{
		private:
			cstr s;
			TypeString(cstr _s) : s(_s) {}
		public:
			static TypeString From(cstr s) { return TypeString(s); }
			static TypeString From(sexstring s) { return TypeString(s->Buffer); }
			cstr c_str() const { return s; }
		};

		inline bool IsContainerType(VARTYPE v)
		{
			switch (v)
			{
			case VARTYPE_Array:
				return true;
			case VARTYPE_List:
				return true;
			case VARTYPE_Map:
				return true;
			}

			return false;
		}

		inline bool IsPrimitiveType(VARTYPE v)
		{
			switch (v)
			{
			case VARTYPE_Bad:
			case VARTYPE_Derivative:
			case VARTYPE_Closure:
			case VARTYPE_Array:
			case VARTYPE_List:
			case VARTYPE_Map:
				return false;
			default:
				return true;
			}
		}

		inline bool IsNumericType(VARTYPE v)
		{
			return v == VARTYPE_Int32 || v == VARTYPE_Int64 || v == VARTYPE_Float32 || v == VARTYPE_Float64;
		}

		inline bool IsNumericTypeOrBoolean(VARTYPE v)
		{
			return IsNumericType(v) || v == VARTYPE_Bool;
		}

		enum MEMBERALIGN
		{
			MEMBERALIGN_1 = 1,
			MEMBERALIGN_2 = 2,
			MEMBERALIGN_4 = 4,
			MEMBERALIGN_8 = 8,
			MEMBERALIGN_BITMASK = 0xFFFFFFFF
		};

		enum INSTANCEALIGN
		{
			INSTANCEALIGN_1 = 1,
			INSTANCEALIGN_2 = 2,
			INSTANCEALIGNALIGN_4 = 4,
			INSTANCEALIGN_8 = 8,
			INSTANCEALIGN_16 = 16,
			INSTANCEALIGN_SSE = 16, // Used for aligning SSEx data
			INSTANCEALIGN_128 = 128,
			INSTANCEALIGN_CACHELINE = 128,	// Used for aligning data structures on a memory cache line
			INSTANCEALIGN_BITMASK = 0xFFFFFFFF
		};

		struct StructurePrototype
		{
			MEMBERALIGN MemberAlignment;				// Gives the memory alignment for members of the described structure
			INSTANCEALIGN InstanceAlignment;		// Gives the memory alignment of instances of the described structure
			bool IsFixedSequence;								// True if the compiler is not allowed to reorder members
			const IArchetype* archetype;
			bool IsClass;												// True if this structure represents the guts of a class

			StructurePrototype(MEMBERALIGN _memberAlign, INSTANCEALIGN _instanceAlign, bool _isFixedSequence, const IArchetype* _archeType, bool _isClass) :
				MemberAlignment(_memberAlign),
				InstanceAlignment(_instanceAlign),
				IsFixedSequence(_isFixedSequence),
				archetype(_archeType),
				IsClass(_isClass)
			{
			}
		};

		enum ARGUMENTUSAGE
		{
			ARGUMENTUSAGE_BYVALUE,
			ARGUMENTUSAGE_BYREFERENCE
		};

		enum ARGDIRECTION
		{
			ARGDIRECTION_INPUT,
			ARGDIRECTION_OUTPUT
		};

#pragma pack(push, 1)
		struct ObjectDescFlags
		{
			uint64 IsSystem : 1; // 1 => object created by trusted native C++, 0 => object created by Sexy
		};

		struct ObjectDesc
		{
			ID_BYTECODE Zero; // Should be zero, gives the number of bytes to the start of the object, which is this stub
			ID_BYTECODE DestructorId;
			IStructure* TypeInfo;
			ObjectDescFlags flags;
		};

		struct VirtualTable
		{
			ptrdiff_t OffsetToInstance;
			ID_BYTECODE FirstMethodId;
		};

		typedef VirtualTable** InterfacePointer;

		struct ObjectStub
		{
			static const int BYTECOUNT_INSTANCE_TO_INTERFACE0 = 16;
			ObjectDesc* Desc;
			int64 refCount;
			VirtualTable* pVTables[1];
			enum : int64 { NO_REF_COUNT = 0x4000000000000000 };

			inline InterfacePointer AddressOfVTable0()
			{
				return &pVTables[0];
			}
		};

		enum { SF_TO_DESTRUCTED_OBJECT_OFFSET = -40 };

		struct ComponentObject
		{
			ObjectStub stub;
			Rococo::Components::IComponentBase* component;
			Rococo::Components::IComponentLife* life;
			int64 roid;
		};

		struct ObjectStubWithHandle
		{
			ObjectStub stub;
			void* handle;
		};

		enum SPEC
		{
			SPEC_E = 1,
			SPEC_F = 2,
			SPEC_G = 3,
		};

		enum class StringBuilderFlags
		{
			None = 0, // Default
			ThrowIfWouldTruncate = 1 // If set then attempting to write past the capacity of the builder throws an exception
		};

		struct FastStringBuilder
		{
			ObjectStub stub;
			int32 length;
			char* buffer;
			int32 capacity;
			int formatBase;
			char prefix[12];
			SPEC spec;
			int32 flags;
		};

		inline int GetInstanceToInterfaceOffset(int interfaceIndex)
		{
			int stubsize = sizeof(ObjectStub);
			int ptrsize = sizeof(VirtualTable*);
			return stubsize - ptrsize + interfaceIndex * ptrsize;
		}

		struct InlineString
		{
			ObjectStub stub;
			int32 length;
			cstr buffer;
		} TIGHTLY_PACKED;
#pragma pack(pop)

		inline uint8* GetInterfacePtr(const ObjectStub& stub)
		{
			return (uint8*)&stub.pVTables[0];
		}

		ROCOCO_INTERFACE IArchetype
		{
			virtual cstr Name() const = 0;
			virtual const int NumberOfOutputs() const = 0;
			virtual const int NumberOfInputs() const = 0;
			virtual const IStructure& GetArgument(int index) const = 0;
			virtual cstr GetArgName(int index) const = 0;
			virtual const bool IsVirtualMethod() const = 0;
			virtual const IStructure* GetGenericArg1(int index) const = 0;
			virtual const void* Definition() const = 0;
			virtual const Rococo::Script::NativeSecurityHandler* Security() const = 0;
		};

		ROCOCO_INTERFACE IArgument
		{
			virtual cstr Name() const = 0;
			virtual cstr TypeString() const = 0;
			virtual const ARGUMENTUSAGE Usage() const = 0;
			virtual const ARGDIRECTION Direction() const = 0;
			virtual const IStructure* ResolvedType() const = 0;
			virtual const IStructure* GenericTypeArg1() const = 0;
			virtual const IStructure* GenericTypeArg2() const = 0;
			virtual bool IsClosureInput() const = 0;
			virtual const IFunction& Parent() const = 0;
			virtual void* Userdata() const = 0;
		};

		ROCOCO_INTERFACE IFunction : public IArchetype
		{
			virtual const IModule& Module() const = 0;
			virtual IPublicProgramObject& Object() const = 0;
			virtual const IFunctionCode& Code() const = 0;
			virtual const IFunction* Parent() const = 0;
			virtual const IArgument& Arg(int index) const = 0;
			virtual const IArgument* GetArgumentByName(cstr name) const = 0;
			virtual const IStructure* GetType() const = 0;
			virtual const int32 GetExtraPopBytes() const = 0; // Number of extra bytes to unwind from stack after function recalls
		};

		inline int ArgCount(const IArchetype& archetype) { return archetype.NumberOfInputs() + archetype.NumberOfOutputs(); }

		ROCOCO_INTERFACE IFunctionAlias
		{
			virtual const IFunction& GetFunction() const = 0;
			virtual cstr GetPublicName() const = 0;
		};

		ROCOCO_INTERFACE IStructAlias
		{
			virtual const IStructure& GetStructure() const = 0;
			virtual cstr GetPublicName() const = 0;
		};

		ROCOCO_INTERFACE IFunctionEnumerator
		{
			virtual int FunctionCount() const = 0;

			virtual const IFunctionAlias& operator[](int index) const = 0;
		};

		SEXYUTIL_API const IFunction* FindByName(const IFunctionEnumerator& e, cstr publicName);

		ROCOCO_INTERFACE IMember
		{
			virtual cstr Name() const = 0;
			virtual const int SizeOfMember() const = 0;
			virtual const bool IsResolved() const = 0;
			virtual const IStructure* UnderlyingType() const = 0;
			virtual const IStructure* UnderlyingGenericArg1Type() const = 0;
			virtual const IStructure* UnderlyingGenericArg2Type() const = 0;
			virtual bool IsInterfaceVariable() const = 0;
		};

		ROCOCO_INTERFACE IStructure
		{
			virtual IPublicProgramObject& Object() const = 0;
			virtual cstr Name() const = 0;
			virtual const StructurePrototype& Prototype() const = 0;
			virtual const IModule& Module() const = 0;

			virtual bool HasInterfaceMembers() const = 0;
			virtual int MemberCount() const = 0;
			virtual const IMember& GetMember(int index) const = 0;
			virtual int SizeOfStruct() const = 0;
			virtual const VARTYPE VarType() const = 0;
			virtual const IArchetype* Archetype() const = 0;
			virtual bool IsResolved() const = 0;
			virtual int InterfaceCount() const = 0;
			virtual const IInterface& GetInterface(int index) const = 0;

			virtual const ID_BYTECODE* GetVirtualTable(int interfaceIndex) const = 0;
			virtual ID_BYTECODE GetDestructorId() const = 0;
			virtual const Sex::ISExpression* Definition() const = 0;
			virtual const IFunction* Constructor() const = 0;

			virtual int32 AttributeCount() const = 0;

			virtual bool IsNullType() const = 0;
			virtual bool IsStrongType() const = 0;

			// Enumerate methods and cache the result permanently in the IStructure and return the result
			virtual int CountMethodsInDefiningModule() const = 0;

			// Enumerate methods and cache the result permanently in the IStructure and return the cached method reference
			virtual const IArchetype& GetMethodFromModule(int methodIndex) const = 0;

			// Retrieve the ith attribute. isCustom is an out parameter, and is set to true if element 1 of the returned attributeDef was a system attribute
			// System attributes are specified by using atomic tokens rather than string literals and are validated against the known list of system attributes.
			virtual Rococo::Sex::cr_sex GetAttributeDef(int32 index, bool& isCustom) const = 0;
		};

		inline bool operator == (const IStructure& a, const IStructure& b) { return &a == &b; }
		inline bool operator != (const IStructure& a, const IStructure& b) { return &a != &b; }

		ROCOCO_INTERFACE  IModule
		{
			virtual int GetVersion() const = 0;
			virtual const INamespace* DefaultNamespace() const = 0;

			virtual const IFunction* FindFunction(cstr name) const = 0;
			virtual const IStructure* FindStructure(cstr name) const = 0;

			virtual cstr Name() const = 0;
			virtual IPublicProgramObject& Object() const = 0;

			virtual const IFunction& GetFunction(int index) const = 0;
			virtual const IStructure& GetStructure(int index) const = 0;

			virtual int FunctionCount() const = 0;
			virtual int StructCount() const = 0;
			virtual int PrefixCount() const = 0;
			virtual const INamespace& GetPrefix(int index) const = 0;

			// Check to see if the  module has been marked as a system object. See MakeSystem()
			virtual bool IsSystem() const = 0;

			// Mark this module as a system object. Objects created herein will also be marked in their vtable[0] ObjectDesc
			virtual void MakeSystem() = 0; 
		};

		inline bool IsNullType(const IStructure& s) { return s.IsNullType(); }

		enum VARLOCATION
		{
			VARLOCATION_NONE = 0,
			VARLOCATION_INPUT,
			VARLOCATION_OUTPUT,
			VARLOCATION_TEMP,
		};

		struct MemberDef
		{
			const IStructure* ResolvedType;
			int SFOffset; // The offset in the stack frame to the containing variable, or variable ref
			int MemberOffset; // The offset from the variable pointer to the member
			ARGUMENTUSAGE Usage;
			bool IsParentValue; // True if this member is from a function that contains the closure using the member item
			void* Userdata;
			int AllocSize;
			size_t pcStart;
			size_t pcEnd;
			VARLOCATION location;
			bool IsContained;
			bool CapturesLocalVariables;
		};

		enum ADDNAMESPACEFLAGS
		{
			ADDNAMESPACEFLAGS_NORMAL = 0x00000000,
			ADDNAMESPACEFLAGS_CREATE_ROOTS = 0x00000001
		};

		ROCOCO_INTERFACE  IAttributes
		{
			virtual bool AddAttribute(cstr name, const void* value) = 0;
			virtual const int AttributeCount() const = 0;
			virtual const bool FindAttribute(cstr name, OUT const void*& value) const = 0;
			virtual cstr GetAttribute(int index, OUT const void*& value) const = 0;
		};

		ROCOCO_INTERFACE IInterface
		{
			virtual const IAttributes& Attributes() const = 0;
			virtual const IInterface* Base() const = 0;
			virtual const IArchetype& GetMethod(int index) const = 0;
			virtual const int MethodCount() const = 0;
			virtual cstr Name() const = 0;
			virtual const IStructure& NullObjectType() const = 0;
			virtual ObjectStub* UniversalNullInstance() const = 0;
		};

		inline bool operator == (const IInterface& a, const IInterface& b)
		{
			return &a == &b;
		}

		ROCOCO_INTERFACE IFactory
		{
			virtual cstr Name() const = 0;
			virtual const IFunction& Constructor() const = 0;
			virtual const IInterface& ThisInterface() const = 0;
			virtual sexstring InterfaceType() const = 0;
			virtual const IFunction* InlineConstructor() const = 0; // if not NULL indicates the factory trivially calls a constructor with matching parameters
			virtual const IStructure* InlineClass() const = 0; // if not NULL indicates the concrete class of the inline constructor

		};

		ROCOCO_INTERFACE IMacro
		{
			virtual cstr Name() const = 0;
			virtual const void* Expression() const = 0;
			virtual const INamespace& NS() const = 0;
			virtual const IFunction& Implementation() const = 0;
		};

		ROCOCO_INTERFACE INamespace
		{
			virtual const INamespace* FindSubspace(cstr subpath) const = 0;
			virtual const IStructure* FindStructure(cstr name) const = 0;
			virtual const IFunction* FindFunction(cstr name) const = 0;
			virtual const IArchetype* FindArchetype(cstr name) const = 0;

			virtual const IFactory* FindFactory(cstr name) const = 0;

			virtual const IInterface* FindInterface(cstr name) const = 0;
			virtual const IMacro* FindMacro(cstr name) const = 0;

		    virtual const INamespace& GetChild(size_t index) const = 0;
			virtual size_t ChildCount() const = 0;
			virtual const sexstring FullName() const = 0;
			virtual const sexstring Name() const = 0;

			virtual IPublicProgramObject& Object() const = 0;

			virtual int InterfaceCount() const = 0;
			virtual const IInterface& GetInterface(int index) const = 0;

			virtual void EnumerateFactories(ICallback<const IFactory, cstr>& onFactory) const = 0;
			virtual void EnumerateStrutures(ICallback<const IStructure, cstr>& onStructure) const = 0;
			virtual void EnumerateFunctions(ICallback<const IFunction, cstr>& onFunction) const = 0;
			virtual void EnumerateArchetypes(ICallback<const IArchetype>& onArchetype) const = 0;
		};

		enum SEXY_CLASS_ID : size_t
		{
			SEXY_CLASS_ID_STRINGBUILDER = 0
		};

		struct LeakArgs
		{
			const ObjectStub* object;
		};

		ROCOCO_INTERFACE IScriptObjectAllocator
		{
			virtual void* AllocateObject(size_t nBytes) = 0;
			virtual void FreeObject(void* pMemory) = 0;

			virtual refcount_t AddRef() = 0;
			virtual refcount_t ReleaseRef() = 0;
			virtual size_t FreeAll(IEventCallback<LeakArgs>* leakCallback) = 0;
		};

		struct AllocatorBinding
		{
			void* operator new(size_t nBytes);
			void operator delete(void* buffer);

			IScriptObjectAllocator* memoryAllocator;
			const IStructure* associatedStructure;
			bool standardDestruct = true;
		};

		struct IAllocatorMap
		{
			virtual AllocatorBinding* GetAllocator(const IStructure& s) = 0;
			virtual void SetAllocator(const IStructure* s, AllocatorBinding* binding) = 0;
		};

		ROCOCO_INTERFACE IPublicProgramObject
		{
			virtual const IModule& GetModule(int index) const = 0;
			virtual const INamespace& GetRootNamespace() const = 0;
			virtual int ModuleCount() const = 0;
			virtual const IModule& IntrinsicModule() const = 0;

			virtual Rococo::VM::IVirtualMachine& VirtualMachine() = 0;
			virtual Rococo::VM::IProgramMemory& ProgramMemory() = 0;
			virtual const Rococo::VM::IProgramMemory& ProgramMemory() const = 0;
			virtual ILog& Log() = 0;
			virtual void Free() = 0;
			virtual void SetProgramAndEntryPoint(const IFunction& f) = 0;
			virtual void SetProgramAndEntryPoint(ID_BYTECODE byteCodeId) = 0;
			virtual const IStructure* GetSysType(SEXY_CLASS_ID id) = 0;
			virtual IAllocatorMap& AllocatorMap() = 0;
			virtual size_t FreeLeakedObjects(IEventCallback<LeakArgs>* leakCallback = nullptr) = 0;
		};

		SEXYUTIL_API const IFunction* GetFunctionForBytecode(IPublicProgramObject& obj, ID_BYTECODE id);
		SEXYUTIL_API bool DoesClassImplementInterface(const IStructure& s, const IInterface& testInterf);

		enum ERRORCODE
		{
			ERRORCODE_NO_ERROR = 0,
			ERRORCODE_NULL_POINTER,
			ERRORCODE_EMPTY_STRING,
			ERRORCODE_BAD_ARGUMENT,
			ERRORCODE_COMPILE_ERRORS,
			ERRORCODE_SEALED,
			ERRORCODE_ALIGN = 0xFFFFFFFF
		};

		class STCException : public IException
		{
		private:
			enum { MAX_MSG_LEN = 256 };
			ERRORCODE code;
			char message[MAX_MSG_LEN];
			char source[MAX_MSG_LEN];

		public:
			STCException(ERRORCODE _code, cstr _source, cstr _msg);

			cstr Source() const
			{
				return source;
			}

			cstr Message() const override
			{
				return message;
			}

			int ErrorCode() const override
			{
				return 0;
			}

			ERRORCODE Code() const
			{
				return code;
			}

			Debugging::IStackFrameEnumerator* StackFrames() override { return nullptr; }
		};

		struct CodeSection
		{
			ID_BYTECODE Id;
			size_t Start;
			size_t End;
		};

		struct SymbolValue
		{
			cstr Text;
			const void* SourceExpression;
		};

		ROCOCO_INTERFACE IFunctionCode
		{
			virtual const IFunction& Owner() const = 0;
			virtual void GetCodeSection(CodeSection& section) const = 0;
			virtual SymbolValue GetSymbol(size_t pcAddressOffset) const = 0;
			virtual const IModule& Module() const = 0;
			virtual int GetLocalVariableSymbolCount() const = 0;
			virtual void GetLocalVariableSymbolByIndex(OUT MemberDef& def, OUT cstr& name, IN int index) const = 0;
			virtual bool GetLocalVariableSymbolByName(OUT MemberDef& def, IN cstr name, IN size_t pcAddress) const = 0;
			virtual int GetOffset(size_t variableIndex) const = 0;
		};

		SEXYUTIL_API const IMember* FindMember(const IStructure& s, cstr name, OUT int& offset);
		SEXYUTIL_API const IStructure* FindMember(const IStructure& s, cstr name);
		SEXYUTIL_API bool GetMethodIndices(OUT int& interfaceIndex, OUT int& methodIndex, const IStructure& s, cstr interfaceName, cstr methodName);
		SEXYUTIL_API bool GetMethodIndices(OUT int& interfaceIndex, OUT int& methodIndex, const IStructure& s, cstr methodName);
		SEXYUTIL_API cstr GetFriendlyName(const IStructure& s);

		SEXYUTIL_API BITCOUNT GetBitCount(VARTYPE type);
	}
}

#endif