
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

	3. THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW. EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM �AS IS� WITHOUT
	WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK AS TO THE QUALITY
	AND PERFORMANCE OF THE PROGRAM IS WITH YOU. SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.

	4. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MODIFIES AND/OR CONVEYS THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR
	DAMAGES, INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR DATA BEING RENDERED
	INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF
	SUCH DAMAGES.

	5. Any program that distributes this software in source or binary form must include an obvious credit to the the language, its copyright holder, and the language homepage. This must be the case in its
	principal credit screen and its principal readme file.
*/
#ifndef SEXY_SCRIPT_H
#define SEXY_SCRIPT_H

#ifndef SEXY_H
# error include "sexy.types.h" before including this file
#endif

#ifndef sexy_compiler_public_h
# error include "sexy.compiler.public.h" before including this file
#endif

#ifndef SEXY_DEBUG_H
# error include "sexy.debug.types.h" before including this file
#endif

#ifndef SCRIPT_IS_LIBRARY
# define SCRIPT_IS_LIBRARY
#endif

#ifdef SCRIPT_IS_LIBRARY
# define SCRIPTEXPORT_API
#else
# ifndef IS_SCRIPT_DLL
#  define SCRIPTEXPORT_API __declspec(dllimport)
# else
#  define SCRIPTEXPORT_API __declspec(dllexport)
# endif
#endif

namespace Sexy
{
	struct ILog;

	namespace Compiler
	{
		struct IProgramObject;
	}

	namespace Sex
	{
		struct ISParser;
		struct ISParserTree;
	}

	namespace VM
	{
		struct CPU;
	}
}

namespace Sexy { namespace Script
{
	struct NO_VTABLE IFreeable
	{
		virtual void Free() = 0;
	};

	struct IPublicScriptSystem;

	struct NativeCallEnvironment
	{
		IPublicScriptSystem& ss;
		const Compiler::IFunction& function;
		const  Compiler::IFunctionCode& code;
		VM::CPU& cpu;
		void* context;

		NativeCallEnvironment(IPublicScriptSystem& _ss, const  Compiler::IFunction& _function, VM::CPU& _cpu, void* _context);
	};

	typedef void (_cdecl *FN_NATIVE_CALL)(NativeCallEnvironment& e);

	void ThrowBadNativeArg(int index, csexstr source, csexstr message);	

#ifdef _DEBUG
	inline void ValidateOutputIndex(int index, const Compiler::IFunctionCode& code)
	{
		if (index < 0 || index >= code.Owner().NumberOfOutputs())
		{
			ThrowBadNativeArg(index, code.Owner().Name(), SEXTEXT("Bad output argument"));
		}
	}

	inline void ValidateInputIndex(int index, const Compiler::IFunctionCode& code)
	{
		if (index < 0 || index >= code.Owner().NumberOfInputs())
		{
			ThrowBadNativeArg(index, code.Owner().Name(), SEXTEXT("Bad input argument"));
		}
	}
#else
	inline void ValidateOutputIndex(int index, const Compiler::IFunctionCode& code) {}
	inline void ValidateInputIndex(int index, const Compiler::IFunctionCode& code) {}
#endif

	template<class T> void ReadInput(T& value, const uint8 *SF, ptrdiff_t offset)
	{
		const uint8* inputStart = SF + offset;
		uint8* readPos = (uint8*) inputStart;
		T* pValue = (T*) readPos;
		value = *pValue;
	}

	template<class T>void WriteOutput(T value, const uint8 *SF, ptrdiff_t offset)
	{
		const uint8* outputStart = SF + offset;
		uint8* writePos = (uint8*) outputStart;
		T* pValue = (T*) writePos;
		*pValue = value;
	}

	template<class T>void WriteOutput(int index, T value, NativeCallEnvironment& e)
	{
		ValidateOutputIndex(index, e.code);
		int offset = e.code.GetOffset(index);
		WriteOutput(value, e.cpu.SF(), offset);
	}

	template<class T> void ReadInput(int index, T& value, Compiler::IPublicProgramObject& po, const Compiler::IFunction& f)
	{
		ValidateInputIndex(index, f.Code());
		int offset = f.Code().GetOffset(index + f.NumberOfOutputs());
		ReadInput(value,po.VirtualMachine().Cpu().SF(), offset);
	}

	template<class T> void ReadInput(int index, T& value, NativeCallEnvironment& e)
	{
		ValidateInputIndex(index, e.code);
		int offset = e.code.GetOffset(index + e.function.NumberOfOutputs());
		ReadInput(value, e.cpu.SF(), offset);
	}

	struct ScriptException: public Rococo::IException
	{
		wchar_t message[256];
		int errNumber;

		virtual int ErrorCode() const { return errNumber; }
		virtual const wchar_t* Message() const { return message; } 
	};

#pragma pack(push,1)
	struct CClassDesc
	{
		ptrdiff_t instanceOffset; // should be zero
		ID_BYTECODE destructorId;
		Compiler::IStructure* structDef;
	};


	struct CClassHeader // TODO -> merge with ObjectStub
	{
		CClassDesc* _typeInfo;
		int32 _allocSize;
		VTABLEDEF _vTables[1];
	};

#pragma pack(pop)

	struct IScriptSystem;

#pragma pack(push,1)
	struct CClassExpression
	{
		CClassHeader Header;
		Sex::ISExpression* ExpressionPtr;
	};

	struct CClassExpressionBuilder
	{
		CClassHeader Header;
		Sex::ISExpressionBuilder* BuilderPtr;
	};

	struct CStringConstant
	{
		CClassHeader header;
		int32 length;
		csexstr pointer;
		void* srcExpression;
	};

	struct CScriptSystemClass
	{
		CClassHeader header;
	};

	struct CReflectedClass
	{
		CClassHeader header;
		void* context;
	};
#pragma pack(pop)

	enum ENUM_REPRESENT
	{
		ENUM_REPRESENT_BREAK,
		ENUM_REPRESENT_CONTINUE,
		ENUM_REPRESENT_DELETE
	};

	struct NO_VTABLE IRepresentationEnumeratorCallback
	{
		virtual ENUM_REPRESENT OnRepresentation(CReflectedClass* rep) = 0;
	};

	struct NO_VTABLE IPublicScriptSystem : public IFreeable
	{
		virtual void AddCommonSource(const Sexy::SEXCHAR* dynamicLinkLibOfNativeCalls) = 0;
		virtual void AddNativeCall(const Compiler::INamespace& ns, FN_NATIVE_CALL callback, void* context, csexstr archetype, bool checkName = true) = 0; // Example: AddNativeCall(ns, ANON::CpuHz, NULL, "CpuHz -> (Int64 hz)");
		virtual const Compiler::INamespace& AddNativeNamespace(csexstr name) = 0;
		virtual void AddNativeLibrary(const Sexy::SEXCHAR *sexySourceFile) = 0;
		virtual Compiler::IModule* AddTree(Sex::ISParserTree& tree) = 0;
		virtual void Compile() = 0;
		virtual csexstr GetSymbol(const void* ptr) const = 0;
		virtual Compiler::IPublicProgramObject& PublicProgramObject() = 0;
		virtual Sex::ISParser& SParser() = 0;
		virtual void ReleaseTree(Sex::ISParserTree* tree) = 0;
		virtual void ThrowNative(int errorNumber, csexstr source, csexstr message) = 0;
		virtual Sex::ISParserTree* GetSourceCode(const Compiler::IModule& module) const = 0;
		virtual void ThrowFromNativeCode(int32 errorCode, csexstr staticRefMessage) = 0;
		virtual int32 GetIntrinsicModuleCount() const = 0;
		virtual bool ValidateMemory() = 0;
		virtual void SetGlobalVariablesToDefaults() = 0;

		virtual void CancelRepresentation(void* pSourceInstance) = 0;
		virtual void EnumRepresentations(IRepresentationEnumeratorCallback& callback) = 0;
		virtual CReflectedClass* GetRepresentation(void* pSourceInstance) = 0;
		virtual CReflectedClass* Represent(const Sexy::Compiler::IStructure& st, void* pSourceInstance) = 0;
	};	
		
	struct NO_VTABLE IScriptSystem : IPublicScriptSystem
	{
		virtual Compiler::IProgramObject& ProgramObject() = 0;
		
		virtual const CClassExpression* GetExpressionReflection(const Sex::ISExpression& s) = 0;
		virtual CStringConstant* GetStringReflection(csexstr s) = 0;
		virtual CScriptSystemClass* GetScriptSystemClass() = 0;
		virtual CReflectedClass* GetReflectedClass(void* ptr) = 0;
		virtual CReflectedClass* CreateReflectionClass(csexstr className, void* context) = 0;
		virtual bool ConstructExpressionBuilder(CClassExpressionBuilder& builderContainer, Sexy::Sex::ISExpressionBuilder* builder) = 0;
		virtual const void* GetMethodMap() = 0;
		virtual void* AlignedMalloc(int32 alignment, int32 capacity) = 0;
		virtual void AlignedFree(void* buffer) = 0;
		virtual int NextID() = 0;
	};	

	struct NO_VTABLE INativeLib
	{
		virtual void AddNativeCalls() = 0;
		virtual void ClearResources() = 0;
		virtual void Release() = 0;
	};

	typedef INativeLib* (*FN_CreateLib)(Sexy::Script::IScriptSystem& ss);

	struct NO_VTABLE MemberEnumeratorCallback
	{
		virtual void OnMember(IPublicScriptSystem& ss, csexstr childName, const Sexy::Compiler::IMember& member, const uint8* sfItem) = 0;
	};

	// Debugging Helpers API
	SCRIPTEXPORT_API void EnumerateRegisters(Sexy::VM::CPU& cpu, Sexy::Debugger::IRegisterEnumerationCallback& cb);
	SCRIPTEXPORT_API const Sexy::Sex::ISExpression* GetSexSymbol(VM::CPU& cpu, const uint8* pcAddress, Sexy::Script::IPublicScriptSystem& ss);
	SCRIPTEXPORT_API const Sexy::Compiler::IFunction* GetFunctionFromBytecode(const Sexy::Compiler::IModule& module, Sexy::ID_BYTECODE id);
	SCRIPTEXPORT_API const Sexy::Compiler::IFunction* GetFunctionFromBytecode(Sexy::Compiler::IPublicProgramObject& obj, Sexy::ID_BYTECODE id);
	SCRIPTEXPORT_API const Sexy::Compiler::IFunction* GetFunctionAtAddress(Sexy::Compiler::IPublicProgramObject& po, size_t pcOffset);
	SCRIPTEXPORT_API const uint8* GetCallerSF(Sexy::VM::CPU& cpu, const uint8* sf);
	SCRIPTEXPORT_API const uint8* GetReturnAddress(Sexy::VM::CPU& cpu, const uint8* sf);
	SCRIPTEXPORT_API const uint8* GetPCAddress(Sexy::VM::CPU& cpu, int32 callDepth);
	SCRIPTEXPORT_API const uint8* GetStackFrame(Sexy::VM::CPU& cpu, int32 callDepth);
	SCRIPTEXPORT_API bool GetVariableByIndex(csexstr& name, Sexy::Compiler::MemberDef& def, const Sexy::Compiler::IStructure*& pseudoType, const uint8*& SF, IPublicScriptSystem& ss, size_t index, size_t callOffset);
	SCRIPTEXPORT_API bool GetCallDescription(const uint8*& sf, const uint8*& pc, const Sexy::Compiler::IFunction*& f, size_t& fnOffset, IPublicScriptSystem& ss, size_t callDepth);
	SCRIPTEXPORT_API size_t GetCurrentVariableCount(IPublicScriptSystem& ss, size_t callDepth);
	SCRIPTEXPORT_API void ForeachStackLevel(Sexy::Compiler::IPublicProgramObject& obj, Sexy::Debugger::ICallStackEnumerationCallback& cb);
	SCRIPTEXPORT_API void ForeachVariable(Sexy::Script::IPublicScriptSystem& ss, Sexy::Debugger::IVariableEnumeratorCallback& variableEnum, size_t callOffset);
	SCRIPTEXPORT_API void FormatValue(IPublicScriptSystem& ss, char* buffer, size_t bufferCapacity, VARTYPE type, const void* pVariableData);
	SCRIPTEXPORT_API void SkipJIT(Sexy::Compiler::IPublicProgramObject& po);
	SCRIPTEXPORT_API bool GetMembers(IPublicScriptSystem& ss, const Sexy::Compiler::IStructure& s, csexstr parentName, const uint8* instance, ptrdiff_t offset, MemberEnumeratorCallback& enumCallback);
	SCRIPTEXPORT_API const Sexy::uint8* GetInstance(const Sexy::Compiler::MemberDef& def, const Sexy::Compiler::IStructure* pseudoType, const uint8* SF);
	SCRIPTEXPORT_API csexstr GetShortName(const Sexy::Compiler::IStructure& s);
	SCRIPTEXPORT_API csexstr GetInstanceTypeName(const Sexy::Compiler::MemberDef& def, const Sexy::Compiler::IStructure* pseudoType);
	SCRIPTEXPORT_API csexstr GetInstanceVarName(csexstr name, const Sexy::Compiler::IStructure* pseudoType);
	SCRIPTEXPORT_API bool FindVariableByName(Sexy::Compiler::MemberDef& def, const Sexy::Compiler::IStructure*& pseudoType, const Sexy::uint8*& SF, IPublicScriptSystem& ss, csexstr searchName, size_t callOffset);
	SCRIPTEXPORT_API const Sexy::Compiler::IStructure* FindStructure(IPublicScriptSystem& ss, csexstr fullyQualifiedName);
}}

namespace Sexy { namespace Sex
{
	SCRIPTEXPORT_API void AssertCompound(cr_sex e);
	SCRIPTEXPORT_API void AssertAtomic(cr_sex e);
	SCRIPTEXPORT_API void AssertStringLiteral(cr_sex e);
	SCRIPTEXPORT_API void AssertNotTooManyElements(cr_sex e, int32 maxElements);
	SCRIPTEXPORT_API void AssertNotTooFewElements(cr_sex e, int32 minElements);
	SCRIPTEXPORT_API cr_sex GetAtomicArg(cr_sex e, int argIndex);
	SCRIPTEXPORT_API void Throw(cr_sex e, csexstr message);
}}

#ifndef THIS_IS_THE_SEXY_CORE_LIBRARY

extern "C" SCRIPTEXPORT_API Sexy::Script::IPublicScriptSystem* CreateScriptV_1_1_0_0(const Sexy::ProgramInitParameters& pip, Sexy::ILog& logger);

namespace Sexy { namespace Script
{
	class CScriptSystemProxy
	{
	private:
		IPublicScriptSystem* instance;

	public:
		IPublicScriptSystem& operator()() { return *instance; }

		CScriptSystemProxy(const ProgramInitParameters& pip, ILog& logger)
		{
			instance = CreateScriptV_1_1_0_0(pip, logger);
		}

		~CScriptSystemProxy()
		{
			if (instance) instance->Free();
		}
	};	
}}

#endif

#endif // SEXY_SCRIPT_H