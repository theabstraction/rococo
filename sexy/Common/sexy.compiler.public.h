/*
	Sexy Scripting Language - Copright(c)2013. Mark Anthony Taylor. All rights reserved.

	http://www.sexiestengine.com

	Email: mark.anthony.taylor@gmail.com

	The Sexy copyright holder is Mark Anthony Taylor, the English author of 'Lazy Bloke Ghosts of Parliament', 'Lazy Bloke in the 35th Century', 'King of the Republic', 'Origin of the Species' 
	and 'Society of Demons.'

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

namespace Sexy { namespace VM
{
	struct IVirtualMachine;
	struct IAssembler;
	struct IProgramMemory;
}} // Sexy::VM

namespace Sexy { namespace Compiler
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
		csexstr Name;
		bool IsMethod;
		FunctionPrototype(csexstr _name, bool _isMethod): Name(_name), IsMethod(_isMethod) {}
	};

	class NameString
	{
	private:
		csexstr s;
		NameString(csexstr _s): s(_s) {}
	public:
		static NameString From(csexstr s) {	return NameString(s);	}
		static NameString From(sexstring s) {	return NameString(s->Buffer);	}
		csexstr c_str() const { return s; }
	};

	class TypeString
	{
	private:
		csexstr s;
		TypeString(csexstr _s): s(_s) {}
	public:
		static TypeString From(csexstr s) {	return TypeString(s);	}
		static TypeString From(sexstring s) {	return TypeString(s->Buffer);	}
		csexstr c_str() const { return s; }
	};

	inline bool IsPrimitiveType(VARTYPE v)
	{
		return v != VARTYPE_Bad && v != VARTYPE_Derivative && v != VARTYPE_Closure;
	}

	inline bool IsNumericType(VARTYPE v)
	{
		return v == VARTYPE_Int32 || v == VARTYPE_Int64 || v ==  VARTYPE_Float32 || v == VARTYPE_Float64;
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
		INSTANCEALIGN_1					= 1,
		INSTANCEALIGN_2					= 2,
		INSTANCEALIGNALIGN_4		= 4,
		INSTANCEALIGN_8					= 8,
		INSTANCEALIGN_16				= 16,
		INSTANCEALIGN_SSE				= 16, // Used for aligning SSEx data
		INSTANCEALIGN_128				= 128,
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

		StructurePrototype(MEMBERALIGN _memberAlign, INSTANCEALIGN _instanceAlign, bool _isFixedSequence, const IArchetype* _archeType, bool _isClass):
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

	// TODO -> merge with CClassHeader

	struct ObjectDesc
	{
		int32 Zero; // Should be zero, gives the number of bytes to the start of the object, which is this stub
		ID_BYTECODE DestructorId;
		IStructure* TypeInfo;
	};

	struct VirtualTable
	{
		ptrdiff_t OffsetToInstance;
		ID_BYTECODE FirstMethodId;
	};

#pragma pack(push, 1)
	struct ObjectStub
	{
		ObjectDesc* Desc;
		int32 AllocSize;
		VirtualTable* pVTable1;
	};
#pragma pack(pop)

	inline int GetInstanceToInterfaceOffset(int interfaceIndex) // TODO -> make all vtable calculations use this function where appropriate
	{
		int stubsize = sizeof(ObjectStub);
		int ptrsize = sizeof(VirtualTable*);
		return stubsize - ptrsize + interfaceIndex * ptrsize;
	}

	struct InlineString
	{
		ObjectStub stub;
		int32 length;
		csexstr buffer;
	};

	inline uint8* GetInterfacePtr(const ObjectStub& stub)
	{
		return (uint8*) &stub.pVTable1;
	}

	struct NO_VTABLE IArchetype
	{
		virtual csexstr Name() const = 0;
		virtual const int NumberOfOutputs() const = 0;
		virtual const int NumberOfInputs() const = 0;
		virtual const IStructure& GetArgument(int index) const = 0;
		virtual csexstr GetArgName(int index) const = 0;
		virtual const bool IsVirtualMethod() const = 0;
		virtual const IStructure* GetGenericArg1(int index) const = 0;
		virtual const void* Definition() const = 0;
	};

	struct NO_VTABLE IArgument
	{
		virtual csexstr Name() const = 0;
		virtual csexstr TypeString() const = 0;
		virtual const ARGUMENTUSAGE Usage() const = 0;
		virtual const ARGDIRECTION Direction() const = 0;
		virtual const IStructure* ResolvedType() const = 0;
		virtual const IStructure* GenericTypeArg1() const = 0;
		virtual const IStructure* GenericTypeArg2() const = 0;
		virtual bool IsClosureInput() const = 0;
		virtual const IFunction& Parent() const = 0;
		virtual void* Userdata() const = 0;
	};

	struct NO_VTABLE IFunction : public IArchetype
	{
		virtual const IModule& Module() const = 0;
		virtual IPublicProgramObject& Object() const = 0;
		virtual const IFunctionCode& Code() const = 0;
		virtual const IFunction* Parent() const = 0;
		virtual const IArgument& Arg(int index) const = 0;	
		virtual const IArgument* GetArgumentByName(csexstr name) const = 0;	
		virtual const IStructure* GetType() const = 0;
	};

	inline int ArgCount(const IArchetype& archetype) { return archetype.NumberOfInputs() + archetype.NumberOfOutputs(); }

	struct NO_VTABLE IFunctionAlias
	{
		virtual const IFunction& GetFunction() const = 0;
		virtual csexstr GetPublicName() const = 0;
	};

	struct NO_VTABLE IStructAlias
	{
		virtual const IStructure& GetStructure() const = 0;
		virtual csexstr GetPublicName() const = 0;
	};

	struct NO_VTABLE IFunctionEnumerator
	{
		virtual int FunctionCount() const = 0;
		
		virtual const IFunctionAlias& operator[](int index) const = 0;
	};

	const IFunction* FindByName(const IFunctionEnumerator& e, csexstr publicName);

	struct NO_VTABLE IMember
	{
		virtual csexstr Name() const = 0;
		virtual const int SizeOfMember() const = 0;
		virtual const bool IsResolved() const = 0;		
		virtual const IStructure* UnderlyingType() const = 0;
		virtual const IStructure* UnderlyingGenericArg1Type() const = 0;
		virtual const IStructure* UnderlyingGenericArg2Type() const = 0;
		virtual bool IsPseudoVariable() const = 0;
	};

	struct NO_VTABLE  IStructure
	{
		virtual IPublicProgramObject& Object() const = 0;
		virtual csexstr Name() const = 0;
		virtual const StructurePrototype& Prototype() const = 0;
		virtual const IModule& Module() const = 0;
		
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
		virtual const void* Definition() const = 0;
		virtual const IFunction* Constructor() const = 0;
	};

	inline bool operator == (const IStructure& a, const IStructure& b) { return &a == &b; } 
	inline bool operator != (const IStructure& a, const IStructure& b) { return &a != &b; } 

	struct NO_VTABLE  IModule
	{
		virtual int GetVersion() const = 0;
		
		virtual const IFunction* FindFunction(csexstr name) const = 0;
		virtual const IStructure* FindStructure(csexstr name) const = 0;

		virtual csexstr Name() const = 0;
		virtual IPublicProgramObject& Object() const = 0;
		
		virtual const IFunction& GetFunction(int index) const = 0;
		virtual const IStructure& GetStructure(int index) const = 0;
		
		virtual int FunctionCount() const = 0;
		virtual int StructCount() const = 0;
		virtual int PrefixCount() const = 0;
		virtual const INamespace& GetPrefix(int index) const = 0;
	};

	bool IsNullType(const IStructure& s);

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
		ADDNAMESPACEFLAGS_NORMAL	   = 0x00000000,
		ADDNAMESPACEFLAGS_CREATE_ROOTS = 0x00000001
	};

	struct NO_VTABLE  IAttributes
	{
		virtual bool AddAttribute(csexstr name, const void* value) = 0;
		virtual const int AttributeCount() const = 0;
		virtual const bool FindAttribute(csexstr name, OUT const void*& value) const = 0;		
		virtual csexstr GetAttribute(int index, OUT const void*& value) const = 0;	
	};

	struct NO_VTABLE IInterface
	{
		virtual const IAttributes& Attributes() const = 0;
		virtual const IInterface* Base() const = 0;
		virtual const IArchetype& GetMethod(int index) const = 0;
		virtual const int MethodCount() const = 0;
		virtual csexstr Name() const = 0;
		virtual const IStructure& NullObjectType() const = 0;
		virtual ObjectStub* UniversalNullInstance() const = 0;
	};

	inline bool operator == (const IInterface& a, const IInterface& b)
	{
		return &a == &b;
	}

	struct NO_VTABLE IFactory
	{
		virtual csexstr Name() const = 0;
		virtual const IFunction& Constructor() const = 0;		
		virtual const IInterface& ThisInterface() const = 0;
		virtual sexstring InterfaceType() const = 0;
		virtual const IFunction* InlineConstructor() const = 0; // if not NULL indicates the factory trivially calls a constructor with matching parameters
		virtual const IStructure* InlineClass() const = 0; // if not NULL indicates the concrete class of the inline constructor
		
	};

	struct NO_VTABLE IMacro
	{
		virtual csexstr Name() const = 0;
		virtual const void* Expression() const = 0;
		virtual const INamespace& NS() const = 0;
		virtual const IFunction& Implementation() const = 0;
	};

	struct NO_VTABLE INamespace
	{	
		virtual const INamespace* FindSubspace(csexstr subpath) const = 0;
		virtual const IStructure* FindStructure(csexstr name) const = 0;
		virtual const IFunction* FindFunction(csexstr name) const = 0;
		virtual const IArchetype* FindArchetype(csexstr name) const = 0;
		
		virtual const IFactory* FindFactory(csexstr name) const = 0;
		
		virtual const IInterface* FindInterface(csexstr name) const = 0;
		virtual const IMacro* FindMacro(csexstr name) const = 0;
				
		virtual size_t ChildCount() const = 0;
		virtual const sexstring FullName() const = 0;
		virtual const sexstring Name() const = 0;
		
		virtual IPublicProgramObject& Object() const = 0;
			
		virtual int InterfaceCount() const = 0;		
		virtual const IInterface& GetInterface(int index) const = 0;

		virtual void EnumerateFactories(ICallback<const IFactory, csexstr>& onFactory) const = 0;
		virtual void EnumerateStrutures(ICallback<const IStructure, csexstr>& onStructure) const = 0;
		virtual void EnumerateFunctions(ICallback<const IFunction, csexstr>& onFunction) const = 0;
		virtual void EnumerateArchetypes(ICallback<const IArchetype>& onArchetype) const = 0;
	};

	struct NO_VTABLE IPublicProgramObject
	{
		virtual const IModule& GetModule(int index) const = 0;
		virtual const INamespace& GetRootNamespace() const = 0;
		virtual int ModuleCount() const = 0;
		virtual const IModule& IntrinsicModule() const = 0;

		virtual Sexy::VM::IVirtualMachine& VirtualMachine() = 0;
		virtual Sexy::VM::IProgramMemory& ProgramMemory() = 0;
		virtual const Sexy::VM::IProgramMemory& ProgramMemory() const = 0;
		virtual ILog& Log() = 0;
		virtual void Free() = 0;
		virtual void SetProgramAndEntryPoint(const IFunction& f) = 0;
	};

	const IFunction* GetFunctionForBytecode(IPublicProgramObject& obj, ID_BYTECODE id);
	bool DoesClassImplementInterface(const IStructure& s, const IInterface& testInterf);
	
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

	class STCException: public IException
	{
	private:
		enum {MAX_MSG_LEN = 256};
		ERRORCODE code;
		wchar_t message[MAX_MSG_LEN];
		SEXCHAR source[MAX_MSG_LEN];

	public:
		STCException(ERRORCODE _code, csexstr _source, csexstr _msg): code(_code)
		{
			CopyString(message, MAX_MSG_LEN, _msg, -1);
			CopyString(source, MAX_MSG_LEN, _source, -1);
		}

		virtual csexstr Source() const
		{
			return source;
		}
	
		virtual const wchar_t* Message() const
		{
			return message;
		}

		virtual int ErrorCode() const
		{
			return -1;
		}

		virtual ERRORCODE Code() const
		{
			return code;
		}
	};

	struct CodeSection
	{
		ID_BYTECODE Id;
		size_t Start;
		size_t End;
	};

	struct SymbolValue
	{
		csexstr Text;
		const void* SourceExpression;
	};

	struct NO_VTABLE IFunctionCode
	{
		virtual const IFunction& Owner() const = 0;
		virtual void GetCodeSection(CodeSection& section) const = 0;
		virtual SymbolValue GetSymbol(size_t pcAddressOffset) const = 0;
		virtual const IModule& Module() const = 0;
		virtual int GetLocalVariableSymbolCount() const = 0;
		virtual void GetLocalVariableSymbolByIndex(OUT MemberDef& def, OUT csexstr& name, IN int index) const = 0;
		virtual bool GetLocalVariableSymbolByName(OUT MemberDef& def, IN csexstr name, IN size_t pcAddress) const = 0;		
		virtual int GetOffset(size_t variableIndex) const = 0;
	};

	const IMember* FindMember(const IStructure& s, csexstr name, OUT int& offset);
	const IStructure* FindMember(const IStructure& s, csexstr name);
	bool GetMethodIndices(OUT int& interfaceIndex, OUT int& methodIndex, const IStructure& s, csexstr interfaceName, csexstr methodName);
	bool GetMethodIndices(OUT int& interfaceIndex, OUT int& methodIndex, const IStructure& s, csexstr methodName);
	csexstr GetFriendlyName(const IStructure& s);

	BITCOUNT GetBitCount(VARTYPE type);
}}

#endif