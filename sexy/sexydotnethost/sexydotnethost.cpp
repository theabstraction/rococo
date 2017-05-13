// This is the main DLL file.

#include "stdafx.h"
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
#include "sexy.lib.util.h"

#include <vector>

namespace Rococo
{
   namespace Compiler
   {
      csexstr GetTypeName(const IStructure& s);
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
			ToSS(nativeHandle)->Compile();
			goodstate = true;
			BeginExecution();
		}
		catch (ParseException& e)
		{
			csexstr s = e.Message();
			csexstr n = e.Name();
#ifdef SEXCHAR_IS_WIDE
			String^ msg = gcnew String(s);				
			String^ name = gcnew String(n);
#else
			String^ msg = gcnew String(s, 0, StringLength(s), Encoding::ASCII);				
			String^ name = gcnew String(n, 0, StringLength(n), Encoding::ASCII);
#endif
			SourceLocation start = SourceLocation(e.Start().x,e.Start().y);
			SourceLocation end = SourceLocation(e.End().x,e.End().y);
			throw gcnew CompileError(name, msg, start, end);
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

	void SexyScriptLanguage::BeginExecution()
	{		
		if (!initializedVM)
		{
			if (!goodstate) throw gcnew Exception("The program object is not in an executable state");
			IScriptSystem& ss = *ToSS(nativeHandle);
			IPublicProgramObject& po = ss.PublicProgramObject();

			const INamespace* ns = po.GetRootNamespace().FindSubspace(SEXTEXT("EntryPoint"));
			if(ns == NULL)
			{
				throw gcnew CompileError("<All source modules>", "Could not find the EntryPoint namespace in the source modules. Execution aborted", SourceLocation(0,0), SourceLocation(0,0));
			}

			const IFunction* f = ns->FindFunction(SEXTEXT("Main"));
			if (f == NULL)
			{
				throw gcnew CompileError("<All source modules>", "Could not find the EntryPoint.Main function in the source modules. Execution aborted", SourceLocation(0,0), SourceLocation(0,0));
			}

			po.SetProgramAndEntryPoint(*f);
			
			IVirtualMachine& vm = po.VirtualMachine();

			CStepCallback* sc = (CStepCallback*) stepCallbackHandle.ToPointer();

			vm.SetStepCallback(sc);

			vm.Push(0); // Push space for 32-bit exit code

			vm.Cpu().SetSF(vm.Cpu().SP());

			initializedVM = true;

			vm.StepInto();

			SkipJIT(po);

			ThrowOnCompileError();

		//	Disassemble();

		//	UpdateDisassembly();
		}
	}

	void SexyScriptLanguage::ThrowOnCompileError()
	{
		CLogger* logger = (CLogger*) logHandle.ToPointer();

		ParseException ex;
		logger->PopLastException(OUT ex);

		if (IsException(ex))
		{
			csexstr s = ex.Message();

#ifdef SEXCHAR_IS_WIDE
			String^ msg = gcnew String(s);
#else
			String^ msg = gcnew String(s, 0, StringLength(s), IsSexUnicode ? Encoding::Unicode : Encoding::ASCII);
#endif
				
			csexstr t = ex.Name();
#ifdef SEXCHAR_IS_WIDE
			String^ src = gcnew String(t);
#else
			String^ src = gcnew String(t, 0, StringLength(t), IsSexUnicode ? Encoding::Unicode : Encoding::ASCII);
#endif

			SourceLocation start = SourceLocation(ex.Start().x,ex.Start().y);
			SourceLocation end = SourceLocation(ex.End().x,ex.End().y);
			throw gcnew CompileError(src, msg, start, end);
		}
	}

	int SexyScriptLanguage::Execute()
	{
		BeginExecution();
		
		IScriptSystem& ss = *ToSS(nativeHandle);
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
			throw gcnew CompileError("<All source modules>", "Execution failed. EXECUTERESULT=" + iResult, SourceLocation(0,0), SourceLocation(0,0));
		}
	}

	bool SexyScriptLanguage::StepInto()
	{
		BeginExecution();

		IScriptSystem& ss = *ToSS(nativeHandle);
		IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();
		vm.StepInto();

		SkipJIT(ss.PublicProgramObject());

		ThrowOnCompileError();

		return UpdateDisassembly();
	}

	bool SexyScriptLanguage::StepOver()
	{
		BeginExecution();

		IScriptSystem& ss = *ToSS(nativeHandle);
		IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();
		vm.StepOver();

		SkipJIT(ss.PublicProgramObject());

		ThrowOnCompileError();

		return UpdateDisassembly();
	}

	bool SexyScriptLanguage::StepOut()
	{
		BeginExecution();

		IScriptSystem& ss = *ToSS(nativeHandle);
		IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();
		vm.StepOut();

		ThrowOnCompileError();

		return UpdateDisassembly();
	}

	SourceModule^ SexyScriptLanguage::AddModule(String^ moduleFullPath)
	{
		IScriptSystem& ss = *ToSS(nativeHandle);

		if (sourceModules->ContainsKey(moduleFullPath))
		{
			throw gcnew Exception("The system already contains the module " + moduleFullPath);
		}

		FileStream ^input = File::OpenRead(moduleFullPath);
		if (input->Length > (int64) HostLimits::MAX_SOURCE_FILE_KILOBYTES * 1024)
		{
			delete input;
			throw gcnew CompileError(moduleFullPath, "The file length of the module exceeded that as HostLimits::MAX_SOURCE_FILE_KILOBYTES", SourceLocation(0,0), SourceLocation(0,0));
		}

		int32 fileLength = (int32) input->Length;

		if (fileLength == 0)
		{
			delete input;
			throw gcnew CompileError(moduleFullPath, "The module was zero length", SourceLocation(0,0), SourceLocation(0,0));
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

		SEXCHAR sexName[256];
		if (!CopyUnicodeToSexChar(sexName, 256, namePtr))
		{
			throw gcnew CompileError(moduleFullPath, "Cannot convert the unicode name to sex characters", SourceLocation(0,0), SourceLocation(0,0));
		}

		ISourceCode* sc;

		// Assume that if the number of bytes in the file is even and the first high-byte is zero then we have a unicode file
		if (fileLength % 2 == 0 && inputBuffer[1] == 0)
		{
			sc = AddUnicodeModule(inputPtr, ss, fileLength / 2, sexName);
			if (sc == NULL)
			{
				throw gcnew Exception("Error adding the UNICODE source code as " + SEXCHARENCODING + "module");
			}
		}
		else
		{
			sc = AddAsciiModule(inputPtr, ss, fileLength, sexName);
			if (sc == NULL)
			{
				throw gcnew Exception("Error adding the ASCII source code as " + SEXCHARENCODING + "module");
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
			csexstr s = e.Message();

#ifdef SEXCHAR_IS_WIDE
			String^ msg = gcnew String(s);
#else
			String^ msg = gcnew String(s, 0, StringLength(s), IsSexUnicode ? Encoding::Unicode : Encoding::ASCII);
#endif

			SourceLocation start = SourceLocation(e.Start().x,e.Start().y);
			SourceLocation end = SourceLocation(e.End().x,e.End().y);
			throw gcnew CompileError(moduleFullPath, msg, start, end);
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
		IScriptSystem& ss = *ToSS(nativeHandle);

		List<VariableDesc>^ vars = gcnew List<VariableDesc>();

		size_t nVariables = Rococo::Script::GetCurrentVariableCount(ss, callDepth);

		for(size_t i = 0; i < nVariables; ++i)
		{
			csexstr name;
			MemberDef def;
			const IStructure* pseudoType;
			const uint8* SF;
			if (!GetVariableByIndex(REF name, REF def, REF pseudoType, SF, ss, i, callDepth)) continue;

			Char unicodeName[256];
			CopySexCharToUnicode(unicodeName, 256, name);

			Char unicodeValue[256];
			Char unicodeType[256];

			const void* pVariableData = SF + def.SFOffset;
			if (def.Usage == ARGUMENTUSAGE_BYVALUE)
			{
				CopySexCharToUnicode(unicodeType, 256, Compiler::GetTypeName(*def.ResolvedType));				
				FormatValue(ss, unicodeValue, 256, def.ResolvedType->VarType(), pVariableData);
			}
			else
			{
				unicodeType[0] = L'*';
				CopySexCharToUnicode(unicodeType+1, 255, Compiler::GetTypeName(*def.ResolvedType));

				const void** ppData = (const void**) pVariableData;

				__try
				{
					const void* pData = *ppData;
					_snwprintf_s(unicodeValue, _TRUNCATE, L"%p (-> %p)", pVariableData, pData);
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

		SEXCHAR Name[NAME_CAPACITY];		
		SEXCHAR Type[TYPE_CAPACITY];
		SEXCHAR Value[VALUE_CAPACITY];
		VariableKind Location;

		const uint8* Address;
	};

	typedef std::vector<NativeVariableDesc> TVariableList;

	struct ListVariableDescBuilder: public MemberEnumeratorCallback
	{
		TVariableList& listVars;
		VariableKind parentKind;

		ListVariableDescBuilder(TVariableList& _listVars, VariableKind _vk): listVars(_listVars), parentKind(_vk) {}

		virtual void OnMember(IPublicScriptSystem& ss, csexstr childName, const Rococo::Compiler::IMember& member, const uint8* sfItem)
		{
			NativeVariableDesc desc;
			CopyString(desc.Name, NativeVariableDesc::NAME_CAPACITY, childName, -1);
			CopyString(desc.Type, NativeVariableDesc::TYPE_CAPACITY, Compiler::GetTypeName(*member.UnderlyingType()), -1);
			if (member.IsPseudoVariable()) StringCat(desc.Type, SEXTEXT("(pseudo)"), NativeVariableDesc::TYPE_CAPACITY);

			char value[NativeVariableDesc::VALUE_CAPACITY];
			FormatValue(ss, value, NativeVariableDesc::VALUE_CAPACITY, member.UnderlyingType()->VarType(), sfItem);

			CopyAsciiToToSEXCHAR(desc.Value, NativeVariableDesc::VALUE_CAPACITY, value);
			desc.Address = sfItem;

			desc.Location = parentKind;
			
			listVars.push_back(desc);

			if (!member.IsPseudoVariable())
			{
				ListVariableDescBuilder builder(listVars, parentKind);
				GetMembers(ss, *member.UnderlyingType(), childName, sfItem, 0, builder);
			}
		}
	};

	List<VariableDesc>^ SexyScriptLanguage::GetElements(String^ variableName, Int32 callDepth)
	{
		IScriptSystem& ss = *ToSS(nativeHandle);

		List<VariableDesc>^ vars = gcnew List<VariableDesc>();

		array<Char>^ nameArray = variableName->ToCharArray();
		pin_ptr<Char> namePin = &nameArray[0];
		const Char* namePtr = namePin;

		SEXCHAR sxchVariableName[256];
		if (!CopyUnicodeToSexChar(sxchVariableName, 256, namePtr))
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
			if (s.VarType() == VARTYPE_Derivative)
			{
				const Rococo::uint8* pInstance = GetInstance(def, pseudoType, SF);
				if (pInstance != NULL)
				{
					size_t subMemberOffset = 0;
						
					TVariableList nativeVars;
					ListVariableDescBuilder builder(nativeVars, vk);
					GetMembers(ss, s, sxchVariableName, pInstance, 0, builder);

					for(auto i = nativeVars.begin(); i != nativeVars.end(); ++i)
					{
						const NativeVariableDesc& desc = *i;
						vars->Add(VariableDesc(def.SFOffset, gcnew String(desc.Name), gcnew String(desc.Type), gcnew String(desc.Value), desc.Location, IntPtr((void*)desc.Address)));
					}
				}
			}
		}

		return vars;
	}

	IntPtr SexyScriptLanguage::GetCallerSF(IntPtr sf)
	{
		CPU& cpu = ToSS(nativeHandle)->PublicProgramObject().VirtualMachine().Cpu();
		const uint8* callerSF = Rococo::Script::GetCallerSF(cpu, (const uint8*) sf.ToPointer());
		return IntPtr((void*)callerSF);
	}

	IntPtr SexyScriptLanguage::GetPCAddress(Int32 callDepth)
	{
		CPU& cpu = ToSS(nativeHandle)->PublicProgramObject().VirtualMachine().Cpu();
		const uint8* pc = Rococo::Script::GetPCAddress(cpu, callDepth);
		return IntPtr((void*)pc);
	}

	IntPtr SexyScriptLanguage::GetReturnAddress(IntPtr sf)
	{
		CPU& cpu = ToSS(nativeHandle)->PublicProgramObject().VirtualMachine().Cpu();
		const uint8* returnAddress = Rococo::Script::GetReturnAddress(cpu, (const uint8*) sf.ToPointer());
		return IntPtr((void*)returnAddress);
	}

	IntPtr SexyScriptLanguage::GetStackFrame(Int32 callDepth)
	{
		CPU& cpu = ToSS(nativeHandle)->PublicProgramObject().VirtualMachine().Cpu();
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

		ToSS(nativeHandle)->Free();
		delete ToLog(logHandle);

		twiddler.Free();
		twiddler2.Free();
		
		CStepCallback* sc = (CStepCallback*) stepCallbackHandle.ToPointer();
		delete sc;
	}

	void CStepCallback::OnStep(Rococo::VM::IDebugger& debugger)
	{
		int stepIndex = nextStepIndex;

		while(stepIndex == nextStepIndex)
		{
			if (!routeMessages())
			{
				return;
			}
		}
	}
}} // SexyDotNet::Host