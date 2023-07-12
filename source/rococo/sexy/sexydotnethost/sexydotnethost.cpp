// This is the main DLL file.

#include "stdafx.h"
#include "rococo.debugging.h"
#include <string.h>
#include <sexy.strings.h>
#include <stdlib.h>
#include <stdio.h>
#include "sexy.debug.types.h"
#include "sexy.compiler.public.h"
#include "sexy.script.h"
#include "sexy.s-parser.h"

#include "sexy.vm.h"
#include "sexy.vm.cpu.h"
#include "sexydotnethost.h"

#include <rococo.sexy.map.expert.h>

#include <vector>

namespace Rococo
{
   namespace Compiler
   {
	   SCRIPTEXPORT_API cstr GetTypeName(const IStructure& s);
   }
}

namespace SexyDotNet { namespace Host
{
	SourceModule::SourceModule(SexyScriptLanguage^ parent, IntPtr moduleHandle, IntPtr scHandle, IntPtr expressionTree, String^ name)
	{
		this->parent = parent;
		this->moduleHandle = moduleHandle;
		this->scHandle = scHandle;		
		this->expressionTree = expressionTree;
		this->name = name;
		this->sections = gcnew List<AssemblySection^>();
		this->bytecodeVersion = -1;

		/*if (expressionTree.ToPointer() != NULL)
		{
			ToSS(parent->NativeHandle)->AddTree(*ToTree(expressionTree));
		}*/
	}

	void SourceModule::ReleaseAll()
	{
		if (scHandle.ToPointer() != NULL)
		{
			ToSC(scHandle)->Release();
			scHandle = IntPtr::Zero;

			ISParserTree* tree = ToTree(expressionTree);
			ToSS(parent->NativeHandle)->ReleaseTree(tree);
			tree->Release();
		}
	}

	void SexyScriptLanguage::Compile()
	{
		initializedVM = false;
		goodstate = false;

		try
		{
			ToSS(nativeSS)->Compile();
			goodstate = true;
			BeginExecution();
		}
		catch (ParseException& e)
		{
			cstr s = e.Message();
			cstr n = e.Name();

			String^ msg = gcnew String(s, 0, StringLength(s), Encoding::ASCII);				
			String^ name = gcnew String(n, 0, StringLength(n), Encoding::ASCII);

			SourceLocation start = SourceLocation(e.Start().x+1,e.Start().y+1);
			SourceLocation end = SourceLocation(e.End().x+1,e.End().y+1);
			throw gcnew CompileError(nullptr, name, msg, start, end);
		}
	}

	void SexyScriptLanguage::BeginExecution()
	{		
		if (!initializedVM)
		{
			if (!goodstate) throw gcnew Exception("The program object is not in an executable state");
			IScriptSystem& ss = *ToSS(nativeSS);
			IPublicProgramObject& po = ss.PublicProgramObject();

			const INamespace* ns = po.GetRootNamespace().FindSubspace(("EntryPoint"));
			if(ns == NULL)
			{
				throw gcnew CompileError(nullptr, "<All source modules>", "Could not find the EntryPoint namespace in the source modules. Execution aborted", SourceLocation(0,0), SourceLocation(0,0));
			}

			const IFunction* f = ns->FindFunction(("Main"));
			if (f == NULL)
			{
				throw gcnew CompileError(nullptr, "<All source modules>", "Could not find the EntryPoint.Main function in the source modules. Execution aborted", SourceLocation(0,0), SourceLocation(0,0));
			}

			po.SetProgramAndEntryPoint(*f);
			
			IVirtualMachine& vm = po.VirtualMachine();

			//CStepCallback* sc = (CStepCallback*) stepCallbackHandle.ToPointer();

			//vm.SetStepCallback(sc);

			vm.Push(0); // Push space for 32-bit exit code

			vm.Cpu().SetSF(vm.Cpu().SP());

			initializedVM = true;

			vm.StepInto();

			SkipJIT(po);

			ThrowOnCompileError();

			CStepCallback* sc = (CStepCallback*)stepCallbackHandle.ToPointer();

			vm.SetStepCallback(sc);

		//	Disassemble();

		//	UpdateDisassembly();
		}
	}

	void SexyScriptLanguage::ThrowOnCompileError()
	{
		CLogger* logger = (CLogger*) logHandle.ToPointer();

		CompileError^ predecessor = nullptr;

		ParseException ex;
		while (logger->PopLastException(OUT ex))
		{
			if (IsException(ex))
			{
				String^ msg = gcnew String(ex.Message(), 0, StringLength(ex.Message()), Encoding::ASCII);
				String^ src = gcnew String(ex.Name(), 0, StringLength(ex.Name()), Encoding::ASCII);

				SourceLocation start = SourceLocation(ex.Start().x, ex.Start().y);
				SourceLocation end = SourceLocation(ex.End().x, ex.End().y);
				predecessor = gcnew CompileError(predecessor, src, msg, start, end);
			}
		}

		if (predecessor != nullptr)
		{
			throw predecessor;
		}
	}

	int SexyScriptLanguage::Execute()
	{
		BeginExecution();
		
		IScriptSystem& ss = *ToSS(nativeSS);
		IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();
		EXECUTERESULT result = vm.Execute(ExecutionFlags(false, true, false));

		ThrowOnCompileError();

		int iResult = (int) result;
		if (result == EXECUTERESULT_TERMINATED)
		{
			int exitCode = vm.PopInt32();

			EvTerminated(exitCode);

			return exitCode;
		}
		else
		{
			throw gcnew CompileError(nullptr, "<All source modules>", "Execution failed. EXECUTERESULT=" + iResult, SourceLocation(0,0), SourceLocation(0,0));
		}
	}

	bool SexyScriptLanguage::StepInto()
	{
		BeginExecution();

		IScriptSystem& ss = *ToSS(nativeSS);
		IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();
		vm.StepInto();

		SkipJIT(ss.PublicProgramObject());

		ThrowOnCompileError();

		return UpdateDisassembly();
	}

	bool SexyScriptLanguage::StepOver()
	{
		BeginExecution();

		IScriptSystem& ss = *ToSS(nativeSS);
		IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();
		vm.StepOver();

		SkipJIT(ss.PublicProgramObject());

		ThrowOnCompileError();

		return UpdateDisassembly();
	}

	bool SexyScriptLanguage::StepOut()
	{
		BeginExecution();

		IScriptSystem& ss = *ToSS(nativeSS);
		IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();
		vm.StepOut();

		ThrowOnCompileError();

		return UpdateDisassembly();
	}

	SourceModule^ SexyScriptLanguage::AddModule(String^ moduleFullPath)
	{
		IScriptSystem& ss = *ToSS(nativeSS);

		if (sourceModules->ContainsKey(moduleFullPath))
		{
			throw gcnew Exception("The system already contains the module " + moduleFullPath);
		}

		FileStream ^input = File::OpenRead(moduleFullPath);
		if (input->Length > (int64) HostLimits::MAX_SOURCE_FILE_KILOBYTES * 1024)
		{
			delete input;
			throw gcnew CompileError(nullptr, moduleFullPath, "The file length of the module exceeded that as HostLimits::MAX_SOURCE_FILE_KILOBYTES", SourceLocation(0,0), SourceLocation(0,0));
		}

		int32 fileLength = (int32) input->Length;

		if (fileLength == 0)
		{
			delete input;
			throw gcnew CompileError(nullptr, moduleFullPath, "The module was zero length", SourceLocation(0,0), SourceLocation(0,0));
		}

		array<Byte>^ inputBuffer = gcnew array<Byte>(fileLength); 

		if (input->Read(inputBuffer, 0, fileLength) != fileLength)
		{
			delete input;
			throw gcnew Exception("Unexpected error reading file data. Read was cut short" + moduleFullPath);
		}

		delete input;

		pin_ptr<Byte> inputPin = &inputBuffer[0];
		const Byte* inputPtr = inputPin;

		array<Char>^ nameArray = moduleFullPath->ToCharArray();

		pin_ptr<Char> namePin = &nameArray[0];
		const Char* namePtr = namePin;


		ISourceCode* sc;

		// Assume that if the number of bytes in the file is even and the first high-byte is zero then we have a unicode file
		if (fileLength % 2 == 0 && inputBuffer[1] == 0)
		{
			sc = AddUnicodeModule(inputPtr, ss, fileLength / 2, namePtr);
			if (sc == NULL)
			{
				throw gcnew Exception("Error adding the UNICODE source code as " + charENCODING + "module");
			}
		}
		else
		{
			sc = AddAsciiModule(inputPtr, ss, fileLength, namePtr);
			if (sc == NULL)
			{
				throw gcnew Exception("Error adding the ASCII source code as " + charENCODING + "module");
			}
		}		

		ISParserTree* tree;
		IModule* pNativeModule;

		try
		{
			tree = ss.SParser().CreateTree(*sc);
			pNativeModule = ss.AddTree(*tree);
		}
		catch (ParseException& e)
		{
			cstr s = e.Message();

#ifdef char_IS_WIDE
			String^ msg = gcnew String(s);
#else
			String^ msg = gcnew String(s, 0, StringLength(s), IsSexUnicode ? Encoding::Unicode : Encoding::ASCII);
#endif

			SourceLocation start = SourceLocation(e.Start().x,e.Start().y);
			SourceLocation end = SourceLocation(e.End().x,e.End().y);
			throw gcnew CompileError(nullptr, moduleFullPath, msg, start, end);
		}

		if (sourceModules->Count == 0)
		{
			for(int i = 0; i < ss.GetIntrinsicModuleCount(); ++i)
			{
				// Add the hard-coded module			
				const IModule& module = ss.PublicProgramObject().GetModule(i);
				IntPtr moduleHandle((void*)&module);
				String^ hcName = gcnew String((char*) module.Name(), 0, StringLength(module.Name()), IsSexUnicode ? Encoding::Unicode : Encoding::ASCII);
				SourceModule^ dotnetModule = gcnew SourceModule(this, moduleHandle, IntPtr::Zero, IntPtr::Zero, hcName);
				sourceModules->Add(hcName, dotnetModule);			
			}
		}

		IntPtr moduleHandle(pNativeModule);
		IntPtr scHandle(sc);
		IntPtr treeHandle(tree);
		SourceModule^ module = gcnew SourceModule(this, moduleHandle, scHandle, treeHandle, moduleFullPath);
		sourceModules->Add(moduleFullPath, module);		
		return module;
	}

	List<VariableDesc>^ SexyScriptLanguage::GetVariables(Int32 callDepth)
	{
		IScriptSystem& ss = *ToSS(nativeSS);

		List<VariableDesc>^ vars = gcnew List<VariableDesc>();

		size_t nVariables = Rococo::Script::GetCurrentVariableCount(ss, callDepth);

		for(size_t i = 0; i < nVariables; ++i)
		{
			cstr name;
			MemberDef def;
			const IStructure* pseudoType;
			const uint8* SF;
			if (!GetVariableByIndex(REF name, REF def, REF pseudoType, SF, ss, i, callDepth)) continue;

			Char unicodeName[256];
			CopycharToUnicode(unicodeName, 256, name);

			Char unicodeValue[256];
			Char unicodeType[256];

			const void* pVariableData = SF + def.SFOffset;
			if (def.Usage == ARGUMENTUSAGE_BYVALUE)
			{
				CopycharToUnicode(unicodeType, 256, Compiler::GetTypeName(*def.ResolvedType));				
				FormatValue(ss, unicodeValue, 256, def.ResolvedType->VarType(), pVariableData);
			}
			else
			{
				unicodeType[0] = L'*';
				CopycharToUnicode(unicodeType+1, 255, Compiler::GetTypeName(*def.ResolvedType));

				const void** ppData = (const void**) pVariableData;

				__try
				{
					const void* pData = *ppData;
					_snwprintf_s(unicodeValue, _TRUNCATE, L"%1llX -> %1llX", (int64) pVariableData, (int64) pData);
				}
				__except(1)
				{
					_snwprintf_s(unicodeValue, _TRUNCATE, L"Bad pointer");
				}
			}

			IntPtr address((Void*) pVariableData);

			VariableKind vk;
			switch(def.location)
			{
			case VARLOCATION_NONE:
				vk = VariableKind::Pseudo;
				break;
			case VARLOCATION_INPUT:
				vk = VariableKind::Input;
				break;
			case VARLOCATION_OUTPUT:
				vk = VariableKind::Output;
				break;
			case VARLOCATION_TEMP:
				vk = VariableKind::Local;
				break;
			}

			if (vk == VariableKind::Pseudo)
			{
				vars->Add(VariableDesc(0, gcnew String(unicodeName), gcnew String(unicodeType), gcnew String(""), vk, IntPtr::Zero));
			}
			else
			{
				vars->Add(VariableDesc(def.SFOffset, gcnew String(unicodeName), gcnew String(unicodeType), gcnew String(unicodeValue), vk, address));
			}
		}

		return vars;
	}

	struct NativeVariableDesc
	{
		enum { VALUE_CAPACITY = 128, TYPE_CAPACITY = 128, NAME_CAPACITY = 128 };

		char Name[NAME_CAPACITY];		
		char Type[TYPE_CAPACITY];
		char Value[VALUE_CAPACITY];
		VariableKind Location;

		const uint8* Address;
	};

	typedef std::vector<NativeVariableDesc> TVariableList;

	struct ListVariableDescBuilder: public MemberEnumeratorCallback
	{
		TVariableList& listVars;
		VariableKind parentKind;

		ListVariableDescBuilder(TVariableList& _listVars, VariableKind _vk): listVars(_listVars), parentKind(_vk) {}

		bool IsIString(const IStructure& s) const
		{
			if (s.InterfaceCount() > 0)
			{
				const auto& pInterface0 =s.GetInterface(0);
				for (auto* pInterface = &pInterface0; pInterface != nullptr; pInterface = pInterface->Base())
				{
					if (Eq(pInterface->Name(), "IString"))
					{
						return true;
					}
				}
			}

			return false;
		}

		void OnMember(IPublicScriptSystem& ss, cstr childName, const Rococo::Compiler::IMember& member, const uint8* sfItem, int offset, int depth) override
		{
			if (depth > 5) return;

			NativeVariableDesc desc;
			CopyString(desc.Name, NativeVariableDesc::NAME_CAPACITY, childName);
			CopyString(desc.Type, NativeVariableDesc::TYPE_CAPACITY, Compiler::GetTypeName(*member.UnderlyingType()));
			if (member.IsInterfaceVariable()) StringCat(desc.Type, ("(interface)"), NativeVariableDesc::TYPE_CAPACITY);

			char value[NativeVariableDesc::VALUE_CAPACITY];
			FormatValue(ss, value, NativeVariableDesc::VALUE_CAPACITY, member.UnderlyingType()->VarType(), sfItem);

			if (IsIString(*member.UnderlyingType()))
			{
				ObjectStub* object;
				__try
				{
					object = (ObjectStub*)(sfItem + (*(InterfacePointer)sfItem)->OffsetToInstance);
					const auto* cstring = reinterpret_cast<const CStringConstant*>(object);
					SafeFormat(desc.Value, "%s: '%64s'", value, cstring->pointer);
				}
				__except(0)
				{
					SafeFormat(desc.Value, "%s", value);
				}
			}
			else
			{
				SafeFormat(desc.Value, "%s", value);
			}

			desc.Address = sfItem;

			desc.Location = parentKind;
			
			listVars.push_back(desc);

			ListVariableDescBuilder builder(listVars, parentKind);
			GetMembers(ss, *member.UnderlyingType(), childName, sfItem, 0, builder, depth);
		}

		void OnListMember(IPublicScriptSystem& ss, cstr childName, const Rococo::Compiler::IMember& member, const ListImage* l, const uint8* sfItem, int offset, int recurseDepth)
		{
			if (recurseDepth > 5) return;

			NativeVariableDesc desc;
			SafeFormat(desc.Name, NativeVariableDesc::NAME_CAPACITY, childName);
			SafeFormat(desc.Type, NativeVariableDesc::TYPE_CAPACITY, "list %s", GetFriendlyName(*member.UnderlyingGenericArg1Type()));
			SafeFormat(desc.Value, "0x%p", l);

			desc.Address = sfItem;

			desc.Location = parentKind;

			listVars.push_back(desc);

			if (l == nullptr)
			{
				return;
			}

			NativeVariableDesc lenDesc;
			SafeFormat(lenDesc.Name, "#.Length");
			SafeFormat(lenDesc.Type, "Int32");
			SafeFormat(lenDesc.Value, "%d", l->NumberOfElements);
			lenDesc.Address = (const uint8*)&l->NumberOfElements;
			lenDesc.Location = parentKind;
			listVars.push_back(lenDesc);

			NativeVariableDesc typeDesc;
			SafeFormat(typeDesc.Name, "#.ElementType");
			SafeFormat(typeDesc.Type, "IStructure");
			SafeFormat(typeDesc.Value, "%s", GetFriendlyName(*l->ElementType));
			typeDesc.Address = (const uint8*)&l->ElementType;
			typeDesc.Location = parentKind;
			listVars.push_back(typeDesc);

			NativeVariableDesc sizeDesc;
			SafeFormat(sizeDesc.Name, "#.Head");
			SafeFormat(sizeDesc.Type, "Pointer");
			SafeFormat(sizeDesc.Value, "0x%p", l->Head);
			sizeDesc.Address = (const uint8*)&l->Head;
			sizeDesc.Location = parentKind;
			listVars.push_back(sizeDesc);

			NativeVariableDesc lockDesc;
			SafeFormat(lockDesc.Name, "#.Tail");
			SafeFormat(lockDesc.Type, "Pointer");
			SafeFormat(lockDesc.Value, "0x%p", l->Tail);
			lockDesc.Address = (const uint8*)&l->Tail;
			lockDesc.Location = parentKind;
			listVars.push_back(lockDesc);
		}

		void OnArrayMember(IPublicScriptSystem& ss, cstr childName, const Rococo::Compiler::IMember& member, const ArrayImage* pArray, const uint8* sfItem, int offset, int depth) override
		{
			if (depth > 5) return;

			NativeVariableDesc desc;
			SafeFormat(desc.Name, NativeVariableDesc::NAME_CAPACITY, childName);
			if (member.UnderlyingGenericArg1Type()->InterfaceCount() > 0)
			{
				SafeFormat(desc.Type, NativeVariableDesc::TYPE_CAPACITY, "array %s", member.UnderlyingGenericArg1Type()->GetInterface(0).Name());
			}
			else
			{
				SafeFormat(desc.Type, NativeVariableDesc::TYPE_CAPACITY, "array %s", member.UnderlyingGenericArg1Type()->Name());
			}

			SafeFormat(desc.Value, "0x%p", pArray);

			desc.Address = sfItem;

			desc.Location = parentKind;

			listVars.push_back(desc);

			if (pArray == nullptr)
			{
				return;
			}

			NativeVariableDesc ptrDesc;
			SafeFormat(ptrDesc.Name, "#.C-Array-Ptr");
			SafeFormat(ptrDesc.Type, "void*");
			SafeFormat(ptrDesc.Value, "0x%p", pArray->Start);
			ptrDesc.Address = (const uint8*)&pArray->Start;
			ptrDesc.Location = parentKind;
			listVars.push_back(ptrDesc);

			NativeVariableDesc lenDesc;
			SafeFormat(lenDesc.Name, "#.Length");
			SafeFormat(lenDesc.Type, "Int32");
			SafeFormat(lenDesc.Value, "%d", pArray->NumberOfElements);
			lenDesc.Address = (const uint8*) &pArray->NumberOfElements;
			lenDesc.Location = parentKind;
			listVars.push_back(lenDesc);

			NativeVariableDesc capacityDesc;
			SafeFormat(capacityDesc.Name, "#.Capacity");
			SafeFormat(capacityDesc.Type, "Int32");
			SafeFormat(capacityDesc.Value, "%d", pArray->ElementCapacity);
			capacityDesc.Address = (const uint8*)&pArray->ElementCapacity;
			capacityDesc.Location = parentKind;
			listVars.push_back(capacityDesc);	

			NativeVariableDesc typeDesc;
			SafeFormat(typeDesc.Name, "#.ElementType");
			SafeFormat(typeDesc.Type, "IStructure");
			SafeFormat(typeDesc.Value, "%s", GetFriendlyName(*pArray->ElementType));
			typeDesc.Address = (const uint8*)&pArray->ElementType;
			typeDesc.Location = parentKind;
			listVars.push_back(typeDesc);

			NativeVariableDesc sizeDesc;
			SafeFormat(sizeDesc.Name, "#.ElementLength");
			SafeFormat(sizeDesc.Type, "Int32");
			SafeFormat(sizeDesc.Value, "%d", pArray->ElementLength);
			sizeDesc.Address = (const uint8*)&pArray->ElementLength;
			sizeDesc.Location = parentKind;
			listVars.push_back(sizeDesc);
			
			NativeVariableDesc lockDesc;
			SafeFormat(lockDesc.Name, "#.LockNumber");
			SafeFormat(lockDesc.Type, "Int32");
			SafeFormat(lockDesc.Value, "%d", pArray->LockNumber);
			lockDesc.Address = (const uint8*)&pArray->LockNumber;
			lockDesc.Location = parentKind;
			listVars.push_back(lockDesc);

			NativeVariableDesc refDesc;
			SafeFormat(refDesc.Name, "#.RefCount");
			SafeFormat(refDesc.Type, "Int32");
			SafeFormat(refDesc.Value, "%lld", pArray->RefCount);
			refDesc.Address = (const uint8*)&pArray->RefCount;
			refDesc.Location = parentKind;
			listVars.push_back(refDesc);	

			enum { MAX_ITEMS_VISIBLE = 20 };

			for (int i = 0; i < pArray->NumberOfElements; ++i)
			{
				const uint8* pElement = ((const uint8*)pArray->Start) + pArray->ElementLength * i;

				NativeVariableDesc itemDesc;
				SafeFormat(itemDesc.Name, "%s.#item_%d", childName, i);
				SafeFormat(itemDesc.Type, "Element");
				SafeFormat(itemDesc.Value, "");
				itemDesc.Address = (const uint8*) pElement;
				itemDesc.Location = parentKind;

				ListVariableDescBuilder builder(listVars, parentKind); 

				ObjectStub* object = nullptr;

				if (pArray->ElementType->InterfaceCount() > 0)
				{
					// element is an interface pointer
					pElement = *(const uint8**)pElement;

					__try
					{
						object = (ObjectStub*)(pElement + (*(InterfacePointer)pElement)->OffsetToInstance);
						const auto* type = object->Desc->TypeInfo;
						cstr name = GetFriendlyName(*type);
						SafeFormat(itemDesc.Type, "Element -> %s", name);
					}
					__except (0)
					{
						object = nullptr;
					}
				}

				listVars.push_back(itemDesc);

				if (object && IsIString(*pArray->ElementType))
				{				
					__try
					{
						const auto* cstring = reinterpret_cast<const CStringConstant*>(object);

						NativeVariableDesc textDesc;
						SafeFormat(textDesc.Name, "%s.#item_%d.(cstr)buffer", childName, i);
						SafeFormat(textDesc.Type, "const char*");
						SafeFormat(textDesc.Value, cstring->pointer);
						textDesc.Address = (const uint8*)pElement;
						textDesc.Location = parentKind;
						listVars.push_back(textDesc);
					}
					__except (0)
					{
						
					}
				}

				GetMembers(ss, *pArray->ElementType, itemDesc.Name, pElement, 0, builder, depth);

				if (i > MAX_ITEMS_VISIBLE) break;
			}	
		}

		void OnMapMember(IPublicScriptSystem& ss, cstr childName, const Rococo::Compiler::IMember& member, const MapImage* pMap, const uint8* sfItem, int offset, int depth) override
		{
			if (depth > 5) return;

			NativeVariableDesc desc;
			SafeFormat(desc.Name, NativeVariableDesc::NAME_CAPACITY, childName);
			if (member.UnderlyingGenericArg1Type()->InterfaceCount() > 0)
			{
				SafeFormat(desc.Type, NativeVariableDesc::TYPE_CAPACITY, "map %s %s", member.UnderlyingGenericArg1Type()->GetInterface(0).Name(), member.UnderlyingGenericArg2Type()->Name());
			}
			else
			{
				SafeFormat(desc.Type, NativeVariableDesc::TYPE_CAPACITY, "map %s %s ", member.UnderlyingGenericArg1Type()->Name(), member.UnderlyingGenericArg2Type()->Name());
			}

			SafeFormat(desc.Value, "0x%p", pMap);

			desc.Address = sfItem;

			desc.Location = parentKind;

			listVars.push_back(desc);

			if (pMap == nullptr)
			{
				return;
			}

			NativeVariableDesc lenDesc;
			SafeFormat(lenDesc.Name, "NumberOfElements");
			SafeFormat(lenDesc.Type, "int32");
			SafeFormat(lenDesc.Value, "0x%d", pMap->NumberOfElements);
			lenDesc.Address = (const uint8*)&pMap->NumberOfElements;
			lenDesc.Location = parentKind;
			listVars.push_back(lenDesc);

			NativeVariableDesc refDesc;
			SafeFormat(refDesc.Name, "Reference Count");
			SafeFormat(refDesc.Type, "Int64");
			SafeFormat(refDesc.Value, "%ld", pMap->refCount);
			refDesc.Address = (const uint8*)&pMap->refCount;
			refDesc.Location = parentKind;
			listVars.push_back(refDesc);

			NativeVariableDesc keyTypeDesc;
			SafeFormat(keyTypeDesc.Name, "KeyType");
			SafeFormat(keyTypeDesc.Type, "IStructure");
			SafeFormat(keyTypeDesc.Value, "%s", GetFriendlyName(*pMap->KeyType));
			keyTypeDesc.Address = (const uint8*) &pMap->KeyType;
			keyTypeDesc.Location = parentKind;
			listVars.push_back(keyTypeDesc);

			NativeVariableDesc valueTypeDesc;
			SafeFormat(valueTypeDesc.Name, "ValueType");
			SafeFormat(valueTypeDesc.Type, "IStructure");
			SafeFormat(valueTypeDesc.Value, "%s", GetFriendlyName(*pMap->ValueType));
			valueTypeDesc.Address = (const uint8*) &pMap->ValueType;
			valueTypeDesc.Location = parentKind;
			listVars.push_back(valueTypeDesc);

			NativeVariableDesc headDesc;
			SafeFormat(headDesc.Name, "Head");
			SafeFormat(headDesc.Type, "Int32");
			SafeFormat(headDesc.Value, "%p", pMap->Head);
			headDesc.Address = (const uint8*)&pMap->Head;
			headDesc.Location = parentKind;
			listVars.push_back(headDesc);

			NativeVariableDesc tailDesc;
			SafeFormat(tailDesc.Name, "Tail");
			SafeFormat(tailDesc.Type, "Int32");
			SafeFormat(tailDesc.Value, "%p", pMap->Tail);
			tailDesc.Address = (const uint8*)&pMap->Tail;
			tailDesc.Location = parentKind;
			listVars.push_back(tailDesc);
		}
	};

	List<VariableDesc>^ SexyScriptLanguage::GetElements(String^ variableName, Int32 callDepth)
	{
		IScriptSystem& ss = *ToSS(nativeSS);

		List<VariableDesc>^ vars = gcnew List<VariableDesc>();

		array<Char>^ nameArray = variableName->ToCharArray();
		pin_ptr<Char> namePin = &nameArray[0];
		const Char* namePtr = namePin;

		char sxchVariableName[256];
		if (!CopyUnicodeTochar(sxchVariableName, 256, namePtr))
		{
			return vars;
		}

		MemberDef def;
		const IStructure* pseudoType;
		const Rococo::uint8* SF;

		if (FindVariableByName(def, pseudoType, SF, ss, sxchVariableName, callDepth))
		{
			VariableKind vk;
			switch(def.location)
			{
			case VARLOCATION_NONE:
				vk = VariableKind::Pseudo;
				break;
			case VARLOCATION_INPUT:
				vk = VariableKind::Input;
				break;
			case VARLOCATION_OUTPUT:
				vk = VariableKind::Output;
				break;
			case VARLOCATION_TEMP:
			default:
				vk = VariableKind::Local;
				break;
			}
					
			const IStructure& s = *def.ResolvedType;
			if (s.VarType() == VARTYPE_Derivative || IsContainerType(s.VarType()))
			{
				const Rococo::uint8* pInstance = GetInstance(def, pseudoType, SF);
				if (pInstance != NULL)
				{
					if (s.VarType() == VARTYPE_Map)
					{
						auto* m = (MapImage*)pInstance;
						char buffer[32];
						SafeFormat(buffer, "%d", m->NumberOfElements);
						vars->Add(VariableDesc(def.SFOffset, gcnew String("NumberOfElements"), gcnew String("Int32"), gcnew String(buffer), VariableKind::Local, IntPtr((void*)m)));

						SafeFormat(buffer, "%ld", m->refCount);
						vars->Add(VariableDesc(def.SFOffset, gcnew String("Reference Count"), gcnew String("Int64"), gcnew String(buffer), VariableKind::Local, IntPtr((void*)m)));

						SafeFormat(buffer, "%s", GetFriendlyName(*m->KeyType));
						vars->Add(VariableDesc(def.SFOffset, gcnew String("KeyType"), gcnew String("IStructure"), gcnew String(buffer), VariableKind::Local, IntPtr((void*)m->KeyType)));

						SafeFormat(buffer, "%s", GetFriendlyName(*m->ValueType));
						vars->Add(VariableDesc(def.SFOffset, gcnew String("ValueType"), gcnew String("IStructure"), gcnew String(buffer), VariableKind::Local, IntPtr((void*)m->ValueType)));
					}
					else if (s.VarType() == VARTYPE_List)
					{
						auto* l = (ListImage*) pInstance;

						char buffer[32];

						SafeFormat(buffer, "%lld", l->refCount);
						vars->Add(VariableDesc(def.SFOffset, gcnew String("Reference Count"), gcnew String("Int64"), gcnew String(buffer), VariableKind::Local, IntPtr((void*)&l->refCount)));

						SafeFormat(buffer, "%d", l->NumberOfElements);
						vars->Add(VariableDesc(def.SFOffset, gcnew String("NumberOfElements"), gcnew String("Int32"), gcnew String(buffer), VariableKind::Local, IntPtr((void*)&l->NumberOfElements)));

						SafeFormat(buffer, "%s", GetFriendlyName(*l->ElementType));
						vars->Add(VariableDesc(def.SFOffset, gcnew String("ElementType"), gcnew String("IStructure"), gcnew String(buffer), VariableKind::Local, IntPtr((void*)&l->ElementType)));

						SafeFormat(buffer, "0x%p", l->Head);
						vars->Add(VariableDesc(def.SFOffset, gcnew String("Head"), gcnew String("Pointer"), gcnew String(buffer), VariableKind::Local, IntPtr((void*)&l->Head)));

						SafeFormat(buffer, "0x%p", l->Tail);
						vars->Add(VariableDesc(def.SFOffset, gcnew String("Tail"), gcnew String("Pointer"), gcnew String(buffer), VariableKind::Local, IntPtr((void*)&l->Tail)));
					}
					else
					{
						size_t subMemberOffset = 0;

						TVariableList nativeVars;
						ListVariableDescBuilder builder(nativeVars, vk);
						GetMembers(ss, s, sxchVariableName, pInstance, 0, builder, 0);

						for (auto i = nativeVars.begin(); i != nativeVars.end(); ++i)
						{
							const NativeVariableDesc& desc = *i;
							vars->Add(VariableDesc(def.SFOffset, gcnew String(desc.Name), gcnew String(desc.Type), gcnew String(desc.Value), desc.Location, IntPtr((void*)desc.Address)));
						}
					}
				}
			}
		}

		return vars;
	}

	IntPtr SexyScriptLanguage::GetCallerSF(IntPtr sf)
	{
		CPU& cpu = ToSS(nativeSS)->PublicProgramObject().VirtualMachine().Cpu();
		const uint8* callerSF = Rococo::Script::GetCallerSF(cpu, (const uint8*) sf.ToPointer());
		return IntPtr((void*)callerSF);
	}

	IntPtr SexyScriptLanguage::GetPCAddress(Int32 callDepth)
	{
		CPU& cpu = ToSS(nativeSS)->PublicProgramObject().VirtualMachine().Cpu();
		const uint8* pc = Rococo::Script::GetPCAddress(cpu, callDepth);
		return IntPtr((void*)pc);
	}

	IntPtr SexyScriptLanguage::GetReturnAddress(IntPtr sf)
	{
		CPU& cpu = ToSS(nativeSS)->PublicProgramObject().VirtualMachine().Cpu();
		const uint8* returnAddress = Rococo::Script::GetReturnAddress(cpu, (const uint8*) sf.ToPointer());
		return IntPtr((void*)returnAddress);
	}

	IntPtr SexyScriptLanguage::GetStackFrame(Int32 callDepth)
	{
		CPU& cpu = ToSS(nativeSS)->PublicProgramObject().VirtualMachine().Cpu();
		const uint8* SF = Rococo::Script::GetStackFrame(cpu, callDepth);
		return IntPtr((void*)SF);
	}

	void SexyScriptLanguage::ReleaseAll()
	{
		for each(SourceModule^ i in sourceModules->Values)
		{
			delete i;
		}

		ToDisassembler(disassemblerHandle)->Free();

		ToSS(nativeSS)->Free();

		IScriptSystemFactory* factory = (IScriptSystemFactory*)nativeFactory.ToPointer();
		factory->Free();

		delete ToLog(logHandle);

		twiddler.Free();
		twiddler2.Free();
		
		CStepCallback* sc = (CStepCallback*) stepCallbackHandle.ToPointer();
		delete sc;
	}

	void CStepCallback::OnStep(Rococo::VM::IDebugger& debugger)
	{
		int stepIndex = nextStepIndex;

		// Route messages will allow the UI to handle StepNext etc, which advances nextStepIndex
		while(stepIndex == nextStepIndex)
		{
			if (!routeMessages())
			{
				return;
			}
		}
	}
}} // SexyDotNet::Host