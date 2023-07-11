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

#include "sexy.script.stdafx.h"

#include <rococo.strings.h>
#include "..\stc\stccore\sexy.compiler.helpers.h"
#include "sexy.s-parser.h"

#include <stdlib.h>
#include <stdarg.h>
#include <algorithm>
#include <unordered_map>
#include <list>
#include <sexy.stdstrings.h>

#include <rococo.api.h>
#include <rococo.sexy.api.h>
#include <rococo.os.h>

#include <rococo.package.h>

using namespace Rococo;
using namespace Rococo::Script;
using namespace Rococo::Compiler;
using namespace Rococo::Sex;
using namespace Rococo::VM;

namespace
{
	WideFilePath defaultNativeSourcePath;

	// Careful that we had enough buffer space
	void AddSlashToDirectory(wchar_t* buffer)
	{
		// Terminate with slash

		wchar_t* s = buffer;
		for (; *s != 0; s++)
		{

		}

		if (s[-1] != L'\\' && s[-1] != L'/')
		{
			s[0] = OS_DIRECTORY_SLASH;
			s[1] = 0;
		}
	}
}

namespace Rococo::OS
{
	void GetEnvVariable(wchar_t* data, size_t capacity, const wchar_t* envVariable);
}

namespace Rococo::Script
{
	void AddSysIO(IScriptSystem& ss);

	void AppendDeconstructAll(CCompileEnvironment& ce, cr_sex sequence);

	SCRIPTEXPORT_API void SetDefaultNativeSourcePath(const wchar_t* pathname)
	{
		if (pathname == nullptr)
		{
			defaultNativeSourcePath.buf[0] = 0;
			return;
		}

		Format(defaultNativeSourcePath, L"%ls", pathname);

		// Terminate with slash
		AddSlashToDirectory(defaultNativeSourcePath.buf);
	}

	ISexyPackagerSupervisor* CreatePackager(IScriptSystem& ss);
}

namespace Rococo
{
	namespace OS
	{
		FN_CreateLib GetLibCreateFunction(const wchar_t* dynamicLinkLibOfNativeCalls, bool throwOnError);
		FN_CreateLib GetLibCreateFunction(cstr origin, const void* DLLmemoryBuffer, int32 nBytesLength);
	}

	namespace Script
	{
		typedef stringmap<cstr> TMapMethodToMember;
	}

	namespace Compiler
	{
		SEXYUTIL_API INamespaceBuilder* MatchNamespace(IModuleBuilder& module, cstr name);
		SEXYUTIL_API IStructureBuilder* MatchStructure(ILog& logger, cstr type, IModuleBuilder& module);
	}

	typedef std::unordered_map<stdstring, ISParserTree*, std::hash<stdstring>, std::equal_to<stdstring>, Memory::SexyAllocator<std::pair<const stdstring, ISParserTree*>>> TMapNameToSTree;
}

#ifdef _WIN32
# define VM_CALLBACK_CONVENTION _cdecl
#else
# define VM_CALLBACK_CONVENTION
#endif
#define VM_CALLBACK(x) void VM_CALLBACK_CONVENTION OnInvoke##x(VariantValue* registers, void* context)

#ifdef __APPLE__
# define _strcmpi strcasecmp
#endif
namespace Rococo {
	namespace Script
	{
		const char* const THIS_POINTER_TOKEN = ("this");

		void DeleteMembers(IScriptSystem& ss, const IStructure& type, uint8* pInstance);

		VM_CALLBACK(Serialize)
		{
			InterfacePointer sip = (InterfacePointer)registers[5].vPtrValue;
			auto* sStub = InterfaceToInstance(sip);

			IScriptSystem* ss = (IScriptSystem*)context;

			if (sStub->Desc->TypeInfo != &ss->ProgramObject().Common().SysReflectionExpression())
			{
				Throw(0, "serialize <src> -> <target>: src must be an interface to class Expression from Sys.Reflection.sxy");
			}

			auto* cce = (CClassExpression*)sStub;

			cr_sex s = *cce->ExpressionPtr;

			auto* target = registers[7].vPtrValue;
			auto* targetType = (const IStructure*)registers[4].vPtrValue;

			uint8* targetData;

			if (targetType->InterfaceCount() == 0)
			{
				targetData = (uint8*)target;
			}
			else
			{
				auto* targetStub = InterfaceToInstance((InterfacePointer)target);
				targetData = (uint8*)targetStub;
				targetType = targetStub->Desc->TypeInfo;
			}

			cstr err = "Expecting a compound element with three atomic elements: (<type> <name> <value>)";

			if (s.NumberOfElements() != 3)
			{
				Throw(s, err);
			}

			cr_sex sType = s[0];
			cr_sex sName = s[1];
			cr_sex sValue = s[2];

			if (!IsAtomic(sType)) Throw(sType, err);
			if (!IsAtomic(sName)) Throw(sName, err);
			if (!IsAtomic(sValue) && !IsStringLiteral(sValue)) Throw(sValue, err);

			auto srcType = sType.String();
			auto srcName = sName.String();
			auto srcValue = sValue.String();

			int offset = 0;
			const IMember* member = nullptr;

			for (int i = 0; i < targetType->MemberCount(); ++i)
			{
				auto& m = targetType->GetMember(i);

				if (m.Name()[0] != '_' && _strcmpi(srcName->Buffer, m.Name()) == 0)
				{
					member = &m;
					break;
				}

				offset += m.SizeOfMember();
			}

			if (member == nullptr)
			{
				Throw(sName, "Cannot find %s in %s of %s", srcName->Buffer, targetType->Name(), targetType->Module().Name());
			}

			switch (member->UnderlyingType()->VarType())
			{
			case VARTYPE_Bool:
				if (!AreEqual(srcType, "B32")) Throw(sType, "Expecting B32 to match Bool %s in %s of %s", srcName->Buffer, targetType->Name(), targetType->Module().Name());
				{
					boolean32 bValue = AreEqual(srcValue->Buffer, "true") ? 1 : 0;
					boolean32* bTargetValue = (boolean32*)(targetData + offset);
					*bTargetValue = bValue;
				}
				break;
			case VARTYPE_Int32:
				if (!AreEqual(srcType, "I32")) Throw(sType, "Expecting I32 to match Int32 %s in %s of %s", srcName->Buffer, targetType->Name(), targetType->Module().Name());
				{
					int32 value = atoi(srcValue->Buffer);
					int32* bTargetValue = (int32*)(targetData + offset);
					*bTargetValue = value;
				}
				break;
			case VARTYPE_Int64:
				if (!AreEqual(srcType, "I64")) Throw(sType, "Expecting I64 to match Int64 %s in %s of %s", srcName->Buffer, targetType->Name(), targetType->Module().Name());
				{
					int64 value = atoll(srcValue->Buffer);
					int64* bTargetValue = (int64*)(targetData + offset);
					*bTargetValue = value;
				}
				break;
			case VARTYPE_Float32:
				if (!AreEqual(srcType, "F32")) Throw(sType, "Expecting F32 to match Float32 %s in %s of %s", srcName->Buffer, targetType->Name(), targetType->Module().Name());
				{
					float32 value = (float)atof(srcValue->Buffer);
					float32* fTargetValue = (float32*)(targetData + offset);
					*fTargetValue = value;
				}
				break;
			case VARTYPE_Float64:
				if (!AreEqual(srcType, "F64")) Throw(sType, "Expecting F64 to match Float64 %s in %s of %s", srcName->Buffer, targetType->Name(), targetType->Module().Name());
				{
					float64 value = atof(srcValue->Buffer);
					float64* fTargetValue = (float64*)(targetData + offset);
					*fTargetValue = value;
				}
				break;
			case VARTYPE_Derivative:
				if (&member->UnderlyingType()->GetInterface(0) == &ss->ProgramObject().Common().SysTypeIString())
				{
					auto** ipString = (InterfacePointer*)(targetData + offset);
					ss->ProgramObject().DecrementRefCount(*ipString);
					
					auto* sc = ss->GetStringReflection(srcValue->Buffer);
					auto* scip = (InterfacePointer)&sc->header.pVTables[0];
					ss->ProgramObject().IncrementRefCount(scip);
					*ipString = scip;
				}
				else
				{
					Throw(sName, "Member variable %s from %s of %s. Cannot serialize derivative type", member->Name(), targetType->Name(), targetType->Module().Name());
				}
				break;
			default:
				Throw(sName, "Member variable%s from %s of %s. Cannot serialize this type", member->Name(), targetType->Name(), targetType->Module().Name());
			}
		}
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
#include "sexy.script.containers.inl"
#include "sexy.script.arithmetic.expression.parser.inl"
#include "sexy.script.predicates.expression.parser.inl"
#include "sexy.script.conditional.expression.parser.inl"
#include "sexy.script.exception.logic.inl"
#include "sexy.script.modules.inl"
#include "sexy.script.exceptions.inl"
#include "sexy.script.casts.inl"
#include "sexy.script.JIT.inl"
#include "sexy.script.stringbuilders.inl"

void NativeSysOSClockHz(NativeCallEnvironment& _nce)
{
	Rococo::uint8* _sf = _nce.cpu.SF();
	ptrdiff_t _offset = 2 * sizeof(size_t);

	int64 value = Rococo::Time::TickHz();
	_offset += sizeof(value);
	WriteOutput(value, _sf, -_offset);
}

void NativeSysOSClockTicks(NativeCallEnvironment& _nce)
{
	Rococo::uint8* _sf = _nce.cpu.SF();
	ptrdiff_t _offset = 2 * sizeof(size_t);

	int64 value = Rococo::Time::TickCount();
	_offset += sizeof(value);
	WriteOutput(value, _sf, -_offset);
}

void NativeAppendCTime(NativeCallEnvironment& _nce)
{
	InterfacePointer interfacePtr;
	ReadInput(0, interfacePtr, _nce);
	auto* stub = InterfaceToInstance(interfacePtr);
	auto& ss = (IScriptSystem&)_nce.ss;

	auto* sbType = ss.ProgramObject().GetModule(0).FindStructure("FastStringBuilder");

	if (stub->Desc->TypeInfo != sbType)
	{
		Throw(0, "NativeAppendCTime -> The string builder must be a FastStringBuilder");
	}

	auto& sb = *(FastStringBuilder*)stub;

	char timestamp[26];
	Rococo::GetTimestamp(timestamp);

	int32 nMax = sb.capacity - sb.length;

	int32 chars;
	if (nMax > 0)
	{
		chars = SafeFormat(sb.buffer, (size_t)nMax, "%.24s", timestamp);
		sb.length += chars;
	}
	else
	{
		chars = 0;
	}

	WriteOutput(0, chars, _nce);
	
}

namespace Rococo::Sex
{
	IExpressionTransform* CreateExpressionTransform(cr_sex s);
}

namespace Rococo::Script
{
	IIOSystem* CreateIOSystem(IScriptSystem& ss);

	void CopyStringTochar(char* output, size_t bufferCapacity, const char* input, size_t inputLength)
	{
		for (size_t i = 0; i < inputLength; ++i)
		{
			output[i] = input[i];
		}
	}
#ifdef char_IS_WIDE
	void CopyStringTochar(char* output, size_t bufferCapacity, cstr input, size_t inputLength)
	{
		for (size_t i = 0; i < inputLength; ++i)
		{
			output[i] = (char)input[i];
		}
	}
#endif

	struct NativeFunction
	{
		NativeCallEnvironment e;
		FN_NATIVE_CALL NativeCallback;
		rstdstring Archetype;
		rstdstring sourceFile;
		int lineNumber;

		NativeFunction(IPublicScriptSystem& ss, const IFunction& f, CPU& cpu, void* context, cstr _sourceFile, int _lineNumber) :
			e(ss, f, cpu, context), sourceFile(_sourceFile), lineNumber(_lineNumber) {}
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
		ReadInput(0, (void*&)pData, e);

		if (pData == NULL) pData = ("<null>");
		int nullLen = StringLength(pData);
		e.ss.PublicProgramObject().Log().Write(pData);
		WriteOutput(0, nullLen, e);
	}

	typedef std::unordered_map<stdstring, NativeFunction*, std::hash<stdstring>, std::equal_to<stdstring>, Memory::SexyAllocator<std::pair<const stdstring, NativeFunction*>>> TMapFQNToNativeCall;
	typedef std::list<INativeLib*, Memory::SexyAllocator<INativeLib*>> TNativeLibs;

	void CALLTYPE_C RouteToNative(VariantValue* registers, void* context)
	{
		NativeFunction* nf = (NativeFunction*)context;
		nf->NativeCallback(nf->e);
	}

	static const char* NativeModuleSrc = ("_NativeModule_");

	void AppendNativeCallsAsAliases(IN const TMapFQNToNativeCall& nativeCalls, StringBuilder& sb)
	{
		sb.AppendFormat("(SexyDeclarations - containing %llu functions)\n", nativeCalls.size());

		int index = 1;
		for (auto i = nativeCalls.begin(); i != nativeCalls.end(); ++i)
		{
			cstr fqName = i->first.c_str();
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

			sb.AppendFormat("\n(function %s%d %s : (source \"%s\" %d))\n", publicName, index, nf.Archetype.c_str() + strlen(publicName), nf.sourceFile.c_str(), nf.lineNumber);
			sb.AppendFormat("(alias %s%d %s)\n", publicName, index, i->first.c_str());

			index++;
		}
	}

	void InstallNativeCallNamespaces(IN const TMapFQNToNativeCall& nativeCalls, REF INamespaceBuilder& rootNS)
	{
		for (auto i = nativeCalls.begin(); i != nativeCalls.end(); ++i)
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

	void InstallRawReflections(stringmap<RawReflectionBinding*>& rawReflections, Rococo::VM::ICore& core)
	{
		struct ANON
		{
			static void CALLTYPE_C RouteToRawReflection(VariantValue* registers, void* context)
			{
				RawReflectionBinding& reflect = *(RawReflectionBinding*)context;
				auto* ss = reinterpret_cast<IPublicScriptSystem*>(registers[VM::REGISTER_D9].vPtrValue);
				auto* sInvocation = reinterpret_cast<const ISExpression*>(registers[VM::REGISTER_D10].vPtrValue);
				auto* lhsType = reinterpret_cast<const IStructure*>(registers[VM::REGISTER_D11].vPtrValue);
				auto* lhsValue = registers[VM::REGISTER_D12].vPtrValue;
				auto* rhsName = reinterpret_cast<cstr>(registers[VM::REGISTER_D13].vPtrValue);
				auto* rhsType = reinterpret_cast<const IStructure*>(registers[VM::REGISTER_D14].vPtrValue);
				auto* rhsValue = registers[VM::REGISTER_D15].vPtrValue;

				ReflectionArguments args
				{
					*sInvocation, *lhsType, lhsValue, rhsName, *rhsType, rhsValue, *ss
				};

				reflect.fnCall(reflect.context, args);
			}
		};

		int counter = 0;

		for (auto& i : rawReflections)
		{
			cstr functionName = i.first;
			auto* binding = i.second;

			char symbol[256];
			SafeFormat(symbol, "Reflect-C++: %s%d", functionName, counter++);

			binding->callbackId = core.RegisterCallback(ANON::RouteToRawReflection, binding, symbol);
		}
	}

	// This may well be one of the most CPU intensive functions where there are huge numbers of scripts that
	// need compiling per execution session. I guess having TMapFQNToNativeCall & f->TryResolveArguments() compiled once
	// globally would be a great optimization -> MAT

	void InstallNativeCalls(IN const TMapFQNToNativeCall& nativeCalls, REF INamespaceBuilder& rootNS)
	{
		for (auto i = nativeCalls.begin(); i != nativeCalls.end(); ++i)
		{
			NativeFunction& nf = *i->second;
			NamespaceSplitter splitter(i->first.c_str());

			cstr body, publicName;
			if (!splitter.SplitTail(OUT body, OUT publicName))
			{
				char fullError[2048];
				SafeFormat(fullError, "%s: Expecting fully qualified name A.B.C.D.", nf.Archetype.c_str());
				ParseException nativeError(Vec2i{ 0,0 }, Vec2i{ 0,0 }, NativeModuleSrc, fullError, (""), NULL);
				Throw(nativeError);
			}

			INamespaceBuilder& ns = *rootNS.FindSubspace(body);

			IFunctionBuilder* f = (IFunctionBuilder*)&nf.e.function;
			ns.Alias(publicName, *f);

			if (!f->TryResolveArguments())
			{
				char fullError[2048];
				StackStringBuilder sb(fullError, sizeof(fullError));
				sb << nf.Archetype.c_str() << ": Could not resolve argument\n";
				sb << "Module: " << nf.e.function.Module().Name() << "\n";
				ParseException nativeError(Vec2i{ 0,0 }, Vec2i{ 0,0 }, NativeModuleSrc, fullError, (""), NULL);
				Throw(nativeError);
			}

			ICodeBuilder& builder = f->Builder();

			try
			{
				builder.Begin();
				ID_API_CALLBACK idCallback = rootNS.Object().VirtualMachine().Core().RegisterCallback(RouteToNative, i->second, i->first.c_str());
				builder.Assembler().Append_Invoke(idCallback);
				builder.Assembler().Append_Pop(f->GetExtraPopBytes());
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
		NativeFunction* nf = (NativeFunction*)context;
		nf->NativeCallback(nf->e);
	}

	void AddNativeCallViaTree(REF TMapFQNToNativeCall& nativeCalls, REF IModuleBuilder& module, REF IScriptSystem& ss, IN const INamespace& ns, IN FN_NATIVE_CALL callback, void* context, IN Sex::ISParserTree& tree, IN int nativeCallIndex, cstr sourceFile, int lineNumber, bool checkName, int popBytes)
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

		enum { MAX_ARGS_PER_NATIVE_CALL = 40 };
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

		TokenBuffer fullyQualifiedName;
		StringPrint(fullyQualifiedName, ("%s.%s"), ns.FullName()->Buffer, publicName);

		auto fqn = nativeCalls.find(fullyQualifiedName.Text);
		if (fqn != nativeCalls.end())
		{
			Throw(archetype, "%s: Duplicate native call name: %s", __func__, fullyQualifiedName.Text);
		}

		IFunctionBuilder& f = module.DeclareFunction(FunctionPrototype(nativeName, false), &archetype, popBytes);

		for (int i = mapIndex + 1; i < archetype.NumberOfElements(); ++i)
		{
			cr_sex outputDef = archetype.GetElement(i);
			cstr type = GetArgTypeFromExpression(outputDef);
			cstr name = GetArgNameFromExpression(outputDef);
			f.AddOutput(NameString::From(name), TypeString::From(type), (void*)&outputDef);
		}

		for (int i = 1; i < mapIndex; ++i)
		{
			cr_sex inputDef = archetype.GetElement(i);
			cstr type = GetArgTypeFromExpression(inputDef);
			cstr name = GetArgNameFromExpression(inputDef);
			f.AddInput(NameString::From(name), TypeString::From(type), (void*)&inputDef);
		}

		NativeFunction* nf = new NativeFunction(ss, f, ss.ProgramObject().VirtualMachine().Cpu(), context, sourceFile, lineNumber);
		nf->NativeCallback = callback;
		nf->Archetype = tree.Source().SourceStart();

		nativeCalls.insert(std::make_pair(rstdstring(fullyQualifiedName.Text), nf));
	}

	void AddNullFields(IStructureBuilder* s)
	{
		s->AddMember(NameString::From(("_typeInfo")), TypeString::From(("Pointer")));
		s->AddMember(NameString::From(("_refCount")), TypeString::From(("Int64")));
		s->AddMember(NameString::From(("_vTable1")), TypeString::From(("Pointer")));
	}

	const IStructure& GetTypeForSource(IProgramObject& po, cstr concreteType, cstr sourceFile)
	{
		for (int i = 0; i < po.ModuleCount(); ++i)
		{
			auto& m = po.GetModule(i);
			if (Eq(m.Name(), sourceFile))
			{
				auto* type = m.FindStructure(concreteType);
				if (!type)
				{
					Throw(0, "No object found for %s of \"%s\". Source found, but type unrecognized", concreteType, sourceFile);
				}
				return *type;
			}
		}

		if (Eq(sourceFile, "!Intrinsics!"))
		{
			auto* type = po.IntrinsicModule().FindStructure(concreteType);
			if (!type)
			{
				Throw(0, "No object found for %s of \"%s\". Source found, but type unrecognized", concreteType, sourceFile);
			}
			return *type;
		}

		Throw(0, "No universal object found for %s of \"%s\". Source not found", concreteType, sourceFile);
	}
}

namespace Rococo::Script
{
	class CScriptSystem : public IScriptSystem
	{
	private:
		TMemoAllocator memoAllocator;
		AutoFree<IProgramObject> progObjProxy;
		Auto<ISParser> sexParserProxy;
		CScripts* scripts;
		ScriptCallbacks callbacks;

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
		ID_API_CALLBACK serializeId;

		WideFilePath srcEnvironment;

		TMapMethodToMember methodMap;

		typedef std::unordered_map<const Sex::ISExpression*, Script::CClassExpression*, std::hash<const Sex::ISExpression*>, std::equal_to<const Sex::ISExpression*>, Memory::SexyAllocator<std::pair<const Sex::ISExpression* const, Script::CClassExpression*>>> TSReflectMap;
		TSReflectMap sreflectMap;

		struct CharSequence
		{
			std::vector<char, Memory::SexyAllocator<char>> buffer;
		};

		std::vector<CharSequence, Memory::SexyAllocator<CharSequence>> persistentStrings;

		AutoFree<ISexyPackagerSupervisor> packager;

		bool hasNullFunction = false;

		void InstallNullFunction()
		{
			if (!hasNullFunction)
			{
				IFunctionBuilder& nullFunction = progObjProxy->IntrinsicModule().DeclareFunction(FunctionPrototype(("_nothing"), false), NULL);
				nullFunction.Builder().Begin();
				nullFunction.Builder().End();
				nullFunction.Builder().Assembler().Clear();
				hasNullFunction = true;
			}
		}

		IStructure* stringConstantStruct;

		IModuleBuilder& ReflectionModule()
		{
			return this->progObjProxy->GetModule(3);
		}

		IModuleBuilder& ReflectionModule() const
		{
			return const_cast<CScriptSystem&>(*this).ReflectionModule();
		}

		IModule& SysTypeMemoModule()
		{
			return this->progObjProxy->GetModule(0);
		}

		IIOSystem& IOSystem()
		{
			if (!ioSystem) Throw(0, "IOSystem has not been initialized");
			return *ioSystem;
		}

		std::vector<HString> commandLineArgs;

		void SetCommandLine(int argc, char* argv[]) override
		{
			commandLineArgs.reserve(argc);
			for (int i = 0; i < argc; i++)
			{
				commandLineArgs.push_back(HString(argv[i]));
			}
		}

		cstr GetCommandLineArg(int i) override
		{
			if (i < 0 || i >= (int32) commandLineArgs.size())
			{
				return "";
			}
			else
			{
				return commandLineArgs[i];
			}
		}

		typedef std::unordered_map<cstr, CStringConstant*, std::hash<cstr>, std::equal_to<cstr>, Memory::SexyAllocator<std::pair<cstr const, CStringConstant*>>> TReflectedStrings;
		TReflectedStrings reflectedStrings;

		typedef std::unordered_map<const void*, stdstring, std::hash<const void*>, std::equal_to<const void*>, Memory::SexyAllocator<std::pair<const void* const, stdstring>>> TSymbols;
		TSymbols symbols;

		typedef std::unordered_map<void*, CReflectedClass*, std::hash<void*>, std::equal_to<void*>, Memory::SexyAllocator<std::pair<void* const, CReflectedClass*>>> TReflectedPointers;
		TReflectedPointers reflectedPointers;
		TReflectedPointers representations;

		CScriptSystemClass* reflectionRoot;

		ArrayCallbacks arrayCallbacks;
		ListCallbacks listCallbacks;
		MapCallbacks mapCallbacks;

		TMapNameToSTree& nativeSources;
		int nextId;

		TTransformMap mapExpressionToTransform;

		bool usesSysIO;

		AutoFree<IIOSystem> ioSystem;
	public:
		CScriptSystem(
			TMapNameToSTree& _nativeSources, 
			const ProgramInitParameters& pip, 
			ILog& _logger) :

			progObjProxy(CreateProgramObject_1_0_0_0(pip, _logger)), 
			nativeCallIndex(1), 
			stringConstantStruct(NULL),
			reflectionRoot(NULL), 
			nextId(0), 
			nativeSources(_nativeSources), 
			sexParserProxy(Sexy_CreateSexParser_2_0(Memory::GetSexyAllocator())),
			packager(CreatePackager(*this)),
			usesSysIO(pip.addIO)
		{
			try
			{
				if (pip.NativeSourcePath != 0)
				{
					Format(srcEnvironment, L"%ls", pip.NativeSourcePath);
				}
				else if (*defaultNativeSourcePath != 0)
				{
					Format(srcEnvironment, L"%ls", defaultNativeSourcePath.buf);
				}
				else
				{
					OS::GetEnvVariable(srcEnvironment.buf, _MAX_PATH, L"SEXY_NATIVE_SRC_DIR");
				}
				AddSlashToDirectory(srcEnvironment.buf);
			}
			catch (IException& innerEx)
			{
				char message[1024];
				SafeFormat(message, sizeof(message), "%hs:\nFailed to get sexy environment.\nUse Rococo::Script::SetDefaultNativeSourcePath(...) or ProgramInitParameters or environment variable SEXY_NATIVE_SRC_DIR", innerEx.Message());
				_logger.Write(message);
				Rococo::Throw(innerEx.ErrorCode(), "%s", message);
			}

			scripts = new CScripts(*progObjProxy, *this);

			nativeInt32 = &progObjProxy->AddIntrinsicStruct("Int32", sizeof(int32), VARTYPE_Int32, NULL);
			nativeInt64 = &progObjProxy->AddIntrinsicStruct("Int64", sizeof(int64), VARTYPE_Int64, NULL);
			nativeFloat32 = &progObjProxy->AddIntrinsicStruct("Float32", sizeof(float32), VARTYPE_Float32, NULL);
			nativeFloat64 = &progObjProxy->AddIntrinsicStruct("Float64", sizeof(float64), VARTYPE_Float64, NULL);
			nativeBool = &progObjProxy->AddIntrinsicStruct("Bool", sizeof(int32), VARTYPE_Bool, NULL);
			nativePtr = &progObjProxy->AddIntrinsicStruct("Pointer", sizeof(size_t), VARTYPE_Pointer, NULL);

			try
			{
				AddCommonSource("Sys.Type.Strings.sxy"); // Module 0 -> This comes first, as module 0 is directly used to get a concrete class for string literals
				AddCommonSource("Sys.Type.sxy"); // Module 1
				AddCommonSource("Sys.Maths.sxy"); // Module 2			
				AddCommonSource("Sys.Reflection.sxy"); // Module 3

				if (pip.addIO)
				{
					AddCommonSource("Sys.IO.sxy"); // Module 4
				}

#ifdef _WIN32
				bool useDebug = pip.useDebugLibs;
#else
				bool useDebug = false; // APPLE build script doesn't generate debug extension, so just use whatever files have been built
#endif

				AddNativeLibrary(useDebug ? "sexy.nativelib.reflection.debug" : "sexy.nativelib.reflection");
				AddNativeLibrary(useDebug ? "sexy.nativelib.maths.debug" : "sexy.nativelib.maths");

				if (pip.addCoroutineLib)
				{
					AddNativeLibrary(useDebug ? "sexy.nativelib.coroutines.debug" : "sexy.nativelib.coroutines");
				}
			}
			catch (ParseException& ex)
			{
				char msg[2048];
				SafeFormat(msg, "Sexy: Error reading native files %ls\n", srcEnvironment.buf);
				_logger.Write(msg);

				SafeFormat(msg, "Mesage: %d\n", ex.Message());
				SafeFormat(msg, "%s (%d,%d) to (%d,%d)\n", ex.Name(), ex.Start().x, ex.Start().y, ex.End().x, ex.End().y);
				_logger.Write(msg);

				delete scripts;
				throw;
			}
			catch (IException& ex)
			{
				char msg[2048];
				SafeFormat(msg, sizeof(msg), "Sexy: Error reading native files: %s\nExpecting them in %ls\n", ex.Message(), srcEnvironment.buf);
				_logger.Write(msg);
				delete scripts;
				throw;
			}

			auto& core = progObjProxy->VirtualMachine().Core();

			jitId = core.RegisterCallback(Compile_JIT, this, "Compile_JIT");

			arrayCallbacks.ArrayAssign = core.RegisterCallback(OnInvokeArrayAssign, this, "ArrayAssign");
			arrayCallbacks.ArrayGetLastIndexToD12 = core.RegisterCallback(OnInvokeArrayGetLastIndexToD12, this, "ArrayGetLastIndexToD12");
			arrayCallbacks.ArrayGetRefUnchecked = core.RegisterCallback(OnInvokeArrayGetRefUnchecked, this, "ArrayGetRefUnchecked");
			arrayCallbacks.ArrayLock = core.RegisterCallback(OnInvokeArrayLock, this, "ArrayLock");
			arrayCallbacks.ArrayUnlock = core.RegisterCallback(OnInvokeArrayUnlock, this, "ArrayUnlock");
			arrayCallbacks.ArrayClear = core.RegisterCallback(OnInvokeArrayClear, this, "ArrayClear");
			arrayCallbacks.ArrayPushAndGetRef = core.RegisterCallback(OnInvokeArrayPushAndGetRef, this, "ArrayPushAndGetRef");
			arrayCallbacks.ArrayPushByRef = core.RegisterCallback(OnInvokeArrayPushByRef, this, "ArrayPushByRef");
			arrayCallbacks.ArrayUpdateRefCounts = core.RegisterCallback(OnInvokeArrayUpdateRefCounts, this, "ArrayUpdateRefCounts");
			arrayCallbacks.ArrayPush32 = core.RegisterCallback(OnInvokeArrayPush32, this, "ArrayPush32");
			arrayCallbacks.ArrayPushInterface = core.RegisterCallback(OnInvokeArrayPushInterface, this, "ArrayPushInterface");
			arrayCallbacks.ArrayPush64 = core.RegisterCallback(OnInvokeArrayPush64, this, "ArrayPush64");
			arrayCallbacks.ArrayGet32 = core.RegisterCallback(OnInvokeArrayGet32, this, "ArrayGet32");
			arrayCallbacks.ArrayGet64 = core.RegisterCallback(OnInvokeArrayGet64, this, "ArrayGet64");
			arrayCallbacks.ArrayGetMember32 = core.RegisterCallback(OnInvokeArrayGetMember32, this, "ArrayGetMember32");
			arrayCallbacks.ArrayGetMember64 = core.RegisterCallback(OnInvokeArrayGetMember64, this, "ArrayGetMember64");
			arrayCallbacks.ArrayCopyByRef = core.RegisterCallback(OnInvokeArrayCopyByRef, this, "ArrayCopyByRef");
			arrayCallbacks.ArrayInit = core.RegisterCallback(OnInvokeArrayInit, this, "ArrayInit");
			arrayCallbacks.ArrayRelease = core.RegisterCallback(OnInvokeArrayReleaseRef, this, "ArrayReleaseRef");
			arrayCallbacks.ArraySet32 = core.RegisterCallback(OnInvokeArraySet32, this, "ArraySet32");
			arrayCallbacks.ArraySet64 = core.RegisterCallback(OnInvokeArraySet64, this, "ArraySet64");
			arrayCallbacks.ArraySetByRef = core.RegisterCallback(OnInvokeArraySetByRef, this, "ArraySetByRef");
			arrayCallbacks.ArrayPop = core.RegisterCallback(OnInvokeArrayPop, this, "ArrayPop");
			arrayCallbacks.ArrayPopOut32 = core.RegisterCallback(OnInvokeArrayPopOut32, this, "ArrayPopOut32");
			arrayCallbacks.ArrayPopOut64 = core.RegisterCallback(OnInvokeArrayPopOut64, this, "ArrayPopOut64");
			arrayCallbacks.ArrayDestructElements = core.RegisterCallback(OnInvokeArrayDestructElements, this, "ArrayDestructElements");
			arrayCallbacks.ArrayGetInterfaceUnchecked = core.RegisterCallback(OnInvokeArrayGetInterfaceUnchecked, this, "ArrayGetInterface");
			arrayCallbacks.ArrayGetLength = core.RegisterCallback(OnInvokeArrayGetLength, this, "ArrayGetLength");
			arrayCallbacks.ArrayGetLastIndex = core.RegisterCallback(OnInvokeArrayGetLastIndex, this, "ArrayGetLastIndex");
			arrayCallbacks.ArrayReturnLength = core.RegisterCallback(OnInvokeArrayReturnLength, this, "ArrayReturnLength");
			arrayCallbacks.ArrayReturnCapacity = core.RegisterCallback(OnInvokeArrayReturnCapacity, this, "ArrayReturnCapacity");

			listCallbacks.ListInit = core.RegisterCallback(OnInvokeListInit, this, "ListInit");
			listCallbacks.ListAppend = core.RegisterCallback(OnInvokeListAppend, this, "ListAppend");
			listCallbacks.ListAppendAndGetRef = core.RegisterCallback(OnInvokeListAppendAndGetRef, this, "ListAppendAndGetRef");
			listCallbacks.ListAppend32 = core.RegisterCallback(OnInvokeListAppend32, this, "ListAppend32");
			listCallbacks.ListAppend64 = core.RegisterCallback(OnInvokeListAppend64, this, "ListAppend64");
			listCallbacks.ListAppendInterface = core.RegisterCallback(OnInvokeListAppendInterface, this, "ListAppendInterface");
			listCallbacks.ListPrepend = core.RegisterCallback(OnInvokeListPrepend, this, "ListPrepend");
			listCallbacks.ListPrependAndGetRef = core.RegisterCallback(OnInvokeListPrependAndGetRef, this, "ListPrependAndGetRef");
			listCallbacks.ListPrepend32 = core.RegisterCallback(OnInvokeListPrepend32, this, "ListPrepend32");
			listCallbacks.ListPrepend64 = core.RegisterCallback(OnInvokeListPrepend64, this, "ListPrepend64");
			listCallbacks.ListPrependInterface = core.RegisterCallback(OnInvokeListPrependInterface, this, "ListPrependInterface");
			listCallbacks.ListGetHead = core.RegisterCallback(OnInvokeListGetHead, this, "ListGetHead");
			listCallbacks.ListGetTail = core.RegisterCallback(OnInvokeListGetTail, this, "ListGetTail");
			listCallbacks.ListGetLength = core.RegisterCallback(OnInvokeListGetLength, this, "ListGetLength");
			listCallbacks.NodeGet32 = core.RegisterCallback(OnInvokeNodeGet32, this, "NodeGet32");
			listCallbacks.NodeGet64 = core.RegisterCallback(OnInvokeNodeGet64, this, "NodeGet64");
			listCallbacks.NodeGetInterface = core.RegisterCallback(OnInvokeNodeGetInterface, this, "NodeGetInterface");
			listCallbacks.NodeGetElementRef = core.RegisterCallback(OnInvokeNodeGetElementRef, this, "NodeGetElementRef");
			listCallbacks.NodeNext = core.RegisterCallback(OnInvokeNodeNext, this, "NodeNext");
			listCallbacks.NodePrevious = core.RegisterCallback(OnInvokeNodePrevious, this, "NodePrevious");
			listCallbacks.NodeAppend = core.RegisterCallback(OnInvokeNodeAppend, this, "NodeAppend");
			listCallbacks.NodeAppendInterface = core.RegisterCallback(OnInvokeNodeAppendInterface, this, "NodeAppendInterface");
			listCallbacks.NodeAppend32 = core.RegisterCallback(OnInvokeNodeAppend32, this, "NodeAppend32");
			listCallbacks.NodeAppend64 = core.RegisterCallback(OnInvokeNodeAppend64, this, "NodeAppend64");
			listCallbacks.NodePrepend = core.RegisterCallback(OnInvokeNodePrepend, this, "NodePrepend");
			listCallbacks.NodePrependInterface = core.RegisterCallback(OnInvokeNodePrependInterface, this, "NodePrependInterface");
			listCallbacks.NodePrepend32 = core.RegisterCallback(OnInvokeNodePrepend32, this, "NodePrepend32");
			listCallbacks.NodePrepend64 = core.RegisterCallback(OnInvokeNodePrepend64, this, "NodePrepend64");
			listCallbacks.NodePop = core.RegisterCallback(OnInvokeNodePop, this, "NodePop");
			listCallbacks.NodeEnumNext = core.RegisterCallback(OnInvokeNodeEnumNext, this, "NodeEnumNext");
			listCallbacks.NodeHasNext = core.RegisterCallback(OnInvokeNodeHasNext, this, "NodeHasNext");
			listCallbacks.NodeHasPrevious = core.RegisterCallback(OnInvokeNodeHasPrevious, this, "NodeHasPrevious");
			listCallbacks.NodeReleaseRef = core.RegisterCallback(OnInvokeNodeReleaseRef, this, "NodeReleaseRef");
			listCallbacks.ListRelease = core.RegisterCallback(OnInvokeListRelease, this, "ListRelease");
			listCallbacks.ListClear = core.RegisterCallback(OnInvokeListClear, this, "ListClear");
			listCallbacks.ListAssign = core.RegisterCallback(OnInvokeListAssign, this, "ListAssign");
			listCallbacks.NodeGoPrevious = core.RegisterCallback(OnInvokeNodeGoPrevious, this, "NodeGoPrevious");
			listCallbacks.NodeGoNext = core.RegisterCallback(OnInvokeNodeGoNext, this, "NodeGoNext");

			mapCallbacks.MapAssign = core.RegisterCallback(OnInvokeMapAssign, this, "MapAssign");
			mapCallbacks.MapRelease = core.RegisterCallback(OnInvokeMapRelease, this, "MapRelease");
			mapCallbacks.NodeEnumNext = core.RegisterCallback(OnInvokeMapNodeEnumNext, this, "MapNodeEnumNext");
			mapCallbacks.MapGetHead = core.RegisterCallback(OnInvokeMapGetHead, this, "MapGetHead");
			mapCallbacks.MapInit = core.RegisterCallback(OnInvokeMapInit, this, "MapInit");
			mapCallbacks.DoesMapNodeExist = core.RegisterCallback(OnInvokeDoesMapNodeExist, this, "DoesMapNodeExist");
			mapCallbacks.MapInsert32 = core.RegisterCallback(OnInvokeMapInsert32, this, "MapInsert32");
			mapCallbacks.MapInsert64 = core.RegisterCallback(OnInvokeMapInsert64, this, "MapInsert64");
			mapCallbacks.MapInsertValueByRef = core.RegisterCallback(OnInvokeMapInsertValueByRef, this, "MapInsertValueByRef");
			mapCallbacks.MapInsertAndGetRef = core.RegisterCallback(OnInvokeMapInsertAndGetRef, this, "MapInsertAndGetRef");
			mapCallbacks.MapInsertInterface = core.RegisterCallback(OnInvokeMapInsertInterface, this, "MapInsertInterface");
			mapCallbacks.MapTryGet = core.RegisterCallback(OnInvokeMapTryGet, this, "MapTryGet");
			mapCallbacks.MapNodeGet32 = core.RegisterCallback(OnInvokeMapNodeGet32, this, "MapNodeGet32");
			mapCallbacks.MapNodeGet64 = core.RegisterCallback(OnInvokeMapNodeGet64, this, "MapNodeGet64");
			mapCallbacks.MapNodeGetInterface = core.RegisterCallback(OnInvokeMapNodeGetInterface, this, "MapNodeGetInterface");
			mapCallbacks.MapNodeGetKey32 = core.RegisterCallback(OnInvokeMapNodeGetKey32, this, "MapNodeGetKey32");
			mapCallbacks.MapNodeGetKey64 = core.RegisterCallback(OnInvokeMapNodeGetKey64, this, "MapNodeGetKey64");
			mapCallbacks.MapNodeGetKeyIString = core.RegisterCallback(OnInvokeMapNodeGetKeyIString, this, "MapNodeGetKeyIString");
			mapCallbacks.MapNodeGetRef = core.RegisterCallback(OnInvokeMapNodeGetRef, this, "MapNodeGetRef");
			mapCallbacks.MapNodePop = core.RegisterCallback(OnInvokeMapNodePop, this, "MapNodePop");
			mapCallbacks.MapNodeReleaseRef = core.RegisterCallback(OnInvokeMapNodeReleaseRef, this, "MapNodeReleaseRef");
			mapCallbacks.MapGetLength = core.RegisterCallback(OnInvokeMapGetLength, this, "MapGetLength");
			mapCallbacks.MapUpdateRefCounts = core.RegisterCallback(OnInvokeMapUpdateRefCounts, this, "MapUpdateRefCounts");

			callbacks.idThrowNullRef = core.RegisterCallback(OnInvokeThrowNullRef, this, "ThrowNullRef");
			callbacks.idTestD4neqD5_retBoolD7 = core.RegisterCallback(OnInvokeTestD4neqD5_retBoolD7, &ProgramObject().VirtualMachine(), "TestD4neqD5_retBoolD7");
			callbacks.idYieldMicroseconds = core.RegisterCallback(OnInvokeYieldMicroseconds, &ProgramObject().VirtualMachine(), "YieldMicroseconds");
			callbacks.idDynamicDispatch = core.RegisterCallback(OnInvokeDynamicDispatch, this, "Dispatch");
			callbacks.idIsSameObject = core.RegisterCallback(OnInvokeIsSameObject, this, "IsSameObject");
			callbacks.idIsDifferentObject = core.RegisterCallback(OnInvokeIsDifferentObject, this, "IsDifferentObject");
			callbacks.idStringIndexToChar = core.RegisterCallback(OnInvokeStringIndexToChar, this, "StringIndexToChar");
			callbacks.idTransformAt_D4D5retIExpressionBuilderD7 = core.RegisterCallback(OnInvokeTransformAt_D4D5retD7, this, "TransformAt");
			callbacks.idTransformParent_D4retIExpressionBuilderD7 = core.RegisterCallback(OnInvokeTransformParent_D4retD7, this, "TransformParent");
			methodMap[("Capacity")] = ("_elementCapacity");
			methodMap[("Length")] = ("_length");
			serializeId = core.RegisterCallback(OnInvokeSerialize, this, "serialize");
		}

		ISExpressionBuilder* CreateMacroTransform(cr_sex src) override
		{
			auto i = mapExpressionToTransform.find(&src);
			if (i != mapExpressionToTransform.end())
			{
				return &i->second.transform->Root();
			}
			else
			{
				TransformData td;
				td.transform = Rococo::Sex::CreateExpressionTransform(src);
				mapExpressionToTransform[&src] = td;
				return &td.transform->Root();
			}
		}

		ISExpressionBuilder* CreateMacroTransformClonedFromParent(cr_sex sChild) override
		{
			auto* pParent = sChild.Parent();
			if (pParent == nullptr)
			{
				Throw(sChild, "%s: expression has no parent", __FUNCTION__);
			}

			cr_sex sParent = *pParent;

			const ISExpression* sTParent = GetTransform(sParent);
			if (!sTParent)
			{
				ISExpressionBuilder* sParentTransform = CreateMacroTransform(sParent);

				for (int i = 0; i < sParent.NumberOfElements(); ++i)
				{
					auto& sElement = sParent[i];

					const ISExpression* sTElement = GetTransform(sElement);
					if (sTElement)
					{
						sParentTransform->AddRef(*sTElement);
					}
					else
					{
						sParentTransform->AddRef(sElement);
					}
				}

				return sParentTransform;
			}

			return (ISExpressionBuilder*)sTParent;
		}

		const ISExpression* GetTransform(cr_sex src) override
		{
			auto i = mapExpressionToTransform.find(&src);
			return i == mapExpressionToTransform.end() ? nullptr : &i->second.transform->Root();
		}

		const ScriptCallbacks& GetScriptCallbacks() override
		{
			return callbacks;
		}

		~CScriptSystem();

		const IStructure& GetTypeForSource(cstr concreteType, cstr sourceFile)
		{
			return ::GetTypeForSource(*progObjProxy, concreteType, sourceFile);
		}

		ObjectStub* CreateScriptObject(cstr instanceType, cstr instanceSource) override
		{
			if (StartsWith(instanceType, "_Null"))
			{
				InterfacePointer ip = GetUniversalNullObject(instanceType, instanceSource);
				return InterfaceToInstance(ip);
			}

			const IStructure& type = GetTypeForSource(instanceType, instanceSource);

			int allocSize = type.SizeOfStruct();

			auto& allocator = progObjProxy->GetDefaultObjectAllocator();

			auto* object = (ObjectStub*) allocator.AllocateObject(allocSize);
			memset(object, 0, allocSize);

			object->Desc = (ObjectDesc*)type.GetVirtualTable(0);
			object->refCount = 1;

			int nInterfaces = type.InterfaceCount();
			for (int i = 0; i < nInterfaces; ++i)
			{
				object->pVTables[i] = (VirtualTable*)type.GetVirtualTable(i + 1);
			}

			return object;
		}

		ObjectStub* CreateScriptObjectDirect(size_t sizeofObject, const IStructure& compilersView) override
		{
			int allocSize = compilersView.SizeOfStruct();
			if (allocSize > sizeofObject)
			{
				Throw(0, "%s: the supplied size of %llu bytes was insufficient to allocate objects of type %s", __FUNCTION__, sizeofObject, GetFriendlyName(compilersView));
			}

			auto& allocator = progObjProxy->GetDefaultObjectAllocator();

			auto* object = (ObjectStub*)allocator.AllocateObject(sizeofObject);
			memset(object, 0, sizeofObject);

			object->Desc = (ObjectDesc*)compilersView.GetVirtualTable(0);
			object->refCount = 1;

			int nInterfaces = compilersView.InterfaceCount();
			for (int i = 0; i < nInterfaces; ++i)
			{
				object->pVTables[i] = (VirtualTable*)compilersView.GetVirtualTable(i + 1);
			}

			return object;
		}

		ArrayImage* CreateArrayImage(const IStructure& elementType) override
		{
			return Rococo::Script::CreateArrayImage(*this, elementType, 0);
		}

		MapImage* CreateMapImage(const IStructure& keyType, const IStructure& valueType) override
		{
			return Rococo::Script::CreateMapImage(*this, keyType, valueType);
		}

		ListImage* CreateListImage(const IStructure& valueType) override
		{
			return Rococo::Script::CreateListImage(*this, valueType);
		}

		uint8* AppendListNode(ListImage& image) override
		{
			return Rococo::Script::AppendListNode(*this, image);
		}

		InterfacePointer GetUniversalNullObject(cstr instanceType, cstr instanceSource) override
		{
			if (!StartsWith(instanceType, "_Null"))
			{
				Throw(0, "No universal object found for %s of \"%s\". instance type was not a universal null object", instanceType, instanceSource);
			}

			const IStructure& type = GetTypeForSource(instanceType, instanceSource);
			if (type.InterfaceCount() < 1)
			{
				Throw(0, "No universal object found for %s of \"%s\". Interface count was zero", instanceType, instanceSource);
			}
			ObjectStub* stub = type.GetInterface(0).UniversalNullInstance();
			return stub->pVTables;
		}

		void RegisterPackage(IPackage* package) override
		{
			packager->RegisterNamespacesInPackage(package);
		}

		void LoadSubpackages(cstr namespaceFilter, cstr packageName) override
		{
			packager->LoadSubpackages(namespaceFilter, packageName);
		}

		ID_API_CALLBACK GetIdSerializeCallback() const override
		{
			return serializeId;
		}

		IStringPool* stringPool = nullptr;

		IStringPool& StringPool()
		{
			return *stringPool;
		}

		const IStructure* GetStringBuilderType() const override
		{
			return stringPool->FastStringBuilderType();
		}

		cstr GetPersistentString(cstr text, int textLength = -1) override
		{
			if (textLength < 0) textLength = StringLength(text);

			CharSequence cs;
			cs.buffer.resize((size_t)(textLength + 1));
			memcpy_s(cs.buffer.data(), cs.buffer.size(), text, textLength);
			cs.buffer[textLength] = 0;

			persistentStrings.push_back(cs);

			return persistentStrings.back().buffer.data();
		}

		typedef std::unordered_map<rstdstring, MethodInfo, std::hash<rstdstring>, std::equal_to<rstdstring>, Memory::SexyAllocator<std::pair<const rstdstring, MethodInfo >>> TMapNameToMethod;
		typedef std::unordered_map<const Rococo::Compiler::IStructure*, TMapNameToMethod, std::hash<const Rococo::Compiler::IStructure*>, std::equal_to<const Rococo::Compiler::IStructure*>, Memory::SexyAllocator<std::pair<const Rococo::Compiler::IStructure* const, TMapNameToMethod>>> TMapTypeToMethodMap;
		TMapTypeToMethodMap typeToMethodMap;

		TMapTypeToMethodMap::iterator CacheAllMethods(const Rococo::Compiler::IStructure& concreteClassType)
		{
			TMapNameToMethod nameToMethod;
			auto& module = concreteClassType.Module();

			for (int j = 0; j < module.FunctionCount(); j++)
			{
				auto& f = module.GetFunction(j);
				auto* ftype = f.GetType();
				if (ftype == &concreteClassType)
				{
					NamespaceSplitter splitter(f.Name());
					cstr instance, method;
					splitter.SplitTail(instance, method);

					if (AreEqual(method, "Construct") || AreEqual(method, "Destruct"))
					{
						// do not allow cache these methods
					}
					else
					{
						ptrdiff_t offset;

						int index = GetInterfaceImplementingMethod(concreteClassType, method);
						if (index < 0)
						{				
							// method does not contribute to an implementation of an interface. so use instance
							offset = 0;
						}
						else
						{
							offset = ObjectStub::BYTECOUNT_INSTANCE_TO_INTERFACE0 + sizeof(VirtualTable*) * index;
						}

						nameToMethod[method] = MethodInfo{ &f, offset };
					}
				}
			}

			return typeToMethodMap.insert(std::make_pair(&concreteClassType, nameToMethod)).first;
		}

		const MethodInfo GetMethodByName(cstr methodName, const Rococo::Compiler::IStructure& concreteClassType) override
		{
			auto i = typeToMethodMap.find(&concreteClassType);
			if (i == typeToMethodMap.end())
			{
				i = CacheAllMethods(concreteClassType);
			}

			auto& nameToMethod = i->second;
			auto k = nameToMethod.find(methodName);
			return k != nameToMethod.end() ? k->second : MethodInfo{ nullptr,0 };
		}

		virtual CReflectedClass* GetRepresentation(void* pSourceInstance) override
		{
			auto i = representations.find(pSourceInstance);
			return (i != representations.end()) ? i->second : NULL;
		}

		virtual CReflectedClass* Represent(const Rococo::Compiler::IStructure& st, void* pSourceInstance) override
		{
			auto i = representations.find(pSourceInstance);
			if (i == representations.end())
			{
				if (st.InterfaceCount() != 1)
				{
					LogError(progObjProxy->Log(), ("The structure '%s' must implement one and only one interface to reflect a native object"), st.Name());
					return NULL;
				}

				if (IsNullType(st))
				{
					LogError(progObjProxy->Log(), ("Null structures, including '%s', cannot be used to reflect a native object"), st.Name());
					return NULL;
				}

				CReflectedClass* rep = (CReflectedClass*)AlignedMalloc(sizeof(size_t), sizeof(CReflectedClass));
				rep->context = pSourceInstance;
				rep->header.Desc = (ObjectDesc*)st.GetVirtualTable(0);
				rep->header.refCount = ObjectStub::NO_REF_COUNT;
				rep->header.pVTables[0] = (VirtualTable*) st.GetVirtualTable(1);

				i = representations.insert(std::make_pair(pSourceInstance, rep)).first;
			}

			return i->second;
		}

		void CancelRepresentation(void* pSourceInstance) override
		{
			auto i = representations.find(pSourceInstance);
			if (i != representations.end())
			{
				CReflectedClass* rep = i->second;
				AlignedFree(rep);
				representations.erase(i);
			}
		}

		void EnumRepresentations(IRepresentationEnumeratorCallback& callback) override
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

		void DispatchToSexyClosure(void* pArgBuffer, const ArchetypeCallback& target) override
		{
			VM::IVirtualMachine& vm(ProgramObject().VirtualMachine());
			VM::CPU& cpu(vm.Cpu());
			VM::IProgramMemory& program(ProgramObject().ProgramMemory());

			const uint8* context = cpu.SF();
			const uint8 *returnAddress = cpu.PC();
			const uint8* sp = cpu.SP();

			cpu.Push(target);
			cpu.Push(pArgBuffer);

			cpu.SetPC(cpu.ProgramStart);
			cpu.D[5].byteCodeIdValue = target.byteCodeId;

			VM::ExecutionFlags currentFlags;
			vm.GetLastFlags(currentFlags);

			VM::ExecutionFlags flags{ currentFlags.ThrowToQuit, true, true };
			EXECUTERESULT er = vm.Execute(flags, nullptr);

			if (er == EXECUTERESULT_TERMINATED)
			{
				vm.SetStatus(EXECUTERESULT_RUNNING);

				cpu.SetPC(returnAddress);
				cpu.SetSF(context);
				cpu.D[VM::REGISTER_SP].uint8PtrValue = (uint8*)sp;
			}
			else
			{
				Throw(0, "Callback terminated prematurely");
			}
		}

		bool ValidateMemory() override
		{
			return true;
		}

		void SetGlobalVariablesToDefaults() override
		{
			auto& vm = progObjProxy->VirtualMachine();
			scripts->SetGlobalVariablesToDefaults(vm);
		}

		virtual int NextID() override
		{
			return nextId++;
		}

		void ThrowFromNativeCode(int32 errorCode, cstr staticRefMessage) override
		{
			scripts->ExceptionLogic().ThrowFromNativeCode(errorCode, staticRefMessage);
		}

		void* AlignedMalloc(int32 alignment, int32 capacity) override
		{
			return Rococo::Memory::AlignedAlloc(capacity, alignment, Rococo::Memory::AllocateSexyMemory);
		}

		void AlignedFree(void* alignedData) override
		{
			return Rococo::Memory::AlignedFree(alignedData, Rococo::Memory::FreeSexyUnknownMemory);
		}

		const void* GetMethodMap() override
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

		virtual CReflectedClass* GetReflectedClass(void* ptr) override
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
				progObjProxy->Log().Write(token);
				return NULL;
			}
			return s;
		}

		CReflectedClass* CreateReflectionClass(cstr className, void* context) override
		{
			IStructure* s = GetClassFromModuleElseLog(ReflectionModule(), className);

			if (s->SizeOfStruct() != sizeof(CReflectedClass))
			{
				TokenBuffer token;
				StringPrint(token, ("%s in reflection module is not equivalent to CReflectedClass"), className);
				progObjProxy->Log().Write(token);
				return NULL;
			}

			CReflectedClass* instance = (CReflectedClass*)DynamicCreateClass(*s, 0);
			reflectedPointers.insert(std::make_pair(context, instance));
			instance->context = context;
			return instance;
		}

		bool ConstructExpressionBuilder(CClassExpressionBuilder& builderContainer, Rococo::Sex::ISExpressionBuilder* builder) override
		{
			IStructure* s = GetClassFromModuleElseLog(ReflectionModule(), "ExpressionBuilder");
			if (s == NULL)
			{
				return false;
			}
			else
			{
				builderContainer.BuilderPtr = builder;
				builderContainer.Header.refCount = ObjectStub::NO_REF_COUNT;
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
				instance->refCount = ObjectStub::NO_REF_COUNT;

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

		mutable IStructure* expressStruct = nullptr;

		const IStructure* GetExpressionType() const
		{
			if (expressStruct == nullptr)
			{
				IModuleBuilder& reflectionModule = ReflectionModule();
				expressStruct = reflectionModule.FindStructure(("Expression"));
			}

			return expressStruct;
		}

		mutable IStructure* expressBuilderStruct = nullptr;

		const IStructure* GetExpressionBuilderType() const
		{
			if (expressBuilderStruct == nullptr)
			{
				IModuleBuilder& reflectionModule = ReflectionModule();
				expressBuilderStruct = reflectionModule.FindStructure(("ExpressionBuilder"));
			}

			return expressBuilderStruct;
		}

		CClassExpression* CreateReflection(cr_sex s)
		{
			if (expressStruct == nullptr)
			{
				IModuleBuilder& reflectionModule = ReflectionModule();
				expressStruct = reflectionModule.FindStructure(("Expression"));

				if (expressStruct == NULL)
				{
					Rococo::Throw(0, ("Cannot find 'Expression' in the reflection module"));
				}
			}

			CClassExpression* express = (CClassExpression*)DynamicCreateClass(*expressStruct, 0);
			express->ExpressionPtr = (Rococo::Sex::ISExpression*) &s;
			return express;
		}

		const CClassExpression* GetExpressionReflection(cr_sex s) override
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

		CStringConstant* GetStringReflection(cstr s, int stringLength) override
		{
			if (stringLength < 0) stringLength = StringLength(s);

			auto i = reflectedStrings.find(s);
			if (i == reflectedStrings.end())
			{
				const IStructure& stringConstantStruct = *SysTypeMemoModule().FindStructure(("StringConstant"));

#ifdef _DEBUG
				if (sizeof(CStringConstant) != stringConstantStruct.SizeOfStruct())
				{
					Throw(0, "sizeof(CStringConstant) != stringConstantStruct.SizeOfStruct() -> internal compiler error");
				}
#endif
				CStringConstant* pSC = (CStringConstant*)DynamicCreateClass(stringConstantStruct, 0);
				pSC->pointer = s;
				pSC->length = stringLength;
				pSC->srcExpression = NULL;
				reflectedStrings.insert(std::make_pair(s, pSC));
				return pSC;
			}
			else
			{
				return i->second;
			}
		}

		FastStringBuilder* CreateAndPopulateFastStringBuilder(const fstring& text, int32 capacity)
		{
			if (!stringPool)
			{
				Throw(0, "%s: No string pool", __FUNCTION__);
			}

			if (capacity < 1 || capacity > 1024_megabytes)
			{
				Throw(0, "CreateAndPopulateFastStringBuilder: capacity must be greater than zero and less than 1 gigabyte. %d was specified", capacity);
			}

			if (text.length >= capacity || text.length < 0)
			{
				Throw(0, "CreateAndPopulateFastStringBuilder(..., %d,%d): must range from 0 to capacity - 1", text.length, capacity);
			}

			auto* sb = stringPool->CreateAndInitFields(capacity);
			memcpy_s(sb->buffer, sb->capacity, text, text.length);
			sb->buffer[text.length] = 0;
			sb->length = text.length;
			return sb;
		}

		CStringConstant* DuplicateStringAsConstant(cstr source, int32 stringLength = -1) override
		{
			if (source == nullptr)
			{
				const IStructure& stringConstantStruct = *SysTypeMemoModule().FindStructure(("StringConstant"));

				CStringConstant* pSC = (CStringConstant*)DynamicCreateClass(stringConstantStruct, 0);
				pSC->pointer = nullptr;
				pSC->length = 0;
				pSC->srcExpression = NULL;
				return pSC;
			}
			cstr persistentString = GetPersistentString(source, stringLength);
			return GetStringReflection(persistentString, stringLength);
		}

		CScriptSystemClass* GetScriptSystemClass() override
		{
			if (reflectionRoot == NULL)
			{
				const IStructure& classStruct = *ReflectionModule().FindStructure(("ScriptSystem"));
				reflectionRoot = (CScriptSystemClass*)DynamicCreateClass(classStruct, 0);
			}

			return reflectionRoot;
		}

		IProgramObject& ProgramObject() override
		{
			return  *progObjProxy;
		}

		IPublicProgramObject& PublicProgramObject() override
		{
			return *progObjProxy;
		}

		Sex::ISParser& SParser() override
		{
			return sexParserProxy();
		}

		IModule* AddTree(ISParserTree& tree) override
		{
			CScript* m = scripts->CreateModule(tree);
			return &m->ProgramModule();
		}

	    void ReleaseTree(Sex::ISParserTree* tree) override
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

			AddNativeCall(sysTypes, Print, NULL, "NativePrint (Sys.Type.Pointer s) -> (Int32 charCount)", __FILE__, __LINE__, false, 0);
		}

		const INamespace& AddNativeNamespace(cstr name) override
		{
			return progObjProxy->GetRootNamespace().AddNamespace(name, Compiler::ADDNAMESPACEFLAGS_CREATE_ROOTS);
		}

		void DefineSysNative(const INamespace& sysNative)
		{
			AddNativeCall(sysNative, NewStringBuilder, stringPool, "NewStringBuilder (Int32 capacity) -> (Sys.Type.IStringBuilder sb)", __FILE__, __LINE__, false, 0);
			AddNativeCall(sysNative, DestructStringBuilder, stringPool, "DestructStringBuilder (Sys.Type.IStringBuilder sb)->", __FILE__, __LINE__, false, 0);
			AddNativeCall(sysNative, CreateMemoString, &memoAllocator, "CreateMemoString (Pointer src) (Int32 srcLen) -> (Pointer dest) (Int32 destLength)", __FILE__, __LINE__, false, 0);
			AddNativeCall(sysNative, FreeMemoString, &memoAllocator, "FreeMemoString (Pointer src) ->", __FILE__, __LINE__, false, 0);
			AddNativeCall(sysNative, DynamicCast, nullptr, "_DynamicCast (Pointer interface) (Pointer instanceRef) ->", __FILE__, __LINE__, false, 0);
			AddNativeCall(sysNative, StringCompare, NULL, "StringCompare  (Pointer s) (Pointer t) -> (Int32 diff)", __FILE__, __LINE__, false, 0);
			AddNativeCall(sysNative, StringCompareI, NULL, "StringCompareI  (Pointer s) (Pointer t) -> (Int32 diff)", __FILE__, __LINE__, false, 0);
			AddNativeCall(sysNative, StringFindLeft, NULL, "StringFindLeft (Pointer containerBuffer) (Int32 containerLength) (Int32 startPos) (Pointer substringBuffer) (Int32 substringLength) (Bool caseIndependent)-> (Int32 position)", __FILE__, __LINE__, false, 0);
			AddNativeCall(sysNative, StringFindRight, NULL, "StringFindRight (Pointer containerBuffer) (Int32 containerLength) (Int32 rightPos) (Pointer substringBuffer) (Int32 substringLength) (Bool caseIndependent)-> (Int32 position)", __FILE__, __LINE__, false, 0);
			AddNativeCall(sysNative, ::AlignedMalloc, this, "AlignedMalloc (Int32 capacity) (Int32 alignment)-> (Pointer data)", __FILE__, __LINE__, false, 0);
			AddNativeCall(sysNative, ::AlignedFree, this, "AlignedFree (Pointer data)->", __FILE__, __LINE__, false, 0);
			AddNativeCall(sysNative, CScriptSystem::_PublishAPI, this, "PublishAPI ->", __FILE__, __LINE__, false, 0);
			AddNativeCall(sysNative, FastStringBuilderAppendIString, stringPool, "FastStringBuilderAppendIString (Sys.Type.IStringBuilder sb) (Pointer src) (Int32 srclength) ->", __FILE__, __LINE__, false, 0);
			AddNativeCall(sysNative, FastStringBuilderThrowIfAppendWouldTruncate, stringPool, "FastStringBuilderThrowIfAppendWouldTruncate (Sys.Type.IStringBuilder sb) ->", __FILE__, __LINE__, false, 0);
			AddNativeCall(sysNative, FastStringBuilderAppendChar, stringPool, "FastStringBuilderAppendChar (Sys.Type.IStringBuilder sb) (Int32 asciiValue) ->", __FILE__, __LINE__, false, 0);
			AddNativeCall(sysNative, FastStringBuilderAppendInt32, stringPool, "FastStringBuilderAppendInt32 (Sys.Type.IStringBuilder sb) (Int32 x) ->", __FILE__, __LINE__, false, 0);
			AddNativeCall(sysNative, FastStringBuilderAppendInt64, stringPool, "FastStringBuilderAppendInt64 (Sys.Type.IStringBuilder sb) (Int64 x) ->", __FILE__, __LINE__, false, 0);
			AddNativeCall(sysNative, FastStringBuilderAppendFloat32, stringPool, "FastStringBuilderAppendFloat32 (Sys.Type.IStringBuilder sb) (Float32 x) ->", __FILE__, __LINE__, false, 0);
			AddNativeCall(sysNative, FastStringBuilderAppendFloat64, stringPool, "FastStringBuilderAppendFloat64 (Sys.Type.IStringBuilder sb) (Float64 x) ->", __FILE__, __LINE__, false, 0);
			AddNativeCall(sysNative, FastStringBuilderAppendBool, stringPool, "FastStringBuilderAppendBool (Sys.Type.IStringBuilder sb) (Bool x) ->", __FILE__, __LINE__, false, 0);
			AddNativeCall(sysNative, FastStringBuilderAppendPointer, stringPool, "FastStringBuilderAppendPointer (Sys.Type.IStringBuilder sb) (Pointer x) ->", __FILE__, __LINE__, false, 0);
			AddNativeCall(sysNative, FastStringBuilderClear, stringPool, "FastStringBuilderClear (Sys.Type.IStringBuilder sb) ->", __FILE__, __LINE__, false, 0);			
			AddNativeCall(sysNative, FastStringBuilderAppendAsDecimal, stringPool, "FastStringBuilderAppendAsDecimal (Sys.Type.IStringBuilder sb) ->", __FILE__, __LINE__, false, 0);
			AddNativeCall(sysNative, FastStringBuilderAppendAsHex, stringPool, "FastStringBuilderAppendAsHex (Sys.Type.IStringBuilder sb) -> ", __FILE__, __LINE__, false, 0);
			AddNativeCall(sysNative, FastStringBuilderAppendAsSpec, stringPool, "FastStringBuilderAppendAsSpec (Sys.Type.IStringBuilder sb) (Int32 type) -> ", __FILE__, __LINE__, false, 0);
			AddNativeCall(sysNative, FastStringBuilderSetFormat, stringPool, "FastStringBuilderSetFormat  (Sys.Type.IStringBuilder sb) (Int32 precision) (Int32 width) (Bool isZeroPrefixed) (Bool isRightAligned)->", __FILE__, __LINE__, false, 0);
			AddNativeCall(sysNative, FastStringBuilderAppendSubstring, stringPool, "FastStringBuilderAppendSubstring (Sys.Type.IStringBuilder sb) (Pointer s) (Int32 sLen) (Int32 startPos) (Int32 charsToAppend) ->", __FILE__, __LINE__, false, 0);
			AddNativeCall(sysNative, FastStringBuilderSetLength, stringPool, "FastStringBuilderSetLength (Sys.Type.IStringBuilder sb)(Int32 length) ->", __FILE__, __LINE__, false, 0);
			AddNativeCall(sysNative, FastStringBuilderSetCase, stringPool, "FastStringBuilderSetCase (Sys.Type.IStringBuilder sb) (Int32 start) (Int32 end) (Bool toUpper)->", __FILE__, __LINE__, false, 0);
			AddNativeCall(sysNative, StringEndsWith, NULL, "StringEndsWith (IString bigString)(IString suffix) -> (Bool isSo)", __FILE__, __LINE__, false, 0);
			AddNativeCall(sysNative, StringStartsWith, NULL, "StringStartsWith (IString bigString)(IString prefix) -> (Bool isSo)", __FILE__, __LINE__, false, 0);
			AddNativeCall(sysNative, FastStringBuilderMakeSysSlashes, stringPool, "MakeSysSlashes (Sys.Type.IStringBuilder sb) ->", __FILE__, __LINE__, false, 0);
			AddNativeCall(sysNative, FastStringBuilderReplace, stringPool, "FastStringBuilderReplace (Sys.Type.IStringBuilder sb)(Int32 startPosition)(IString from)(IString to) ->", __FILE__, __LINE__, false, 0);
		}

		static void _PublishAPI(NativeCallEnvironment& nce)
		{
			CScriptSystem* ss = (CScriptSystem*) nce.context;
			ss->PublishAPI(nce);
		}

		void PublishAPI(NativeCallEnvironment& nce)
		{
			auto& nsRoot = progObjProxy->GetRootNamespace();

			AutoFree<IDynamicStringBuilder> sbc ( CreateDynamicStringBuilder(65536) );

			auto& sb = sbc->Builder();

			sb << "<html<body>";
			sb << "<h1>Sexy API</h1>";
			sb << "<p>Documentation was automatically generated by a call to (Sys.Publish API) on ";

			char theTime[26];
			GetTimestamp(theTime);
	
			sb.AppendFormat("%s", theTime + 4);
			sb << "<p><table>";

			for (int i = 0; i < nsRoot.ChildCount(); ++i)
			{
				auto& nsc = nsRoot.GetChild(i);
				sb.AppendFormat("<tr><td><a href=\"%s.html\"><b>%s</b></a></td></tr>", nsc.FullName()->Buffer, nsc.FullName()->Buffer);
			}

			sb << "</table></body></html>";

			Rococo::OS::EnsureUserDocumentFolderExists(L"sexydoc");
			Rococo::OS::SaveAsciiTextFile(OS::TargetDirectory_UserDocuments, L"sexydoc\\index.html", *sbc->Builder());

			for (int i = 0; i < nsRoot.ChildCount(); ++i)
			{
				Publish_NS_API(nsRoot.GetChild(i), nce, 1);
			}
		}

		void Publish_NS_API(const INamespace& ns, NativeCallEnvironment& nce, int depth)
		{
			AutoFree<IDynamicStringBuilder> sbc ( CreateDynamicStringBuilder(65536) );
			sbc->Builder() << "<html><style>" <<
				"ul { font-family: \"Courier New\"; }" <<
				".factory { color:blue; font-weight: bold; } " <<
				".interface { color:green; font-weight: bold; }" <<
				".struct { color:darkblue; font-weight: bold; }" <<
				".function { color:red; font-weight: bold; }" <<
				"</style><body>";

			auto& sb = sbc->Builder();

			struct : ICallback<const IFunction, cstr>
			{
				StringBuilder* sb;
				const INamespace* ns;
				CALLBACK_CONTROL operator()(const IFunction& f, cstr name) override
				{
					if (strstr(ns->FullName()->Buffer, "Native") == 0 && name[0] != '_')
					{
						sb->AppendFormat("<li><span class=\"function\">function</span> %s.%s\n", ns->FullName()->Buffer, name);
						int nInputs = f.NumberOfInputs();
						int nOutputs = f.NumberOfOutputs();
						int nArgs = nInputs + nOutputs;

						for (int i = 0; i < nInputs; i++)
						{
							cstr argName = f.GetArgName(i + nOutputs);
							auto& argType = f.GetArgument(i + nOutputs);

							sb->AppendFormat(" (<span class=\"struct\">%s</span> %s)", GetFriendlyName(argType), argName);
						}

						if (nOutputs > 0)
						{
							*sb << " ->";
						}

						for (int i = 0; i < nOutputs; i++)
						{
							cstr argName = f.GetArgName(i);
							auto& argType = f.GetArgument(i);
							sb->AppendFormat(" (<span class=\"struct\">%s</span> %s)", GetFriendlyName(argType), argName);
						}

						*sb << "</li>\n";
					}
					return CALLBACK_CONTROL_CONTINUE;
				}
			} forEachFunction;
			forEachFunction.sb = &sb;
			forEachFunction.ns = &ns;

			sb.AppendFormat("<b>%s</b>\n", ns.FullName()->Buffer);

			sb << "<ul>\n";
			ns.EnumerateFunctions(forEachFunction);

			sb << "<p></p>\n";

			for (int i = 0; i < ns.InterfaceCount(); i++)
			{
				auto& I = ns.GetInterface(i);

				sb.AppendFormat("<li><span class=\"interface\">interface</span> %s\n<ul>", I.Name());

				for (int j = 0; j < I.MethodCount(); j++)
				{
					auto& m = I.GetMethod(j);
					sb.AppendFormat("<li>(method %s", m.Name());
					int nInputs = m.NumberOfInputs();
					int nOutputs = m.NumberOfOutputs();
					int nArgs = nInputs + nOutputs;

					for (int k = 0; k < nInputs - 1; k++)
					{
						cstr argName = m.GetArgName(k + nOutputs);
						auto& argType = m.GetArgument(k + nOutputs);

						sb.AppendFormat(" (<span class=\"struct\">%s</span> %s)", GetFriendlyName(argType), argName);
					}

					if (nOutputs > 0)
					{
						sb << " ->";
					}

					for (int k = 0; k < nOutputs; k++)
					{
						cstr argName = m.GetArgName(k);
						auto& argType = m.GetArgument(k);
						sb.AppendFormat(" (<span class=\"struct\">%s</span> %s)", GetFriendlyName(argType), argName);
					}
					
					sb << ")</li>";
				}
				sb << "<p></p></ul></li>";
			}

			struct : ICallback<const IFactory, cstr>
			{
				StringBuilder* sb;
				const INamespace* ns;
				CALLBACK_CONTROL operator()(const IFactory& fac, cstr name) override
				{
					sb->AppendFormat("<li><span class=\"factory\">factory</span> %s.%s <span class=\"interface\">%s</span>", ns->FullName()->Buffer, name, fac.ThisInterface().Name());
					auto& f = fac.Constructor();

					for (int i = 0; i < f.NumberOfInputs() - 1; ++i)
					{
						cstr argName = f.GetArgName(i);
						auto& argType = f.GetArgument(i);
						sb->AppendFormat(" (<span class=\"struct\">%s</span> %s)", GetFriendlyName(argType), argName);
					}

					*sb << "</li><p></p>";

					return CALLBACK_CONTROL_CONTINUE;
				}
			} foreachFactory;
			foreachFactory.sb = &sb;
			foreachFactory.ns = &ns;


			ns.EnumerateFactories(foreachFactory);

			struct : ICallback<const IStructure, cstr>
			{
				StringBuilder* sb;
				const INamespace* ns;
				CALLBACK_CONTROL operator()(const IStructure& s, cstr name) override
				{
					auto type = s.VarType();

					if (!IsNullType(s) && type != VARTYPE_Closure)
					{
						if (type != VARTYPE_Derivative)
						{
							sb->AppendFormat("<li><span class=\"struct\">alias</span> %s.%s for ", ns->FullName()->Buffer, name);
							
							switch (type)
							{
							case VARTYPE_Bool:
								*sb << "Bool (32-bit boolean)";
								break;
							case VARTYPE_Float32:
								*sb << "Float32";
								break;
							case VARTYPE_Float64:
								*sb << "Float64";
								break;
							case VARTYPE_Int32:
								*sb << "Int32";
								break;
							case VARTYPE_Int64:
								*sb << "Int64";
								break;
							case VARTYPE_Pointer:
								*sb << "Pointer";
								break;
							default:
								*sb << "Type unknown ???";
								break;
							}

							*sb << "</li>";
						}
						else
						{
							sb->AppendFormat("<li><span class=\"struct\">struct</span> %s.%s</li>", ns->FullName()->Buffer, name);
							*sb << "<ul>";

							for (int i = 0; i < s.MemberCount(); i++)
							{
								auto& m = s.GetMember(i);
								if (m.UnderlyingType() != nullptr) sb->AppendFormat("<li><span class=\"struct\">%s</span> %s</li>", GetFriendlyName(*m.UnderlyingType()), m.Name());
							}

							*sb << "</ul>";
						}

						*sb << "<p>";
					}

					return CALLBACK_CONTROL_CONTINUE;
				}
			} forEachStruct;
			forEachStruct.sb = &sb;
			forEachStruct.ns = &ns;

			ns.EnumerateStrutures(forEachStruct);

			sb << "</ul>\n";

			sb << "<table>";

			sb.AppendFormat("<tr><td><a href=\"index.html\"><b>Home</b></a></td></tr>");

			auto* parent = ((INamespaceBuilder&)ns).Parent();
			if (parent != &this->progObjProxy->GetRootNamespace())
			{
				sb.AppendFormat("<tr><td><a href=\"%s.html\"><b>%s (parent)</b></a></td></tr>", parent->FullName()->Buffer, parent->FullName()->Buffer);
			}

			for (int i = 0; i < ns.ChildCount(); ++i)
			{
				auto& nsc = ns.GetChild(i);
				sb.AppendFormat("<tr><td><a href=\"%s.html\"><b>%s</b></a></td></tr>", nsc.FullName()->Buffer, nsc.FullName()->Buffer);
			}

			sb << "</table>";

			sbc->Builder() << "</body></html>";

			WideFilePath filename;
			Format(filename, L"sexydoc\\%hs.html", ns.FullName()->Buffer);
			Rococo::OS::SaveAsciiTextFile(OS::TargetDirectory_UserDocuments, filename, *sbc->Builder());

			for (int i = 0; i < ns.ChildCount(); ++i)
			{
				Publish_NS_API(ns.GetChild(i), nce, depth + 1);
			}
		}

		void AddSymbol(cstr symbol, const void* ptr)
		{
			symbols.insert(std::make_pair(ptr, symbol));
		}

		cstr GetSymbol(const void* ptr) const override
		{
			auto i = symbols.find(ptr);
			return i == symbols.end() ? NULL : i->second.c_str();
		}

		void BuildExtraSymbols()
		{
			for (int j = 0; j < progObjProxy->ModuleCount(); ++j)
			{
				IModuleBuilder& module = progObjProxy->GetModule(j);
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

					SafeFormat(symbol, 256, "%s%s", AreEqual(s.Name(), "_Null_", 6) ? "null " : "", GetFriendlyName(s));
					AddSymbol(symbol, &s);
				}
			}
		}

		void Clear()
		{
			hasNullFunction = false;
			expressStruct = nullptr;
			expressBuilderStruct = nullptr;
			ioSystem = nullptr;

			symbols.clear();

			for (auto& t : mapExpressionToTransform)
			{
				t.second.transform->Free();
			}

			mapExpressionToTransform.clear();

			scripts->ExceptionLogic().Clear();
			progObjProxy->GetRootNamespace().Clear();
			progObjProxy->ClearCustomAllocators();

			for (auto i = reflectedStrings.begin(); i != reflectedStrings.end(); ++i)
			{
				FreeDynamicClass(&i->second->header);
			}

			reflectedStrings.clear();

			typeToMethodMap.clear();

			for (auto j = sreflectMap.begin(); j != sreflectMap.end(); ++j)
			{
				FreeDynamicClass(&j->second->Header);
			}

			FreeDynamicClass(&reflectionRoot->header);

			for (auto k = sreflectMap.begin(); k != sreflectMap.end(); ++k)
			{
				FreeDynamicClass(&k->second->Header);
			}

			for (auto i = nativeLibs.begin(); i != nativeLibs.end(); ++i)
			{
				INativeLib* lib = *i;
				lib->ClearResources();
			}

			if (stringPool)
			{
				stringPool->Free();
			}
		}

		void BeginPartialCompilation(StringBuilder* declarationBuilder) override
		{
			Clear();

			stringPool = NewStringPool();

			INamespaceBuilder& sysOS = progObjProxy->GetRootNamespace().AddNamespace("Sys.OS", ADDNAMESPACEFLAGS_CREATE_ROOTS);
			AddNativeCall(sysOS, NativeAppendCTime, NULL, "AppendCTime (Sys.Type.IStringBuilder sb)->(Int32 nChars)", __FILE__, __LINE__, false, 0);

			INamespaceBuilder& sysTypes = progObjProxy->GetRootNamespace().AddNamespace("Sys.Type", ADDNAMESPACEFLAGS_CREATE_ROOTS);
			DefinePrimitives(sysTypes);

			INamespaceBuilder& sysNative = progObjProxy->GetRootNamespace().AddNamespace("Sys.Native", ADDNAMESPACEFLAGS_CREATE_ROOTS);
			DefineSysNative(sysNative);

			AddNativeCall(sysOS, NativeSysOSClockHz, nullptr, "ClockHz -> (Int64 hz)", __FILE__, __LINE__, false, 0);
			AddNativeCall(sysOS, NativeSysOSClockTicks, nullptr, "ClockTicks -> (Int64 tickCount)", __FILE__, __LINE__, false, 0);

			const void* pSrcError = nullptr;
			progObjProxy->ResolveNativeTypes(&pSrcError);

			scripts->Clear();
			scripts->ExceptionLogic().InstallThrowHandler();

			for (auto i = nativeLibs.begin(); i != nativeLibs.end(); ++i)
			{
				INativeLib* lib = *i;
				lib->AddNativeCalls();
			}

			InstallNativeCallNamespaces(IN nativeCalls, REF ProgramObject().GetRootNamespace());
		}

		void PartialCompile(StringBuilder* declarationBuilder) override
		{
			scripts->CompileTopLevelMacros();

			scripts->CompileNamespaces();
			scripts->CompileDeclarations();

			InstallNullFunction();

			if (usesSysIO && !ioSystem) ioSystem = CreateIOSystem(*this);

			if (declarationBuilder)
			{
				AppendNativeCallsAsAliases(IN nativeCalls, *declarationBuilder);
			}

			InstallNativeCalls(IN nativeCalls, REF ProgramObject().GetRootNamespace());
			InstallRawReflections(IN rawReflectionBindings, ProgramObject().VirtualMachine().Core());

			scripts->CompileBytecode();

			BuildExtraSymbols();

			scripts->SetGlobalVariablesToDefaults(this->progObjProxy->VirtualMachine());

			auto* typeFSB = progObjProxy->GetModule(0).FindStructure("FastStringBuilder");
			if (typeFSB == nullptr) Throw(0, "Cannot find FastStringBuilder in module 0");
			stringPool->SetStringBuilderType(typeFSB);

			progObjProxy->AllocatorMap().SetAllocator(typeFSB, stringPool->GetBinding());;
		}

		void Compile(StringBuilder* declarationBuilder) override
		{
			BeginPartialCompilation(declarationBuilder);
			PartialCompile(declarationBuilder);
		}

		void Free() override
		{
			this->~CScriptSystem();
			Memory::FreeSexyMemory(this, sizeof CScriptSystem);
		}

		Sex::ISParserTree* GetSourceCode(const IModule& module) const override
		{
			return scripts->GetSourceCode(module);
		}

		void AddNativeCall(const Compiler::INamespace& ns, FN_NATIVE_CALL callback, void* context, cstr archetype, cstr sourceFile, int lineNumber, bool checkName, int popBytes) override
		{
			enum { MAX_ARCHETYPE_LEN = 1024 };

			if (callback == NULL)
			{
				Rococo::Throw(0, "ScriptSystem::AddNativeCall(...callback...): The [callback] pointer was NULL");
			}

			if (archetype == NULL)
			{
				Rococo::Throw(0, "ScriptSystem::AddNativeCall(...archetype...): The [archetype] pointer was NULL");
			}

			size_t len = StringLength(archetype);
			if (len > (MAX_ARCHETYPE_LEN - 1))
			{
				Rococo::Throw(0, "ScriptSystem::AddNativeCall(...archetype...): The [archetype] string length exceed the maximum");
			}

			char sxArchetype[MAX_ARCHETYPE_LEN];
			CopyStringTochar(sxArchetype, MAX_ARCHETYPE_LEN, archetype, len + 1);

			char srcName[MAX_ARCHETYPE_LEN + 64];
			SafeFormat(srcName, MAX_ARCHETYPE_LEN + 64, "Source: '%s'", sxArchetype);
			Auto<ISourceCode> src = SParser().ProxySourceBuffer(sxArchetype, (int)len, Vec2i{ 0,0 }, srcName);

			try
			{
				Auto<ISParserTree> tree = SParser().CreateTree(src());

				AddNativeCallViaTree(REF nativeCalls, REF ProgramObject().IntrinsicModule(), IN *this, IN ns, IN callback, IN context, IN tree(), IN nativeCallIndex, IN sourceFile, IN lineNumber, IN checkName, IN popBytes);
				nativeCallIndex++;
			}
			catch (ParseException& e)
			{
				ParseException ex(e.Start(), e.End(), archetype, e.Message(), e.Specimen(), NULL);
				throw ex;
			}
		}

		stringmap<RawReflectionBinding*> rawReflectionBindings;

		void AddRawNativeReflectionCall(cstr functionId, FN_RAW_NATIVE_REFLECTION_CALL fnCall, void* context) override
		{
			if (functionId == nullptr)
			{
				Throw(0, "AddRawNativeReflectionCall: <functionId> was nullptr");
			}

			if (!isupper(*functionId))
			{
				Throw(0, "AddRawNativeReflectionCall: <%s> expecting first character to be a capital letter", functionId);
			}

			for (cstr c = functionId + 1; *c != 0; c++)
			{
				if (!IsAlphaNumeric(*c))
				{
					Throw(0, "AddRawNativeReflectionCall: <%s> expecting trailing character to be alphanumerics", functionId);
				}
			}

			if (fnCall == nullptr)
			{
				Throw(0, "AddRawNativeReflectionCall: %s: <fnCall> was nullptr", functionId);
			}

			auto i = rawReflectionBindings.find(functionId);
			if (i != rawReflectionBindings.end())
			{
				Throw(0, "Duplicate function-id specified in %s(%s ...)", __FUNCTION__, functionId);
			}

			rawReflectionBindings.insert(functionId, new RawReflectionBinding { context, fnCall });
		}

		ID_API_CALLBACK TryGetRawReflectionCallbackId(cstr functionId) const override
		{
			auto i = rawReflectionBindings.find(functionId);
			return i == rawReflectionBindings.end() ? 0 : i->second->callbackId;
		}
		
		enum { MAX_NATIVE_SRC_LEN = 32768 };
		std::vector<char, Memory::SexyAllocator<char>> sourceBuffer;

		void AddCommonSource(const char *sexySourceFile) override
		{
			CommonSource src;
			src.SexySourceCode = sexySourceFile;

			auto i = nativeSources.find(sexySourceFile);
			if (i == nativeSources.end())
			{
				sourceBuffer.resize(MAX_NATIVE_SRC_LEN);
				
				WideFilePath fullPath;
				Format(fullPath, L"%ls%hs", srcEnvironment.buf, sexySourceFile);

				try
				{
					OS::LoadAsciiTextFile(sourceBuffer.data(), sourceBuffer.size(), fullPath);
				}
				catch (Rococo::IException&)
				{
					throw;
				}

				src.Src = sexParserProxy().DuplicateSourceBuffer(sourceBuffer.data(), -1, Vec2i{ 1,1 }, sexySourceFile);
				src.Tree = sexParserProxy().CreateTree(*src.Src);

				nativeSources.insert(std::make_pair(sexySourceFile, src.Tree));
			}
			else
			{
				src.Tree = i->second;
				src.Src = const_cast<ISourceCode*>(&i->second->Source());
			}

			AddTree(*src.Tree);
		}
		
#ifdef _WIN32
		const char* const DLL_EXT = "dll";
#else
		const char* const DLL_EXT = "mac.dylib";
#endif

		void AddNativeLibrary(const char* dynamicLinkLibOfNativeCalls) override
		{
			WideFilePath srcEnvironmentDll;
			Format(srcEnvironmentDll, L"%ls%hs", srcEnvironment.buf, dynamicLinkLibOfNativeCalls);

			FN_CreateLib create = Rococo::OS::GetLibCreateFunction(srcEnvironmentDll, false);
			if (!create)
			{
				// Could not find DLL in native source path, so check the default paths
				WideFilePath srcDefaultDll;
				Assign(srcDefaultDll, dynamicLinkLibOfNativeCalls);

				try
				{
					create = Rococo::OS::GetLibCreateFunction(srcDefaultDll, true);
				}
				catch(IException& ex)
				{
					Throw(ex.ErrorCode(), "Error loading library: %ls.%hs", srcEnvironmentDll.buf, DLL_EXT);
				}
			}

			INativeLib* lib = create(*this);
			nativeLibs.push_back(lib);
		}

		int32 GetIntrinsicModuleCount() const override
		{
			return 4;
		}

		void ThrowNative(int errorNumber, cstr source, cstr message) override
		{
			char msg[1024];
			SafeFormat(msg, sizeof(msg), "Native Error (%s): %s", source, message);
			progObjProxy->Log().Write(msg);
			progObjProxy->VirtualMachine().Throw();
		}
	}; // CScriptSystem

	CScriptSystem::~CScriptSystem()
	{
		for (auto& t : mapExpressionToTransform)
		{
			t.second.transform->Free();
		}

		mapExpressionToTransform.clear();

		for (auto i = nativeCalls.begin(); i != nativeCalls.end(); ++i)
		{
			NativeFunction* nf = i->second;
			delete nf;
		}

		for (auto& i : rawReflectionBindings)
		{
			delete i.second;
		}

		for (auto i = nativeLibs.begin(); i != nativeLibs.end(); ++i)
		{
			INativeLib* lib = *i;
			lib->Release();
		}

		delete scripts;

		progObjProxy = nullptr;
		if (stringPool) stringPool->Free();
	}

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
			arrayStruct = Object.Common().SysNative().FindStructure("_Array");
		}

		return *arrayStruct;
	}

	const IStructure& CCompileEnvironment::StructExpressionInterface()
	{
		if (expressionInterface == NULL)
		{
			enum { MODULE_REFLECTION = 3 };
			expressionInterface = Object.GetModule(MODULE_REFLECTION).FindStructure("_Null_Sys_Reflection_IExpression");
			if (!expressionInterface)
			{
				Throw(0, "Object.GetModule(MODULE_REFLECTION).FindStructure('_Null_Sys_Reflection_IExpression') returned nullptr");
			}
		}

		return *expressionInterface;
	}

	const IStructure& CCompileEnvironment::StructExpressionBuilderInterface()
	{
		if (expressionBuilderInterface == NULL)
		{
			enum { MODULE_REFLECTION = 3 };
			expressionBuilderInterface = Object.GetModule(MODULE_REFLECTION).FindStructure("_Null_Sys_Reflection_IExpressionBuilder");
			if (!expressionBuilderInterface)
			{
				Throw(0, "Object.GetModule(MODULE_REFLECTION).FindStructure('_Null_Sys_Reflection_IExpressionBuilder') returned nullptr");
			}
		}

		return *expressionBuilderInterface;
	}

	const IStructure& CCompileEnvironment::StructList()
	{
		if (listStruct == NULL)
		{
			listStruct = Object.Common().SysNative().FindStructure("_List");
		}

		return *listStruct;
	}

	const IStructure& CCompileEnvironment::StructMap()
	{
		if (mapStruct == NULL)
		{
			mapStruct = Object.Common().SysNative().FindStructure("_Map");
		}

		return *mapStruct;
	}
	
	CCompileEnvironment::CCompileEnvironment(CScript& script, ICodeBuilder& builder, const IFactory* _factory):
		Builder(builder), Script(script),
		arrayStruct(NULL), listStruct(NULL),
		mapStruct(NULL),
		expressionBuilderInterface(NULL),
		expressionInterface(NULL),
		factory(_factory),
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
}//Rococo::Script

namespace Anon
{
	struct SSFactory : public Rococo::Script::IScriptSystemFactory
	{
		TMapNameToSTree nativeSourceTrees;

		SSFactory()
		{

		}

		~SSFactory()
		{
			for (auto& i : nativeSourceTrees)
			{
				ISourceCode& src = const_cast<ISourceCode&>(i.second->Source());
				src.Release();
				i.second->Release();
			}
		}

		IPublicScriptSystem* CreateScriptSystem(const Rococo::Compiler::ProgramInitParameters& pip, ILog& logger) override
		{
			void* data = Memory::AllocateSexyMemory(sizeof(CScriptSystem));
			CScriptSystem* ss = new (data) CScriptSystem(nativeSourceTrees, pip, logger);
			ss->PublicProgramObject().VirtualMachine().Core().SetLogger(&logger);
			return ss;
		}

		void Free() override
		{
			this->~SSFactory();
			Memory::FreeSexyMemory(this, sizeof SSFactory);
		}
	};
}

extern "C" SCRIPTEXPORT_API Rococo::Script::IScriptSystemFactory* CreateScriptSystemFactory_1_5_0_0()
{
	try
	{
		void* data = Memory::GetSexyAllocator().Allocate(sizeof(Anon::SSFactory));
		auto* factory = new (data) Anon::SSFactory();
		return factory;
	}
	catch(Rococo::IException& ex)
	{
		char errLog[256];
        SafeFormat(errLog, 256, ("Sexy CreateScriptSystemFactory_1_5_0_0(...) returning NULL. Error: %d, %s."), ex.ErrorCode(), ex.Message());
		Throw(ex.ErrorCode(), "%s", errLog);
		return nullptr;
	}
}

namespace Rococo { namespace Script
{
	NativeCallEnvironment::NativeCallEnvironment(IPublicScriptSystem& _ss, const  Compiler::IFunction& _function, CPU& _cpu, void* _context):
		ss(_ss), function(_function), code(_function.Code()), cpu(_cpu), context(_context)
	{
	}

	CScriptSystemProxy::CScriptSystemProxy(const Rococo::Compiler::ProgramInitParameters& pip, ILog& logger)
	{
		factory = CreateScriptSystemFactory_1_5_0_0();
		ss = factory->CreateScriptSystem(pip, logger);
	}

	CScriptSystemProxy::~CScriptSystemProxy()
	{
		if (ss) ss->Free();
		if (factory) factory->Free();
	}
}}