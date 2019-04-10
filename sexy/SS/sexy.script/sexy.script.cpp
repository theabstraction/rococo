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
#include "sexy.compiler.helpers.h"
#include "sexy.s-parser.h"

#include <stdlib.h>
#include <stdarg.h>
#include <algorithm>
#include <unordered_map>
#include <list>
#include <sexy.stdstrings.h>

#include <rococo.api.h>

using namespace Rococo;
using namespace Rococo::Script;
using namespace Rococo::Compiler;
using namespace Rococo::Sex;
using namespace Rococo::VM;

namespace
{
	char defaultNativeSourcePath[256] = { 0 };

	// Careful that we had enough buffer space
	void AddSlashToDirectory(char* buffer)
	{
		// Terminate with slash

		char* s = buffer;
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

namespace Rococo
{
	namespace Script
	{
		void SetDefaultNativeSourcePath(cstr pathname)
		{
			if (pathname == nullptr)
			{
				defaultNativeSourcePath[0] = 0;
				return;
			}

			StackStringBuilder sb(defaultNativeSourcePath, 256);
			sb.AppendFormat("%s", pathname);

			// Terminate with slash
			AddSlashToDirectory(defaultNativeSourcePath);
		}
	}
}

namespace Rococo
{
	namespace OS
	{
		FN_CreateLib GetLibCreateFunction(const char* dynamicLinkLibOfNativeCalls, bool throwOnError);
	}

	namespace Script
	{
		typedef std::unordered_map<CStringKey, cstr, ::hashCStringKey> TMapMethodToMember;
	}

	namespace Compiler
	{
		INamespaceBuilder* MatchNamespace(IModuleBuilder& module, cstr name);
		IStructureBuilder* MatchStructure(ILog& logger, cstr type, IModuleBuilder& module);
	}
}

#ifdef _WIN32
# define VM_CALLBACK_CONVENTION _cdecl
#else
# define VM_CALLBACK_CONVENTION
#endif
#define VM_CALLBACK(x) void VM_CALLBACK_CONVENTION OnInvoke##x(VariantValue* registers, void* context)

namespace Rococo {
	namespace Script
	{
		const char* const THIS_POINTER_TOKEN = ("this");

	} // Script
} // Rococo

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
#include "sexy.script.exception.logic.inl"
#include "sexy.script.modules.inl"
#include "sexy.script.exceptions.inl"
#include "sexy.script.casts.inl"
#include "sexy.script.JIT.inl"
#include "sexy.script.stringbuilders.inl"

namespace Rococo
{
   namespace Script
   {
	void CopyStringTochar(char* output, size_t bufferCapacity, const char* input, size_t inputLength)
	{
		for(size_t i = 0; i < inputLength; ++i)
		{
			output[i] = input[i];
		}
	}
#ifdef char_IS_WIDE
	void CopyStringTochar(char* output, size_t bufferCapacity, cstr input, size_t inputLength)
	{
		for(size_t i = 0; i < inputLength; ++i)
		{
			output[i] = (char) input[i];
		}
	}
#endif

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
		const char* pData;
		ReadInput(0, (void*&) pData, e);

		if (pData == NULL) pData = ("<null>");
		int nullLen = StringLength(pData);
		e.ss.PublicProgramObject().Log().Write(pData);
		WriteOutput(0, nullLen, e);
	}

	typedef std::unordered_map<stdstring,NativeFunction*> TMapFQNToNativeCall;
	typedef std::list<INativeLib*> TNativeLibs;
	
	void CALLTYPE_C RouteToNative(VariantValue* registers, void* context)
	{
		NativeFunction* nf = (NativeFunction*) context;
		nf->NativeCallback(nf->e);
	}

	static const char* NativeModuleSrc = ("_NativeModule_");

	void InstallNativeCallNamespaces(IN const TMapFQNToNativeCall& nativeCalls, REF INamespaceBuilder& rootNS)
	{
		for(auto i = nativeCalls.begin(); i != nativeCalls.end(); ++i)
		{
			NativeFunction& nf = *i->second;
			NamespaceSplitter splitter(i->first.c_str());

			cstr body, publicName;
			if (!splitter.SplitTail(OUT body, OUT publicName))
			{
				char fullError[2048];
            SafeFormat(fullError, 2048, ("%s: Expecting fully qualified name A.B.C.D."), nf.Archetype.c_str());
				ParseException nativeError(Vec2i{ 0,0 }, Vec2i{ 0,0 }, NativeModuleSrc, fullError, (""), NULL);
				Throw(nativeError);
			}

			INamespaceBuilder& ns = rootNS.AddNamespace(body, ADDNAMESPACEFLAGS_CREATE_ROOTS);
		}
	}

   // This may well be one of the most CPU intensive functions where there are huge numbers of scripts that
   // need compiling per execution session. I guess having TMapFQNToNativeCall & f->TryResolveArguments() compiled once
   // globally would be a great optimization -> MAT

	void InstallNativeCalls(IN const TMapFQNToNativeCall& nativeCalls, REF INamespaceBuilder& rootNS)
	{
		for(auto i = nativeCalls.begin(); i != nativeCalls.end(); ++i)
		{
			NativeFunction& nf = *i->second;
			NamespaceSplitter splitter(i->first.c_str());

			cstr body, publicName;
			if (!splitter.SplitTail(OUT body, OUT publicName))
			{
				char fullError[2048];
            SafeFormat(fullError, 2048, ("%s: Expecting fully qualified name A.B.C.D."), nf.Archetype.c_str());
				ParseException nativeError(Vec2i{ 0,0 }, Vec2i{ 0,0 }, NativeModuleSrc, fullError, (""), NULL);
				Throw(nativeError);
			}

			INamespaceBuilder& ns = *rootNS.FindSubspace(body);

			IFunctionBuilder* f = (IFunctionBuilder*) &nf.e.function;
			ns.Alias(publicName, *f);

			if (!f->TryResolveArguments())
			{
				char fullError[2048];
            SafeFormat(fullError, 2048, ("%s: Could not resolve all arguments. Check the log."), nf.Archetype.c_str());
				ParseException nativeError(Vec2i{ 0,0 }, Vec2i{ 0,0 }, NativeModuleSrc, fullError, (""), NULL);
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
				char fullError[2048];
            SafeFormat(fullError, 2048, ("%s: %s"), nf.Archetype.c_str(), e.Message());
				ParseException nativeError(Vec2i{ 0,0 }, Vec2i{ 0,0 }, NativeModuleSrc, fullError, (""), NULL);
				Throw(nativeError);
			}							
		}
	}

	cstr GetArgTypeFromExpression(cr_sex s)
	{
		AssertCompound(s);
		AssertNotTooFewElements(s, 2);
		AssertNotTooManyElements(s, 2);

		return GetAtomicArg(s, 0).String()->Buffer;
	}

	cstr GetArgNameFromExpression(cr_sex s)
	{
		AssertCompound(s);
		AssertNotTooFewElements(s, 2);
		AssertNotTooManyElements(s, 2);

		return GetAtomicArg(s, 1).String()->Buffer;
	}

	void CALLTYPE_C OnCallbackInvoked(VariantValue* registers, void* context)
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
			char fullError[512];
         SafeFormat(fullError, 512, ("Element defined in %s had one element. Ensure that the native call spec is not encapsulated in parenthesis"), ns.FullName()->Buffer);
			Throw(archetype, fullError);
		}

		AssertNotTooFewElements(archetype, 2);

		enum {MAX_ARGS_PER_NATIVE_CALL = 40};
		AssertNotTooManyElements(archetype, MAX_ARGS_PER_NATIVE_CALL);
		
		cr_sex fnameArg = GetAtomicArg(archetype, 0);
		cstr publicName = fnameArg.String()->Buffer;

		TokenBuffer nativeName;
		StringPrint(nativeName, ("_%d_%s"), nativeCallIndex, publicName);

		int mapIndex = GetIndexOf(1, archetype, ("->"));
		if (mapIndex < 0)
		{
			char fullError[512];
         SafeFormat(fullError, 512, ("Cannot find the mapping token -> in the archetype: %s.%s"), ns.FullName()->Buffer, publicName);
			Throw(archetype, fullError);
		}

		if (checkName) AssertValidFunctionName(fnameArg);

		IFunctionBuilder& f = module.DeclareFunction(FunctionPrototype(nativeName, false), &archetype);

		for(int i = mapIndex+1; i < archetype.NumberOfElements(); ++i)
		{
			cr_sex outputDef = archetype.GetElement(i);
			cstr type = GetArgTypeFromExpression(outputDef);
			cstr name = GetArgNameFromExpression(outputDef);
			f.AddOutput(NameString::From(name), TypeString::From(type), (void*)&outputDef);
		}

		for(int i = 1; i < mapIndex; ++i)
		{
			cr_sex inputDef = archetype.GetElement(i);
			cstr type = GetArgTypeFromExpression(inputDef);
			cstr name = GetArgNameFromExpression(inputDef);
			f.AddInput(NameString::From(name), TypeString::From(type), (void*)&inputDef);
		}

		TokenBuffer fullyQualifiedName;
		StringPrint(fullyQualifiedName, ("%s.%s"), ns.FullName()->Buffer, publicName);

		NativeFunction* nf = new NativeFunction(ss, f, ss.ProgramObject().VirtualMachine().Cpu(), context);
		nf->NativeCallback = callback;
		nf->Archetype = tree.Source().SourceStart();
		nativeCalls[fullyQualifiedName.Text] = nf;
	}

	void AddNullFields(IStructureBuilder* s)
	{
		s->AddMember(NameString::From(("_typeInfo")), TypeString::From(("Pointer")));
		s->AddMember(NameString::From(("_refCount")), TypeString::From(("Int64")));
		s->AddMember(NameString::From(("_vTable1")), TypeString::From(("Pointer")));
	}

	class CScriptSystem : public IScriptSystem
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
		TNativeLibs nativeLibs;

		ID_API_CALLBACK jitId;
		ID_API_CALLBACK arrayPushId;

		char srcEnvironment[_MAX_PATH];

		TMapMethodToMember methodMap;

		typedef std::unordered_map<const Sex::ISExpression*, Script::CClassExpression*> TSReflectMap;
		TSReflectMap sreflectMap;

		void InstallNullFunction()
		{
			IFunctionBuilder& nullFunction = progObjProxy().IntrinsicModule().DeclareFunction(FunctionPrototype(("_nothing"), false), NULL);
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

		typedef std::unordered_map<cstr, CStringConstant*> TReflectedStrings;
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
		CScriptSystem(const ProgramInitParameters& pip, ILog& _logger) : progObjProxy(pip, _logger), nativeCallIndex(1), stringConstantStruct(NULL), reflectionRoot(NULL), nextId(0)
		{
			try
			{
				StackStringBuilder sb(srcEnvironment, 256);
				if (pip.NativeSourcePath != 0)
				{
					sb.AppendFormat("%s", pip.NativeSourcePath);
				}
				else if (*defaultNativeSourcePath != 0)
				{
					sb.AppendFormat("%s", defaultNativeSourcePath);
				}
				else
				{
					OS::GetEnvVariable(srcEnvironment, _MAX_PATH, ("SEXY_NATIVE_SRC_DIR"));
				}
				AddSlashToDirectory(srcEnvironment);
			}
			catch (IException& innerEx)
			{
				char message[1024];
				SafeFormat(message, sizeof(message), ("%s:\nFailed to get sexy environment.\nUse Rococo::Script::SetDefaultNativeSourcePath(...) or ProgramInitParameters or environment variable SEXY_NATIVE_SRC_DIR"), innerEx.Message());
				_logger.Write(message);
				Rococo::Throw(innerEx.ErrorCode(), ("%s"), message);
			}

			scripts = new CScripts(progObjProxy(), *this);

			nativeInt32 = &progObjProxy().AddIntrinsicStruct(("Int32"), sizeof(int32), VARTYPE_Int32, NULL);
			nativeInt64 = &progObjProxy().AddIntrinsicStruct(("Int64"), sizeof(int64), VARTYPE_Int64, NULL);
			nativeFloat32 = &progObjProxy().AddIntrinsicStruct(("Float32"), sizeof(float32), VARTYPE_Float32, NULL);
			nativeFloat64 = &progObjProxy().AddIntrinsicStruct(("Float64"), sizeof(float64), VARTYPE_Float64, NULL);
			nativeBool = &progObjProxy().AddIntrinsicStruct(("Bool"), sizeof(int32), VARTYPE_Bool, NULL);
			nativePtr = &progObjProxy().AddIntrinsicStruct(("Pointer"), sizeof(size_t), VARTYPE_Pointer, NULL);

			try
			{
				AddCommonSource(("Sys.Type.Strings.sxy")); // Module 0 -> This comes first, as module 0 is directly used to get a concrete class for string literals
				AddCommonSource(("Sys.Type.sxy")); // Module 1
				AddCommonSource(("Sys.Maths.sxy")); // Module 2			
				AddCommonSource(("Sys.Reflection.sxy")); // Module 3

				AddNativeLibrary(("Sexy.NativeLib.Reflection"));
				AddNativeLibrary(("Sexy.NativeLib.Maths"));
			}
			catch (IException& ex)
			{
				char msg[2048];
				SafeFormat(msg, sizeof(msg), "Sexy: Error reading native files: %s\nExpecting them in %s\n", ex.Message(), srcEnvironment);
				_logger.Write(msg);
				delete scripts;
				throw;
			}

			jitId = progObjProxy().VirtualMachine().Core().RegisterCallback(OnJITRoutineNeedsCompiling, this, ("OnJITRoutineNeedsCompiling"));

			arrayCallbacks.ArrayGetRefUnchecked = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayGetRefUnchecked, this, ("ArrayGetRefUnchecked"));
			arrayCallbacks.ArrayLock = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayLock, this, ("ArrayLock"));
			arrayCallbacks.ArrayUnlock = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayUnlock, this, ("ArrayUnlock"));
			arrayCallbacks.ArrayPushAndGetRef = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayPushAndGetRef, this, ("ArrayPushAndGetRef"));
			arrayCallbacks.ArrayPushByRef = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayPushByRef, this, ("ArrayPushByRef"));
			arrayCallbacks.ArrayPush32 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayPush32, this, ("ArrayPush32"));
			arrayCallbacks.ArrayPush64 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayPush64, this, ("ArrayPush64"));
			arrayCallbacks.ArrayGet32 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayGet32, this, ("ArrayGet32"));
			arrayCallbacks.ArrayGet64 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayGet64, this, ("ArrayGet64"));
			arrayCallbacks.ArrayGetMember32 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayGetMember32, this, ("ArrayGetMember32"));
			arrayCallbacks.ArrayGetMember64 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayGetMember64, this, ("ArrayGetMember64"));
			arrayCallbacks.ArrayGetByRef = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayGetByRef, this, ("ArrayGetByRef"));
			arrayCallbacks.ArrayInit = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayInit, this, ("ArrayInit"));
			arrayCallbacks.ArrayDelete = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayDelete, this, ("ArrayDelete"));
			arrayCallbacks.ArraySet32 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArraySet32, this, ("ArraySet32"));
			arrayCallbacks.ArraySet64 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArraySet64, this, ("ArraySet64"));
			arrayCallbacks.ArraySetByRef = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArraySetByRef, this, ("ArraySetByRef"));
			arrayCallbacks.ArrayPop = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayPop, this, ("ArrayPop"));
			arrayCallbacks.ArrayPopOut32 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayPopOut32, this, ("ArrayPopOut32"));
			arrayCallbacks.ArrayPopOut64 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayPopOut64, this, ("ArrayPopOut64"));
			arrayCallbacks.ArrayDestructElements = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeArrayDestructElements, this, ("ArrayDestructElements"));

			listCallbacks.ListInit = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeListInit, this, ("ListInit"));
			listCallbacks.ListAppend = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeListAppend, this, ("ListAppend"));
			listCallbacks.ListAppendAndGetRef = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeListAppendAndGetRef, this, ("ListAppendAndGetRef"));
			listCallbacks.ListAppend32 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeListAppend32, this, ("ListAppend32"));
			listCallbacks.ListAppend64 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeListAppend64, this, ("ListAppend64"));
			listCallbacks.ListPrepend = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeListPrepend, this, ("ListPrepend"));
			listCallbacks.ListPrependAndGetRef = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeListPrependAndGetRef, this, ("ListPrependAndGetRef"));
			listCallbacks.ListPrepend32 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeListPrepend32, this, ("ListPrepend32"));
			listCallbacks.ListPrepend64 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeListPrepend64, this, ("ListPrepend64"));
			listCallbacks.ListGetHead = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeListGetHead, this, ("ListGetHead"));
			listCallbacks.ListGetTail = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeListGetTail, this, ("ListGetTail"));
			listCallbacks.NodeGet32 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeNodeGet32, this, ("NodeGet32"));
			listCallbacks.NodeGet64 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeNodeGet64, this, ("NodeGet64"));
			listCallbacks.NodeGetElementRef = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeNodeGetElementRef, this, ("NodeGetElementRef"));
			listCallbacks.NodeNext = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeNodeNext, this, ("NodeNext"));
			listCallbacks.NodePrevious = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeNodePrevious, this, ("NodePrevious"));
			listCallbacks.NodeAppend = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeNodeAppend, this, ("NodeAppend"));
			listCallbacks.NodeAppend32 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeNodeAppend32, this, ("NodeAppend32"));
			listCallbacks.NodeAppend64 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeNodeAppend64, this, ("NodeAppend64"));
			listCallbacks.NodePrepend = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeNodePrepend, this, ("NodePrepend"));
			listCallbacks.NodePrepend32 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeNodePrepend32, this, ("NodePrepend32"));
			listCallbacks.NodePrepend64 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeNodePrepend64, this, ("NodePrepend64"));
			listCallbacks.NodePop = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeNodePop, this, ("NodePop"));
			listCallbacks.NodeEnumNext = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeNodeEnumNext, this, ("NodeEnumNext"));
			listCallbacks.NodeHasNext = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeNodeHasNext, this, ("NodeHasNext"));
			listCallbacks.NodeHasPrevious = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeNodeHasPrevious, this, ("NodeHasPrevious"));
			listCallbacks.NodeReleaseRef = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeNodeReleaseRef, this, ("NodeReleaseRef"));
			listCallbacks.ListClear = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeListClear, this, ("ListClear"));

			mapCallbacks.MapClear = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeMapClear, this, ("MapClear"));
			mapCallbacks.NodeEnumNext = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeMapNodeEnumNext, this, ("MapNodeEnumNext"));
			mapCallbacks.MapGetHead = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeMapGetHead, this, ("MapGetHead"));
			mapCallbacks.MapInit = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeMapInit, this, ("MapInit"));
			mapCallbacks.MapInsert32 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeMapInsert32, this, ("MapInsert32"));
			mapCallbacks.MapInsert64 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeMapInsert64, this, ("MapInsert64"));
			mapCallbacks.MapInsertValueByRef = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeMapInsertValueByRef, this, ("MapInsertValueByRef"));
			mapCallbacks.MapInsertAndGetRef = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeMapInsertAndGetRef, this, ("MapInsertAndGetRef"));
			mapCallbacks.MapTryGet = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeMapTryGet, this, ("MapTryGet"));
			mapCallbacks.MapNodeGet32 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeMapNodeGet32, this, ("MapNodeGet32"));
			mapCallbacks.MapNodeGet64 = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeMapNodeGet64, this, ("MapNodeGet64"));
			mapCallbacks.MapNodeGetRef = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeMapNodeGetRef, this, ("MapNodeGetRef"));
			mapCallbacks.MapNodePop = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeMapNodePop, this, ("MapNodePop"));
			mapCallbacks.MapNodeReleaseRef = progObjProxy().VirtualMachine().Core().RegisterCallback(OnInvokeMapNodeReleaseRef, this, ("MapNodeReleaseRef"));

			methodMap[("Capacity")] = ("_elementCapacity");
			methodMap[("Length")] = ("_length");
		}

		~CScriptSystem()
		{
			for (auto i = nativeCalls.begin(); i != nativeCalls.end(); ++i)
			{
				NativeFunction* nf = i->second;
				delete nf;
			}

			for (auto i = nativeLibs.begin(); i != nativeLibs.end(); ++i)
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

		virtual CReflectedClass* Represent(const Rococo::Compiler::IStructure& st, void* pSourceInstance)
		{
			auto i = representations.find(pSourceInstance);
			if (i == representations.end())
			{
				if (st.InterfaceCount() != 1)
				{
					LogError(progObjProxy().Log(), ("The structure '%s' must implement one and only one interface to reflect a native object"), st.Name());
					return NULL;
				}

				if (IsNullType(st))
				{
					LogError(progObjProxy().Log(), ("Null structures, including '%s', cannot be used to reflect a native object"), st.Name());
					return NULL;
				}

				CReflectedClass* rep = (CReflectedClass*)AlignedMalloc(sizeof(size_t), sizeof(CReflectedClass));
				rep->context = pSourceInstance;
				rep->header.Desc = (ObjectDesc*)st.GetVirtualTable(0);
				rep->header.refCount = 1;
				rep->header.pVTables[0] = (VirtualTable*) st.GetVirtualTable(1);

				i = representations.insert(std::make_pair(pSourceInstance, rep)).first;
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
			for (auto i = representations.begin(); i != representations.end();)
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
				Rococo::LogError(ProgramObject().Log(), ("Memory leak! %u elements found in the aligned allocation map"), alignedAllocationMap.size());
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

		void ThrowFromNativeCode(int32 errorCode, cstr staticRefMessage)
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

					size_t tooManyBytes = (size_t)data & alignment;
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
				uint8* raw = (uint8*)i->second;
				delete raw;

				alignedAllocationMap.erase(i);
			}
			else
			{
				progObjProxy().Log().Write(("Sys.Native.FreeAligned(...) was passed a pointer that is not currently defined in the aligned heap"));
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
			return i != reflectedPointers.end() ? i->second : NULL;
		}

		IStructure* GetClassFromModuleElseLog(IModuleBuilder& module, cstr className)
		{
			IStructure *s = module.FindStructure(className);
			if (s == NULL)
			{
				TokenBuffer token;
				StringPrint(token, ("Cannot find %s in module %s"), className, module.Name());
				progObjProxy().Log().Write(token);
				return NULL;
			}
			return s;
		}

		virtual CReflectedClass* CreateReflectionClass(cstr className, void* context)
		{
			IStructure* s = GetClassFromModuleElseLog(ReflectionModule(), className);

			if (s->SizeOfStruct() != sizeof(CReflectedClass))
			{
				TokenBuffer token;
				StringPrint(token, ("%s in reflection module is not equivalent to CReflectedClass"), className);
				progObjProxy().Log().Write(token);
				return NULL;
			}

			CReflectedClass* instance = (CReflectedClass*)DynamicCreateClass(*s, 0);
			reflectedPointers.insert(std::make_pair(context, instance));
			instance->context = context;
			return instance;
		}

		virtual bool ConstructExpressionBuilder(CClassExpressionBuilder& builderContainer, Rococo::Sex::ISExpressionBuilder* builder)
		{
			IStructure* s = GetClassFromModuleElseLog(ReflectionModule(), ("ExpressionBuilder"));
			if (s == NULL)
			{
				return false;
			}
			else
			{
				builderContainer.BuilderPtr = builder;
				builderContainer.Header.refCount = 1;
				builderContainer.Header.Desc = (ObjectDesc*)s->GetVirtualTable(0);
				builderContainer.Header.pVTables[0] = (VirtualTable*) s->GetVirtualTable(1);
				return true;
			}
		}

		void* DynamicCreateClass(const IStructure& s, int interfaceIndex)
		{
			int nBytes = s.SizeOfStruct();
			if (nBytes <= 0)
			{
				Rococo::Throw(0, ("The structure size was not postive"));
			}

			ObjectStub* instance = (ObjectStub*) new char[nBytes];

			if (s.Prototype().IsClass)
			{
				instance->Desc = (ObjectDesc*)s.GetVirtualTable(0);
				instance->refCount = 1;

				for (int i = 0; i < s.InterfaceCount(); ++i)
				{
					instance->pVTables[i] = (VirtualTable*)s.GetVirtualTable(i + 1);
				}
			}

			return instance;
		}

		void FreeDynamicClass(ObjectStub* header)
		{
			delete[] (char*) header;
		}

		CClassExpression* CreateReflection(cr_sex s)
		{
			IModuleBuilder& reflectionModule = ReflectionModule();
			IStructure* expressStruct = reflectionModule.FindStructure(("Expression"));
			if (expressStruct == NULL)
			{
				Rococo::Throw(0, ("Cannot find 'Expression' in the reflection module"));
			}

			CClassExpression* express = (CClassExpression*)DynamicCreateClass(*expressStruct, 0);
			express->ExpressionPtr = (Rococo::Sex::ISExpression*) &s;
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

		virtual CStringConstant* GetStringReflection(cstr s)
		{
			auto i = reflectedStrings.find(s);
			if (i == reflectedStrings.end())
			{
				const IStructure& stringConstantStruct = *SysTypeMemoModule().FindStructure(("StringConstant"));
				CStringConstant* pSC = (CStringConstant*)DynamicCreateClass(stringConstantStruct, 0);
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
				const IStructure& classStruct = *ReflectionModule().FindStructure(("ScriptSystem"));
				reflectionRoot = (CScriptSystemClass*)DynamicCreateClass(classStruct, 0);
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
			sysTypes.Alias(("Int32"), *nativeInt32);
			sysTypes.Alias(("Int64"), *nativeInt64);
			sysTypes.Alias(("Float32"), *nativeFloat32);
			sysTypes.Alias(("Float64"), *nativeFloat64);
			sysTypes.Alias(("Bool"), *nativeBool);
			sysTypes.Alias(("Pointer"), *nativePtr);

			AddNativeCall(sysTypes, Print, NULL, ("NativePrint (Sys.Type.Pointer s) -> (Int32 charCount)"), false);
		}

		const INamespace& AddNativeNamespace(cstr name)
		{
			return progObjProxy().GetRootNamespace().AddNamespace(name, Compiler::ADDNAMESPACEFLAGS_CREATE_ROOTS);
		}

		void DefineSysNative(const INamespace& sysNative)
		{
			AddNativeCall(sysNative, CreateMemoString, &memoAllocator, ("CreateMemoString (Pointer src) (Int32 srcLen) -> (Pointer dest) (Int32 destLength)"), false);
			AddNativeCall(sysNative, FreeMemoString, &memoAllocator, ("FreeMemoString (Pointer src) ->"), false);
			AddNativeCall(sysNative, DynamicCast, &stringBuilders, ("_DynamicCast (Pointer interface) (Pointer instanceRef) ->"), false);
			AddNativeCall(sysNative, CreateStringBuilder, &stringBuilders, ("CreateStringBuilder (Int32 capacity) -> (Pointer sbHandle)"), false);
			AddNativeCall(sysNative, FreeStringBuilder, &stringBuilders, ("FreeStringBuilder (Pointer sbHandle) ->"), false);
			AddNativeCall(sysNative, StringBuilderAppendIString, &stringBuilders, ("StringBuilderAppendIString (Pointer buffer) (Pointer src) (Int32 srclength) -> (Int32 newLength)"), false);
			AddNativeCall(sysNative, StringBuilderAppendInt32, &stringBuilders, ("StringBuilderAppendInt32 (Pointer buffer) (Int32 x) -> (Int32 newLength)"), false);
			AddNativeCall(sysNative, StringBuilderAppendInt64, &stringBuilders, ("StringBuilderAppendInt64 (Pointer buffer) (Int64 x) -> (Int32 newLength)"), false);
			AddNativeCall(sysNative, StringBuilderAppendFloat32, &stringBuilders, ("StringBuilderAppendFloat32 (Pointer buffer) (Float32 x) -> (Int32 newLength)"), false);
			AddNativeCall(sysNative, StringBuilderAppendFloat64, &stringBuilders, ("StringBuilderAppendFloat64 (Pointer buffer) (Float64 x) -> (Int32 newLength)"), false);
			AddNativeCall(sysNative, StringBuilderAppendBool, &stringBuilders, ("StringBuilderAppendBool (Pointer buffer) (Bool x) -> (Int32 newLength)"), false);
			AddNativeCall(sysNative, StringBuilderAppendPointer, &stringBuilders, ("StringBuilderAppendPointer (Pointer buffer) (Pointer x) -> (Int32 newLength)"), false);
			AddNativeCall(sysNative, StringBuilderClear, &stringBuilders, ("StringBuilderClear (Pointer buffer) ->"), false);
			AddNativeCall(sysNative, StringBuilderAppendAsDecimal, &stringBuilders, ("StringBuilderAppendAsDecimal (Pointer buffer) ->"), false);
			AddNativeCall(sysNative, StringBuilderAppendAsHex, &stringBuilders, ("StringBuilderAppendAsHex (Pointer buffer) -> "), false);
			AddNativeCall(sysNative, StringBuilderAppendAsSpec, &stringBuilders, ("StringBuilderAppendAsSpec (Pointer buffer) (Int32 type) -> "), false);
			AddNativeCall(sysNative, StringBuilderSetFormat, &stringBuilders, ("StringBuilderSetFormat  (Pointer buffer) (Int32 precision) (Int32 width) (Bool isZeroPrefixed) (Bool isRightAligned)->"), false);
			AddNativeCall(sysNative, StringCompare, NULL, ("StringCompare  (Pointer s) (Pointer t) -> (Int32 diff)"), false);
			AddNativeCall(sysNative, StringCompareI, NULL, ("StringCompareI  (Pointer s) (Pointer t) -> (Int32 diff)"), false);
			AddNativeCall(sysNative, StringFindLeft, NULL, ("StringFindLeft (Pointer containerBuffer) (Int32 containerLength) (Int32 startPos) (Pointer substringBuffer) (Int32 substringLength) (Bool caseIndependent)-> (Int32 position)"), false);
			AddNativeCall(sysNative, StringFindRight, NULL, ("StringFindRight (Pointer containerBuffer) (Int32 containerLength) (Int32 rightPos) (Pointer substringBuffer) (Int32 substringLength) (Bool caseIndependent)-> (Int32 position)"), false);
			AddNativeCall(sysNative, StringBuilderAppendSubstring, NULL, ("StringBuilderAppendSubstring (Pointer builder) (Pointer s) (Int32 sLen) (Int32 startPos) (Int32 charsToAppend) -> (Int32 length)"), false);
			AddNativeCall(sysNative, StringBuilderSetLength, NULL, ("StringBuilderSetLength (Pointer builder) (Int32 length) -> (Int32 newlength)"), false);
			AddNativeCall(sysNative, StringBuilderSetCase, NULL, ("StringBuilderSetCase (Pointer builder) (Int32 start) (Int32 end) (Bool toUpper)->"), false);
			AddNativeCall(sysNative, ::AlignedMalloc, this, ("AlignedMalloc (Int32 capacity) (Int32 alignment)-> (Pointer data)"), false);
			AddNativeCall(sysNative, ::AlignedFree, this, ("AlignedFree (Pointer data)->"), false);
		}

		void AddSymbol(cstr symbol, const void* ptr)
		{
			symbols.insert(std::make_pair(ptr, symbol));
		}

		virtual cstr GetSymbol(const void* ptr) const
		{
			auto i = symbols.find(ptr);
			return i == symbols.end() ? NULL : i->second.c_str();
		}

		void BuildExtraSymbols()
		{
			for (int j = 0; j < progObjProxy().ModuleCount(); ++j)
			{
				IModuleBuilder& module = progObjProxy().GetModule(j);
				for (int i = 0; i < module.StructCount(); ++i)
				{
					char symbol[256];
					IStructure& s = module.GetStructure(i);
					if (s.Prototype().IsClass)
					{
						SafeFormat(symbol, 256, ("%s-typeInfo"), s.Name());
						AddSymbol(symbol, s.GetVirtualTable(0));

						for (int k = 1; k <= s.InterfaceCount(); ++k)
						{
							SafeFormat(symbol, 256, ("%s-vTable%d"), s.Name(), k);
							AddSymbol(symbol, s.GetVirtualTable(k));
						}
					}

					SafeFormat(symbol, 256, ("typeof(%s)"), s.Name());
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

			for (auto i = reflectedStrings.begin(); i != reflectedStrings.end(); ++i)
			{
				FreeDynamicClass(&i->second->header);
			}

			reflectedStrings.clear();

			for (auto j = sreflectMap.begin(); j != sreflectMap.end(); ++j)
			{
				FreeDynamicClass(&j->second->Header);
			}

			FreeDynamicClass(&reflectionRoot->header);

			for (auto k = sreflectMap.begin(); k != sreflectMap.end(); ++k)
			{
				FreeDynamicClass(&k->second->Header);
			}

			for (auto i = alignedAllocationMap.begin(); i != alignedAllocationMap.end(); ++i)
			{
				uint8* buffer = (uint8*)i->second;
				delete buffer;
			}

			alignedAllocationMap.clear();

			for (auto i = nativeLibs.begin(); i != nativeLibs.end(); ++i)
			{
				INativeLib* lib = *i;
				lib->ClearResources();
			}
		}

		virtual void Compile()
		{
			Clear();

			INamespaceBuilder& sysTypes = progObjProxy().GetRootNamespace().AddNamespace(("Sys.Type"), ADDNAMESPACEFLAGS_CREATE_ROOTS);
			DefinePrimitives(sysTypes);

			INamespaceBuilder& sysNative = progObjProxy().GetRootNamespace().AddNamespace(("Sys.Native"), ADDNAMESPACEFLAGS_CREATE_ROOTS);
			DefineSysNative(sysNative);

			progObjProxy().ResolveNativeTypes();

			scripts->ExceptionLogic().InstallThrowHandler();

			for (auto i = nativeLibs.begin(); i != nativeLibs.end(); ++i)
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

		virtual void AddNativeCall(const Compiler::INamespace& ns, FN_NATIVE_CALL callback, void* context, cstr archetype, bool checkName)
		{
			enum { MAX_ARCHETYPE_LEN = 1024 };

			if (callback == NULL)
			{
				Rococo::Throw(0, ("ScriptSystem::AddNativeCall(...callback...): The [callback] pointer was NULL"));
			}

			if (archetype == NULL)
			{
				Rococo::Throw(0, ("ScriptSystem::AddNativeCall(...archetype...): The [archetype] pointer was NULL"));
			}

			size_t len = StringLength(archetype);
			if (len > (MAX_ARCHETYPE_LEN - 1))
			{
				Rococo::Throw(0, ("ScriptSystem::AddNativeCall(...archetype...): The [archetype] string length exceed the maximum"));
			}

			char sxArchetype[MAX_ARCHETYPE_LEN];
			CopyStringTochar(sxArchetype, MAX_ARCHETYPE_LEN, archetype, len + 1);

			char srcName[MAX_ARCHETYPE_LEN + 64];
			SafeFormat(srcName, MAX_ARCHETYPE_LEN + 64, ("Source: '%s'"), sxArchetype);
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

		static std::unordered_map<stdstring, ISParserTree*> commonGlobalSources;

		virtual void AddCommonSource(const char *sexySourceFile)
		{
			struct Anon
			{
				static void CleanupGlobalSources()
				{
					for (auto& i : commonGlobalSources)
					{
						ISourceCode& src = const_cast<ISourceCode&>(i.second->Source());
						src.Release();
						i.second->Release();
					}
				}
			};

			static bool cleanupQueued = false;
			if (!cleanupQueued)
			{
				atexit(Anon::CleanupGlobalSources);
				cleanupQueued = true;
			}

			CommonSource src;
			src.SexySourceCode = sexySourceFile;

			auto i = commonGlobalSources.find(sexySourceFile);
			if (i == commonGlobalSources.end())
			{
				enum { MAX_NATIVE_SRC_LEN = 32768 };

				char srcCode[MAX_NATIVE_SRC_LEN];

				char fullPath[_MAX_PATH];
				SafeFormat(fullPath, _MAX_PATH, ("%s%s"), srcEnvironment, sexySourceFile);

				try
				{
					OS::LoadAsciiTextFile(srcCode, MAX_NATIVE_SRC_LEN, fullPath);
				}
				catch (Rococo::IException&)
				{
					throw;
				}

				src.Src = sexParserProxy().DuplicateSourceBuffer(srcCode, -1, Vec2i{ 0,0 }, fullPath);
				src.Tree = sexParserProxy().CreateTree(*src.Src);

				commonGlobalSources.insert(std::make_pair(sexySourceFile, src.Tree));
			}
			else
			{
				src.Tree = i->second;
				src.Src = const_cast<ISourceCode*>(&i->second->Source());
			}

			AddTree(*src.Tree);

			// commonSources.push_back(src);
		}

		virtual void AddNativeLibrary(const char* dynamicLinkLibOfNativeCalls)
		{
			char srcEnvironmentDll[_MAX_PATH];
			SafeFormat(srcEnvironmentDll, sizeof(srcEnvironmentDll), ("%s%s"), srcEnvironment, dynamicLinkLibOfNativeCalls);

			FN_CreateLib create;

			create = Rococo::OS::GetLibCreateFunction(srcEnvironmentDll, false);
			if (!create)
				create = Rococo::OS::GetLibCreateFunction(dynamicLinkLibOfNativeCalls, true);

			INativeLib* lib = create(*this);
			nativeLibs.push_back(lib);
		}

		virtual int32 GetIntrinsicModuleCount() const
		{
			return 4;
		}

		virtual void ThrowNative(int errorNumber, cstr source, cstr message)
		{
			sexstringstream<1024> streamer;
			streamer.sb << ("Native Error (") << source << ("): ") << message;
			progObjProxy().Log().Write(*streamer.sb);
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
			arrayStruct = Object.Common().SysNative().FindStructure(("_Array"));
		}

		return *arrayStruct;
	}

	const IStructure& CCompileEnvironment::StructList()
	{
		if (listStruct == NULL)
		{
			listStruct = Object.Common().SysNative().FindStructure(("_List"));
		}

		return *listStruct;
	}

	const IStructure& CCompileEnvironment::StructMap()
	{
		if (mapStruct == NULL)
		{
			mapStruct = Object.Common().SysNative().FindStructure(("_Map"));
		}

		return *mapStruct;
	}
	
	CCompileEnvironment::CCompileEnvironment(CScript& script, ICodeBuilder& builder, const IFactory* _factory):
		Builder(builder), Script(script), arrayStruct(NULL), listStruct(NULL), mapStruct(NULL), factory(_factory),
		RootNS(script.Object().GetRootNamespace()),
		methodMap(*(const TMapMethodToMember*) script.System().GetMethodMap()),
		SS(script.System()),
		Object(script.Object())
	{

	}

	GlobalValue* GetGlobalValue(CScript& script, cstr buffer)
	{
		return script.GetGlobalValue(buffer);
	}

    std::unordered_map<stdstring, ISParserTree*> CScriptSystem::commonGlobalSources;
	} // Script
}//Rococo

extern "C" SCRIPTEXPORT_API Rococo::Script::IScriptSystem* CreateScriptV_1_2_0_0(const ProgramInitParameters& pip, ILog& logger)
{
	try
	{
		CScriptSystem* ss = new CScriptSystem(pip, logger);
		return ss;
	}
	catch(Rococo::IException& ex)
	{
		char errLog[256];
      SafeFormat(errLog, 256, ("Sexy CreateScriptV_1_1_0_0(...) returning NULL. Error: %d, %s."), ex.ErrorCode(), ex.Message());
		logger.Write(errLog);
		return NULL;
	}
}

namespace Rococo { namespace Script
{
	NativeCallEnvironment::NativeCallEnvironment(IPublicScriptSystem& _ss, const  Compiler::IFunction& _function, CPU& _cpu, void* _context):
		ss(_ss), function(_function), code(_function.Code()), cpu(_cpu), context(_context)
	{
	}
}}