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

#include "sexy.compiler.stdafx.h"
#include "sexy.validators.h"
#include "sexy.vm.h"
#include "sexy.vm.cpu.h"
#include "sexy.compiler.helpers.h"

#define ROCOCO_USE_SAFE_V_FORMAT
#include "sexy.strings.h"
#include "sexy.stdstrings.h"

#include <list>
#include <unordered_set>

#include <rococo.api.h>

#include <rococo.allocators.h> // Provides _alliged_maloc

using namespace Rococo;
using namespace Rococo::Compiler;
using namespace Rococo::VM;
using namespace Rococo::Memory;

namespace Rococo { namespace Compiler {
	ICodeBuilder* CreateBuilder(IFunctionBuilder& f, bool mayUseParentsSF);
	void Throw(STCException& e);
}} // Rococo::Compiler


#include "sexy.compiler.inl"
#include "sexy.compiler.attributes.inl"
#include "sexy.compiler.functions.inl"
#include "sexy.compiler.modules.inl"
#include "sexy.compiler.structures.inl"
#include "sexy.compiler.namespaces.inl"
#include "sexy.compiler.symbols.inl"

using namespace Rococo::Compiler::Impl;

namespace Anon
{
	ID_BYTECODE BuildStub(IAssembler& a, IProgramMemory& program)
	{
		a.Append_CallByIdIndirect(VM::REGISTER_D5);
		a.Append_Exit(VM::REGISTER_D4);

		ID_BYTECODE stubId = program.AddBytecode();
		program.UpdateBytecode(stubId, a);
		a.Clear();
		return stubId;
	}

	struct DefaultScriptObjectAllocator : IScriptObjectAllocator
	{
		std::unordered_set<ObjectStub*> objects;

		~DefaultScriptObjectAllocator()
		{
			FreeAll(nullptr);
		}

		size_t FreeAll(IEventCallback<LeakArgs>* leakCallback) override
		{
			for (ObjectStub* p : objects)
			{
				LeakArgs args{ p };
				if (leakCallback) leakCallback->OnEvent(args);
				_aligned_free(p);
			}

			size_t nFreed = objects.size();
			objects.clear();
			return nFreed;
		}

		void* AllocateObject(size_t nBytes) override
		{
			auto* p = _aligned_malloc(nBytes, 8);
			objects.insert((ObjectStub*)p);
			return p;
		}

		void FreeObject(void* pMemory) override
		{
			_aligned_free(pMemory);
			objects.erase((ObjectStub*)pMemory);
		}

		refcount_t AddRef() override
		{
			return 1;
		}

		refcount_t ReleaseRef() override
		{
			return 1;
		}
	};

	// D4 points to address that holds ObjectStub, i.e an ObjectStub**, and D5 points to Allocator binding
	static void NewObject(VariantValue* registers, void* context)
	{
		auto* binding = (AllocatorBinding*)registers[VM::REGISTER_D5].vPtrValue;
		auto* type = binding->associatedStructure;
		int allocSize = type->SizeOfStruct();
		auto* object = (ObjectStub*)binding->memoryAllocator->AllocateObject(allocSize);
		object->Desc = (ObjectDesc*)type->GetVirtualTable(0);
		object->refCount = 1;
		int nInterfaces = type->InterfaceCount();
		for (int i = 0; i < nInterfaces; ++i)
		{
			object->pVTables[i] = (VirtualTable*)type->GetVirtualTable(i + 1);
		}

		ptrdiff_t delta = registers[VM::REGISTER_D7].ptrDiffValue;

		uint8* pObjectByByte = (uint8*) object;
		auto pInterface = (InterfacePointer)(pObjectByByte + delta);

		auto *pInterfaceVariable = (InterfacePointer*) (registers[VM::REGISTER_D4].uint8PtrValue);
		*pInterfaceVariable = pInterface;

		registers[VM::REGISTER_D4].vPtrValue = object;
	};

	static void IncRefCount(InterfacePointer pInterface)
	{
		ObjectStub* object = InterfaceToInstance(pInterface);
		object->refCount += (object->refCount & ObjectStub::NO_REF_COUNT) ? 0 : 1;
	}

	static void IncrementRefCount(VariantValue* registers, void* context)
	{
		uint8* rawInterface = registers[VM::REGISTER_D4].uint8PtrValue;
		auto pInterface = (InterfacePointer)rawInterface;
		IncRefCount(pInterface);
	}

	static void Destruct(ObjectStub* object, VM::IVirtualMachine& vm)
	{
		auto destructorId = object->Desc->DestructorId;

		EXECUTERESULT lastStatus = vm.Status();

		vm.SetStatus(EXECUTERESULT_RUNNING);

		vm.Push(object);
		EXECUTERESULT status = vm.ExecuteFunction(destructorId);
		vm.PopPointer();

		vm.SetStatus(lastStatus);

		if (status != EXECUTERESULT_RETURNED)
		{
			Throw(0, "Error destructing %s at 0x%X", object->Desc->TypeInfo->Name(), object);
		}
	}

	void DecObjRefCount(ObjectStub* object, IAllocatorMap& map);

	void DecRefCountOnMembers(const IStructure& type, ObjectStub* object, size_t offset, IAllocatorMap& map)
	{
		auto nMembers = type.MemberCount();

		for (int i = 0; i < nMembers; ++i) // Ignore the first three members, they are part of the stub
		{
			auto& m = type.GetMember(i);
			const int dm = m.SizeOfMember();

			if (m.IsInterfaceVariable())
			{
				const uint8* child = ((const uint8*)object) + offset;
				InterfacePointer childObj = *(InterfacePointer*) child;
				const uint8* objRaw = ((const uint8*)childObj) + (*childObj)->OffsetToInstance;
				auto* childStub = (ObjectStub*) objRaw;
				DecObjRefCount(childStub, map);
			}
			else
			{
				if (m.UnderlyingType())
				{
					DecRefCountOnMembers(*m.UnderlyingType(), object, offset, map);
				}
			}

			offset += dm;
		}
	}

	void DecRefCountOnCircularReferences(ObjectStub* object, IAllocatorMap& map)
	{
		if (object->Desc->TypeInfo->HasInterfaceMembers())
		{

		}
	}

	void DecObjRefCount(ObjectStub* object, IAllocatorMap& map)
	{
		if (object->refCount & ObjectStub::NO_REF_COUNT) return;
		object->refCount -= 1;

		if (object->refCount > 0)
		{
			DecRefCountOnCircularReferences(object, map);
		}

		if (object->refCount <= 0)
		{
			object->refCount = 0;

			auto& type = *object->Desc->TypeInfo;

			if (object->Desc->TypeInfo->HasInterfaceMembers())
			{
				DecRefCountOnMembers(type, object, 0, map);
			}

			if (object->Desc->TypeInfo->Name()[0] != '_')
			{
				auto* allocator = map.GetAllocator(*object->Desc->TypeInfo);
				Destruct(object, allocator->associatedStructure->Object().VirtualMachine());
				allocator->memoryAllocator->FreeObject(object);
			}
		}
	}

	void DecrementRefCount(VariantValue* registers, void* allocatorContext)
	{
		uint8* rawInterface = registers[VM::REGISTER_D4].uint8PtrValue;
		auto pInterface = (InterfacePointer)rawInterface;
		auto vTable = static_cast<VirtualTable*>(*pInterface);
		auto* object = (ObjectStub*)(vTable->OffsetToInstance + rawInterface);
		auto& map = *(IAllocatorMap*)allocatorContext;
		DecObjRefCount(object, map);
	}

	void UpdateRefsOnSourceAndTarget(VariantValue* registers, void* allocatorContext)
	{
		auto* src = (InterfacePointer)registers[VM::REGISTER_D4].uint8PtrValue;
		auto* target = (InterfacePointer)registers[VM::REGISTER_D5].uint8PtrValue;

		if (src != target)
		{
			auto& map = *(IAllocatorMap*)allocatorContext;

			auto vTableTrg = static_cast<VirtualTable*>(*target);
			auto* objectTrg = (ObjectStub*)(vTableTrg->OffsetToInstance + (uint8*)target);

			DecObjRefCount(objectTrg, map);
			IncRefCount(src);
		}
	}

	void GetAllocSize(VariantValue* registers, void* context)
	{
		uint8* pInterface = (uint8*)registers[VM::REGISTER_D7].vPtrValue;
		VirtualTable** pTables = (VirtualTable**)pInterface;
		auto offset = (*pTables)->OffsetToInstance;
		ObjectStub* object = (ObjectStub*)(pInterface + offset);
		registers[VM::REGISTER_D7].int32Value = object->Desc->TypeInfo->SizeOfStruct();
	};

	class ProgramObject : public IProgramObject, private IAllocatorMap
	{
	private:
		typedef std::vector<Module*> TModules;
		TModules modules; // The set of all modules, except the intrinsics
		Namespace* rootNS;
		Module* intrinsics;
		ILog& log;
		ICore* svmCore;
		IVirtualMachine* virtualMachine;
		IProgramMemory* program;
		TSymbols symbols;
		CommonStructures* common;

		std::vector<IStructure*> systemStructures;

		DefaultScriptObjectAllocator defaultAllocator;
		std::unordered_map<const IStructure*, AllocatorBinding*> allocators;

		CallbackIds callbackIds;
	public:
		ILog& Log() override { return log; }
		void Free() override { delete this; }
		IModuleBuilder& GetModule(int index) override { return *modules[index]; }
		const IModule& GetModule(int index)  const override { return *modules[index]; }
		int ModuleCount() const override { return (int32)modules.size(); }
		const INamespace& GetRootNamespace() const override { return *rootNS; }
		INamespaceBuilder& GetRootNamespace() override { return *rootNS; }
		IVirtualMachine& VirtualMachine() override { return *virtualMachine; }
		IProgramMemory& ProgramMemory() override { return *program; }
		const IProgramMemory& ProgramMemory() const override { return *program; }
		IModuleBuilder& IntrinsicModule() override { return *intrinsics; }
		const IModule& IntrinsicModule() const override { return *intrinsics; }
		CommonStructures& Common() override { return *common; }

		ProgramObject(const ProgramInitParameters& pip, ILog& _log) : log(_log)
		{
			CoreSpec spec;
			spec.Reserved = 0;
			spec.SizeOfStruct = sizeof(spec);
			spec.Version = CORE_LIB_VERSION;
			svmCore = CreateSVMCore(&spec);
			program = svmCore->CreateProgramMemory(pip.MaxProgramBytes);
			virtualMachine = svmCore->CreateVirtualMachine();
			rootNS = new Namespace(*this);
			intrinsics = Module::CreateIntrinsics(*this, ("!Intrinsics!"));

			IAssembler* assembler = svmCore->CreateAssembler();
			BuildStub(*assembler, *program);
			assembler->Free();

			common = NULL;
			
			callbackIds.IdUpdateRefsOnSourceAndTarget = svmCore->RegisterCallback(UpdateRefsOnSourceAndTarget, nullptr, "-+ refs");
			callbackIds.IdAllocate = svmCore->RegisterCallback(NewObject, nullptr, "new");
			callbackIds.IdAddRef = svmCore->RegisterCallback(Anon::IncrementRefCount, nullptr, "++ref");
			callbackIds.IdReleaseRef = svmCore->RegisterCallback(Anon::DecrementRefCount, (IAllocatorMap*) this, "--ref");
			callbackIds.IdGetAllocSize = svmCore->RegisterCallback(GetAllocSize, nullptr, "sizeof");
		}

		void DecrementRefCount(InterfacePointer pInterface) override
		{
			uint8* pData = ((uint8*)pInterface) + (*pInterface)->OffsetToInstance;
			ObjectStub* object = (ObjectStub*)pData;
			DecObjRefCount(object, *this);
		}

		void IncrementRefCount(InterfacePointer pInterface) override
		{
			IncRefCount(pInterface);
		}

		const CallbackIds& GetCallbackIds() const override
		{
			return callbackIds;
		}

		size_t lastLeakCount = 0;

		void ClearCustomAllocators() override
		{
			lastLeakCount = FreeLeakedObjects(nullptr);

			for (auto i : allocators)
			{
				if (i.second->memoryAllocator != &defaultAllocator)
				{
					i.second->memoryAllocator->ReleaseRef();
				}

				if (i.second->standardDestruct)
				{
					delete i.second;
				}
			}

			allocators.clear();
		}

		~ProgramObject()
		{
			ClearCustomAllocators();

			ClearSymbols(symbols);

			for (auto i = modules.begin(); i != modules.end(); ++i)
			{
				Module* m = *i;
				delete m;
			}

			modules.clear();

			delete rootNS;
			delete intrinsics;

			virtualMachine->Free();
			program->Release();
			svmCore->Free();

			delete common;
		}

		IAllocatorMap& AllocatorMap() override
		{
			return *this;
		}

		size_t FreeLeakedObjects(IEventCallback<LeakArgs>* leakCallback) override
		{
			return 0;
		}

		void SetAllocator(const IStructure* s, AllocatorBinding* binding) override
		{
			allocators[s] = binding;
		}

		AllocatorBinding* GetAllocator(const IStructure& s) override
		{
			auto i = allocators.find(&s);
			if (i != allocators.end())
			{
				return i->second;
			}
			else
			{
				auto* binding = new AllocatorBinding{ &defaultAllocator, &s };
				allocators[&s] = binding;
				return binding;
			}
		}

		void InitCommon() override
		{
			common = new CommonStructures(*this);
		}

		cstr RegisterSymbol(cstr text) override
		{
			return AddSymbol(symbols, text);
		}

		IStructureBuilder& AddIntrinsicStruct(cstr name, size_t sizeOfType, VARTYPE underlyingType, const IArchetype* archetype) override
		{
			StructurePrototype prototype(MEMBERALIGN_1, INSTANCEALIGN_1, true, archetype, false);

			Structure* s = new Structure(name, prototype, *intrinsics, underlyingType, NULL);
			s->AddMember(NameString::From(("Value")), TypeString::From(name));
			StructureMember& m = s->GetMemberRef(0);
			m.SetSize(sizeOfType);
			s->Seal();
			intrinsics->Structures().Register(s->Name(), *s);
			return *s;
		}

		IModuleBuilder& AddModule(cstr name) override
		{
			Module* m = new Module(*this, name);
			modules.push_back(m);
			return *m;
		}

		void SetProgramAndEntryPoint(const IFunction& f) override
		{
			CodeSection cs;
			f.Code().GetCodeSection(cs);
			SetProgramAndEntryPoint(cs.Id);
		}

		void SetProgramAndEntryPoint(ID_BYTECODE bytecodeId) override
		{
			virtualMachine->SetProgram(program);
			virtualMachine->InitCpu();
			virtualMachine->Cpu().D[VM::REGISTER_D5].byteCodeIdValue = bytecodeId;
		}

		const IStructure* GetSysType(SEXY_CLASS_ID id) override
		{
			if (systemStructures.empty())
			{
				if (!modules.empty())
				{
					auto* s = modules[0]->FindStructure(("StringBuilder"));
					if (s)
					{
						systemStructures.push_back(s);
					}
				}
			}

			return id >= systemStructures.size() ? nullptr : systemStructures[id];
		}

		void ResolveNativeTypes() override
		{
			ResolveTypesInIntrinsics(*this);
		}

		bool ResolveDefinitions() override
		{
			if (!ResolveStructures(*this))
			{
				return false;
			}

			if (!ExpandMembersAndValidate(*this))
			{
				Log().Write(("Failed to compute sizes for all structures. Reduce null objects member dependencies inside structure definitions"));
				return false;
			}

			for (auto i = modules.begin(); i != modules.end(); ++i)
			{
				Module* module = *i;
				if (!module->ResolveDefinitions())
				{
					return false;
				}
			}

			intrinsics->ResolveDefinitions();

			return true;
		}
	};
}

namespace Rococo 
{ 
   namespace Compiler
   {
	   IProgramObject* CreateProgramObject_1_0_0_0(const ProgramInitParameters& pip, ILog& log)
	   {
		   return new Anon::ProgramObject(pip, log);
	   }

	   void ValidateNotNull(void* p)
	   {
		   if (p == NULL)
			   Throw(Rococo::Compiler::ERRORCODE_NULL_POINTER, ("Null ptr"), ("item"));
	   }

	   CommonStructures::CommonStructures(IProgramObject& obj)
	   {
		   ValidateNotNull(this->root = &obj.GetRootNamespace());
		   ValidateNotNull(this->sys = root->FindSubspace("Sys"));
		   ValidateNotNull(this->sysType = sys->FindSubspace("Type"));
		   ValidateNotNull(this->sysNative = sys->FindSubspace("Native"));
		   ValidateNotNull(this->sysReflection = sys->FindSubspace("Reflection"));

		   ValidateNotNull(this->typeInt32 = obj.IntrinsicModule().FindStructure("Int32"));
		   ValidateNotNull(this->typeInt64 = obj.IntrinsicModule().FindStructure("Int64"));
		   ValidateNotNull(this->typeBool = obj.IntrinsicModule().FindStructure("Bool"));
		   ValidateNotNull(this->typeFloat32 = obj.IntrinsicModule().FindStructure("Float32"));
		   ValidateNotNull(this->typeFloat64 = obj.IntrinsicModule().FindStructure("Float64"));
		   ValidateNotNull(this->typePointer = obj.IntrinsicModule().FindStructure("Pointer"));
		   ValidateNotNull(this->typeNode = obj.GetModule(0).FindStructure("_Node"));
		   ValidateNotNull(this->typeArray = obj.GetModule(0).FindStructure("_Array"));
		   ValidateNotNull(this->typeMapNode = obj.GetModule(0).FindStructure("_MapNode"));

		   ValidateNotNull(this->sysTypeIString = sysType->FindInterface("IString"));
		   ValidateNotNull(this->sysTypeIException = sysType->FindInterface("IException"));
		   ValidateNotNull(this->sysTypeIExpression = sysReflection->FindInterface("IExpression"));

		   ValidateNotNull(this->typeStringLiteral = obj.GetModule(0).FindStructure("StringConstant"));

		   ValidateNotNull(this->typeExpression = obj.GetModule(3).FindStructure("Expression"));
	   }

	   void Throw(ERRORCODE code, cstr source, cstr format, ...)
	   {
		   va_list args;
		   va_start(args, format);

		   char message[256];
		   SafeVFormat(message, 256, format, args);
		   STCException e(code, source, message);	
		   Throw(e);
	   }

	   void HighLightText(char* outputBuffer, size_t nBytesOutput, cstr highlightPos, cstr wholeString)
	   {
		   char charbuf[4];
		   SafeFormat(charbuf, 4, ("[%c]"), *highlightPos);
		   int startChars = (int32)(highlightPos - wholeString);
		   CopyString(outputBuffer, std::min<int32>((int32)nBytesOutput, startChars), wholeString);
		   StringCat(outputBuffer, charbuf, (int32)nBytesOutput);
		   StringCat(outputBuffer, highlightPos + 1, (int32)nBytesOutput);
	   }
	
	   bool IsCapital(char c) { return (c >= 'A' && c <= 'Z'); }
	   bool IsLowerCase(char c) { return (c >= 'a' && c <= 'z'); }
	   bool IsNumeral(char c) { return c >= '0' && c < '9'; }
	   bool IsAlpha(char c) { return IsCapital(c) || IsLowerCase(c); }
	   bool IsAlphaNumeral(char c) { return IsAlpha(c) || IsNumeral(c); }
	
	   void ValidateCapitalLetter(cstr s, cstr stringStart, cstr name, cstr functionSymbol)
	   {
		   if (!IsCapital(*s))
		   {
			   char text[256];
			   HighLightText(text, 256, s, stringStart);
			   Throw(ERRORCODE_BAD_ARGUMENT, functionSymbol, ("%s, Expecting capital letter A-Z: %s"), name, text);
		   }
	   }

	   void ValidateLowerCaseLetter(cstr s, cstr stringStart, cstr name, cstr functionSymbol)
	   {
		   if (!IsLowerCase(*s))
		   {
			   char text[256];
			   HighLightText(text, 256, s, stringStart);
			   Throw(ERRORCODE_BAD_ARGUMENT, functionSymbol, ("%s, Expecting lower case a-z: %s"), name, text);
		   }
	   }

	   void ValidateNumeral(cstr s, cstr stringStart, cstr name, cstr functionSymbol)
	   {
		   if (!IsNumeral(*s))
		   {
			   char text[256];
			   HighLightText(text, 256, s, stringStart);
			   Throw(ERRORCODE_BAD_ARGUMENT, functionSymbol, ("%s, Expecting numeral 0-9: %s"), name, text);
		   }
	   }

	   void ValidateAlpha(cstr s, cstr stringStart, cstr name, cstr functionSymbol)
	   {		
		   if (!IsAlpha(*s)) 
		   {
			   char text[256];
			   HighLightText(text, 256, s, stringStart);
			   Throw(ERRORCODE_BAD_ARGUMENT, functionSymbol, ("%s, Expecting letter A-Z or a-z: %s"), name, text);
		   }
	   }

	   void ValidateAlphaNumeral(cstr s, cstr stringStart, cstr name, cstr functionSymbol)
	   {		
		   if (!IsAlphaNumeral(*s)) 
		   {
			   char text[256];
			   HighLightText(text, 256, s, stringStart);
			   Throw(ERRORCODE_BAD_ARGUMENT, functionSymbol, ("%s, Expecting alphanumeric A-Z or a-z or 0-9: %s"), name, text);
		   }
	   }

	   void ValidateNamespaceString(cstr s, cstr name, cstr functionSymbol)
	   {
		   if (s == NULL) Throw(ERRORCODE_NULL_POINTER, functionSymbol, ("[%s] was NULL"), name);
		   if (s[0] == 0) Throw(ERRORCODE_EMPTY_STRING, functionSymbol, ("[%s] was empty string"), name);

		   enum STATE
		   {
			   STATE_START_BRANCH,
			   STATE_MID_BRANCH,
		   } state = STATE_START_BRANCH;

		   for(cstr p = s; *p != 0; p++)
		   {
			   if (state == STATE_START_BRANCH)
			   {
				   ValidateCapitalLetter(p, s, name, functionSymbol);
				   state = STATE_MID_BRANCH;
			   }
			   else // MID_BRANCH
			   {
				   if (*p == '.')
				   {
					   state = STATE_START_BRANCH;
				   }
				   else
				   {
					   ValidateAlphaNumeral(p, s, name, functionSymbol);
				   }
			   }
		   }

		   if (state == STATE_START_BRANCH)
		   {
			   Throw(ERRORCODE_BAD_ARGUMENT, functionSymbol, ("[%s] terminated in a dot: %s"), name, s);
		   }
	   }

	   const IFunction* GetCurrentFunction(IPublicProgramObject& po, size_t& programOffset, size_t& pcOffset)
	   {
		   IVirtualMachine& vm = po.VirtualMachine();
		   IProgramMemory& mem = po.ProgramMemory();

		   pcOffset = vm.Cpu().PC() - vm.Cpu().ProgramStart;
		   ID_BYTECODE runningId = mem.GetFunctionContaingAddress(pcOffset);
		   if (runningId != 0)
		   {
			   programOffset = mem.GetFunctionAddress(runningId);
			   const IFunction* f = GetFunctionForBytecode(po, runningId);
			   return f;
		   }

		   return NULL;
	   }
   }
} // Rococo::Compiler

namespace Rococo
{
	STCException::STCException(ERRORCODE _code, cstr _source, cstr _msg) : code(_code)
	{
		CopyString(message, MAX_MSG_LEN, _msg);
		CopyString(source, MAX_MSG_LEN, _source);
	}
}
