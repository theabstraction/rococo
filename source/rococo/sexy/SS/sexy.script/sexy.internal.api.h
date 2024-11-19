#pragma once

#include <rococo.types.h>

#ifdef _WIN32
# define VM_CALLBACK_CONVENTION _cdecl
#else
# define VM_CALLBACK_CONVENTION
#endif
#define VM_CALLBACK(x) void VM_CALLBACK_CONVENTION OnInvoke##x(VariantValue* registers, void* context)

namespace Rococo
{
	class NamespaceSplitter;
	union VariantValue;
}

namespace Rococo::Compiler
{
	DECLARE_ROCOCO_INTERFACE IStructure;
	DECLARE_ROCOCO_INTERFACE IArchetype;
	DECLARE_ROCOCO_INTERFACE IProgramObject;
	DECLARE_ROCOCO_INTERFACE INamespaceBuilder;
	DECLARE_ROCOCO_INTERFACE IMacroBuilder;
	DECLARE_ROCOCO_INTERFACE IModuleBuilder;
	DECLARE_ROCOCO_INTERFACE IInterfaceBuilder;
	DECLARE_ROCOCO_INTERFACE IFunctionBuilder;
	DECLARE_ROCOCO_INTERFACE IMemberLifeSupervisor;
	DECLARE_ROCOCO_INTERFACE IModule;
}

namespace Rococo::Sex
{
	DECLARE_ROCOCO_INTERFACE ISExpression;
	DECLARE_ROCOCO_INTERFACE ISParserTree;

	typedef const ISExpression& cr_sex;

	void AssertMacroShortName(cr_sex s, cstr shortName);
	void AssertPascalCaseNameValid(cstr text, int length, int maxLength, cstr desc);
	void AssertSplitTail(NamespaceSplitter& splitter, cr_sex s, OUT cstr& body, OUT cstr& tail);
	Compiler::INamespaceBuilder& AssertGetSubspace(Compiler::IProgramObject& object, cr_sex s, cstr name);
	void ThrowTokenAlreadyDefined(cr_sex s, cstr name, cstr repository, cstr type);
	void AssertLocalVariableOrMember(cr_sex e);
	void AssertValidNamespaceDef(cr_sex e);
	void AssertValidArchetypeName(cr_sex src, cstr name);
	void AssertValidInterfaceName(cr_sex src, cstr name);
	void AssertValidStructureName(cr_sex e);
	void AssertValidFunctionName(cr_sex e);
	void AssertKeyword(cr_sex e, int arg, cstr name);
	void AssertCompoundOrNull(cr_sex e);
	void ValidateNumberOfInputArgs(cr_sex s, const Compiler::IArchetype& callee, int numberOfSuppliedInputArgs);
}

namespace Rococo::VM
{
	DECLARE_ROCOCO_INTERFACE ICore;
	DECLARE_ROCOCO_INTERFACE IVirtualMachine;
}

namespace Rococo::Script
{
	struct ArrayImage;
	struct MapImage;
	struct ListImage;
	class CScript;
	class CScripts;
	struct ArrayCallbacks;
	struct ListCallbacks;
	struct MapCallbacks;
	struct ScriptCallbacks;
	
	DECLARE_ROCOCO_INTERFACE IScriptSystem;

	ROCOCO_INTERFACE IExceptionLogic
	{
		virtual void AddCatchHandler(ID_BYTECODE id, size_t start, size_t end, size_t handlerOffset) = 0;
		virtual void Clear() = 0;
		virtual void InstallThrowHandler() = 0;
		virtual void ThrowFromNativeCode(int32 errorCode, cstr format, va_list args) = 0;
	};

	ROCOCO_INTERFACE IScripts
	{
		virtual ~IScripts() = 0;

		virtual void SetGlobalVariablesToDefaults(VM::IVirtualMachine& vm) = 0;
		virtual CScript* CreateModule(Sex::ISParserTree& tree) = 0;
		virtual IExceptionLogic& ExceptionLogic() = 0;
		virtual void ReleaseModule(Sex::ISParserTree& tree) = 0;

		virtual void Clear() = 0;
		virtual void CompileNamespaces() = 0;
		virtual void CompileBytecode() = 0;
		virtual void CompileDeclarations() = 0;
		virtual void CompileTopLevelMacros(size_t numberOfSysModules) = 0;

		virtual void EnterCompileLimits(size_t startingIndex, size_t maxModuleCount) = 0;
		virtual void ReleaseCompileLimits() = 0;

		virtual Sex::ISParserTree* GetSourceCode(const Compiler::IModule& module) = 0;

	};

	void AddArchiveRegister(CCompileEnvironment& ce, int saveTempDepth, int restoreTempDepth, BITCOUNT bits);
	void AlignedMemcpy(void* __restrict dest, const void* __restrict source, size_t nBytes);
	int AllocFunctionOutput(CCompileEnvironment& ce, const Compiler::IArchetype& callee, Sex::cr_sex sExpr);
	Compiler::IMacroBuilder* DeclareMacro(Sex::cr_sex macroDef, Compiler::IModuleBuilder& module);
	void DestroyElements(ArrayImage& a, IScriptSystem& ss);
	void DestroyObject(const Compiler::IStructure& type, uint8* item, IScriptSystem& ss);
	void IncrementRef(MapImage* mapImage);
	bool IsGetAccessor(const Compiler::IArchetype& callee);
	void ListRelease(ListImage* l, IScriptSystem& ss);
	bool TryCompileStringLiteralInputToTemp(CCompileEnvironment& ce, Sex::cr_sex s, int tempDepth, const Compiler::IStructure& inputType);
	void ValidateReturnType(Sex::cr_sex s, VARTYPE returnType, VARTYPE type);
	bool TryCompileMethodCallWithoutInputAndReturnValue(CCompileEnvironment& ce, Sex::cr_sex s, cstr instance, cstr methodName, VARTYPE returnType, const Compiler::IStructure* returnTypeStruct, const Compiler::IArchetype* returnArchetype);

	class CCompileEnvironment;
	int CompileInstancePointerArgFromTemp(CCompileEnvironment& ce, int tempDepth);

	void CompileMacroFromExpression(Compiler::IMacroBuilder& macro, CScript& script, Sex::cr_sex macroDef);
	void CompileGetStructRef(CCompileEnvironment& ce, Sex::cr_sex s, const Compiler::IStructure& inputType, cstr name);
	VARTYPE CompileMethodCallWithoutInputAndReturnNumericValue(CCompileEnvironment& ce, Sex::cr_sex s, cstr instance, cstr methodName);

	void ReturnOutput(CCompileEnvironment& ce, int outputOffset, VARTYPE returnType);
	Compiler::IInterfaceBuilder* GetInterfaceFQN(Sex::cr_sex baseExpr, CScript& script);
	void CompileClosureBody(Sex::cr_sex closureDef, Compiler::IFunctionBuilder& closure, CScript& script);

	Compiler::IMemberLifeSupervisor* CreateArrayLifetimeManager(IScriptSystem& ss);
	Compiler::IMemberLifeSupervisor* CreateListLifetimeManager(IScriptSystem& ss);
	Compiler::IMemberLifeSupervisor* CreateMapLifetimeManager(IScriptSystem& ss);

	void CALLTYPE_C Compile_JIT(VariantValue* registers, void* context);

	void RegisterArrays(ArrayCallbacks& callbacks, VM::ICore& core, IScriptSystem& ss);
	void RegisterLists(ListCallbacks& callbacks, VM::ICore& core, IScriptSystem& ss);
	void RegisterMaps(MapCallbacks& callbacks, VM::ICore& core, IScriptSystem& ss);
	void RegisterMiscAPI(ScriptCallbacks& callbacks, VM::ICore& core, IScriptSystem& ss);

	IScripts* NewCScripts(Compiler::IProgramObject& _programObject, IScriptSystem& _system);
	void Delete(IScripts* scripts);

	ArrayImage* CreateArrayImage(IScriptSystem& ss, const Compiler::IStructure& elementType, int32 capacity);
	ListImage* CreateListImage(IScriptSystem& ss, const Compiler::IStructure& valueType);
	MapImage* CreateMapImage(IScriptSystem& ss, const Compiler::IStructure& keyType, const Compiler::IStructure& valueType);

	uint8* AppendListNode(IScriptSystem& ss, ListImage& l);

	int GetInterfaceImplementingMethod(const Compiler::IStructure& s, cstr methodName);

	template<uint32 capacity>
	struct Memo
	{
		enum { CAPACITY = capacity };
		char Data[CAPACITY];
	};

	template<class T>
	class TFastAllocator
	{
	private:
		typedef TSexyVector<T*> TItems;
		TItems items;
		TItems freeItems;

	public:
		~TFastAllocator()
		{
			clear();
		}

		DEFINE_SEXY_ALLOCATORS_FOR_CLASS;

		void clear()
		{
			for (auto i = items.begin(); i != items.end(); ++i)
			{
				Rococo::Memory::FreeSexyUnknownMemory(*i);
			}

			items.clear();

			for (auto i = freeItems.begin(); i != freeItems.end(); ++i)
			{
				Rococo::Memory::FreeSexyUnknownMemory(*i);
			}

			freeItems.clear();
		}

		T* alloc()
		{
			T* item;

			if (freeItems.empty())
			{
				item = (T*)Rococo::Memory::AllocateSexyMemory(sizeof T);
			}
			else
			{
				item = freeItems.back();
				items.push_back(item);
				freeItems.pop_back();
			}

			return item;
		}

		void free(T* item)
		{
			freeItems.push_back(item);
		}
	};

	typedef Memo<128> TMemo;

	typedef TFastAllocator<TMemo> TMemoAllocator;

	void DefineSysNative(const Compiler::INamespace& sysNative, IScriptSystem& ss, IStringPool* stringPool, TMemoAllocator& memoAllocator);
	void DefineSysTypeStrings(const Compiler::INamespace& sysTypeStrings, IScriptSystem& ss);
}