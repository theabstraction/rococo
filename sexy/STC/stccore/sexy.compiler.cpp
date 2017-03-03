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

#include "Sexy.Compiler.StdAfx.h"
#include "Sexy.Validators.h"
#include "Sexy.VM.h"
#include "Sexy.VM.CPU.h"
#include "sexy.compiler.helpers.h"
#include "sexy.strings.h"
#include "sexy.stdstrings.h"

using namespace Sexy;
using namespace Sexy::Compiler;
using namespace Sexy::VM;

namespace Sexy { namespace Compiler {
	ICodeBuilder* CreateBuilder(IFunctionBuilder& f, bool mayUseParentsSF);
	void Throw(STCException& e);
}} // Sexy::Compiler


#include "Sexy.Compiler.inl"
#include "sexy.compiler.attributes.inl"
#include "sexy.compiler.Functions.inl"
#include "sexy.compiler.modules.inl"
#include "sexy.compiler.structures.inl"
#include "sexy.compiler.namespaces.inl"
#include "sexy.compiler.symbols.inl"

using namespace Sexy::Compiler::Impl;

namespace
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

	class ProgramObject: public IProgramObject
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
	public:
		virtual ILog& Log()	{	return log;	}
		virtual void Free() { delete this; }
		virtual IModuleBuilder& GetModule(int index) { return *modules[index]; }
		virtual const IModule& GetModule(int index) const { return *modules[index]; }
		virtual int ModuleCount() const { return (int32) modules.size();	}
		virtual const INamespace& GetRootNamespace() const { return *rootNS;	}
		virtual INamespaceBuilder& GetRootNamespace() { return *rootNS;	}
		virtual IVirtualMachine& VirtualMachine() { return *virtualMachine; }
		virtual IProgramMemory& ProgramMemory() { return *program; }
		virtual const IProgramMemory& ProgramMemory() const { return *program; }
		virtual IModuleBuilder& IntrinsicModule() { return *intrinsics;	}
		virtual const IModule& IntrinsicModule() const { return *intrinsics;	}
		virtual CommonStructures& Common() { return *common; }

		ProgramObject(const ProgramInitParameters& pip, ILog& _log): log(_log)
		{
			CoreSpec spec;
			spec.Reserved = 0;
			spec.SizeOfStruct = sizeof(spec);
			spec.Version = CORE_LIB_VERSION;
			svmCore = CreateSVMCore(&spec);
			program = svmCore->CreateProgramMemory(pip.MaxProgramBytes);
			virtualMachine = svmCore->CreateVirtualMachine();			
			rootNS = new Namespace(*this);
			intrinsics = Module::CreateIntrinsics(*this, SEXTEXT("!Intrinsics!"));

			IAssembler* assembler = svmCore->CreateAssembler();
			BuildStub(*assembler, *program);
			assembler->Free();

			common = NULL;
		}

		~ProgramObject()
		{
			ClearSymbols(symbols);

			for(auto i = modules.begin(); i != modules.end(); ++i)
			{
				Module* m = *i;
				delete m;
			}

			modules.clear();
						
			delete rootNS;
			delete intrinsics;

			virtualMachine->Free();
			svmCore->Free();

			delete common;
		}

		void InitCommon()
		{
			common = new CommonStructures(*this);
		}

		csexstr RegisterSymbol(csexstr text)
		{
			return AddSymbol(symbols, text);
		}

		IStructureBuilder& AddIntrinsicStruct(csexstr name, size_t sizeOfType, VARTYPE underlyingType, const IArchetype* archetype)
		{
			StructurePrototype prototype(MEMBERALIGN_1, INSTANCEALIGN_1, true, archetype, false);

			Structure* s = new Structure(name, prototype, *intrinsics, underlyingType, NULL);
			s->AddMember(NameString::From(SEXTEXT("Value")), TypeString::From(name));
			StructureMember& m = s->GetMemberRef(0);
			if (!m.IsPseudoVariable()) m.SetSize(sizeOfType);
			s->Seal();
			intrinsics->Structures().Register(s->Name(), *s);
			return *s;
		}

		virtual IModuleBuilder& AddModule(csexstr name)
		{
			Module* m = new Module(*this, name);
			modules.push_back(m);
			return *m;
		}

		virtual void SetProgramAndEntryPoint(const IFunction& f)
		{
			CodeSection cs;
			f.Code().GetCodeSection(cs);	
			SetProgramAndEntryPoint(cs.Id);
		}

		virtual void SetProgramAndEntryPoint(ID_BYTECODE bytecodeId)
		{
			virtualMachine->SetProgram(program);
			virtualMachine->InitCpu();
			virtualMachine->Cpu().D[VM::REGISTER_D5].byteCodeIdValue = bytecodeId;
		}

      virtual const IStructure* GetSysType(SEXY_CLASS_ID id)
      {
         if (systemStructures.empty())
         {
            if (!modules.empty())
            {
               auto* s = modules[0]->FindStructure(SEXTEXT("StringBuilder"));
               if (s)
               {
                  systemStructures.push_back(s);
               }
            }
         }
        
         return id >= systemStructures.size() ? nullptr : systemStructures[id];
      }

		virtual void ResolveNativeTypes()
		{
			ResolveTypesInIntrinsics(*this);
		}

		virtual bool ResolveDefinitions()
		{
			if (!ResolveStructures(*this))
			{
				return false;
			}

			if (!ExpandMembersAndValidate(*this))
			{
				Log().Write(SEXTEXT("Failed to compute sizes for all structures. Reduce null objects member dependencies inside structure definitions"));
				return false;
			}

			for(auto i = modules.begin(); i != modules.end(); ++i)
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

namespace Sexy { namespace Compiler
{
	COMPILER_API IProgramObject* CreateProgramObject_1_0_0_0(const ProgramInitParameters& pip, ILog& log)
	{
		return new ProgramObject(pip, log);
	}

	void ValidateNotNull(void* p)
	{
		if (p == NULL)
			Throw(Sexy::Compiler::ERRORCODE_NULL_POINTER, SEXTEXT("Null ptr"), SEXTEXT("item"));
	}

	CommonStructures::CommonStructures(IProgramObject& obj)
	{
		ValidateNotNull(this->root = &obj.GetRootNamespace());
		ValidateNotNull(this->sys = root->FindSubspace(SEXTEXT("Sys")));
		ValidateNotNull(this->sysType = sys->FindSubspace(SEXTEXT("Type")));
		ValidateNotNull(this->sysNative = sys->FindSubspace(SEXTEXT("Native")));
		ValidateNotNull(this->sysReflection = sys->FindSubspace(SEXTEXT("Reflection")));

		ValidateNotNull(this->typeInt32 = obj.IntrinsicModule().FindStructure(SEXTEXT("Int32")));
		ValidateNotNull(this->typeInt64 = obj.IntrinsicModule().FindStructure(SEXTEXT("Int64")));
		ValidateNotNull(this->typeBool = obj.IntrinsicModule().FindStructure(SEXTEXT("Bool")));
		ValidateNotNull(this->typeFloat32 = obj.IntrinsicModule().FindStructure(SEXTEXT("Float32")));
		ValidateNotNull(this->typeFloat64 = obj.IntrinsicModule().FindStructure(SEXTEXT("Float64")));
		ValidateNotNull(this->typePointer = obj.IntrinsicModule().FindStructure(SEXTEXT("Pointer")));
		ValidateNotNull(this->typeNode = obj.GetModule(0).FindStructure(SEXTEXT("_Node")));
		ValidateNotNull(this->typeArray = obj.GetModule(0).FindStructure(SEXTEXT("_Array")));
		ValidateNotNull(this->typeMapNode = obj.GetModule(0).FindStructure(SEXTEXT("_MapNode")));

		ValidateNotNull(this->sysTypeIString = sysType->FindInterface(SEXTEXT("IString")));
		ValidateNotNull(this->sysTypeIException = sysType->FindInterface(SEXTEXT("IException")));
		ValidateNotNull(this->sysTypeIExpression = sysReflection->FindInterface(SEXTEXT("IExpression")));

		ValidateNotNull(this->typeStringLiteral = obj.GetModule(0).FindStructure(SEXTEXT("StringConstant")));
	}

	void Throw(ERRORCODE code, csexstr source, csexstr format, ...)
	{
		va_list args;
		va_start(args, format);

		SEXCHAR message[256];
		StringPrintV(message, 256, args, format);
		STCException e(code, source, message);	
		Throw(e);
	}

	void HighLightText(SEXCHAR* outputBuffer, size_t nBytesOutput, csexstr highlightPos, csexstr wholeString)
	{
		SEXCHAR charbuf[4];
		StringPrint(charbuf, 4, SEXTEXT("[%c]"), *highlightPos);
		int startChars = (int32)(highlightPos - wholeString);
		CopyString(outputBuffer, nBytesOutput, wholeString, std::min<int32>((int32)nBytesOutput, startChars));
		StringCat(outputBuffer, charbuf, (int32) nBytesOutput);
		StringCat(outputBuffer, highlightPos+1, (int32) nBytesOutput);
	}
	
	bool IsCapital(SEXCHAR c) { return (c >= 'A' && c <= 'Z'); }
	bool IsLowerCase(SEXCHAR c) { return (c >= 'a' && c <= 'z'); }
	bool IsNumeral(SEXCHAR c) { return c >= '0' && c < '9'; }
	bool IsAlpha(SEXCHAR c) { return IsCapital(c) || IsLowerCase(c); }
	bool IsAlphaNumeral(SEXCHAR c) { return IsAlpha(c) || IsNumeral(c); }
	
	COMPILER_API void ValidateCapitalLetter(csexstr s, csexstr stringStart, csexstr name, csexstr functionSymbol)
	{
		if (!IsCapital(*s))
		{
			SEXCHAR text[256];
			HighLightText(text, 256, s, stringStart);
			Throw(ERRORCODE_BAD_ARGUMENT, functionSymbol, SEXTEXT("%s, Expecting capital letter A-Z: %s"), name, text);
		}
	}

	COMPILER_API void ValidateLowerCaseLetter(csexstr s, csexstr stringStart, csexstr name, csexstr functionSymbol)
	{
		if (!IsLowerCase(*s))
		{
			SEXCHAR text[256];
			HighLightText(text, 256, s, stringStart);
			Throw(ERRORCODE_BAD_ARGUMENT, functionSymbol, SEXTEXT("%s, Expecting lower case a-z: %s"), name, text);
		}
	}

	COMPILER_API void ValidateNumeral(csexstr s, csexstr stringStart, csexstr name, csexstr functionSymbol)
	{
		if (!IsNumeral(*s))
		{
			SEXCHAR text[256];
			HighLightText(text, 256, s, stringStart);
			Throw(ERRORCODE_BAD_ARGUMENT, functionSymbol, SEXTEXT("%s, Expecting numeral 0-9: %s"), name, text);
		}
	}

	COMPILER_API void ValidateAlpha(csexstr s, csexstr stringStart, csexstr name, csexstr functionSymbol)
	{		
		if (!IsAlpha(*s)) 
		{
			SEXCHAR text[256];
			HighLightText(text, 256, s, stringStart);
			Throw(ERRORCODE_BAD_ARGUMENT, functionSymbol, SEXTEXT("%s, Expecting letter A-Z or a-z: %s"), name, text);
		}
	}

	COMPILER_API void ValidateAlphaNumeral(csexstr s, csexstr stringStart, csexstr name, csexstr functionSymbol)
	{		
		if (!IsAlphaNumeral(*s)) 
		{
			SEXCHAR text[256];
			HighLightText(text, 256, s, stringStart);
			Throw(ERRORCODE_BAD_ARGUMENT, functionSymbol, SEXTEXT("%s, Expecting alphanumeric A-Z or a-z or 0-9: %s"), name, text);
		}
	}

	COMPILER_API void ValidateNamespaceString(csexstr s, csexstr name, csexstr functionSymbol)
	{
		if (s == NULL) Throw(ERRORCODE_NULL_POINTER, functionSymbol, SEXTEXT("[%s] was NULL"), name);
		if (s[0] == 0) Throw(ERRORCODE_EMPTY_STRING, functionSymbol, SEXTEXT("[%s] was empty string"), name);

		enum STATE
		{
			STATE_START_BRANCH,
			STATE_MID_BRANCH,
		} state = STATE_START_BRANCH;

		for(csexstr p = s; *p != 0; p++)
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
			Throw(ERRORCODE_BAD_ARGUMENT, functionSymbol, SEXTEXT("[%s] terminated in a dot: %s"), name, s);
		}
	}
}} // Sexy::Compiler

