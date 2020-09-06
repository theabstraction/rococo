
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
#ifndef SEXY_SCRIPT_H
#define SEXY_SCRIPT_H

#include "sexy.types.h" 
#include "sexy.compiler.public.h" 
#include "sexy.debug.types.h" 

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

// #define SEXY_ENABLE_EXTRA_STRING_SECURITY // uncomment to add in extra security code, which slows some strings ops down a bit

namespace Rococo
{
	struct ILog;

	namespace Compiler
	{
		struct IProgramObject;
		struct VirtualTable;
		struct IStructure;
		struct IPublicProgramObject;
		struct IFunction;
	}

	namespace Sex
	{
		struct ISParser;
		struct ISParserTree;
		struct ISExpression;
	}

	namespace VM
	{
		struct CPU;
	}
}

namespace Rococo {
	namespace Script
	{
		ROCOCOAPI IFreeable
		{
			virtual void Free() = 0;
		};

		using namespace Rococo::Compiler;

		struct IPublicScriptSystem;
		struct ArchetypeCallback;

		struct NativeCallEnvironment
		{
			IPublicScriptSystem& ss;
			const Compiler::IFunction& function;
			const  Compiler::IFunctionCode& code;
			VM::CPU& cpu;
			void* context;

			NativeCallEnvironment(IPublicScriptSystem& _ss, const  Compiler::IFunction& _function, VM::CPU& _cpu, void* _context);
		};

		typedef void (CALLTYPE_C *FN_NATIVE_CALL)(NativeCallEnvironment& e);

		void ThrowBadNativeArg(int index, cstr source, cstr message);

		const Rococo::Sex::ISExpression* GetSourceExpression(Rococo::Compiler::IPublicProgramObject& po, const Rococo::Compiler::IFunction& f, size_t pcOffset);

#ifdef _DEBUG
		inline void ValidateOutputIndex(int index, const Compiler::IFunctionCode& code)
		{
			if (index < 0 || index >= code.Owner().NumberOfOutputs())
			{
				ThrowBadNativeArg(index, code.Owner().Name(), ("Bad output argument"));
			}
		}

		inline void ValidateInputIndex(int index, const Compiler::IFunctionCode& code)
		{
			if (index < 0 || index >= code.Owner().NumberOfInputs())
			{
				ThrowBadNativeArg(index, code.Owner().Name(), ("Bad input argument"));
			}
		}
#else
		inline void ValidateOutputIndex(int index, const Compiler::IFunctionCode& code) {}
		inline void ValidateInputIndex(int index, const Compiler::IFunctionCode& code) {}
#endif

		template<class T> void ReadInput(T& value, const uint8 *SF, ptrdiff_t offset)
		{
			const uint8* inputStart = SF + offset;
			uint8* readPos = (uint8*)inputStart;
			T* pValue = (T*)readPos;
			value = *pValue;
		}

		template<class T>void WriteOutput(T value, const uint8 *SF, ptrdiff_t offset)
		{
			const uint8* outputStart = SF + offset;
			uint8* writePos = (uint8*)outputStart;
			T* pValue = (T*)writePos;
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
			ReadInput(value, po.VirtualMachine().Cpu().SF(), offset);
		}

		template<class T> void ReadInput(int index, T& value, NativeCallEnvironment& e)
		{
			ValidateInputIndex(index, e.code);
			int offset = e.code.GetOffset(index + e.function.NumberOfOutputs());
			ReadInput(value, e.cpu.SF(), offset);
		}

		struct IScriptSystem;

#pragma pack(push,1)
		struct CClassExpression
		{
			ObjectStub Header;
			Sex::ISExpression* ExpressionPtr;
		};

		struct CClassExpressionBuilder
		{
			ObjectStub Header;
			Sex::ISExpressionBuilder* BuilderPtr;
		};

		struct CClassSysTypeStringBuilder
		{
			ObjectStub header;
			int32 length;
			char* buffer;
			int32 capacity;
		};

		struct CScriptSystemClass
		{
			ObjectStub header;
		};

		struct CReflectedClass
		{
			ObjectStub header;
			void* context;
		};
#pragma pack(pop)

		enum ENUM_REPRESENT
		{
			ENUM_REPRESENT_BREAK,
			ENUM_REPRESENT_CONTINUE,
			ENUM_REPRESENT_DELETE
		};

		ROCOCOAPI IRepresentationEnumeratorCallback
		{
			virtual ENUM_REPRESENT OnRepresentation(CReflectedClass* rep) = 0;
		};

		struct SubPackageData
		{
			int64 filesize;
			U8FilePath name;
		};

		ROCOCOAPI IPackage
		{
			virtual cstr UniqueName() const = 0;
			virtual void AppendPaths(const char* a, const char* b, U8FilePath & ab) = 0;
			virtual Sex::ISourceCode* LoadFile(const char* resourcePath) = 0;
			virtual void GetFile(const char* resourcePath, int index, SubPackageData& pkg) const = 0;
			virtual void GetDirectory(const char* resourcePath, int index, SubPackageData& pkg) const = 0;
			virtual int CountDirectories(const char* resourcePath) const = 0;
			virtual int CountFiles(const char* resourcePath) const = 0;
		};

		ROCOCOAPI IPublicScriptSystem : public IFreeable
		{
			virtual void AddCommonSource(const char* dynamicLinkLibOfNativeCalls) = 0;
			virtual void AddNativeCall(const Compiler::INamespace& ns, FN_NATIVE_CALL callback, void* context, cstr archetype, bool checkName = true, int popBytes = 0) = 0; // Example: AddNativeCall(ns, ANON::CpuHz, NULL, "CpuHz -> (Int64 hz)");
			virtual const Compiler::INamespace& AddNativeNamespace(cstr name) = 0;
			virtual void AddNativeLibrary(const char *sexyLibraryFile) = 0;
			virtual void LoadPackage_SXYandDLLs(IPackage& loader) = 0;
			virtual Compiler::IModule* AddTree(Sex::ISParserTree& tree) = 0;
			virtual void Compile() = 0;
			virtual void DispatchToSexyClosure(void* pArgBuffer, const ArchetypeCallback& target) = 0;
			virtual cstr GetSymbol(const void* ptr) const = 0;
			virtual Compiler::IPublicProgramObject& PublicProgramObject() = 0;
			virtual Sex::ISParser& SParser() = 0;
			virtual void ReleaseTree(Sex::ISParserTree* tree) = 0;
			virtual void ThrowNative(int errorNumber, cstr source, cstr message) = 0;
			virtual Sex::ISParserTree* GetSourceCode(const Compiler::IModule& module) const = 0;
			virtual void ThrowFromNativeCode(int32 errorCode, cstr staticRefMessage) = 0;
			virtual int32 GetIntrinsicModuleCount() const = 0;
			virtual bool ValidateMemory() = 0;
			virtual void SetGlobalVariablesToDefaults() = 0;
			virtual const IStructure* GetStringBuilderType() const = 0;

			virtual void CancelRepresentation(void* pSourceInstance) = 0;
			virtual void EnumRepresentations(IRepresentationEnumeratorCallback& callback) = 0;
			virtual CReflectedClass* GetRepresentation(void* pSourceInstance) = 0;
			virtual CReflectedClass* Represent(const Rococo::Compiler::IStructure& st, void* pSourceInstance) = 0;
		};

		ROCOCOAPI IScriptSystemFactory : public IFreeable
		{
			virtual IPublicScriptSystem* CreateScriptSystem(const Rococo::Compiler::ProgramInitParameters& pip, ILog& logger) = 0;
		};

#pragma pack(push,1)
		struct CStringConstant
		{
			ObjectStub header;
			int32 length;
			cstr pointer;
			void* srcExpression;
		};
#pragma pack(pop)

		struct ScriptCallbacks
		{
			ID_API_CALLBACK idThrowNullRef;
			ID_API_CALLBACK idTestD4neqD5_retBoolD7;
			ID_API_CALLBACK idYieldMicroseconds;
			ID_API_CALLBACK idDynamicDispatch;
			ID_API_CALLBACK idIsSameObject;
			ID_API_CALLBACK idIsDifferentObject;
			ID_API_CALLBACK idStringIndexToChar;
		};

		struct MethodInfo
		{
			const Rococo::Compiler::IFunction* f;
			ptrdiff_t offset;
		};

		ROCOCOAPI IScriptSystem : IPublicScriptSystem
		{
			virtual Compiler::IProgramObject& ProgramObject() = 0;

			virtual const CClassExpression* GetExpressionReflection(const Sex::ISExpression& s) = 0;
			virtual CStringConstant* GetStringReflection(cstr s) = 0;
			virtual CScriptSystemClass* GetScriptSystemClass() = 0;
			virtual CReflectedClass* GetReflectedClass(void* ptr) = 0;
			virtual CReflectedClass* CreateReflectionClass(cstr className, void* context) = 0;
			virtual bool ConstructExpressionBuilder(CClassExpressionBuilder& builderContainer, Rococo::Sex::ISExpressionBuilder* builder) = 0;
			virtual const void* GetMethodMap() = 0;
			virtual void* AlignedMalloc(int32 alignment, int32 capacity) = 0;
			virtual void AlignedFree(void* buffer) = 0;
			virtual int NextID() = 0;
			virtual const ScriptCallbacks& GetScriptCallbacks() = 0;
			virtual cstr GetPersistentString(cstr txt) = 0;
			virtual const MethodInfo GetMethodByName(cstr methodName,  const Rococo::Compiler::IStructure& concreteClassType) = 0;

			virtual ID_API_CALLBACK GetIdSerializeCallback() const = 0;
		};

		void SetDefaultNativeSourcePath(const wchar_t* pathname);

		ROCOCOAPI INativeLib
		{
			virtual void AddNativeCalls() = 0;
			virtual void ClearResources() = 0;
			virtual void Release() = 0;
		};

		typedef INativeLib* (*FN_CreateLib)(Rococo::Script::IScriptSystem& ss);

		ROCOCOAPI MemberEnumeratorCallback
		{
			virtual void OnMember(IPublicScriptSystem& ss, cstr childName, const Rococo::Compiler::IMember& member, const uint8* sfItem, int offset, int recurseDepth) = 0;
		};

		// Debugging Helpers API
		SCRIPTEXPORT_API void EnumerateRegisters(Rococo::VM::CPU& cpu, Rococo::Debugger::IRegisterEnumerationCallback& cb);
		SCRIPTEXPORT_API const Rococo::Sex::ISExpression* GetSexSymbol(VM::CPU& cpu, const uint8* pcAddress, Rococo::Script::IPublicScriptSystem& ss);
		SCRIPTEXPORT_API const Rococo::Compiler::IFunction* GetFunctionFromBytecode(const Rococo::Compiler::IModule& module, Rococo::ID_BYTECODE id);
		SCRIPTEXPORT_API const Rococo::Compiler::IFunction* GetFunctionFromBytecode(Rococo::Compiler::IPublicProgramObject& obj, Rococo::ID_BYTECODE id);
		SCRIPTEXPORT_API const Rococo::Compiler::IFunction* GetFunctionAtAddress(Rococo::Compiler::IPublicProgramObject& po, size_t pcOffset);
		SCRIPTEXPORT_API const uint8* GetCallerSF(Rococo::VM::CPU& cpu, const uint8* sf);
		SCRIPTEXPORT_API const uint8* GetReturnAddress(Rococo::VM::CPU& cpu, const uint8* sf);
		SCRIPTEXPORT_API const uint8* GetPCAddress(Rococo::VM::CPU& cpu, int32 callDepth);
		SCRIPTEXPORT_API const uint8* GetStackFrame(Rococo::VM::CPU& cpu, int32 callDepth);
		SCRIPTEXPORT_API bool GetVariableByIndex(cstr& name, Rococo::Compiler::MemberDef& def, const Rococo::Compiler::IStructure*& pseudoType, const uint8*& SF, IPublicScriptSystem& ss, size_t index, size_t callOffset);
		SCRIPTEXPORT_API bool GetCallDescription(const uint8*& sf, const uint8*& pc, const Rococo::Compiler::IFunction*& f, size_t& fnOffset, IPublicScriptSystem& ss, size_t callDepth, size_t& pcOffset);
		SCRIPTEXPORT_API size_t GetCurrentVariableCount(IPublicScriptSystem& ss, size_t callDepth);
		SCRIPTEXPORT_API void ForeachStackLevel(Rococo::Compiler::IPublicProgramObject& obj, Rococo::Debugger::ICallStackEnumerationCallback& cb);
		SCRIPTEXPORT_API void ForeachVariable(Rococo::Script::IPublicScriptSystem& ss, Rococo::Debugger::IVariableEnumeratorCallback& variableEnum, size_t callOffset);
		SCRIPTEXPORT_API void FormatValue(IPublicScriptSystem& ss, char* buffer, size_t bufferCapacity, VARTYPE type, const void* pVariableData);
		SCRIPTEXPORT_API void SkipJIT(Rococo::Compiler::IPublicProgramObject& po);
		SCRIPTEXPORT_API bool GetMembers(IPublicScriptSystem& ss, const Rococo::Compiler::IStructure& s, cstr parentName, const uint8* instance, ptrdiff_t offset, MemberEnumeratorCallback& enumCallback, int recurseDepth);
		SCRIPTEXPORT_API const Rococo::uint8* GetInstance(const Rococo::Compiler::MemberDef& def, const Rococo::Compiler::IStructure* pseudoType, const uint8* SF);
		SCRIPTEXPORT_API cstr GetShortName(const Rococo::Compiler::IStructure& s);
		SCRIPTEXPORT_API cstr GetInstanceTypeName(const Rococo::Compiler::MemberDef& def, const Rococo::Compiler::IStructure* pseudoType);
		SCRIPTEXPORT_API cstr GetInstanceVarName(cstr name, const Rococo::Compiler::IStructure* pseudoType);
		SCRIPTEXPORT_API bool FindVariableByName(Rococo::Compiler::MemberDef& def, const Rococo::Compiler::IStructure*& pseudoType, const Rococo::uint8*& SF, IPublicScriptSystem& ss, cstr searchName, size_t callOffset);
		SCRIPTEXPORT_API const Rococo::Compiler::IStructure* FindStructure(IPublicScriptSystem& ss, cstr fullyQualifiedName);
}}

namespace Rococo {
   namespace Helpers // Used by Benny Hill to simplify native function integration
   {
      class StringPopulator : public IStringPopulator
      {
         Rococo::Compiler::FastStringBuilder* builder;
      public:
         StringPopulator(Script::NativeCallEnvironment& _nce, Compiler::InterfacePointer pInterface);
         void Populate(cstr text) override;
      };
      const Compiler::IStructure& GetDefaultProxy(cstr fqNS, cstr interfaceName, cstr proxyName, Script::IPublicScriptSystem& ss);
   }
}

namespace Rococo {
   namespace Variants
   {
      bool TryRecast(OUT VariantValue& end, IN const VariantValue& original, VARTYPE orignalType, VARTYPE endType);

      inline VariantValue FromValue(int32 value)
      {
         VariantValue v;
         v.int32Value = value;
         return v;
      }

      inline VariantValue Zero() { return FromValue(0); }
      inline VariantValue ValueTrue() { return FromValue(1); }
      inline VariantValue ValueFalse() { return FromValue(0); }

      inline bool IsAssignableToBoolean(VARTYPE type)
      {
         return type == VARTYPE_Bool;
      }

      VARTYPE GetBestCastType(VARTYPE a, VARTYPE b);

      bool TryRecast(OUT VariantValue& end, IN const VariantValue& original, VARTYPE orignalType, VARTYPE endType);
   }
}


#ifndef THIS_IS_THE_SEXY_CORE_LIBRARY

// Ensure the allocator used for CreateScriptV_1_4_0_0(...) is in scope when you call Sexy_CleanupGlobalSources to clean up global resources
extern "C" SCRIPTEXPORT_API Rococo::Script::IScriptSystemFactory* CreateScriptSystemFactory_1_5_0_0(Rococo::IAllocator& allocator);

namespace Rococo { namespace Script
{
	class CScriptSystemProxy
	{
	private:
		IScriptSystemFactory* factory;
		IPublicScriptSystem* ss;

	public:
		IPublicScriptSystem& operator()() { return *ss; }

		CScriptSystemProxy(const Rococo::Compiler::ProgramInitParameters& pip, ILog& logger, IAllocator& allocator)
		{
			factory = CreateScriptSystemFactory_1_5_0_0(allocator);
			ss = factory->CreateScriptSystem(pip, logger);
		}

		~CScriptSystemProxy()
		{
			if (ss) ss->Free();
			if (factory) factory->Free();
		}
	};	
}}

#endif

#endif // SEXY_SCRIPT_H