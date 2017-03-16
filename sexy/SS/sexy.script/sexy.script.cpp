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

#include "sexy.script.stdafx.h"
#include "sexy.script.h"
#include "..\STC\stccore\Sexy.Compiler.h"
#include "..\STC\stccore\sexy.compiler.helpers.h"
#include "Sexy.S-Parser.h"
#include "Sexy.VM.h"
#include "Sexy.VM.CPU.h"

#include <stdarg.h>
#include <algorithm>
#include <unordered_map>
#include <sexy.stdstrings.h>

using namespace Sexy;
using namespace Sexy::Script;
using namespace Sexy::Compiler;
using namespace Sexy::Sex;
using namespace Sexy::VM;

namespace
{
	class CScript;
	class CScripts;
}

namespace
{
   SEXCHAR defaultNativeSourcePath[256] = { 0 };

   // Careful that we had enough buffer space
   void AddSlashToDirectory(SEXCHAR* buffer)
   {
      // Terminate with slash

      SEXCHAR* s = buffer;
      for (; *s != 0; s++)
      {

      }

      if (s[-1] != '\\' && s[-1] != '/')
      {
         s[0] = OS_DIRECTORY_SLASH;
         s[1] = 0;
      }
   }
}

namespace Sexy
{
   namespace Script
   {
      void SetDefaultNativeSourcePath(csexstr pathname)
      {
         if (pathname == nullptr)
         {
            defaultNativeSourcePath[0] = 0;
            return;
         }

         SafeCopy(defaultNativeSourcePath, 255, pathname, _TRUNCATE);

         // Terminate with slash
         AddSlashToDirectory(defaultNativeSourcePath);
      }
   }
}

namespace Sexy
{
	namespace OS
	{
		FN_CreateLib GetLibCreateFunction(const SEXCHAR* dynamicLinkLibOfNativeCalls, bool throwOnError);
	}

	namespace Script
	{
		typedef std::unordered_map<CStringKey,csexstr,::hashCStringKey> TMapMethodToMember;
	}

	namespace Compiler
	{
		INamespaceBuilder* MatchNamespace(IModuleBuilder& module, csexstr name);
		IStructureBuilder* MatchStructure(ILog& logger, csexstr type, IModuleBuilder& module);
	}
}

#define VM_CALLBACK_CONVENTION _cdecl
#define VM_CALLBACK(x) void VM_CALLBACK_CONVENTION OnInvoke##x(VariantValue* registers, void* context)

namespace
{
	GlobalValue* GetGlobalValue(CScript& script, csexstr buffer);

	class CCompileEnvironment
	{
	private:
		const IStructure* arrayStruct;
		const IStructure* listStruct;
		const IStructure* mapStruct;
		const TMapMethodToMember& methodMap;		

	public: // member variables made public to enhance speed in debug mode
		CScript& Script;
		ICodeBuilder& Builder;
		INamespaceBuilder& RootNS;
		IScriptSystem& SS;
		IProgramObject& Object;
		
		CCompileEnvironment(CScript& script, ICodeBuilder& builder);

		const IStructure& StructArray();
		const IStructure& StructList();
		const IStructure& StructMap();

		csexstr MapMethodToMember(csexstr method)
		{
			auto i = methodMap.find(CStringKey(method));
			return i == methodMap.end() ? NULL : i->second;
		}
	};

	struct ArrayDef
	{
		cr_sex SexDef;
		const IStructure& ElementType;

		ArrayDef(cr_sex sexDef, const IStructure& elementType): SexDef(sexDef), ElementType(elementType) {}
	};

	struct ListDef
	{
		cr_sex SexDef;
		const IStructure& ElementType;

		ListDef(cr_sex sexDef, const IStructure& elementType): SexDef(sexDef), ElementType(elementType) {}
	};

	struct MapDef
	{
		cr_sex SexDef;
		const IStructure& KeyType;
		const IStructure& ValueType;

		MapDef(cr_sex sexDef, const IStructure& keyType, const IStructure& valueType): SexDef(sexDef), KeyType(keyType), ValueType(valueType) {}
		MapDef(const MapDef& s): SexDef(s.SexDef), KeyType(s.KeyType), ValueType(s.ValueType) {}
	};

	struct NodeDef
	{
		cr_sex SexDef;
		const IStructure& ElementType;

		NodeDef(cr_sex sexDef, const IStructure& elementType): SexDef(sexDef), ElementType(elementType) {}
	};

	struct MapNodeDef
	{
		cr_sex SexDef;
		MapDef mapdef;
		stdstring mapName;
		MapNodeDef(cr_sex _sexDef, const MapDef& _mapdef, csexstr _mapName): SexDef(_sexDef), mapdef(_mapdef), mapName(_mapName) {}
	};

	struct ArrayCallbacks
	{
		ID_API_CALLBACK ArrayPushAndGetRef;
		ID_API_CALLBACK ArrayPush32;
		ID_API_CALLBACK ArrayPush64;
		ID_API_CALLBACK ArrayPushByRef;
		ID_API_CALLBACK ArrayGet32;
		ID_API_CALLBACK ArrayGet64;
		ID_API_CALLBACK ArrayGetMember32;
		ID_API_CALLBACK ArrayGetMember64;
		ID_API_CALLBACK ArrayGetByRef;
		ID_API_CALLBACK ArraySet32;
		ID_API_CALLBACK ArraySet64;
		ID_API_CALLBACK ArraySetByRef;
		ID_API_CALLBACK ArrayInit;
		ID_API_CALLBACK ArrayDelete;
		ID_API_CALLBACK ArrayPop;
		ID_API_CALLBACK ArrayPopOut32;
		ID_API_CALLBACK ArrayPopOut64;
		ID_API_CALLBACK ArrayLock;
		ID_API_CALLBACK ArrayUnlock;
		ID_API_CALLBACK ArrayGetRefUnchecked;
		ID_API_CALLBACK ArrayDestructElements;
	};

	struct ListCallbacks
	{
		ID_API_CALLBACK ListInit;
		ID_API_CALLBACK ListAppend;
		ID_API_CALLBACK ListAppendAndGetRef;
		ID_API_CALLBACK ListAppend32;
		ID_API_CALLBACK ListAppend64;
		ID_API_CALLBACK ListClear;
		ID_API_CALLBACK ListPrepend;
		ID_API_CALLBACK ListPrependAndGetRef;
		ID_API_CALLBACK ListPrepend32;
		ID_API_CALLBACK ListPrepend64;
		ID_API_CALLBACK ListGetTail;
		ID_API_CALLBACK ListGetHead;
		ID_API_CALLBACK NodeGet32;
		ID_API_CALLBACK NodeGet64;
		ID_API_CALLBACK NodeGetElementRef;
		ID_API_CALLBACK NodeNext;
		ID_API_CALLBACK NodePrevious;
		ID_API_CALLBACK NodeAppend;
		ID_API_CALLBACK NodeAppend32;
		ID_API_CALLBACK NodeAppend64;
		ID_API_CALLBACK NodePrepend;
		ID_API_CALLBACK NodePrepend32;
		ID_API_CALLBACK NodePrepend64;
		ID_API_CALLBACK NodePop;
		ID_API_CALLBACK NodeEnumNext;
		ID_API_CALLBACK NodeHasNext;
		ID_API_CALLBACK NodeHasPrevious;
		ID_API_CALLBACK NodeReleaseRef;
	};

	struct MapCallbacks
	{
		ID_API_CALLBACK MapInit;
		ID_API_CALLBACK MapInsert32;
		ID_API_CALLBACK MapInsert64;
		ID_API_CALLBACK MapInsertValueByRef;
		ID_API_CALLBACK MapInsertAndGetRef;
		ID_API_CALLBACK MapClear;
		ID_API_CALLBACK MapTryGet;
		ID_API_CALLBACK MapNodeGet32;
		ID_API_CALLBACK MapNodeGet64;
		ID_API_CALLBACK MapNodeGetRef;
		ID_API_CALLBACK MapNodePop;
		ID_API_CALLBACK MapGetHead;
		ID_API_CALLBACK NodeEnumNext;
		ID_API_CALLBACK MapNodeReleaseRef;
	};

	const ArrayCallbacks& GetArrayCallbacks(CCompileEnvironment& ce);
	const ListCallbacks& GetListCallbacks(CCompileEnvironment& ce);
	const MapCallbacks& GetMapCallbacks(CCompileEnvironment& ce);

	const IStructure& GetArrayDef(CCompileEnvironment& ce, cr_sex src, csexstr arrayName);
	const IStructure& GetListDef(CCompileEnvironment& ce, cr_sex src, csexstr name);
	const MapDef GetMapDef(CCompileEnvironment& ce, cr_sex src, csexstr name);
	const IStructure& GetNodeDef(CCompileEnvironment& ce, cr_sex src, csexstr name);
	const MapNodeDef& GetMapNodeDef(CCompileEnvironment& ce, cr_sex src, csexstr name);

	void AddArrayDef(CScript& script, ICodeBuilder& builder, csexstr arrayName, const IStructure& elementType, cr_sex def);
	void AddListDef(CScript& script, ICodeBuilder& builder, csexstr listName, const IStructure& elementType, cr_sex def);
	void AddMapDef(CScript& script, ICodeBuilder& builder, csexstr name, const IStructure& keyType, const IStructure& valueType, cr_sex def);
	void AddNodeDef(CScript& script, ICodeBuilder& builder, csexstr name, const IStructure& elementType, cr_sex s);
	void AddMapNodeDef(CScript& script, ICodeBuilder& builder, const MapDef& def, csexstr nodeName, csexstr mapName, cr_sex s);

	const SEXCHAR* const THIS_POINTER_TOKEN = SEXTEXT("this");

	typedef std::unordered_map<void*,void*> TAllocationMap;

	void GetAtomicValue(CCompileEnvironment& ce, cr_sex parent, csexstr id, VARTYPE type);
	void AppendDeconstruct(CCompileEnvironment& ce, cr_sex sequence);
	void AddPseudoVariable(CCompileEnvironment& ce, const NameString& ns, const IStructure& ts);
	void AddVariableAndSymbol(CCompileEnvironment& ce, csexstr type, csexstr name);
	void ValidateUnusedVariable(cr_sex identifierExpr, ICodeBuilder& builder);
	void AssertGetVariable(OUT MemberDef& def, csexstr name, CCompileEnvironment& ce, cr_sex exceptionSource);
	void CompileAsPopOutFromArray(CCompileEnvironment& ce, cr_sex s, csexstr instanceName, VARTYPE requiredType);
	void CompileArraySet(CCompileEnvironment& ce, cr_sex s);
	void CompileGetArrayElement(CCompileEnvironment& ce, cr_sex s, csexstr instanceName, VARTYPE varType, const IStructure* structType);
	void CompileGetArraySubelement(CCompileEnvironment& ce, cr_sex indexExpr, cr_sex subItemName, csexstr instanceName, VARTYPE type, const IStructure* structType);

	bool TryCompileAsArrayCall(CCompileEnvironment& ce, cr_sex s, csexstr instanceName, csexstr methodName);
	bool TryCompileAsListCall(CCompileEnvironment& ce, cr_sex s, csexstr instanceName, csexstr methodName);
	bool TryCompileAsNodeCall(CCompileEnvironment& ce, cr_sex s, csexstr instanceName, csexstr methodName);
	bool TryCompileAsMapCall(CCompileEnvironment& ce, cr_sex s, csexstr mapName, csexstr methodName);
	bool TryCompileAsMapNodeCall(CCompileEnvironment& ce, cr_sex s, csexstr name, csexstr methodName);
	bool TryCompileAsInlineMapAndReturnValue(CCompileEnvironment& ce, cr_sex s, csexstr instance, csexstr methodName, VARTYPE returnType, const IStructure& instanceStruct, OUT VARTYPE& outputType);

	void ConstructMemberByRef(CCompileEnvironment& ce, cr_sex args, int tempDepth, const IStructure& type, int offset);

	VARTYPE GetAtomicValueAnyNumeric(CCompileEnvironment& ce, cr_sex parent, csexstr id, int tempdepth);
	void AssignVariableToVariable(CCompileEnvironment& ce, cr_sex exceptionSource, csexstr lhs, csexstr rhs);	
	CStringConstant* CreateStringConstant(CScript& script, int length, csexstr s, const ISExpression* srcExpression);
	IFunctionBuilder& DeclareFunction(IModuleBuilder& module, cr_sex source, FunctionPrototype& prototype);
	csexstr GetContext(const CScript& script);
	const ISExpression* GetTryCatchExpression(CScript& script);
	bool TryCompileBooleanExpression(CCompileEnvironment& ce, cr_sex s, bool isExpected, bool& negate);
	void CompileExpressionSequence(CCompileEnvironment& ce, int start, int end, cr_sex sequence);
	void CompileExpression(CCompileEnvironment& ce, cr_sex s);
	void ValidateArchetypeMatchesArchetype(cr_sex s, const IArchetype& f, const IArchetype& archetype, csexstr source);
	int GetIndexOf(int start, cr_sex s, csexstr text);
	void CompileExceptionBlock(CCompileEnvironment& ce, cr_sex s);
	void CompileThrow(CCompileEnvironment& ce, cr_sex s);
	void CompileNextClosures(CScript& script);
	void GetVariableByName(ICodeBuilder& builder, OUT MemberDef& def, csexstr name, cr_sex src);
	void AddCatchHandler(CScript& script, ID_BYTECODE id, size_t start, size_t end);
	IModuleBuilder& GetModule(CScript& script);
	void AppendVirtualCallAssembly(csexstr instanceName, int interfaceIndex, int methodIndex, ICodeBuilder& builder, const IInterface& interf, cr_sex s);
	int GetFirstOutputOffset(ICodeBuilder& builder);
	IModule& GetSysTypeMemoModule(CScript& script);
	INamespaceBuilder& GetNamespaceByFQN(CCompileEnvironment& ce, csexstr ns, cr_sex s);
	void MarkStackRollback(CCompileEnvironment& ce, cr_sex invokeExpression);
	int PushInput(CCompileEnvironment& ce, cr_sex s, int index, const IStructure& inputStruct, const IArchetype* archetype, csexstr inputName, const IStructure* genericArg1);	
	const IArgument& GetInput(cr_sex s, IFunction& f, int index);
	const IArchetype* GetArchetype(cr_sex s, IFunction& f, int index);
	void PushVariableRef(cr_sex s, ICodeBuilder& builder, const MemberDef& def, csexstr name, int interfaceIndex);
	int GetCommonInterfaceIndex(const IStructure& object, const IStructure& argType);
	csexstr GetTypeName(VARTYPE type);

	IFunctionBuilder& MustMatchFunction(IModuleBuilder& module, cr_sex s, csexstr name);
	IInterfaceBuilder* MatchInterface(cr_sex typeExpr, IModuleBuilder& module);
	IStructureBuilder* MatchStructure(cr_sex typeExpr, IModuleBuilder& module);
	IFunctionBuilder* MatchFunction(cr_sex nameExpr, IModuleBuilder& module);

	bool TryCompileAssignArchetype(CCompileEnvironment& ce, cr_sex s, const IStructure& elementType, bool allowClosures);
	bool TryCompileArithmeticExpression(CCompileEnvironment& ce, cr_sex s, bool expected, VARTYPE type);
	
	void CompileFunctionCallAndReturnValue(CCompileEnvironment& ce, cr_sex s, IFunction& callee, VARTYPE returnType, const IArchetype* returnArchetype, const IStructure* returnTypeStruct);
	bool TryCompileMacroInvocation(CCompileEnvironment& ce, cr_sex s, sexstring token);
	IProgramObject& GetProgramObject(CScript& script);
	void CompileJITStub(IFunctionBuilder& f, cr_sex fdef, CScript& script, IScriptSystem& ss);
	void CompileJITStub(IFactoryBuilder* f, cr_sex fdef, CScript& script, IScriptSystem& ss);
	void CompileJITStub(IMacroBuilder* m, CScript& script, IScriptSystem& ss);

	IScriptSystem& GetSystem(CScript& script);
	IScriptSystem& GetSystem(CScripts& scripts);

	IFunctionBuilder& GetNullFunction(CScript& script, const IArchetype& archetype);
}

#include "sexy.script.util.inl"
#include "sexy.script.asserts.inl"
#include "sexy.script.functions.inl"
#include "sexy.script.macro.inl"
#include "sexy.script.matching.inl"
#include "sexy.script.factory.inl"
#include "sexy.script.closure.inl"
#include "sexy.script.array.inl"
#include "sexy.script.list.inl"
#include "sexy.script.map.inl"
#include "sexy.script.arithmetic.expression.parser.inl"
#include "sexy.script.predicates.expression.parser.inl"
#include "sexy.script.conditional.expression.parser.inl"
#include "sexy.script.expression.parser.inl"
#include "sexy.script.exception.logic.inl"
#include "sexy.script.modules.inl"
#include "sexy.script.exceptions.inl"
#include "sexy.script.casts.inl"
#include "sexy.script.JIT.inl"
#include "sexy.script.stringbuilders.inl"

namespace
{
	void CopyStringToSexChar(SEXCHAR* output, size_t bufferCapacity, const char* input, size_t inputLength)
	{
		for(size_t i = 0; i < inputLength; ++i)
		{
			output[i] = input[i];
		}
	}

	void CopyStringToSexChar(SEXCHAR* output, size_t bufferCapacity, const wchar_t* input, size_t inputLength)
	{
		for(size_t i = 0; i < inputLength; ++i)
		{
			output[i] = (SEXCHAR) input[i];
		}
	}

	struct NativeFunction
	{
		NativeCallEnvironment e;
		FN_NATIVE_CALL NativeCallback;
		stdstring Archetype;

		NativeFunction(IPublicScriptSystem& ss, const IFunction& f, CPU& cpu, void* context): e(ss, f, cpu, context) {}		
	};

	struct CommonSource
	{
		stdstring SexySourceCode;
		ISourceCode* Src;
		ISParserTree* Tree;
	};

	void Print(NativeCallEnvironment& e)
	{
		const SEXCHAR* pData;
		ReadInput(0, (void*&) pData, e);

		if (pData == NULL) pData = SEXTEXT("<null>");
		int nullLen = StringLength(pData);
		e.ss.PublicProgramObject().Log().Write(pData);
		WriteOutput(0, nullLen, e);
	}

	typedef std::tr1::unordered_map<stdstring,NativeFunction*> TMapFQNToNativeCall;
	typedef std::list<CommonSource> TCommonSources;
	typedef std::list<INativeLib*> TNativeLibs;
	
	void _cdecl RouteToNative(VariantValue* registers, void* context)
	{
		NativeFunction* nf = (NativeFunction*) context;
		nf->NativeCallback(nf->e);
	}

	static const SEXCHAR* NativeModuleSrc = SEXTEXT("_NativeModule_");

	void InstallNativeCallNamespaces(IN const TMapFQNToNativeCall& nativeCalls, REF INamespaceBuilder& rootNS)
	{
		for(auto i = nativeCalls.begin(); i != nativeCalls.end(); ++i)
		{
			NativeFunction& nf = *i->second;
			NamespaceSplitter splitter(i->first.c_str());

			csexstr body, publicName;
			if (!splitter.SplitTail(OUT body, OUT publicName))
			{
				SEXCHAR fullError[2048];
				StringPrint(fullError, 2048, SEXTEXT("%s: Expecting fully qualified name A.B.C.D."), nf.Archetype.c_str());
				ParseException nativeError(Vec2i{ 0,0 }, Vec2i{ 0,0 }, NativeModuleSrc, fullError, SEXTEXT(""), NULL);
				Throw(nativeError);
			}

			INamespaceBuilder& ns = rootNS.AddNamespace(body, ADDNAMESPACEFLAGS_CREATE_ROOTS);
		}
	}

	void InstallNativeCalls(IN const TMapFQNToNativeCall& nativeCalls, REF INamespaceBuilder& rootNS)
	{
		for(auto i = nativeCalls.begin(); i != nativeCalls.end(); ++i)
		{
			NativeFunction& nf = *i->second;
			NamespaceSplitter splitter(i->first.c_str());

			csexstr body, publicName;
			if (!splitter.SplitTail(OUT body, OUT publicName))
			{
				SEXCHAR fullError[2048];
				StringPrint(fullError, 2048, SEXTEXT("%s: Expecting fully qualified name A.B.C.D."), nf.Archetype.c_str());
				ParseException nativeError(Vec2i{ 0,0 }, Vec2i{ 0,0 }, NativeModuleSrc, fullError, SEXTEXT(""), NULL);
				Throw(nativeError);
			}

			INamespaceBuilder& ns = *rootNS.FindSubspace(body);

			IFunctionBuilder* f = (IFunctionBuilder*) &nf.e.function;
			ns.Alias(publicName, *f);

			if (!f->TryResolveArguments())
			{
				SEXCHAR fullError[2048];
				StringPrint(fullError, 2048, SEXTEXT("%s: Could not resolve all arguments. Check the log."), nf.Archetype.c_str());
				ParseException nativeError(Vec2i{ 0,0 }, Vec2i{ 0,0 }, NativeModuleSrc, fullError, SEXTEXT(""), NULL);
				Throw(nativeError);
			}

			ICodeBuilder& builder = f->Builder();

			try
			{
				builder.Begin();
				ID_API_CALLBACK idCallback = rootNS.Object().VirtualMachine().Core().RegisterCallback(RouteToNative, i->second, i->first.c_str());
				builder.Assembler().Append_Invoke(idCallback);
				builder.End();
				builder.Assembler().Clear();
			}
			catch (IException& e)
			{
				SEXCHAR fullError[2048];
				StringPrint(fullError, 2048, SEXTEXT("%s: %s"), nf.Archetype.c_str(), e.Message());
				ParseException nativeError(Vec2i{ 0,0 }, Vec2i{ 0,0 }, NativeModuleSrc, fullError, SEXTEXT(""), NULL);
				Throw(nativeError);
			}							
		}
	}

	csexstr GetArgTypeFromExpression(cr_sex s)
	{
		AssertCompound(s);
		AssertNotTooFewElements(s, 2);
		AssertNotTooManyElements(s, 2);

		return GetAtomicArg(s, 0).String()->Buffer;
	}

	csexstr GetArgNameFromExpression(cr_sex s)
	{
		AssertCompound(s);
		AssertNotTooFewElements(s, 2);
		AssertNotTooManyElements(s, 2);

		return GetAtomicArg(s, 1).String()->Buffer;
	}

	void _cdecl OnCallbackInvoked(VariantValue* registers, void* context)
	{
		NativeFunction* nf = (NativeFunction*) context;
		nf->NativeCallback(nf->e);
	}

	void AddNativeCallViaTree(REF TMapFQNToNativeCall& nativeCalls, REF IModuleBuilder& module, REF IScriptSystem& ss, IN const INamespace& ns, IN FN_NATIVE_CALL callback, void* context, IN Sex::ISParserTree& tree, IN int nativeCallIndex, bool checkName)
	{
		cr_sex archetype = tree.Root();
		AssertCompound(archetype);
		if (archetype.NumberOfElements() == 1)
		{
			SEXCHAR fullError[512];
			StringPrint(fullError, 512, SEXTEXT("Element defined in %s had one element. Ensure that the native call spec is not encapsulated in parenthesis"), ns.FullName()->Buffer);
			Throw(archetype, fullError);
		}

		AssertNotTooFewElements(archetype, 2);

		enum {MAX_ARGS_PER_NATIVE_CALL = 40};
		AssertNotTooManyElements(archetype, MAX_ARGS_PER_NATIVE_CALL);
		
		cr_sex fnameArg = GetAtomicArg(archetype, 0);
		csexstr publicName = fnameArg.String()->Buffer;

		TokenBuffer nativeName;
		StringPrint(nativeName, SEXTEXT("_%d_%s"), nativeCallIndex, publicName);

		int mapIndex = GetIndexOf(1, archetype, SEXTEXT("->"));
		if (mapIndex < 0)
		{
			SEXCHAR fullError[512];
			StringPrint(fullError, 512, SEXTEXT("Cannot find the mapping token -> in the archetype: %s.%s"), ns.FullName()->Buffer, publicName);
			Throw(archetype, fullError);
		}

		if (checkName) AssertValidFunctionName(fnameArg);

		IFunctionBuilder& f = module.DeclareFunction(FunctionPrototype(nativeName, false), &archetype);

		for(int i = mapIndex+1; i < archetype.NumberOfElements(); ++i)
		{
			cr_sex outputDef = archetype.GetElement(i);
			csexstr type = GetArgTypeFromExpression(outputDef);
			csexstr name = GetArgNameFromExpression(outputDef);
			f.AddOutput(NameString::From(name), TypeString::From(type), (void*)&outputDef);
		}

		for(int i = 1; i < mapIndex; ++i)
		{
			cr_sex inputDef = archetype.GetElement(i);
			csexstr type = GetArgTypeFromExpression(inputDef);
			csexstr name = GetArgNameFromExpression(inputDef);
			f.AddInput(NameString::From(name), TypeString::From(type), (void*)&inputDef);
		}

		TokenBuffer fullyQualifiedName;
		StringPrint(fullyQualifiedName, SEXTEXT("%s.%s"), ns.FullName()->Buffer, publicName);

		NativeFunction* nf = new NativeFunction(ss, f, ss.ProgramObject().VirtualMachine().Cpu(), context);
		nf->NativeCallback = callback;
		nf->Archetype = tree.Source().SourceStart();
		nativeCalls[fullyQualifiedName.Text] = nf;
	}

	void AddNullFields(IStructureBuilder* s)
	{
		s->AddMember(NameString::From(SEXTEXT("_typeInfo")), TypeString::From(SEXTEXT("Pointer")));
		s->AddMember(NameString::From(SEXTEXT("_allocSize")), TypeString::From(SEXTEXT("Int32")));
		s->AddMember(NameString::From(SEXTEXT("_vTable1")), TypeString::From(SEXTEXT("Pointer")));
	}

	class CScriptSystem: public IScriptSystem 
	{
	private:
		TStringBuilders stringBuilders;
		TMemoAllocator memoAllocator;
		CProgramObjectProxy progObjProxy;
		CSParserProxy sexParserProxy;
		CScripts* scripts;

		IStructureBuilder* nativeInt32;
		IStructureBuilder* nativeInt64;
		IStructureBuilder* nativeFloat32;
		IStructureBuilder* nativeFloat64;
		IStructureBuilder* nativeBool;
		IStructureBuilder* nativePtr;
		IStructureBuilder* nativeClassType;

		int nativeCallIndex;

		TMapFQNToNativeCall nativeCalls;
		TCommonSources commonSources;
		TNativeLibs nativeLibs;

		ID_API_CALLBACK jitId;
		ID_API_CALLBACK arrayPushId;

		SEXCHAR srcEnvironment[_MAX_PATH];

		TMapMethodToMember methodMap;

		typedef std::unordered_map<const Sex::ISExpression*,Script::CClassExpression*> TSReflectMap;
		TSReflectMap sreflectMap;

		void InstallNullFunction()
		{
			IFunctionBuilder& nullFunction = progObjProxy().IntrinsicModule().DeclareFunction(FunctionPrototype(SEXTEXT("_nothing"), false), NULL);
			nullFunction.Builder().Begin();
			nullFunction.Builder().End();
			nullFunction.Builder().Assembler().Clear();
		}
		
		IStructure* stringConstantStruct;

		IModuleBuilder& ReflectionModule()
		{
			return this->progObjProxy().GetModule(3);
		}

		IModule& SysTypeMemoModule()
		{
			return this->progObjProxy().GetModule(0);
		}

		typedef std::unordered_map<csexstr, CStringConstant*> TReflectedStrings;
		TReflectedStrings reflectedStrings;

		typedef std::unordered_map<const void*, stdstring> TSymbols;
		TSymbols symbols;

		typedef std::unordered_map<void*, CReflectedClass*> TReflectedPointers;
		TReflectedPointers reflectedPointers;
		TReflectedPointers representations;

		CScriptSystemClass* reflectionRoot;		
		TAllocationMap alignedAllocationMap;

		ArrayCallbacks arrayCallbacks;
		ListCallbacks listCallbacks;
		MapCallbacks mapCallbacks;

		int nextId;
	public:
		CScriptSystem(const ProgramInitParameters& pip, ILog& _logger): progObjProxy(pip, _logger), nativeCallIndex(1), stringConstantStruct(NULL), reflectionRoot(NULL), nextId(0)
		{
			try
			{
            if (pip.NativeSourcePath != 0)
            {
               SafeCopy(srcEnvironment, _MAX_PATH-1, pip.NativeSourcePath, _TRUNCATE);
            }
            else if (*defaultNativeSourcePath != 0)
            {
               SafeCopy(srcEnvironment, _MAX_PATH - 1, defaultNativeSourcePath, _TRUNCATE);
            }
            else
            {
               OS::GetEnvVariable(srcEnvironment, _MAX_PATH, SEXTEXT("SEXY_NATIVE_SRC_DIR"));
            }
            AddSlashToDirectory(srcEnvironment);
			}
			catch(IException& innerEx)
			{
            wchar_t message[1024];
            SafeFormat(message, _TRUNCATE, SEXTEXT("%s:\nFailed to get sexy environment.\nUse Sexy::Script::SetDefaultNativeSourcePath(...) or ProgramInitParameters or environment variable SEXY_NATIVE_SRC_DIR"), innerEx.Message());
            _logger.Write(message);
            Sexy::Throw(innerEx.ErrorCode(), SEXTEXT("%s"), message);
			}
			
			scripts = new CScripts(progObjProxy(), *this);
			
			nativeInt32 = &progObjProxy().AddIntrinsicStruct(SEXTEXT("Int32"), sizeof(int32), VARTYPE_Int32, NULL);
			nativeInt64 = &progObjProxy().AddIntrinsicStruct(SEXTEXT("Int64"), sizeof(int64), VARTYPE_Int64, NULL);
			nativeFloat32 = &progObjProxy().AddIntrinsicStruct(SEXTEXT("Float32"), sizeof(float32), VARTYPE_Float32, NULL);
			nativeFloat64 = &progObjProxy().AddIntrinsicStruct(SEXTEXT("Float64"), sizeof(float64), VARTYPE_Float64, NULL);
			nativeBool = &progObjProxy().AddIntrinsicStruct(SEXTEXT("Bool"), sizeof(int32), VARTYPE_Bool, NULL);
			nativePtr = &progObjProxy().AddIntrinsicStruct(SEXTEXT("Pointer"), sizeof(size_t), VARTYPE_Pointer, NULL);	

			try
			{
				AddCommonSource(SEXTEXT("Sys.Type.Strings.sxy")); // Module 0 -> This comes first, as module 0 is directly used to get a concrete class for string literals
				AddCommonSource(SEXTEXT("Sys.Type.sxy")); // Module 1
				AddCommonSource(SEXTEXT("Sys.Maths.sxy")); // Module 2			
				AddCommonSource(SEXTEXT("Sys.Reflection.sxy")); // Module 3
			
				AddNativeLibrary(SEXTEXT("Sexy.NativeLib.Reflection"));
				AddNativeLibrary(SEXTEXT("Sexy.NativeLib.Maths"));
			}
			catch(IException&)
			{
				_logger.Write(SEXTEXT("Sexy: Error reading common source files."));
				delete scripts;
				throw;
			}

			jitId = progObjProxy().VirtualMachine().Core().RegisterCallback(OnJITRoutineNeedsCompiling, this, SEXTEXT("OnJITRoutineNeedsCompiling"));

			arrayCallbacks.ArrayGetRefUnchecked = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayGetRefUnchecked, this, SEXTEXT("ArrayGetRefUnchecked"));
			arrayCallbacks.ArrayLock = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayLock, this, SEXTEXT("ArrayLock"));
			arrayCallbacks.ArrayUnlock = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayUnlock, this, SEXTEXT("ArrayUnlock"));
			arrayCallbacks.ArrayPushAndGetRef = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayPushAndGetRef, this, SEXTEXT("ArrayPushAndGetRef"));
			arrayCallbacks.ArrayPushByRef = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayPushByRef, this, SEXTEXT("ArrayPushByRef"));
			arrayCallbacks.ArrayPush32 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayPush32, this, SEXTEXT("ArrayPush32"));
			arrayCallbacks.ArrayPush64 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayPush64, this, SEXTEXT("ArrayPush64"));
			arrayCallbacks.ArrayGet32 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayGet32, this, SEXTEXT("ArrayGet32"));
			arrayCallbacks.ArrayGet64 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayGet64, this, SEXTEXT("ArrayGet64"));
			arrayCallbacks.ArrayGetMember32 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayGetMember32, this, SEXTEXT("ArrayGetMember32"));
			arrayCallbacks.ArrayGetMember64 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayGetMember64, this, SEXTEXT("ArrayGetMember64"));
			arrayCallbacks.ArrayGetByRef = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayGetByRef, this, SEXTEXT("ArrayGetByRef"));
			arrayCallbacks.ArrayInit = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayInit, this, SEXTEXT("ArrayInit"));
			arrayCallbacks.ArrayDelete = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayDelete, this, SEXTEXT("ArrayDelete"));
			arrayCallbacks.ArraySet32 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArraySet32, this, SEXTEXT("ArraySet32"));
			arrayCallbacks.ArraySet64 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArraySet64, this, SEXTEXT("ArraySet64"));
			arrayCallbacks.ArraySetByRef = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArraySetByRef, this, SEXTEXT("ArraySetByRef"));
			arrayCallbacks.ArrayPop = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayPop, this, SEXTEXT("ArrayPop"));
			arrayCallbacks.ArrayPopOut32 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayPopOut32, this, SEXTEXT("ArrayPopOut32"));
			arrayCallbacks.ArrayPopOut64 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayPopOut64, this, SEXTEXT("ArrayPopOut64"));
			arrayCallbacks.ArrayDestructElements = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayDestructElements, this, SEXTEXT("ArrayDestructElements"));

			listCallbacks.ListInit = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeListInit, this, SEXTEXT("ListInit"));
			listCallbacks.ListAppend = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeListAppend, this, SEXTEXT("ListAppend"));
			listCallbacks.ListAppendAndGetRef = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeListAppendAndGetRef, this, SEXTEXT("ListAppendAndGetRef"));
			listCallbacks.ListAppend32 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeListAppend32, this, SEXTEXT("ListAppend32"));
			listCallbacks.ListAppend64 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeListAppend64, this, SEXTEXT("ListAppend64"));
			listCallbacks.ListPrepend = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeListPrepend, this, SEXTEXT("ListPrepend"));
			listCallbacks.ListPrependAndGetRef = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeListPrependAndGetRef, this, SEXTEXT("ListPrependAndGetRef"));
			listCallbacks.ListPrepend32 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeListPrepend32, this, SEXTEXT("ListPrepend32"));
			listCallbacks.ListPrepend64 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeListPrepend64, this, SEXTEXT("ListPrepend64"));
			listCallbacks.ListGetHead = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeListGetHead, this, SEXTEXT("ListGetHead"));
			listCallbacks.ListGetTail = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeListGetTail, this, SEXTEXT("ListGetTail"));
			listCallbacks.NodeGet32 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeNodeGet32, this, SEXTEXT("NodeGet32"));
			listCallbacks.NodeGet64 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeNodeGet64, this, SEXTEXT("NodeGet64"));
			listCallbacks.NodeGetElementRef = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeNodeGetElementRef, this, SEXTEXT("NodeGetElementRef"));
			listCallbacks.NodeNext = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeNodeNext, this, SEXTEXT("NodeNext"));
			listCallbacks.NodePrevious = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeNodePrevious, this, SEXTEXT("NodePrevious"));
			listCallbacks.NodeAppend = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeNodeAppend, this, SEXTEXT("NodeAppend"));
			listCallbacks.NodeAppend32 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeNodeAppend32, this, SEXTEXT("NodeAppend32"));
			listCallbacks.NodeAppend64 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeNodeAppend64, this, SEXTEXT("NodeAppend64"));
			listCallbacks.NodePrepend = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeNodePrepend, this, SEXTEXT("NodePrepend"));
			listCallbacks.NodePrepend32 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeNodePrepend32, this, SEXTEXT("NodePrepend32"));
			listCallbacks.NodePrepend64 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeNodePrepend64, this, SEXTEXT("NodePrepend64"));
			listCallbacks.NodePop = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeNodePop, this, SEXTEXT("NodePop"));
			listCallbacks.NodeEnumNext = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeNodeEnumNext, this, SEXTEXT("NodeEnumNext"));
			listCallbacks.NodeHasNext = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeNodeHasNext, this, SEXTEXT("NodeHasNext"));
			listCallbacks.NodeHasPrevious = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeNodeHasPrevious, this, SEXTEXT("NodeHasPrevious"));
			listCallbacks.NodeReleaseRef = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeNodeReleaseRef, this, SEXTEXT("NodeReleaseRef"));
			listCallbacks.ListClear = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeListClear, this, SEXTEXT("ListClear"));

			mapCallbacks.MapClear = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeMapClear, this, SEXTEXT("MapClear"));
			mapCallbacks.NodeEnumNext = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeMapNodeEnumNext, this, SEXTEXT("MapNodeEnumNext"));
			mapCallbacks.MapGetHead = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeMapGetHead, this, SEXTEXT("MapGetHead"));
			mapCallbacks.MapInit = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeMapInit, this, SEXTEXT("MapInit"));
			mapCallbacks.MapInsert32 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeMapInsert32, this, SEXTEXT("MapInsert32"));
			mapCallbacks.MapInsert64 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeMapInsert64, this, SEXTEXT("MapInsert64"));
			mapCallbacks.MapInsertValueByRef = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeMapInsertValueByRef, this, SEXTEXT("MapInsertValueByRef"));
			mapCallbacks.MapInsertAndGetRef = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeMapInsertAndGetRef, this, SEXTEXT("MapInsertAndGetRef"));
			mapCallbacks.MapTryGet = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeMapTryGet, this, SEXTEXT("MapTryGet"));
			mapCallbacks.MapNodeGet32 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeMapNodeGet32, this, SEXTEXT("MapNodeGet32"));
			mapCallbacks.MapNodeGet64 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeMapNodeGet64, this, SEXTEXT("MapNodeGet64"));
			mapCallbacks.MapNodeGetRef = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeMapNodeGetRef, this, SEXTEXT("MapNodeGetRef"));
			mapCallbacks.MapNodePop = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeMapNodePop, this, SEXTEXT("MapNodePop"));
			mapCallbacks.MapNodeReleaseRef = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeMapNodeReleaseRef, this, SEXTEXT("MapNodeReleaseRef"));
			
			methodMap[SEXTEXT("Capacity")] = SEXTEXT("_elementCapacity");
			methodMap[SEXTEXT("Length")] = SEXTEXT("_length");
		}
		
		~CScriptSystem()
		{
			for(auto i = commonSources.begin(); i != commonSources.end(); ++i)
			{
				i->Tree->Release();
				i->Src->Release();
			}

			commonSources.clear();

			for(auto i = nativeCalls.begin(); i != nativeCalls.end(); ++i)
			{
				NativeFunction* nf = i->second;
				delete nf;
			}

			for(auto i = nativeLibs.begin(); i != nativeLibs.end(); ++i)
			{
				INativeLib* lib = *i;
				lib->Release();
			}

			delete scripts;
		}

		virtual CReflectedClass* GetRepresentation(void* pSourceInstance)
		{
			auto i = representations.find(pSourceInstance);
			return (i != representations.end()) ? i->second : NULL;
		}

		virtual CReflectedClass* Represent(const Sexy::Compiler::IStructure& st, void* pSourceInstance)
		{
			auto i = representations.find(pSourceInstance);
			if (i == representations.end())
			{
				if (st.InterfaceCount() != 1)
				{
					LogError(progObjProxy().Log(), SEXTEXT("The structure '%s' must implement one and only one interface to reflect a native object"), st.Name());
					return NULL;
				}

				if (IsNullType(st))
				{
					LogError(progObjProxy().Log(), SEXTEXT("Null structures, including '%s', cannot be used to reflect a native object"), st.Name());
					return NULL;
				}

				CReflectedClass* rep = (CReflectedClass*) AlignedMalloc(sizeof(size_t), sizeof(CReflectedClass));
				rep->context = pSourceInstance;
				rep->header._typeInfo = (CClassDesc*) st.GetVirtualTable(0);
				rep->header._allocSize = sizeof(CReflectedClass);
				rep->header._vTables[0].Root = st.GetVirtualTable(1);

				i = representations.insert(std::make_pair(pSourceInstance,rep)).first;
			}

			return i->second;
		}

		virtual void CancelRepresentation(void* pSourceInstance)
		{
			auto i = representations.find(pSourceInstance);
			if (i != representations.end())
			{
				CReflectedClass* rep = i->second;
				AlignedFree(rep);
				representations.erase(i);
			}
		}

		virtual void EnumRepresentations(IRepresentationEnumeratorCallback& callback)
		{
			for(auto i = representations.begin(); i != representations.end();)
			{
				ENUM_REPRESENT result = callback.OnRepresentation(i->second);
				if (result == ENUM_REPRESENT_BREAK)
				{
					return;
				}
				else if (result == ENUM_REPRESENT_CONTINUE)
				{
					++i;					
				}
				else // delete
				{
					AlignedFree(i->second);
					i = representations.erase(i);
				}
			}
		}

		bool ValidateMemory()
		{
			if (!alignedAllocationMap.empty())
			{
				Sexy::LogError(ProgramObject().Log(), SEXTEXT("Memory leak! %u elements found in the aligned allocation map"), alignedAllocationMap.size());
				return false;
			}

			return true;
		}

		virtual void SetGlobalVariablesToDefaults()
		{
			auto& vm = progObjProxy().VirtualMachine();
			scripts->SetGlobalVariablesToDefaults(vm);
		}

		virtual int NextID()
		{
			return nextId++;
		}

		void ThrowFromNativeCode(int32 errorCode, csexstr staticRefMessage)
		{
			scripts->ExceptionLogic().ThrowFromNativeCode(errorCode, staticRefMessage);
		}

		void* AlignedMalloc(int32 alignment, int32 capacity)
		{
			uint8* alignedData;

			if (capacity <= 0 || alignment <= 0)
			{
				 alignedData = NULL;
			}
			else
			{
				if (alignment != 1)
				{
					uint8* data = new uint8[capacity + alignment];
		
					size_t tooManyBytes = (size_t) data & alignment;
					if (tooManyBytes != 0)
					{
						size_t paddingBytes = alignment - tooManyBytes;
						alignedData = data + paddingBytes;			
					}
					else
					{
						alignedData = data;
					}
				
					alignedAllocationMap[alignedData] = data;
				}
				else
				{
					alignedData = new uint8[capacity];
					alignedAllocationMap[alignedData] = alignedData;
				}
			}

			return alignedData;
		}

		void AlignedFree(void* alignedData)
		{
			if (alignedData == NULL) return;

			auto i = alignedAllocationMap.find(alignedData);
			if (i != alignedAllocationMap.end())
			{
				uint8* raw = (uint8*) i->second;
				delete raw;

				alignedAllocationMap.erase(i);
			}
			else
			{
				progObjProxy().Log().Write(SEXTEXT("Sys.Native.FreeAligned(...) was passed a pointer that is not currently defined in the aligned heap"));
				progObjProxy().VirtualMachine().Throw();
			}
		}

		virtual const void* GetMethodMap()
		{
			return &methodMap;
		}

		ID_API_CALLBACK JITCallbackId() const
		{
			return jitId;
		}

		const ArrayCallbacks& GetArrayCallbacks() const
		{
			return arrayCallbacks;
		}

		const ListCallbacks& GetListCallbacks() const 
		{
			return listCallbacks;
		}

		const MapCallbacks& GetMapCallbacks() const 
		{
			return mapCallbacks;
		}

		virtual CReflectedClass* GetReflectedClass(void* ptr)
		{
			auto i = reflectedPointers.find(ptr);
			return i != reflectedPointers.end()	? i->second : NULL;
		}

		IStructure* GetClassFromModuleElseLog(IModuleBuilder& module, csexstr className)
		{
			IStructure *s = module.FindStructure(className);
			if (s == NULL)
			{
				TokenBuffer token;
				StringPrint(token, SEXTEXT("Cannot find %s in module %s"), className, module.Name());
				progObjProxy().Log().Write(token);
				return NULL;
			} 
			return s;
		}

		virtual CReflectedClass* CreateReflectionClass(csexstr className, void* context) 
		{
			IStructure* s = GetClassFromModuleElseLog(ReflectionModule(), className);
				
			if (s->SizeOfStruct() != sizeof(CReflectedClass))
			{
				TokenBuffer token;
				StringPrint(token, SEXTEXT("%s in reflection module is not equivalent to CReflectedClass"), className);
				progObjProxy().Log().Write(token);
				return NULL;
			}

			CReflectedClass* instance = (CReflectedClass*) DynamicCreateClass(*s, 0);
			reflectedPointers.insert(std::make_pair(context, instance));
			instance->context = context;
			return instance;
		}

		virtual bool ConstructExpressionBuilder(CClassExpressionBuilder& builderContainer, Sexy::Sex::ISExpressionBuilder* builder)
		{
			IStructure* s = GetClassFromModuleElseLog(ReflectionModule(), SEXTEXT("ExpressionBuilder"));
			if (s == NULL)
			{
				return false;
			}
			else
			{
				builderContainer.BuilderPtr = builder;
				builderContainer.Header._allocSize = sizeof(builderContainer);
				builderContainer.Header._typeInfo = (CClassDesc*) s->GetVirtualTable(0);
				builderContainer.Header._vTables[0].Root = s->GetVirtualTable(1);
				return true;
			}
		}

		void* DynamicCreateClass(const IStructure& s, int interfaceIndex)
		{
			int nBytes = s.SizeOfStruct();
			if (nBytes <= 0)
			{
            Sexy::Throw(0, SEXTEXT("The structure size was not postive"));
			}

			CClassHeader* instance = (CClassHeader*) new char[nBytes];

			if (s.Prototype().IsClass)
			{
				instance->_typeInfo = (CClassDesc*) s.GetVirtualTable(0);
				instance->_allocSize = nBytes;

				for(int i = 0; i < s.InterfaceCount(); ++i)
				{
					instance->_vTables[i].Root = (ID_BYTECODE*) s.GetVirtualTable(i+1);
				}
			}
	
			return instance;
		}

		void FreeDynamicClass(CClassHeader* header)
		{
			delete[] header;
		}

		CClassExpression* CreateReflection(cr_sex s)
		{
			IModuleBuilder& reflectionModule = ReflectionModule();
			IStructure* expressStruct = reflectionModule.FindStructure(SEXTEXT("Expression"));
			if (expressStruct == NULL) 
			{
            Sexy::Throw(0, SEXTEXT("Cannot find 'Expression' in the reflection module"));
			}

			CClassExpression* express = (CClassExpression*) DynamicCreateClass(*expressStruct, 0);
			express->ExpressionPtr = (Sexy::Sex::ISExpression*) &s;
			return express;
		}

		virtual const CClassExpression* GetExpressionReflection(cr_sex s)
		{
			auto i = sreflectMap.find(&s);
			if (i == sreflectMap.end())
			{
				CClassExpression* pReflection = CreateReflection(s);
				sreflectMap.insert(std::make_pair(&s, pReflection));
				return pReflection;
			}
			else
			{
				return i->second;
			}
		}

		virtual CStringConstant* GetStringReflection(csexstr s)
		{
			auto i = reflectedStrings.find(s);
			if (i == reflectedStrings.end())
			{
				const IStructure& stringConstantStruct = *SysTypeMemoModule().FindStructure(SEXTEXT("StringConstant"));
				CStringConstant* pSC = (CStringConstant*) DynamicCreateClass(stringConstantStruct, 0);
				pSC->pointer = s;
				pSC->length = StringLength(s);
				pSC->srcExpression = NULL;
				reflectedStrings.insert(std::make_pair(s, pSC));
				return pSC;
			}
			else
			{
				return i->second;
			}
		}

		virtual CScriptSystemClass* GetScriptSystemClass()
		{
			if (reflectionRoot == NULL)
			{
				const IStructure& classStruct = *ReflectionModule().FindStructure(SEXTEXT("ScriptSystem"));
				reflectionRoot = (CScriptSystemClass*) DynamicCreateClass(classStruct, 0);
			}

			return reflectionRoot;
		}

		virtual IProgramObject& ProgramObject()
		{
			return progObjProxy();
		}

		virtual IPublicProgramObject& PublicProgramObject()
		{
			return progObjProxy();
		}

		virtual Sex::ISParser& SParser()
		{
			return sexParserProxy();
		}

		virtual IModule* AddTree(ISParserTree& tree) 
		{
			CScript* m = scripts->CreateModule(tree);	
			return &m->ProgramModule();
		}

		virtual void ReleaseTree(Sex::ISParserTree* tree)
		{
			scripts->ReleaseModule(*tree);
		}

		void DefinePrimitives(INamespaceBuilder& sysTypes)
		{
			sysTypes.Alias(SEXTEXT("Int32"), *nativeInt32);
			sysTypes.Alias(SEXTEXT("Int64"), *nativeInt64);
			sysTypes.Alias(SEXTEXT("Float32"), *nativeFloat32);
			sysTypes.Alias(SEXTEXT("Float64"), *nativeFloat64);
			sysTypes.Alias(SEXTEXT("Bool"), *nativeBool);		
			sysTypes.Alias(SEXTEXT("Pointer"), *nativePtr);	

			AddNativeCall(sysTypes, Print, NULL, SEXTEXT("NativePrint (Sys.Type.Pointer s) -> (Int32 charCount)"), false);
		}

		const INamespace& AddNativeNamespace(csexstr name)
		{
			return progObjProxy().GetRootNamespace().AddNamespace(name, Compiler::ADDNAMESPACEFLAGS_CREATE_ROOTS);
		}

		void DefineSysNative(const INamespace& sysNative)
		{
			AddNativeCall(sysNative, CreateMemoString, &memoAllocator, SEXTEXT("CreateMemoString (Pointer src) (Int32 srcLen) -> (Pointer dest) (Int32 destLength)"), false);
			AddNativeCall(sysNative, FreeMemoString, &memoAllocator, SEXTEXT("FreeMemoString (Pointer src) ->"), false);
			AddNativeCall(sysNative, DynamicCast, &stringBuilders, SEXTEXT("_DynamicCast (Pointer interface) (Pointer instanceRef) ->"), false);
			AddNativeCall(sysNative, CreateStringBuilder, &stringBuilders, SEXTEXT("CreateStringBuilder (Int32 capacity) -> (Pointer sbHandle)"), false);
			AddNativeCall(sysNative, FreeStringBuilder, &stringBuilders, SEXTEXT("FreeStringBuilder (Pointer sbHandle) ->"), false);
			AddNativeCall(sysNative, StringBuilderAppendIString, &stringBuilders, SEXTEXT("StringBuilderAppendIString (Pointer buffer) (Pointer src) (Int32 srclength) -> (Int32 newLength)"), false);
			AddNativeCall(sysNative, StringBuilderAppendInt32, &stringBuilders, SEXTEXT("StringBuilderAppendInt32 (Pointer buffer) (Int32 x) -> (Int32 newLength)"), false);
			AddNativeCall(sysNative, StringBuilderAppendInt64, &stringBuilders, SEXTEXT("StringBuilderAppendInt64 (Pointer buffer) (Int64 x) -> (Int32 newLength)"), false);
			AddNativeCall(sysNative, StringBuilderAppendFloat32, &stringBuilders, SEXTEXT("StringBuilderAppendFloat32 (Pointer buffer) (Float32 x) -> (Int32 newLength)"), false);
			AddNativeCall(sysNative, StringBuilderAppendFloat64, &stringBuilders, SEXTEXT("StringBuilderAppendFloat64 (Pointer buffer) (Float64 x) -> (Int32 newLength)"), false);
			AddNativeCall(sysNative, StringBuilderAppendBool, &stringBuilders, SEXTEXT("StringBuilderAppendBool (Pointer buffer) (Bool x) -> (Int32 newLength)"), false);			
			AddNativeCall(sysNative, StringBuilderAppendPointer, &stringBuilders, SEXTEXT("StringBuilderAppendPointer (Pointer buffer) (Pointer x) -> (Int32 newLength)"), false);			
			AddNativeCall(sysNative, StringBuilderClear, &stringBuilders, SEXTEXT("StringBuilderClear (Pointer buffer) ->"), false);
			AddNativeCall(sysNative, StringBuilderAppendAsDecimal, &stringBuilders, SEXTEXT("StringBuilderAppendAsDecimal (Pointer buffer) ->"), false);	
			AddNativeCall(sysNative, StringBuilderAppendAsHex, &stringBuilders, SEXTEXT("StringBuilderAppendAsHex (Pointer buffer) -> "), false);	
			AddNativeCall(sysNative, StringBuilderAppendAsSpec, &stringBuilders, SEXTEXT("StringBuilderAppendAsSpec (Pointer buffer) (Int32 type) -> "), false);	
			AddNativeCall(sysNative, StringBuilderSetFormat, &stringBuilders, SEXTEXT("StringBuilderSetFormat  (Pointer buffer) (Int32 precision) (Int32 width) (Bool isZeroPrefixed) (Bool isRightAligned)->"), false);
			AddNativeCall(sysNative, StringCompare, NULL, SEXTEXT("StringCompare  (Pointer s) (Pointer t) -> (Int32 diff)"), false);
			AddNativeCall(sysNative, StringCompareI, NULL, SEXTEXT("StringCompareI  (Pointer s) (Pointer t) -> (Int32 diff)"), false);
			AddNativeCall(sysNative, StringFindLeft, NULL, SEXTEXT("StringFindLeft (Pointer containerBuffer) (Int32 containerLength) (Int32 startPos) (Pointer substringBuffer) (Int32 substringLength) (Bool caseIndependent)-> (Int32 position)"), false);
			AddNativeCall(sysNative, StringFindRight, NULL, SEXTEXT("StringFindRight (Pointer containerBuffer) (Int32 containerLength) (Int32 rightPos) (Pointer substringBuffer) (Int32 substringLength) (Bool caseIndependent)-> (Int32 position)"), false);
			AddNativeCall(sysNative, StringBuilderAppendSubstring, NULL, SEXTEXT("StringBuilderAppendSubstring (Pointer builder) (Pointer s) (Int32 sLen) (Int32 startPos) (Int32 charsToAppend) -> (Int32 length)"), false);
			AddNativeCall(sysNative, StringBuilderSetLength, NULL, SEXTEXT("StringBuilderSetLength (Pointer builder) (Int32 length) -> (Int32 newlength)"), false);
			AddNativeCall(sysNative, StringBuilderSetCase, NULL, SEXTEXT("StringBuilderSetCase (Pointer builder) (Int32 start) (Int32 end) (Bool toUpper)->"), false);
			AddNativeCall(sysNative, ::AlignedMalloc, this, SEXTEXT("AlignedMalloc (Int32 capacity) (Int32 alignment)-> (Pointer data)"), false);
			AddNativeCall(sysNative, ::AlignedFree, this, SEXTEXT("AlignedFree (Pointer data)->"), false);
		}

		void AddSymbol(csexstr symbol, const void* ptr)
		{
			symbols.insert(std::make_pair(ptr, symbol));
		}

		virtual csexstr GetSymbol(const void* ptr) const
		{
			auto i = symbols.find(ptr);
			return i == symbols.end() ? NULL : i->second.c_str();
		}

		void BuildExtraSymbols()
		{
			for(int j = 0; j < progObjProxy().ModuleCount(); ++j)
			{
				IModuleBuilder& module = progObjProxy().GetModule(j);
				for(int i = 0; i < module.StructCount(); ++i)
				{
					SEXCHAR symbol[256];
					IStructure& s = module.GetStructure(i);
					if (s.Prototype().IsClass)
					{		
						StringPrint(symbol, 256, SEXTEXT("%s-typeInfo"), s.Name());
						AddSymbol(symbol, s.GetVirtualTable(0));		

						for(int k = 1; k <= s.InterfaceCount(); ++k)
						{					
							StringPrint(symbol, 256, SEXTEXT("%s-vTable%d"), s.Name(), k);
							AddSymbol(symbol, s.GetVirtualTable(k));						
						}						
					}

					StringPrint(symbol, 256, SEXTEXT("typeof(%s)"), s.Name());
					AddSymbol(symbol, &s);
				}
			}
		}

		void Clear()
		{
			symbols.clear();
			::Clear(stringBuilders);

			scripts->ExceptionLogic().Clear();
			progObjProxy().GetRootNamespace().Clear();

			for(auto i = reflectedStrings.begin(); i != reflectedStrings.end(); ++i)
			{
				FreeDynamicClass(&i->second->header);
			}

			reflectedStrings.clear();

			for(auto j = sreflectMap.begin(); j != sreflectMap.end(); ++j)
			{
				FreeDynamicClass(&j->second->Header);
			}

			FreeDynamicClass(&reflectionRoot->header);

			for(auto k = sreflectMap.begin(); k != sreflectMap.end(); ++k)
			{
				FreeDynamicClass(&k->second->Header);
			}

			for(auto i = alignedAllocationMap.begin(); i != alignedAllocationMap.end(); ++i)
			{
				uint8* buffer = (uint8*) i->second;
				delete buffer;
			}

			alignedAllocationMap.clear();

			for(auto i = nativeLibs.begin(); i != nativeLibs.end(); ++i)
			{
				INativeLib* lib = *i;
				lib->ClearResources();
			}
		}

		virtual void Compile()
		{
			Clear();

			INamespaceBuilder& sysTypes = progObjProxy().GetRootNamespace().AddNamespace(SEXTEXT("Sys.Type"), ADDNAMESPACEFLAGS_CREATE_ROOTS);
			DefinePrimitives(sysTypes);

			INamespaceBuilder& sysNative = progObjProxy().GetRootNamespace().AddNamespace(SEXTEXT("Sys.Native"), ADDNAMESPACEFLAGS_CREATE_ROOTS);
			DefineSysNative(sysNative);

			progObjProxy().ResolveNativeTypes();
		
			scripts->ExceptionLogic().InstallThrowHandler();

			for(auto i = nativeLibs.begin(); i != nativeLibs.end(); ++i)
			{
				INativeLib* lib = *i;
				lib->AddNativeCalls();
			}

			InstallNativeCallNamespaces(IN nativeCalls, REF ProgramObject().GetRootNamespace());
			scripts->CompileNamespaces();
			scripts->CompileDeclarations();
	
			InstallNullFunction();
			InstallNativeCalls(IN nativeCalls, REF ProgramObject().GetRootNamespace());

			scripts->CompileBytecode();

			BuildExtraSymbols();

			scripts->SetGlobalVariablesToDefaults(this->progObjProxy().VirtualMachine());
		}

		virtual void Free()
		{
			delete this;
		}

		virtual Sex::ISParserTree* GetSourceCode(const IModule& module) const
		{
			return scripts->GetSourceCode(module);
		}

		virtual void AddNativeCall(const Compiler::INamespace& ns, FN_NATIVE_CALL callback, void* context, csexstr archetype, bool checkName)
		{		
			enum { MAX_ARCHETYPE_LEN = 256};

			if (callback == NULL)
			{
            Sexy::Throw(0, SEXTEXT("ScriptSystem::AddNativeCall(...callback...): The [callback] pointer was NULL"));
			}
			
			if (archetype == NULL)
			{
            Sexy::Throw(0, SEXTEXT("ScriptSystem::AddNativeCall(...archetype...): The [archetype] pointer was NULL"));
			}

			size_t len = StringLength(archetype);
			if (len > (MAX_ARCHETYPE_LEN-1))
			{
            Sexy::Throw(0, SEXTEXT("ScriptSystem::AddNativeCall(...archetype...): The [archetype] string length exceed the maximum"));
			}

			SEXCHAR sxArchetype[MAX_ARCHETYPE_LEN];
			CopyStringToSexChar(sxArchetype, 256, archetype, len+1);

			SEXCHAR srcName[MAX_ARCHETYPE_LEN + 64];
			StringPrint(srcName, MAX_ARCHETYPE_LEN + 64, L"Source: '%s'", sxArchetype);
			Auto<ISourceCode> src = SParser().ProxySourceBuffer(sxArchetype, (int)len, Vec2i{ 0,0 }, srcName);

			try
			{
				Auto<ISParserTree> tree = SParser().CreateTree(src());

				AddNativeCallViaTree(REF nativeCalls, REF ProgramObject().IntrinsicModule(), IN *this, IN ns, IN callback, IN context, IN tree(), IN nativeCallIndex, IN checkName);
				nativeCallIndex++;		
			}
			catch (ParseException& e)
			{
				ParseException ex(e.Start(), e.End(), archetype, e.Message(), e.Specimen(), NULL);
				throw ex;
			}			
		}

		virtual void AddCommonSource(const Sexy::SEXCHAR *sexySourceFile)
		{
			CommonSource src;
			src.SexySourceCode = sexySourceFile;

			enum { MAX_NATIVE_SRC_LEN = 32768 };

			SEXCHAR srcCode[MAX_NATIVE_SRC_LEN];

			SEXCHAR fullPath[_MAX_PATH];
			StringPrint(fullPath, _MAX_PATH, SEXTEXT("%s%s"), srcEnvironment, sexySourceFile);		

			try
			{
				OS::LoadAsciiTextFile(srcCode, MAX_NATIVE_SRC_LEN, fullPath);
			}
			catch(Sexy::IException&)
			{
				throw;
			}

			src.Src = sexParserProxy().DuplicateSourceBuffer(srcCode, -1, Vec2i{ 0,0 }, fullPath);
			src.Tree = sexParserProxy().CreateTree(*src.Src);

			AddTree(*src.Tree);

			commonSources.push_back(src);
		}

		virtual void AddNativeLibrary(const Sexy::SEXCHAR* dynamicLinkLibOfNativeCalls)
		{
         SEXCHAR srcEnvironmentDll[_MAX_PATH];
         SafeFormat(srcEnvironmentDll, _TRUNCATE, SEXTEXT("%s%s"), srcEnvironment, dynamicLinkLibOfNativeCalls);

         FN_CreateLib create;

         create = Sexy::OS::GetLibCreateFunction(srcEnvironmentDll, false);
         if (!create) 
            create = Sexy::OS::GetLibCreateFunction(dynamicLinkLibOfNativeCalls, true);

			INativeLib* lib = create(*this);
			nativeLibs.push_back(lib);
		}

		virtual int32 GetIntrinsicModuleCount() const
		{
			return 4;
		}

		virtual void ThrowNative(int errorNumber, csexstr source, csexstr message)
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Native Error (") << source << SEXTEXT("): ") << message << std::ends;
			progObjProxy().Log().Write(streamer.str().c_str());
			progObjProxy().VirtualMachine().Throw();
		}
	};

	ID_API_CALLBACK JITCallbackId(IScriptSystem& ss)
	{
		CScriptSystem& css = (CScriptSystem&) ss;
		return css.JITCallbackId();
	}

	IProgramObject& GetProgramObject(CScript& script)
	{
		return script.Object();
	}

	const ArrayCallbacks& GetArrayCallbacks(CCompileEnvironment& ce)
	{
		CScriptSystem& css = (CScriptSystem&) GetSystem(ce.Script);
		return css.GetArrayCallbacks();
	}

	const ListCallbacks& GetListCallbacks(CCompileEnvironment& ce)
	{
		CScriptSystem& css = (CScriptSystem&) GetSystem(ce.Script);
		return css.GetListCallbacks();
	}

	const MapCallbacks& GetMapCallbacks(CCompileEnvironment& ce)
	{
		CScriptSystem& css = (CScriptSystem&) GetSystem(ce.Script);
		return css.GetMapCallbacks();
	}

	const IStructure& CCompileEnvironment::StructArray()
	{
		if (arrayStruct == NULL)
		{
			arrayStruct = Object.Common().SysNative().FindStructure(SEXTEXT("_Array"));
		}

		return *arrayStruct;
	}

	const IStructure& CCompileEnvironment::StructList()
	{
		if (listStruct == NULL)
		{
			listStruct = Object.Common().SysNative().FindStructure(SEXTEXT("_List"));
		}

		return *listStruct;
	}

	const IStructure& CCompileEnvironment::StructMap()
	{
		if (mapStruct == NULL)
		{
			mapStruct = Object.Common().SysNative().FindStructure(SEXTEXT("_Map"));
		}

		return *mapStruct;
	}
	
	CCompileEnvironment::CCompileEnvironment(CScript& script, ICodeBuilder& builder):
		Builder(builder), Script(script), arrayStruct(NULL), listStruct(NULL), mapStruct(NULL),
		RootNS(script.Object().GetRootNamespace()),
		methodMap(*(const TMapMethodToMember*) script.System().GetMethodMap()),
		SS(script.System()),
		Object(script.Object())
	{

	}

	GlobalValue* GetGlobalValue(CScript& script, csexstr buffer)
	{
		return script.GetGlobalValue(buffer);
	}
}

extern "C" SCRIPTEXPORT_API Sexy::Script::IScriptSystem* CreateScriptV_1_2_0_0(const ProgramInitParameters& pip, ILog& logger)
{
	try
	{
		CScriptSystem* ss = new CScriptSystem(pip, logger);
		return ss;
	}
	catch(Sexy::IException& ex)
	{
		SEXCHAR errLog[256];
		StringPrint(errLog, 256, SEXTEXT("Sexy CreateScriptV_1_1_0_0(...) returning NULL. Error: %d, %s."), ex.ErrorCode(), ex.Message());
		logger.Write(errLog);
		return NULL;
	}
}

namespace Sexy { namespace Script
{
	NativeCallEnvironment::NativeCallEnvironment(IPublicScriptSystem& _ss, const  Compiler::IFunction& _function, CPU& _cpu, void* _context):
		ss(_ss), function(_function), code(_function.Code()), cpu(_cpu), context(_context)
	{
	}
}}