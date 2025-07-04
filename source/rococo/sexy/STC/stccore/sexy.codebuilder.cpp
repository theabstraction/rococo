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

#include "sexy.compiler.stdafx.h"
#include "sexy.validators.h"
#include "sexy.vm.h"
#include "sexy.compiler.helpers.h"
#include "sexy.strings.h"
#include "sexy.stdstrings.h"

#include <sexy.list.h>

using namespace Rococo;
using namespace Rococo::Compiler;
using namespace Rococo::Parse;
using namespace Rococo::Sex;
using namespace Rococo::Strings;

namespace Rococo
{
	bool IsPointerValid(const void* ptr);

	namespace Compiler
	{
		void UseStackFrameFor(ICodeBuilder& builder, const MemberDef& def)
		{
			if (def.IsParentValue) 
			{
				builder.AssignClosureParentSFtoD6();
				builder.Assembler().Append_SwapRegister(VM::REGISTER_SF, VM::REGISTER_D6);
			}
		}

		void RestoreStackFrameFor(ICodeBuilder& builder, const MemberDef& def)
		{
			// The current function, if a closure, will put the parent's stack frame in register D6
			// If the variable specified by <def> is from the parent's stack, then swap back D6 from SF
			if (def.IsParentValue) { builder.Assembler().Append_SwapRegister(VM::REGISTER_D6, VM::REGISTER_SF); }
		}
	}
}

namespace Rococo
{
	namespace Script
	{
		int GetIndexOfInterface(const IStructure& concreteClass, const IInterface& interf)
		{
			for (int i = 0; i < concreteClass.InterfaceCount(); ++i)
			{
				for (auto* I = &concreteClass.GetInterface(i); I != nullptr; I = I->Base())
				{
					if (I == &interf)
					{
						return i;
					}
				}
			}

			return -1;
		}
	}
}

namespace Rococo::Compiler
{
	bool IsDerivedFrom(const IInterface& sub, const IInterface& super)
	{
		for (const IInterface* i = &sub; i != nullptr; i = i->Base())
		{
			if (i == &super) return true;
		}

		return false;
	}
}

namespace Anon
{
	class Variable
	{
	private:
		stdstring name;
		ARGUMENTUSAGE usage;
		int32 sectionIndex;
		const IStructure& resolvedType;
		int32 offset;
		void* userData;
		int32 allocSize;
		size_t pcStart;
		size_t pcEnd;
		int primaryOffset;
		VARLOCATION location;
		int tempDepthOnRelease;
		bool canCaptureClosureData;
	public:
		Variable(const IArgument& arg, int32 _sectionIndex, void* _userData, int _primaryOffset):
			name(arg.Name()),
			usage(arg.Usage()),
			sectionIndex(_sectionIndex),
			resolvedType(*arg.ResolvedType()),
			userData(_userData),
			allocSize(-1),
			pcStart(0),
			pcEnd((size_t) -1),
			primaryOffset(_primaryOffset),
			tempDepthOnRelease(-1),
			canCaptureClosureData(arg.IsClosureInput())
		{
			location = arg.Direction() == ARGDIRECTION_INPUT ? VARLOCATION_INPUT : VARLOCATION_OUTPUT;
		}

		void* operator new(size_t nBytes)
		{
			return Rococo::Memory::AllocateSexyMemory(nBytes);
		}

		void operator delete(void* buffer)
		{
			return Rococo::Memory::FreeSexyUnknownMemory(buffer);
		}

		Variable(size_t _pcStart, const NameString& name, const IStructure& resolvedType, int32 _sectionIndex, void* _userData, int _allocSize, VARLOCATION _location, bool asRef = false, int _tempDepthOnRelease = -1):
			name(name.c_str()),
			usage(asRef ? ARGUMENTUSAGE_BYREFERENCE : ARGUMENTUSAGE_BYVALUE),
			sectionIndex(_sectionIndex),
			resolvedType(resolvedType),
			userData(_userData),
			allocSize(_allocSize),
			pcStart(_pcStart),
			pcEnd((size_t) - 1),
			primaryOffset(0),
			location(_location),
			tempDepthOnRelease(_tempDepthOnRelease),
			canCaptureClosureData(false)
		{

		}

		void AllowCaptureClosure()
		{
			canCaptureClosureData = true;
		}

		cstr Name() const { return name.c_str(); }
		ARGUMENTUSAGE Usage() const { return usage; }
		const int32 SectionIndex() const { return sectionIndex; }		
		const IStructure& ResolvedType() const { return resolvedType; }		
		void SetStackPosition(int _offset) { offset = _offset; }
		const int Offset() const { return offset; }
		void* Userdata() const { return userData; }
		int AllocSize() const { return allocSize < 0 ? resolvedType.SizeOfStruct() : allocSize;  }
		size_t PCStart() const { return pcStart; }
		size_t PCEnd() const { return pcEnd; }
		void SetPCEnd(size_t _pcEnd) { pcEnd = _pcEnd; }
		const int PrimaryOffset() const { return primaryOffset; }
		VARLOCATION Location() const { return location; }
		const int TempDepthOnRelease() const { return tempDepthOnRelease; } 
		bool CanCaptureClosures() const { return canCaptureClosureData; }
	};

	typedef TSexyVector<Variable*> TVariables;
	typedef TSexyVector<ControlFlowData> TControlStack;
	typedef TSexyHashMap<size_t,StackRecoveryData, std::hash<size_t>, std::equal_to<size_t>> TMapCodeOffsetToStackCorrection;
	typedef TSexyVector<int> TInstancePositions;
	typedef TSexyVector<int> TSectionStack;
	typedef TSexyHashMap<size_t,SymbolValue> TPCSymbols;
	typedef TSexyVector<const IStructure*> TTypeVector;

	struct OutstandingLabel
	{
		cstr labelName;
		int32 sectionIndex;
	};
	typedef std::vector<OutstandingLabel> TOutstandingLabels;

	class CodeBuilder: public ICodeBuilder
	{
	private:
		mutable bool codeReferencesParentsSF;
		size_t functionStartPosition;
		size_t functionEndPosition;
		ID_BYTECODE byteCodeId;
		VM::IAssembler* assembler;
		TMapCodeOffsetToStackCorrection stackCorrections;
		StackRecoveryData nullCorrection;
		TTypeVector posToType;
		TInstancePositions destructorPositions;
		TSectionStack sections;
		TPCSymbols pcSymbols;

		int endOfArgs;
		int32 sectionIndex;
		int32 allocatedIndexLevel;
		bool mayUseParentsSF;
		bool needInitArgs;

		IFunctionBuilder& f;

		int32 thisOffset;
		int32 nextOffset;

		TVariables variables;
		TVariables expiredVariables;
		TControlStack controlStack;

		TOutstandingLabels outstandingLabels;

		struct LabelInfo
		{
			size_t pcOffset;
			int32 sectionIndex;
		};
		TSexyStringMap<LabelInfo> mapLabelToPosition;
		
		Variable* TryGetVariableByName(cstr name, OUT int32& offsetCorrect);		
		const Variable* TryGetVariableByName(cstr name, OUT int32& offsetCorrect) const;

		bool IsVariableDefinedAtLevel(int32 sectionIndex, cstr name);
		
		void AllocateLocalVariable(Variable& v, REF size_t& stackExpansion);
		void GetCodeSection(OUT CodeSection& section) const { section.Start = functionStartPosition; section.End = functionEndPosition; section.Id = byteCodeId; }
		void BuildInputAndOutputOffsets();

		Variable& DemandVariableByName(cstr name, cstr clue, OUT int32& offsetCorrect);		
		int AddVariableToVariables(Variable* v);

		int nextId;

		TSexyHashMap<size_t, cstr, std::hash<size_t>, std::equal_to<size_t>> mapGotoStatementsToLabels;
	public:
		CodeBuilder(IFunctionBuilder& _f, bool _mayUseParentsSF);
		~CodeBuilder(void);

		DEFINE_SEXY_ALLOCATORS_FOR_CLASS;

		Rococo::VM::IAssembler& Assembler() { return *assembler; }

		IModuleBuilder& Module() { return f.Module(); }
		const IModule& Module() const { return f.Module(); }

		virtual int SectionArgCount() const { return sections.back(); }
		virtual int SectionArgCount(size_t index) const { return sections[index]; }

		virtual IFunctionBuilder& Owner() { return f; }
		virtual const IFunction& Owner() const { return f; }
		virtual void AddExpression(IBinaryExpression& tree) override;
		virtual void AddDestructors(size_t startPosition, size_t endPosition) override;
		virtual size_t GetLabelPosition(cstr labelName) override;
		virtual void MarkGoto(size_t gotoPosition, cstr labelName) override;
		virtual void MarkLabel(cstr labelName) override;
		virtual void PreventMoreVariablesUntil(cstr label) override;
		virtual void Begin() override;
		virtual void EnterSection() override;
		virtual void Append_DecRef()  override;
		virtual void Append_IncRef() override;
		virtual void Append_IncDerivativeRefs(cstr value) override;
		virtual void Append_GetAllocSize() override;
		virtual void AddVariable(const NameString& name, const TypeString& type, void* userData) override;
		virtual void AddVariable(const NameString& name, const IStructure& type, void* userData) override;
		virtual void AddVariableRef(const NameString& name, const IStructure& type, void* userData) override;
		virtual void AddInterfaceVariable(const NameString& ns, const IStructure& st, void* userData) override;
		virtual void AssignLiteral(const NameString& name, cstr valueLiteral) override;
		virtual void AssignPointer(const NameString& name, const void* ptr) override;
		virtual void AssignVariableToVariable(cstr source, cstr value, bool isConstructingTarget) override;
		virtual void AssignVariableToTemp(cstr source, int tempIndex, int offsetCorrection) override;
		virtual void AssignVariableAddressToTemp(cstr sourceVariable, int tempDepth) override;
		virtual void AssignVariableRefToTemp(cstr source, int tempDepth, int offset) override;
		virtual void AssignVariableToGlobal(const GlobalValue& g, const MemberDef& def) override;
		virtual void AssignVariableFromGlobal(const GlobalValue& g, const MemberDef& def) override;
		virtual void AssignLiteralToGlobal(const GlobalValue& g, const VariantValue& value) override;
		virtual void AssignTempToVariable(int srcIndex, cstr target) override;
		virtual void BinaryOperatorAdd(int srcInvariantIndex, int trgMutatingIndex, SexyVarType type) override;
		virtual void BinaryOperatorSubtract(int srcInvariantIndex, int trgMutatingIndex, SexyVarType type) override;
		virtual void BinaryOperatorMultiply(int srcInvariantIndex, int trgInvariantIndex, SexyVarType type) override;
		virtual void BinaryOperatorDivide(int srcInvariantIndex, int trgInvariantIndex, SexyVarType type) override;
		virtual void AppendConditional(CONDITION condition, ICompileSection& thenSection, ICompileSection& elseSection) override;
		virtual void AppendDoWhile(ICompileSection& loopBody, ICompileSection& loopCriterion, CONDITION condition) override;
		virtual void AppendWhileDo(ICompileSection& loopCriterion, CONDITION condition, ICompileSection& loopBody, ICompileSection& finalSection) override;
		virtual void Append_UpdateRefsOnSourceAndTarget()  override;
		virtual void LeaveSection()  override;
		virtual void End()  override;
		virtual SexyVarType GetVarType(cstr name) const  override;
		virtual const IStructure* GetVarStructure(cstr name) const  override;
		virtual void AssignClosureParentSFtoD6() override;
		virtual void EnableClosures(cstr targetVariable) override;
		virtual void Free() override { delete this; }
		virtual bool TryGetVariableByName(OUT MemberDef& def, cstr name) const override;
		virtual int GetVariableCount() const override;
		virtual void GetVariableByIndex(OUT MemberDef& def, cstr& name, int index) const override;
		virtual void PopControlFlowPoint() override;
		virtual void PushControlFlowPoint(const ControlFlowData& controlFlowData) override;
		virtual bool TryGetControlFlowPoint(OUT ControlFlowData& data) override;
		virtual const bool NeedsParentsSF() const override { return codeReferencesParentsSF; }

		virtual void PopLastVariables(int count, bool expireVariables) override;
		virtual const StackRecoveryData& GetRequiredStackCorrection(size_t codeOffset) const override;
		virtual void NoteStackCorrection(int stackCorrection) override;
		virtual void NoteDestructorPosition(int instancePosition, const IStructure& type) override;
		virtual int GetDestructorFromInstancePos(int instancePosition) const override
		{
			if (instancePosition < 0 || instancePosition >= (int) destructorPositions.size())
			{
				Throw(ERRORCODE_BAD_ARGUMENT, ("GetDestructorFromInstancePos"), ("Bad instancePosition: %d"), instancePosition);
			}
			return destructorPositions[instancePosition]; 
		}
		virtual const IStructure& GetTypeFromInstancePos(int instancePosition) const override { return *posToType[instancePosition]; }

		virtual void AddSymbol(cstr text) override;
		virtual void MarkExpression(const void* sourceExpression) override;
		virtual void DeleteSymbols() override;
		virtual SymbolValue GetSymbol(size_t pcAddressOffset) const override;

		virtual int GetLocalVariableSymbolCount() const override;
		virtual void GetLocalVariableSymbolByIndex(OUT MemberDef& def, OUT cstr& name, IN int index) const override;
		virtual bool GetLocalVariableSymbolByName(OUT MemberDef& def, OUT cstr name, IN size_t pcAddress) const override;

		virtual void PushVariable(const MemberDef& def) override;
		virtual void PushVariableRef(cstr source, int interfaceIndex) override;

		virtual int GetOffset(size_t argIndex) const override;

		virtual int GetThisOffset() const override;
		virtual void SetThisOffset(int offset) override;

		virtual void ArchiveRegister(int saveTempDepth, int restoreTempDepth, BITCOUNT bits, void* userData) override;
		virtual void AddArgVariable(cstr desc, const TypeString& typeName, void* userData) override;
		virtual void AddArgVariable(cstr desc, const IStructure& type, void* userData) override;

		virtual void AddDynamicAllocateObject(const IStructure& structType, const IInterface& interface) override;

		bool TryAssignClassInterfaceToInterface(cstr source, cstr target, const IStructure* srcType, const IStructure*trgType);

		uint64 NextId() override
		{
			return nextId++;
		}

		std::vector<HString> lambdaVariables;

		void PushLambdaVar(cstr variableName) override
		{
			lambdaVariables.push_back(variableName);
		}

		void PopLambdaVar(OUT char result[MAX_FQ_NAME_LEN]) override
		{
			if (lambdaVariables.empty())
			{
				Throw(0, "lambda variables were empty. Internal compiler error.");
			}

			SafeFormat(OUT result, MAX_FQ_NAME_LEN, "%s", lambdaVariables.back().c_str());
			lambdaVariables.pop_back();
		}
	};

	CodeBuilder::CodeBuilder(IFunctionBuilder& _f, bool _mayUseParentsSF):
		f(_f),
		sectionIndex(0),
		allocatedIndexLevel(-1),
		functionStartPosition((size_t)-1),
		functionEndPosition((size_t)-1),
		nextOffset(0),
		codeReferencesParentsSF(false),
		mayUseParentsSF(_mayUseParentsSF),
		thisOffset(0),
		needInitArgs(true),
		nextId(0)
	{
		byteCodeId = f.Object().ProgramMemory().AddBytecode();
		assembler = _f.Object().VirtualMachine().Core().CreateAssembler();
		nullCorrection.TotalDisplacement = 0;
		nullCorrection.InstancePosCount = 0;
		nullCorrection.InstancePosStart = 0;
	}

	CodeBuilder::~CodeBuilder(void)
	{
		assembler->Free();

		for (auto v: variables)
		{
			delete v;
		}

		for(auto v : expiredVariables)
		{
			delete v;
		}
	}

	void CodeBuilder::AddDynamicAllocateObject(const IStructure& structType, const IInterface& interface)
	{
		char sym[256];

		auto* binding = f.Object().AllocatorMap().GetAllocator(structType);

		int index = Rococo::Script::GetIndexOfInterface(structType, interface);
		if (index < 0) Throw(0, "%s does not support interface %s", GetFriendlyName(structType), interface.Name());

		AssignVariableToTemp("_this", 0, 0); // Interface pointer variable is now in D4

		VariantValue v;
		v.ptrDiffValue = ObjectStub::BYTECOUNT_INSTANCE_TO_INTERFACE0 + sizeof(size_t) * index;

		SafeFormat(sym, sizeof(sym), "Instance offset for %s", structType.Name());
		AddSymbol(sym);
		Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7, v, BITCOUNT_POINTER);
	

		SafeFormat(sym, sizeof(sym), "Allocator for %s", structType.Name());
		AddSymbol(sym);

		VariantValue varBinding;
		varBinding.vPtrValue = binding;
		Assembler().Append_SetRegisterImmediate(VM::REGISTER_D5, varBinding, BITCOUNT_POINTER);
		Assembler().Append_Invoke(f.Object().GetCallbackIds().IdAllocate);
	}

	int CodeBuilder::AddVariableToVariables(Variable* v)
	{
		variables.push_back(v);

		int32 dx = (int32) v->ResolvedType().SizeOfStruct();

		int offset = nextOffset; // SF takes us just beyond the return address of the caller
		nextOffset += dx;

		v->SetStackPosition(offset);

		return dx;
	}

	void CodeBuilder::ArchiveRegister(int saveTempDepth, int restoreTempDepth, BITCOUNT bits, void* userData)
	{
		TokenBuffer name;
		SafeFormat(name.Text, TokenBuffer::MAX_TOKEN_CHARS, ("_archive_D%d_%d"), saveTempDepth + 4, nextId++);

		const IStructure& type =  (bits == BITCOUNT_32) ? Module().Object().Common().TypeInt32() :  Module().Object().Common().TypeInt64();

		Variable *v = new Variable(Assembler().WritePosition(), NameString::From(name), type, sectionIndex, userData, -1, VARLOCATION_TEMP, false, restoreTempDepth);
		int dx = AddVariableToVariables(v);
		if (dx >= 0) Assembler().Append_PushRegister((VM::DINDEX) saveTempDepth + 4, bits);
	}

	void CodeBuilder::AddArgVariable(cstr desc, const IStructure& type, void* userData)
	{
		TokenBuffer name;
		SafeFormat(name.Text, TokenBuffer::MAX_TOKEN_CHARS, ("_arg_%s_%d"), desc, nextId++);

		const IStructure* argType = NULL;

		switch (type.VarType())
		{
		case SexyVarType_Array:
		case SexyVarType_List:
		case SexyVarType_Map:
		case SexyVarType_Derivative:
			argType = &Module().Object().Common().TypePointer();
			break;
		default:
			argType = &type;
			break;
		}

		Variable *v = new Variable(Assembler().WritePosition(), NameString::From(name), *argType, sectionIndex, userData, -1, VARLOCATION_TEMP, false);
		int dx = AddVariableToVariables(v);
		UNUSED(dx);
	}

	void CodeBuilder::AddArgVariable(cstr desc, const TypeString& typeName, void* userData)
	{
		const IStructure* type = MatchStructure(Module().Object().Log(), typeName.c_str(), Module());

		if (type != nullptr)
		{
			AddArgVariable(desc, *type, userData);	
		}
		else
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("Unknown type [%s]"), typeName);
		}
	}

	void CodeBuilder::SetThisOffset(int offset)
	{
		thisOffset = offset;
	}

	int CodeBuilder::GetThisOffset() const
	{
		return thisOffset;
	}

	void CodeBuilder::PushVariable(const MemberDef& def)
	{
		// Scenarios, def corresponds to...
		//    1) an output variable -> disallowed
		//    2) a structure or primitive on the stack ( ARGUMENTUSAGE_BYVALUE ). SF + memberoffset gives the variable
		//    3) a structure passed as input (ARGUMENTUSAGE_BYREFERENCE). 
		// E.g
		// struct GameObject object SF[0] address sf0
		// object.name SF[4]              address sf0 + 4, a pointer to an IString instance with address n0
		// call G(object),  in the body of G, object address sf0 appears by de-referencing sf1
		// object.name in the body of G corresponds to sf0 + 4. Deference that to get n0

		const IStructure& s = *def.ResolvedType;

		UseStackFrameFor(*this, def);

		BITCOUNT bc = GetBitCount(s.VarType());
		if (!def.ResolvedType->Prototype().IsClass)
		{
			// Primitives are pushed directly

			if (s.VarType() == SexyVarType_Closure)
			{
				Assembler().Append_PushStackVariable(def.SFOffset + def.MemberOffset, BITCOUNT_POINTER);
				Assembler().Append_PushStackVariable(def.SFOffset + def.MemberOffset + sizeof(size_t), BITCOUNT_POINTER);
			}
			else if (def.Usage == ARGUMENTUSAGE_BYVALUE)
			{
				Assembler().Append_PushStackVariable(def.SFOffset + def.MemberOffset, bc);
			}
			else // variable is part of an instance that has been passed by reference
			{
				Assembler().Append_GetStackFrameMember(VM::REGISTER_D4, def.SFOffset, def.MemberOffset, bc);
				Assembler().Append_PushRegister(VM::REGISTER_D4, bc);
			}			
		}
		else
		{
			if (def.Usage == ARGUMENTUSAGE_BYVALUE)
			{
				Assembler().Append_PushStackVariable(def.SFOffset + def.MemberOffset, BITCOUNT_POINTER);
			}
			else
			{
				if (def.AllocSize == 8)
				{
					// interface pointer on an object passed to the function. Needs a deref of the object and the interface pointer
					Assembler().Append_GetStackFrameMemberPtr(VM::REGISTER_D4, def.SFOffset, def.MemberOffset);
					Assembler().Append_Dereference_D4();
					Assembler().Append_PushRegister(VM::REGISTER_D4, BITCOUNT_POINTER);
				}
				else
				{
					Assembler().Append_PushStackVariable(def.SFOffset, BITCOUNT_POINTER);
				}
			}		
		}

		RestoreStackFrameFor(*this, def);
	}

	void CodeBuilder::Append_GetAllocSize()
	{
		Assembler().Append_Invoke(f.Object().GetCallbackIds().IdGetAllocSize);
	}

	void CodeBuilder::Append_DecRef()
	{
		Assembler().Append_Invoke(f.Object().GetCallbackIds().IdReleaseRef);
	}

	void CodeBuilder::Append_IncRef()
	{
		Assembler().Append_Invoke(f.Object().GetCallbackIds().IdAddRef);
	}

	void Append_IncMemberRefs(CodeBuilder& builder, const IStructure* type, cstr parentName)
	{
		if (type == nullptr) // primitive, nothing futher to recurse
		{
			return;
		}

		for (int i = 0; i < type->MemberCount(); i++)
		{
			auto& m = type->GetMember(i);

			char fullname[256];
			SafeFormat(fullname, sizeof(fullname), "%s.%s", parentName, m.Name());

			if (m.IsInterfaceVariable())
			{
				builder.AssignVariableToTemp(fullname, 0, 0); // D4
				builder.Append_IncRef();
			}
			else
			{
				Append_IncMemberRefs(builder, m.UnderlyingType(), fullname);
			}
		}
	}

	void CodeBuilder::Append_IncDerivativeRefs(cstr variableName)
	{
		MemberDef def;
		if (!TryGetVariableByName(def, variableName))
		{
			Throw(0, "Cannot increment refs to %s. Variable name not recognized");
		}

		Append_IncMemberRefs(*this, def.ResolvedType, variableName);
	}

	void CodeBuilder::AddSymbol(cstr text)
	{
		size_t address = assembler->WritePosition();

		cstr symbolText = Module().Object().RegisterSymbol(text);

		auto i = pcSymbols.find(address);
		if (i != pcSymbols.end())
		{
			i->second.Text = symbolText;
		}
		else
		{
			SymbolValue symbol;
			symbol.SourceExpression = NULL;
			symbol.Text = symbolText;
			pcSymbols.insert(std::make_pair(address,symbol));
		}
	}

	void CodeBuilder::MarkExpression(const void* sourceExpression)
	{
		size_t address = assembler->WritePosition();
		auto i = pcSymbols.find(address);
		if (i != pcSymbols.end())
		{
			i->second.SourceExpression = sourceExpression;
		}
		else
		{
			SymbolValue symbol;
			symbol.SourceExpression = sourceExpression;
			symbol.Text = ("");

			pcSymbols.insert(std::make_pair(address,symbol));
		}
	}

	void CodeBuilder::DeleteSymbols()
	{
		pcSymbols.clear();
	}

	SymbolValue CodeBuilder::GetSymbol(size_t pcAddressOffset) const
	{
		TPCSymbols::const_iterator j = pcSymbols.find(pcAddressOffset);

		if (j != pcSymbols.end())
		{
			return j->second;
		}
		else
		{
			SymbolValue nullSym;
			nullSym.SourceExpression = NULL;
			nullSym.Text = ("");
			return nullSym;
		}
	}

	int CodeBuilder::GetLocalVariableSymbolCount() const
	{
		return (int) (variables.size() + expiredVariables.size());
	}

	void CodeBuilder::GetLocalVariableSymbolByIndex(OUT MemberDef& def, OUT cstr& name, IN int index) const
	{
		if (index < (int) variables.size())
		{
			GetVariableByIndex(OUT def, OUT name, OUT index);

			const Variable* v = variables[index];
			def.pcStart = v->PCStart();
			def.pcEnd = v->PCEnd();
		}
		else
		{
			const Variable* v = expiredVariables[index - (int) variables.size()];
			def.Usage = v->Usage();
			def.SFOffset = v->Offset();
			def.MemberOffset = 0;
			def.ResolvedType = &v->ResolvedType();
			def.Userdata = v->Userdata();
			def.AllocSize = v->AllocSize();
			def.pcEnd = v->PCEnd();
			def.pcStart = v->PCStart();
			def.location = v->Location();
			name = v->Name();
		}
	}

	bool CodeBuilder::GetLocalVariableSymbolByName(OUT MemberDef& def, cstr _name, size_t pcAddress) const
	{
		int totalVarCount = GetLocalVariableSymbolCount();
		for(int i = 0; i < totalVarCount; ++i)
		{
			cstr name;
			GetLocalVariableSymbolByIndex(OUT def, name, i);
			if (AreEqual(name, _name))
			{
				if (pcAddress >= def.pcStart && pcAddress <= def.pcEnd)
				{
					return true;
				}
			}
		}

		return false;
	}

	void ReleaseInterface(CodeBuilder& builder, cstr name)
	{
		builder.AssignVariableToTemp(name, 0, 0);
		builder.Append_DecRef();
	}

	void ReleaseMembers(CodeBuilder& builder, const IStructure* s, cstr name)
	{
		if (!s) return; // Primitive type

		for (int i = 0; i < s->MemberCount(); ++i)
		{
			auto& m = s->GetMember(i);

			char fullname[256];
			SafeFormat(fullname, sizeof(fullname), "%s.%s", name, m.Name());

			if (m.IsInterfaceVariable())
			{
				ReleaseInterface(builder, fullname);
			}
			else
			{
				ReleaseMembers(builder, m.UnderlyingType(), fullname);
			}
		}
	}

	void CodeBuilder::PopLastVariables(int count, bool expireVariables)
	{
		int bytesToFree = 0;

		if (variables.size() < count) Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, "Serious algorithmic error in compile. An attempt was made to deconstruct a temp variable that does not exist");

		for (int i = 0; i < count; i++)
		{
			Variable* v;
			
			if (!expireVariables)
			{
				v = variables[variables.size() - 1 - i];
			}
			else
			{
				v = variables.back();
			}

			if (v->Offset() < 0)	Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, "Serious algorithmic error in compile. An attempt was made to deconstruct an argument to a function inside the function");

			int vOffset = v->Offset();

			if (v->Location() != VARLOCATION_OUTPUT && v->ResolvedType().InterfaceCount() > 0)
			{
				if (v->ResolvedType().Name()[0] == '_')
				{
					AssignVariableToTemp(v->Name(), 0, 0);
					Append_DecRef();
				}
			}
			else if (v->ResolvedType().VarType() == SexyVarType_Derivative && v->Usage() == ARGUMENTUSAGE_BYVALUE)
			{
				ReleaseMembers(*this, &v->ResolvedType(), v->Name());
			}

			if (expireVariables)
			{
				v->SetPCEnd(Assembler().WritePosition());
				variables.pop_back();
				expiredVariables.push_back(v);
			}

			int tempDepth = v->TempDepthOnRelease();
			if (tempDepth >= 0)
			{
				if (bytesToFree > 0)
				{
					Assembler().Append_StackAlloc(-bytesToFree);
					bytesToFree = 0;
				}

				BITCOUNT bits = v->ResolvedType().SizeOfStruct() == 4 ? BITCOUNT_32 : BITCOUNT_64;
				Assembler().Append_RestoreRegister((VM::DINDEX) tempDepth + VM::REGISTER_D4, bits);
			}
			else
			{
				bytesToFree += v->AllocSize();
			}
					
			if (expireVariables)
			{
				nextOffset = vOffset;
			}
		}

		Assembler().Append_StackAlloc(-bytesToFree);
	}

	const StackRecoveryData& CodeBuilder::GetRequiredStackCorrection(size_t codeOffset) const
	{
		TMapCodeOffsetToStackCorrection::const_iterator i = stackCorrections.find(codeOffset);
		if (i != stackCorrections.end())
		{
			return i->second;
		}

		return nullCorrection;
	}

	void CodeBuilder::NoteStackCorrection(int stackCorrection)
	{
		if (stackCorrection == 0) return; // nothing to pop, so we don't need an entry here

		StackRecoveryData srd;
		srd.TotalDisplacement = stackCorrection;
		srd.InstancePosCount = 0;
		srd.InstancePosStart = 0;
		stackCorrections[Assembler().WritePosition()] = srd;		
	}

	void CodeBuilder::NoteDestructorPosition(int instancePosition, const IStructure& type)
	{
		size_t codeOffset = Assembler().WritePosition();
		auto i = stackCorrections.find(codeOffset);
		StackRecoveryData& srd = i->second;

		if (srd.InstancePosCount == 0)
		{
			srd.InstancePosStart = (int) destructorPositions.size();			
		}

		srd.InstancePosCount++;

		destructorPositions.push_back(instancePosition);
		posToType.push_back(&type);
	}

	void CodeBuilder::PopControlFlowPoint()
	{
		controlStack.pop_back();
	}

	void CodeBuilder::PushControlFlowPoint(const ControlFlowData& controlFlowData)
	{
		controlFlowData.SectionIndex = sections.size();
		controlStack.push_back(controlFlowData);
	}

	bool CodeBuilder::TryGetControlFlowPoint(OUT ControlFlowData& data)
	{
		if (controlStack.empty())
		{
			return false;
		}

		data = controlStack.back();
		return true;
	}

	SexyVarType GetMemberType(const IStructure& s, cstr name)
	{
		NamespaceSplitter splitter(name);
		cstr head, body;
		if (!splitter.SplitHead(OUT head, OUT body))
		{
			int offset;
			const IMember* member = FindMember(s, name, OUT offset);
			if (member != NULL)
			{
				return member->UnderlyingType()->VarType();
			}
			else
			{
				return SexyVarType_Bad;
			}
		}
		else
		{
			int offset;
			const IMember* member = FindMember(s, head, OUT offset);
			if (member != NULL)
			{
				return GetMemberType(*member->UnderlyingType(), body);
			}
			else
			{
				return SexyVarType_Bad;
			}
		}
	}

	int CodeBuilder::GetOffset(size_t argIndex) const
	{
		if (argIndex >= variables.size())
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("The variable index was out of range."));
		}

		const Variable* v = variables[variables.size() - argIndex - 1];
		return v->Offset();
	}

	const IStructure* CodeBuilder::GetVarStructure(cstr varName) const
	{
		MemberDef def;
		if (TryGetVariableByName(OUT def, varName))
		{
			return def.ResolvedType;
		}
		else
		{
			return NULL;
		}
	}

	SexyVarType CodeBuilder::GetVarType(cstr name) const
	{
		MemberDef def;
		if (TryGetVariableByName(OUT def, name))
		{
			return def.ResolvedType->VarType();
		}

		return SexyVarType_Bad;

// 		NamespaceSplitter splitter(name);
// 		cstr head, body;
// 		if (!splitter.SplitHead(OUT head, OUT body))
// 		{
// 			const Variable* v = GetVariableByName(name);
// 			return v == NULL ? SexyVarType_Bad : v->ResolvedType().VarType();
// 		}
// 		else
// 		{
// 			const Variable* v = GetVariableByName(head);
// 			if (v == NULL)
// 			{
// 				return SexyVarType_Bad;
// 			}
// 
// 			const IStructure& s = v->ResolvedType();
// 			return GetMemberType(s, body);
// 		}
	}

	void CodeBuilder::AddExpression(IBinaryExpression& tree)
	{
		struct ANON
		{
			static void AddExpressionRecursive(ICodeBuilder& builder, IProgramObject& object, IBinaryExpression& branch, int& index)
			{
				IBinaryExpression* left = branch.GetLeft();
				IBinaryExpression* right = branch.GetRight();

				int oldIndex = index;

				if (left != NULL)
				{
					index++;
					ANON::AddExpressionRecursive(builder, object, *left, index);
				}				

				if (right != NULL)
				{
					index++;
					ANON::AddExpressionRecursive(builder, object, *right, index);
				}	

				branch.EvaluateBranch(builder, object, oldIndex);
			}
		};
		
		int startIndex = VM::REGISTER_D4;
		ANON::AddExpressionRecursive(*this, f.Object(), tree, startIndex);
	}
	
	Variable* CodeBuilder::TryGetVariableByName(cstr name, OUT int32& offsetCorrect)
	{
		Variable* vThis = Owner().IsVirtualMethod() ? variables[0] : NULL;

		for(TVariables::const_reverse_iterator i = variables.rbegin(); i != variables.rend(); ++i)
		{
			Variable* v = *i;
			if (AreEqual(name, v->Name()))
			{
				if (v == vThis)
				{
					offsetCorrect = -thisOffset;
				}
				else
				{
					offsetCorrect = 0;
				}
				return v;
			}
		}

		return NULL;
	}

	bool GetMemberVariable(OUT MemberDef& def, const Variable& v, const IStructure& struc, cstr memberName)
	{
		NamespaceSplitter splitter(memberName);
		cstr head, body;

		def.Userdata = NULL;
		def.location = v.Location();

		if (!splitter.SplitHead(OUT head, OUT body))
		{
			int offset;
			const IMember* member = FindMember(struc, memberName, OUT offset);
			if (member == NULL)
			{
				def.SFOffset = 0;
				def.MemberOffset = 0;
				def.ResolvedType = NULL;
				return false;
			}
			else
			{
				def.MemberOffset += offset;
				def.ResolvedType = member->UnderlyingType();			
				def.AllocSize = member->SizeOfMember();
				return true;
			}
		}
		else
		{
			int offset;
			const IMember* member = FindMember(struc, head, OUT offset);
			if (member == NULL)
			{
				def.MemberOffset = 0;
				def.ResolvedType = NULL;
				return false;
			}
			else
			{
				def.MemberOffset += offset;
			}

			return GetMemberVariable(OUT def, v, *member->UnderlyingType(), body);
		}
	}

	void CodeBuilder::EnableClosures(cstr targetVariable)
	{
		int offsetCorrection;
		Variable * v = TryGetVariableByName(targetVariable, OUT offsetCorrection);
		if (v == nullptr)
		{
			Throw(ERRORCODE_BAD_ARGUMENT, __SEXFUNCTION__, "Cannot find variable %s", targetVariable);
		}

		if (v->ResolvedType().VarType() != SexyVarType_Closure)
		{
			Throw(ERRORCODE_BAD_ARGUMENT, __SEXFUNCTION__, "Variable %s is not a closure", targetVariable);
		}

		v->AllowCaptureClosure();
	}

	bool CodeBuilder::TryGetVariableByName(OUT MemberDef& def, cstr name) const
	{
		NamespaceSplitter splitter(name);
		cstr head, body;

		def.CapturesLocalVariables = false;
		def.IsParentValue = false;
		def.Userdata = NULL;
		def.AllocSize = 0;
		def.location = VARLOCATION_NONE;
		
		int offsetCorrection;

		if (!splitter.SplitHead(OUT head, OUT body))
		{			
			const Variable* v = TryGetVariableByName(name, OUT offsetCorrection);
			
			if (v == NULL)
			{
				IFunctionBuilder* parent = f.Parent(); 
				if (parent != NULL)
				{
					CodeBuilder& parentBuilder = (CodeBuilder&) parent->Builder();
					v = parentBuilder.TryGetVariableByName(name, OUT offsetCorrection);
					if (v != NULL)
					{
						offsetCorrection = 0; 
						codeReferencesParentsSF = true;
						def.CapturesLocalVariables = v->CanCaptureClosures();
						def.IsParentValue = true;
						def.Userdata = v->Userdata();
						def.AllocSize = v->AllocSize();
						def.MemberOffset += offsetCorrection;
						def.location = v->Location();
						def.IsContained = false;
					}
				}
			}

			if (v == NULL)
			{
				def.Usage = ARGUMENTUSAGE_BYVALUE;
				def.SFOffset = 0;
				def.MemberOffset = 0;
				def.ResolvedType = NULL;	
				def.location = VARLOCATION_NONE;
				def.IsContained = false;
				return false;
			}
			else
			{
				def.CapturesLocalVariables = v->CanCaptureClosures();
				def.Usage = v->Usage();
				def.SFOffset = v->Offset();
				def.MemberOffset = offsetCorrection;
				def.ResolvedType = &v->ResolvedType();
				def.Userdata = v->Userdata();
				def.AllocSize = v->AllocSize();
				def.location = v->Location();
				def.IsContained = false;
				return true;
			}
		}

		const Variable* v = TryGetVariableByName(head, offsetCorrection);

		if (v == NULL)
		{
			IFunctionBuilder* parent = f.Parent(); 
			if (parent != NULL)
			{
				CodeBuilder& parentBuilder = (CodeBuilder&) parent->Builder();
				bool success = parentBuilder.TryGetVariableByName(def, name);
				if (success)
				{
					def.IsParentValue = true;
					return true;
				}
				else
				{
					return false;
				}
			}
		}

		if (v == NULL)
		{
			def.CapturesLocalVariables = false;
			def.Usage = ARGUMENTUSAGE_BYVALUE;
			def.SFOffset = 0;
			def.MemberOffset = 0;
			def.ResolvedType = NULL;
			def.IsParentValue = false;
			def.location = VARLOCATION_NONE;
			def.IsContained = false;
			return false;
		}
		else
		{
			def.CapturesLocalVariables = v->CanCaptureClosures();
			def.Usage = v->Usage();
			def.SFOffset = v->Offset();
			def.MemberOffset = offsetCorrection;
			def.Userdata = v->Userdata();
			def.location = v->Location();
			def.IsContained = true;
						
			return GetMemberVariable(def, *v, v->ResolvedType(), body);
		}
	}

	int CodeBuilder::GetVariableCount() const
	{
		return (int) variables.size();
	}

	void CodeBuilder::GetVariableByIndex(OUT MemberDef& def, cstr& name, int index) const
	{
		const Variable* v = variables[index];
		def.Usage = v->Usage();
		def.SFOffset = v->Offset();
		def.MemberOffset = 0;
		def.ResolvedType = &v->ResolvedType();
		def.Userdata = v->Userdata();
		def.AllocSize = v->AllocSize();
		def.location = v->Location();
		def.IsContained = false;
		name = v->Name();
	}

	const Variable* CodeBuilder::TryGetVariableByName(cstr name, OUT int32& offsetCorrect) const
	{		
		if (variables.empty()) return NULL;

		if (Owner().IsVirtualMethod() && AreEqual(name, "this"))
		{
			Variable* vThis = variables[0];
			offsetCorrect = -thisOffset;
			return vThis;
		}

		for(TVariables::const_reverse_iterator i = variables.rbegin(); i != variables.rend(); ++i)
		{
			const Variable* v = *i;
			if (AreEqual(name, v->Name()))
			{
				offsetCorrect = 0;
				return v;
			}
		}

		return NULL;
	}

	Variable& CodeBuilder::DemandVariableByName(cstr name, cstr clue, OUT int32& offsetCorrect)
	{
		Variable* v = CodeBuilder::TryGetVariableByName(name, OUT offsetCorrect);
		if (v == NULL) Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("Could not find %s variable [%s]"), clue, name);
		return *v;
	}

	void CodeBuilder::AllocateLocalVariable(Variable& v, REF size_t& stackExpansion)
	{
		if (v.Usage() == ARGUMENTUSAGE_BYREFERENCE)
		{
			v.SetStackPosition((int32)stackExpansion);
			stackExpansion += sizeof(VariantValue);
		}
		else
		{
			size_t instAlign = v.ResolvedType().Prototype().InstanceAlignment;
			size_t alignCorrectionValue = (instAlign - (instAlign % stackExpansion)) % instAlign;

			stackExpansion *= alignCorrectionValue;
			v.SetStackPosition((int32)alignCorrectionValue);
			stackExpansion += v.AllocSize();
		}
	}

	int GetHighestOffset(const TVariables& variables)
	{
		int offset = 0;
		for(TVariables::const_iterator i = variables.begin(); i != variables.end(); ++i)
		{
			const Variable* v = *i;
			offset = std::max(offset, v->Offset());
		}

		return offset;
	}

	bool CodeBuilder::IsVariableDefinedAtLevel(int32 sectionIndex, cstr name)
	{
		for(TVariables::const_reverse_iterator i = variables.rbegin(); i != variables.rend(); ++i)
		{
			Variable* v = *i;
			if (v->SectionIndex() > sectionIndex)
			{
				continue;
			}
			else if (v->SectionIndex() < sectionIndex)
			{
				return false;
			}
			else if (AreEqual(name, v->Name()))
			{
				return true;
			}
		}

		return false;
	}

	void CodeBuilder::BuildInputAndOutputOffsets()
	{
		// Determine the stack frame offsets of input and output arguments, which are given negative offsets, as they are pushed as arguments
		// onto the stack, followed by the copy of a callers SF pointer, and then finally by the value of the return address for the function call.

		// SF+S is the return address of the function where S is sizeof(void*)
		// SF-2S points to the SF value of the caller
		// SF-2S-x is the xth byte offset into the argument list, in reverse order of the argument list.
		// Suppose our first argument J is a float and sizeof(void*) = 8, then SF-20 is the address of the firsr argument &J. 
		// StackFrame = [Output1]...[OutputN][Input1]...[InputN][OldSF.Ptr][ReturnAddress.Ptr]. REGISTER_SF points to the return address.
		size_t x = 2 * sizeof(void*);
		int32 sfOffset = -(int32)x;

		int32 argCount = ArgCount(f);

		for (int32 i = argCount - 1; i >= 0; --i)
		{
			const IArgument& arg = f.Arg(i);
			if (arg.ResolvedType() == NULL)
			{
				Throw(ERRORCODE_NULL_POINTER, __SEXFUNCTION__, "Could not resolve type (Arg #%d) %s %s", i, arg.TypeString(), arg.Name());
			}

			int varOffset = 0;
			if (i == argCount - 1)
			{
				varOffset = thisOffset;
			}
			Variable* v = new Variable(arg, /* section index */ 0, arg.Userdata(), varOffset);

			int32 dx;
			// Derivative types are always passed by reference (a pointer)
			switch (v->ResolvedType().VarType())
			{
			case SexyVarType_Derivative:
			case SexyVarType_Array:
			case SexyVarType_List:
			case SexyVarType_Map:
				dx = (int32)sizeof size_t;
				break;
			default:
				dx = (int32)v->AllocSize();
				break;
			}
			
			sfOffset -= dx;
			v->SetStackPosition(sfOffset);
			variables.push_back(v);
		}
	}

	void CodeBuilder::AssignClosureParentSFtoD6()
	{
		if (f.Parent() != NULL)
		{
			int ptrSize = sizeof(void*);
			int sfSize = sizeof(void*);
			// This is a closure, which means the 64-bit parents SF was pushed on the stack just before the first output value
			int offset;

			int nArgs = f.NumberOfInputs() + f.NumberOfOutputs();
			if (nArgs > 0)
			{
				Variable* v = variables[nArgs-1];
				offset = v->Offset() - sfSize;
			}
			else
			{
				offset = -2 * ptrSize - 2 * sfSize;
			}

			Assembler().Append_GetStackFrameValue(offset, VM::REGISTER_D6, BITCOUNT_POINTER);
		}
	}

	void CodeBuilder::Begin()
	{
		if (needInitArgs)
		{
			needInitArgs = false;

			BuildInputAndOutputOffsets();

			endOfArgs = 0;
			if (!variables.empty())
			{
				Variable* finalVar = variables.back();
				endOfArgs = finalVar->Offset();
			}
		}

		sectionIndex = 1;
		functionStartPosition = Assembler().WritePosition();

		AssignClosureParentSFtoD6();
	}

	void CodeBuilder::AddDestructors(size_t startPosition, size_t endPosition)
	{
		int64 startIndex = INT64_MAX;
		int64 lastIndex = -1LL;

		for (int64 i = 0; i < (int64) variables.size(); i++)
		{
			auto& v = variables[i];
			size_t pos = v->PCStart();
			if (pos > startPosition && pos < endPosition)
			{
				startIndex = min(startIndex, i);
				lastIndex = max(lastIndex, i);
			}
		}

		int32 bytesToFree = 0;

		for (int64 i = lastIndex; i >= startIndex; i--)
		{
			auto& v = *variables[i];

			int vOffset = v.Offset();

			if (vOffset < 0) Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, "Serious algorithmic error in compile. An attempt was made to deconstruct an argument to a function inside the function");

			if (v.Location() != VARLOCATION_OUTPUT && v.ResolvedType().InterfaceCount() > 0)
			{
				if (v.ResolvedType().Name()[0] == '_')
				{
					AssignVariableToTemp(v.Name(), 0, 0);
					Append_DecRef();
				}
			}
			else if (v.ResolvedType().VarType() == SexyVarType_Derivative && v.Usage() == ARGUMENTUSAGE_BYVALUE)
			{
				ReleaseMembers(*this, &v.ResolvedType(), v.Name());
			}

			int tempDepth = v.TempDepthOnRelease();
			if (tempDepth >= 0)
			{
				if (bytesToFree > 0)
				{
					Assembler().Append_StackAlloc(-bytesToFree);
					bytesToFree = 0;
				}

				BITCOUNT bits = v.ResolvedType().SizeOfStruct() == 4 ? BITCOUNT_32 : BITCOUNT_64;
				Assembler().Append_RestoreRegister((VM::DINDEX)tempDepth + VM::REGISTER_D4, bits);
			}
			else
			{
				bytesToFree += v.AllocSize();
			}
		}

		Assembler().Append_StackAlloc(-bytesToFree);
	}

	size_t CodeBuilder::GetLabelPosition(cstr labelName)
	{
		auto i = mapLabelToPosition.find(labelName);
		if (i == mapLabelToPosition.end())
		{
			return (size_t)-1LL;
		}

		return i->second.pcOffset;
	}

	void CodeBuilder::MarkGoto(size_t gotoPosition, cstr labelName)
	{
		// [gotoPosition] is the bytecode address from the start of the function of our (goto <labelName>) expression
		// [labelName] is the name of a succeeding (never preceding) label (label <labelName>). Assumes the [labelName] pointer is valid for the lifetime of the CodeBuilder

		mapGotoStatementsToLabels[gotoPosition] = labelName;
	}

	int32 Diff(cstr src, size_t gotoPosition, size_t labelPosition)
	{
		int64 a = (int64)gotoPosition;
		int64 b = (int64)labelPosition;
		int64 c = a - b;
		if (c < (int64)INT_MIN || c >(int64) INT_MAX)
		{
			Throw(ERRORCODE_COMPILE_ERRORS, src, "Offset too large, reduce function size");
		}

		return (int32)c;
	}

	void CodeBuilder::MarkLabel(cstr labelName)
	{
		LabelInfo label;
		label.pcOffset = assembler->WritePosition();
		label.sectionIndex = sectionIndex;

		if (!mapLabelToPosition.insert(labelName, label).second)
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __ROCOCO_FUNCTION__, "duplicate label: %s", labelName);
		}

		for (auto i = outstandingLabels.begin(); i != outstandingLabels.end(); i++)
		{
			if (Eq(i->labelName, labelName))
			{
				if (i->sectionIndex >= sectionIndex)
				{
					i->labelName = nullptr;
				}
				else
				{
					Throw(ERRORCODE_COMPILE_ERRORS, __ROCOCO_FUNCTION__, "Label %s was nested too deeply out of scope of the previous goto statement", labelName);
				}
			}
		}

		auto firstRemovedItem = std::remove_if(outstandingLabels.begin(), outstandingLabels.end(),
			[](const OutstandingLabel& label)
			{
				return label.labelName == nullptr;
			}
		);

		outstandingLabels.erase(firstRemovedItem, outstandingLabels.end());

		// for each (goto <labelName>) set the program counter displacement to point to the labelWritePosition. Anything that maps is stripped from the list, so that 
		// at the end of the function the map should be empty, as all labels have been resolved
		auto j = mapGotoStatementsToLabels.begin();
		while (j != mapGotoStatementsToLabels.end())
		{
			auto& g = *j;
			if (Eq(g.second, labelName))
			{
				int32 PCOffset = Diff(__ROCOCO_FUNCTION__, label.pcOffset, g.first);
				assembler->SetWriteModeToOverwrite(g.first);
				assembler->Append_Branch(PCOffset);
				
				j = mapGotoStatementsToLabels.erase(j);
			}
			else
			{
				j++;
			}
		}

		assembler->SetWriteModeToAppend();

		assembler->Append_NoOperation();
	}

	void CodeBuilder::PreventMoreVariablesUntil(cstr label)
	{
		// Appending variables to the code builder suspended until outstandingLabels is empty(). This prevents gotos from circumventing variable initialization. 
		OutstandingLabel x;
		x.labelName = label;
		x.sectionIndex = sectionIndex;
		outstandingLabels.push_back(x);
	}

	void CodeBuilder::End()
	{
		sectionIndex--;
		if (sectionIndex > 0)
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, "Section not closed");
		}

		if (!mapGotoStatementsToLabels.empty())
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, "Unmatched goto statement. Expecting label %s in either a sibling or ancestor of the goto statement", mapGotoStatementsToLabels.begin()->second);
		}

		int endOfTemps = 0;

		int nTemps = GetVariableCount() - ArgCount(Owner());
		if (nTemps > 0)
		{
			Variable* finalVar = variables.back();
			endOfTemps = finalVar->Offset() + finalVar->AllocSize();
		}

		if (endOfTemps > 0)
		{
			Assembler().Append_StackAlloc(-endOfTemps);
		}

		functionEndPosition = Assembler().WritePosition();

		// This is one of the most frequently formatted strings in the sexy script system
		// Instead of safe format we hand craft the string using faster functions that append substrings
		TokenBuffer symbol;
		_itoa_s((int)functionEndPosition, symbol.Text, 10);
		StringCat(symbol.Text, " bytes", TokenBuffer::MAX_TOKEN_CHARS);

		AddSymbol(symbol);
		Assembler().Append_Return();

		if (codeReferencesParentsSF && !mayUseParentsSF)
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, "The closure body required access to the parent's stackframe, which is prohibited in its context");
		}

		if (!f.Object().ProgramMemory().UpdateBytecode(byteCodeId, Assembler()))
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, "Insufficient program memory. Either reduce program size or increase MaxProgramBytes in ProgramInitParameters");
		}

		Module().IncVersion();
	}

	void CodeBuilder::EnterSection()
	{
		sectionIndex++;
		sections.push_back(GetVariableCount());
	}

	void CodeBuilder::AddVariableRef(const NameString& name, const IStructure& type, void* userData)
	{
		if (IsVariableDefinedAtLevel(sectionIndex, name.c_str()))
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, "Variable [%s] already defined in this scope", name.c_str());
		}

		if (IsPointerValid(&type))
		{
			Variable *v = new Variable(Assembler().WritePosition(), name, type, sectionIndex, userData, sizeof(size_t), VARLOCATION_TEMP, true);
			variables.push_back(v);

			int32 dx = sizeof(size_t);

			int offset = nextOffset; // SF takes us just beyond the return address of the caller
			nextOffset += dx;

			v->SetStackPosition(offset);

			if (dx >= 0) Assembler().Append_StackAlloc(dx);
		}
		else
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, "Could not add variable [%s]. Type null", name.c_str());
		}
	}

	void CodeBuilder::AddVariable(const Rococo::Compiler::NameString& name, const Rococo::Compiler::TypeString& type, void* userData)
	{
		if (!outstandingLabels.empty())
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, "Variables cannot be added between the previous goto expression and the label %s. The jump would circumvent the variable definition", (cstr) outstandingLabels.begin()->labelName);
		}

		if (IsVariableDefinedAtLevel(sectionIndex, name.c_str()))
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, "Variable [%s] already defined in this scope", name.c_str());
		}

		const IStructure* s = Compiler::MatchStructure(Module().Object().Log(), type.c_str(), f.Module());
		if (s != NULL)
		{
			Variable *v = new Variable(Assembler().WritePosition(), name, *s, sectionIndex, userData, -1, VARLOCATION_TEMP);
			variables.push_back(v);
			
			int32 dx = (int32) s->SizeOfStruct();

			int offset = nextOffset; // SF takes us just beyond the return address of the caller
			nextOffset += dx;

			v->SetStackPosition(offset);

			if (dx >= 0) Assembler().Append_StackAlloc(dx);
		}
		else
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, "Could not add variable [%s]", name.c_str());
		}
	}

	void CodeBuilder::AddInterfaceVariable(const NameString& name, const IStructure& type, void* userData)
	{
		if (!outstandingLabels.empty())
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, "Variables cannot be added between the previous goto expression and the label %s. The jump would circumvent the variable definition", (cstr)outstandingLabels.begin()->labelName);
		}

		if (IsVariableDefinedAtLevel(sectionIndex, name.c_str()))
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, "Variable [%s] already defined in this scope", name.c_str());
		}

		if (IsPointerValid(&type))
		{
			Variable *v = new Variable(Assembler().WritePosition(), name, type, sectionIndex, userData, sizeof(InterfacePointer), VARLOCATION_TEMP);
			variables.push_back(v);
			v->SetStackPosition(nextOffset);
			nextOffset += sizeof(void*);
			Assembler().Append_StackAlloc(sizeof(void*));
		}
		else
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, "Could not add variable [%s]. Type null", name.c_str());
		}
	}

	void CodeBuilder::AddVariable(const Rococo::Compiler::NameString& name, const IStructure& type, void* userData)
	{
		if (!outstandingLabels.empty())
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, "Variables cannot be added between the previous goto expression and the label %s. The jump would circumvent the variable definition", (cstr)outstandingLabels.begin()->labelName);
		}

		if (IsVariableDefinedAtLevel(sectionIndex, name.c_str()))
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("Variable [%s] already defined in this scope"), name.c_str());
		}

		if (IsPointerValid(&type))
		{
			Variable *v = new Variable(Assembler().WritePosition(), name, type, sectionIndex, userData, -1, VARLOCATION_TEMP);
			variables.push_back(v);

			int32 dx = (int32) type.SizeOfStruct();

			int offset = nextOffset; // SF takes us just beyond the return address of the caller
			nextOffset += dx;

			v->SetStackPosition(offset);

			if (dx >= 0) Assembler().Append_StackAlloc(dx);
		}
		else
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("Could not add variable [%s]. Type null"), name.c_str());
		}
	}

	void CodeBuilder::AssignLiteral(const Rococo::Compiler::NameString& name, cstr literalValue)
	{
		if (literalValue == NULL) Throw(0, "CodeBuilder::AssignLiteral: literalValue was null"); 

		MemberDef def;
		if (!TryGetVariableByName(OUT def, name.c_str()))
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("Could not assign '%s' to [%s]. Could not find the variable."), literalValue, name.c_str());
		}
		
		VariantValue value;

		SexyVarType vType = def.ResolvedType->VarType();
		if (vType == SexyVarType_Derivative)
		{
			if (def.ResolvedType->InterfaceCount() == 0)
			{
				if (def.ResolvedType->MemberCount() == 1)
				{
					// Could be a memberwise assignment to element 0
					auto& member0 = def.ResolvedType->GetMember(0);
					auto& member0Type = *member0.UnderlyingType();
					if (IsPrimitiveType(member0Type.VarType()))
					{
						char fqMember[128];
						SecureFormat(fqMember, "%s.%s", name.c_str(), member0.Name());
						AssignLiteral(NameString::From(fqMember), literalValue);
						return;
					}
				}
				else
				{
					Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, "Could not assign '%s' to [%s]. The variable is not of atomic type and has no interface.", literalValue, name.c_str());
				}
			}

			const IInterface& interf = def.ResolvedType->GetInterface(0);

			if (!AreEqual("0", literalValue))
			{
				Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, "Could not assign a non zero literal to interface [%s]", name.c_str());
			}

			value.vPtrValue =  interf.UniversalNullInstance()->pVTables;
		}
		else if (vType == SexyVarType_Pointer)
		{
			if (!AreEqual("0", literalValue))
			{
				Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, "Could not assign a non zero literal to array [%s]", name.c_str());
			}

			value.vPtrValue = NULL;


		}
		else if (vType == SexyVarType_Array)
		{
			if (!AreEqual("0", literalValue))
			{
				Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, "Could not assign a non zero literal to array [%s]", name.c_str());
			}

			value.vPtrValue = NULL;

			UseStackFrameFor(*this, def);
			if (!def.IsContained || def.Usage == ARGUMENTUSAGE_BYVALUE)
			{
				Assembler().Append_SetStackFrameImmediate(def.SFOffset + def.MemberOffset, value, GetBitCount(vType));
			}
			else //ByRef
			{
				Assembler().Append_SetRegisterImmediate(VM::REGISTER_D5, value, GetBitCount(vType));
				Assembler().Append_SetSFMemberByRefFromRegister(VM::REGISTER_D5, def.SFOffset, def.MemberOffset, GetBitCount(vType));
			}
			RestoreStackFrameFor(*this, def);
			return;
		}
		else
		{
			PARSERESULT result = TryParse(OUT value, vType, literalValue);
			if (result != PARSERESULT_GOOD)
			{
				Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("Cannot parse literal value '%s' to [%s]"), literalValue, name.c_str());
			}
		}

		UseStackFrameFor(*this, def);
		if (def.Usage == ARGUMENTUSAGE_BYVALUE)
		{
			Assembler().Append_SetStackFrameImmediate(def.SFOffset + def.MemberOffset, value, GetBitCount(vType));
		}
		else //ByRef
		{
			Assembler().Append_SetRegisterImmediate(VM::REGISTER_D5, value, GetBitCount(vType));
			Assembler().Append_SetSFMemberByRefFromRegister(VM::REGISTER_D5, def.SFOffset, def.MemberOffset, GetBitCount(vType));
		}
		RestoreStackFrameFor(*this, def);
	}

	void CodeBuilder::AssignPointer(const NameString& name, const void* ptr)
	{
		MemberDef def;
		if (!TryGetVariableByName(OUT def, name.c_str()))
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("Could not assign '%p' to [%s]. Could not find the variable."), ptr, name.c_str());
		}
		
		VariantValue value;

		SexyVarType vType = def.ResolvedType->VarType();

		bool isArray = def.ResolvedType->VarType() == SexyVarType_Array;

		if (!(isArray || (vType == SexyVarType_Derivative && def.ResolvedType->InterfaceCount() > 0) || vType == SexyVarType_Pointer))
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("Could not assign '%p' to [%s]. Variable must of type Pointer."), ptr, name.c_str());
		}

		value.vPtrValue = (void*) ptr;

		UseStackFrameFor(*this, def);
		
		if (def.Usage == ARGUMENTUSAGE_BYVALUE || isArray)
		{
			Assembler().Append_SetStackFrameImmediate(def.SFOffset + def.MemberOffset, value, GetBitCount(vType));
		}
		else //ByRef
		{
			Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, value, GetBitCount(vType));
			Assembler().Append_SetSFMemberByRefFromRegister(VM::REGISTER_D4, def.SFOffset, def.MemberOffset, GetBitCount(vType));
		}

		RestoreStackFrameFor(*this, def);
	}

	void AssignVariableToVariable_ByRefSlow(ICodeBuilder& builder, const MemberDef& sourceDef, const MemberDef& targetDef, cstr source, cstr target)
	{
		const IStructure& s = *sourceDef.ResolvedType;
		const IStructure& t = *targetDef.ResolvedType;
		VM::IAssembler& assembler = builder.Assembler();

		if (sourceDef.location != VARLOCATION_OUTPUT && targetDef.location != VARLOCATION_OUTPUT)
		{
			if (&s != &t)
			{
				Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("Cannot copy %s %s to %s %s. They must be the same type"), GetFriendlyName(s), source, GetFriendlyName(t), target);
			}
			if (s.Prototype().IsClass)
			{
				MemberDef refDef;
				if (!builder.TryGetVariableByName(refDef, target))
				{
					Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("Cannot copy %s %s. No associated reference to %s."), GetFriendlyName(s), source, target);
				}
				
				UseStackFrameFor(builder, sourceDef);
				assembler.Append_GetStackFrameValue(sourceDef.SFOffset + sourceDef.MemberOffset, VM::REGISTER_D5, BITCOUNT_POINTER);
				RestoreStackFrameFor(builder, sourceDef);
				UseStackFrameFor(builder, refDef);
				assembler.Append_SetSFMemberByRefFromRegister(VM::REGISTER_D5, refDef.SFOffset, refDef.MemberOffset, BITCOUNT_POINTER);
				RestoreStackFrameFor(builder, refDef);
				return;
			}

			UseStackFrameFor(builder, sourceDef);
			assembler.Append_GetStackFrameMemberPtr(VM::REGISTER_D5, sourceDef.SFOffset, sourceDef.MemberOffset);
			RestoreStackFrameFor(builder, sourceDef);
			UseStackFrameFor(builder, targetDef);
			assembler.Append_GetStackFrameMemberPtr(VM::REGISTER_D4, targetDef.SFOffset, targetDef.MemberOffset);
			RestoreStackFrameFor(builder, targetDef);

			size_t nBytesSource = sourceDef.AllocSize;
			assembler.Append_CopyMemory(VM::REGISTER_D4, VM::REGISTER_D5, nBytesSource);
		}
		else if (sourceDef.location == VARLOCATION_TEMP && targetDef.location == VARLOCATION_OUTPUT)
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("Cannot return a reference of %s to %s. The source would be freed before the function returns"), source, target);
		}
		else if (sourceDef.location == VARLOCATION_OUTPUT)
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("Cannot copy a value from %s. It is an output value"), target);
		}
		else // Copy a reference of the input to the output
		{
			if (targetDef.MemberOffset != 0)
			{
				Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("Did not expect a member offset in the output %s"), target);
			}

			MemberDef refDef;
			if (!builder.TryGetVariableByName(OUT refDef, source))
			{
				Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("Could not find the reference for %s for variable. %s"), (cstr)source, (cstr)source);
			}

			if (!(refDef.IsParentValue ^ targetDef.IsParentValue))
			{
				UseStackFrameFor(builder, refDef);
				assembler.Append_SetSFValueFromSFMemberRef(refDef.SFOffset, refDef.MemberOffset, targetDef.SFOffset, sizeof(size_t));
				RestoreStackFrameFor(builder, refDef);
			}
			else
			{
				UseStackFrameFor(builder, refDef);
				assembler.Append_GetStackFrameMemberPtr(VM::REGISTER_D4, refDef.SFOffset, refDef.MemberOffset);
				RestoreStackFrameFor(builder, refDef);

				UseStackFrameFor(builder, targetDef);
				assembler.Append_SetStackFrameValue(VM::REGISTER_D4, (VM::DINDEX) targetDef.SFOffset, BITCOUNT_POINTER);
				RestoreStackFrameFor(builder, targetDef);
			}
		}
	}


	void AssignVariableToVariable_ByRef(ICodeBuilder& builder, const MemberDef& sourceDef, const MemberDef& targetDef, cstr source, cstr target)
	{
		const IStructure& s = *sourceDef.ResolvedType;
		const IStructure& t = *targetDef.ResolvedType;
		VM::IAssembler& assembler = builder.Assembler();

		if (sourceDef.location != VARLOCATION_OUTPUT && targetDef.location != VARLOCATION_OUTPUT)
		{
			if (&s != &t)
			{
				Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("Cannot copy %s %s to %s %s. They must be the same type"), GetFriendlyName(s), source, GetFriendlyName(t), target);
			}
			if (s.Prototype().IsClass)
			{
				MemberDef refDef;
				if (!builder.TryGetVariableByName(refDef, target))
				{
					Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("Cannot copy %s %s. No associated reference to %s."), GetFriendlyName(s), source, target);
				}

				assembler.Append_GetStackFrameValue(sourceDef.SFOffset + sourceDef.MemberOffset, VM::REGISTER_D5, BITCOUNT_POINTER);
				assembler.Append_SetSFMemberByRefFromRegister(VM::REGISTER_D5, refDef.SFOffset, refDef.MemberOffset, BITCOUNT_POINTER);
				return;
			}

			assembler.Append_GetStackFrameMemberPtr(VM::REGISTER_D5, sourceDef.SFOffset, sourceDef.MemberOffset);
			assembler.Append_GetStackFrameMemberPtr(VM::REGISTER_D4, targetDef.SFOffset, targetDef.MemberOffset);

			size_t nBytesSource = sourceDef.AllocSize;
			assembler.Append_CopyMemory(VM::REGISTER_D4, VM::REGISTER_D5, nBytesSource);
		}
		else if (sourceDef.location == VARLOCATION_TEMP && targetDef.location == VARLOCATION_OUTPUT)
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("Cannot return a reference of %s to %s. The source would be freed before the function returns"), source, target);
		}	
		else if (sourceDef.location == VARLOCATION_OUTPUT)
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("Cannot copy a value from %s. It is an output value"), target);
		}	
		else // Copy a reference of the input to the output
		{
			if (targetDef.MemberOffset != 0)
			{
				Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("Did not expect a member offset in the output %s"), target);
			}

			MemberDef refDef;
			if (!builder.TryGetVariableByName(OUT refDef, source))
			{
				Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("Could not find the reference for %s for variable. %s"), (cstr)source, (cstr) source);
			}

			assembler.Append_SetSFValueFromSFMemberRef(refDef.SFOffset, refDef.MemberOffset, targetDef.SFOffset, sizeof(size_t));
		}
	}

	void AssignVariableToVariableBothByValue(ICodeBuilder& builder, const MemberDef& src, const MemberDef& dest, BITCOUNT bitcount)
	{
		VM::IAssembler& assembler = builder.Assembler();

		if (!(src.IsParentValue ^ dest.IsParentValue))
		{
			UseStackFrameFor(builder, src);
			assembler.Append_SetSFValueFromSFValue(dest.SFOffset + dest.MemberOffset, src.SFOffset + src.MemberOffset, bitcount);
			RestoreStackFrameFor(builder, src);
		}
		else
		{
			UseStackFrameFor(builder, src);
			assembler.Append_GetStackFrameValue(src.SFOffset + src.MemberOffset, VM::REGISTER_D4, bitcount);
			RestoreStackFrameFor(builder, src);
			UseStackFrameFor(builder, dest);
			assembler.Append_SetStackFrameValue(dest.SFOffset + dest.MemberOffset, VM::REGISTER_D4, bitcount);
			RestoreStackFrameFor(builder, dest);
		}
	}

	void AssignInterfaceToInterface(ICodeBuilder& builder, const MemberDef& sourceDef, const MemberDef& targetDef, cstr source, cstr target, bool isConstructingTarget)
	{
		bool bFoundBase = false;
		for (auto* I = &sourceDef.ResolvedType->GetInterface(0); I != nullptr; I = I->Base())
		{
			if (I == &targetDef.ResolvedType->GetInterface(0))
			{
				bFoundBase = true;
				break;
			}
		}

		if (!bFoundBase)
		{
			Throw(0, "Cannot assign %s to %s. Target interface %s was not derived from the source %s", source, target, targetDef.ResolvedType->GetInterface(0).Name(), sourceDef.ResolvedType->GetInterface(0).Name());
		}

		if (isConstructingTarget)
		{
			builder.AssignVariableToTemp(source, 0); // D4
			builder.AssignTempToVariable(0, target); 
			builder.Append_IncRef();
		}
		else
		{
			builder.AssignVariableToTemp(source, 0); // D4
			builder.AssignVariableToTemp(target, 1); // D5
			builder.Append_UpdateRefsOnSourceAndTarget();
			builder.AssignTempToVariable(0, target);
		}
	}

	void CodeBuilder::Append_UpdateRefsOnSourceAndTarget()
	{
		assembler->Append_Invoke(f.Object().GetCallbackIds().IdUpdateRefsOnSourceAndTarget);
	}

	bool CodeBuilder::TryAssignClassInterfaceToInterface(cstr source, cstr target, const IStructure* srcType, const IStructure* trgType)
	{
		if (!IsNullType(*trgType) || trgType->InterfaceCount() == 0)
		{
			return false;
		}

		if (srcType->InterfaceCount() == 0 || IsNullType(*srcType))
		{
			return false;
		}

		auto& interf0 = trgType->GetInterface(0);

		int index = Rococo::Script::GetIndexOfInterface(*srcType, interf0);
		if (index == -1)
		{
			Throw(0, "The class %s does not implement interface %s", srcType->Name(), interf0.Name());
		}

		int offset = GetThisOffset();
		int targetOffset = ObjectStub::BYTECOUNT_INSTANCE_TO_INTERFACE0 + index * sizeof(size_t);
		int delta = (targetOffset - offset);

		AssignVariableRefToTemp(source, 0, delta); // D4
		AssignVariableRefToTemp(target, 1, 0); // D5
		this->Append_UpdateRefsOnSourceAndTarget();

		AssignTempToVariable(0, target);

		return true;
	}

	void CodeBuilder::AssignVariableToVariable(cstr source, cstr target, bool isConstructingTarget)
	{
		if (source == nullptr || *source == 0) Rococo::Throw(0, "CodeBuilder::AssignVariableToVariable: source was blank");
		if (target == nullptr || *target == 0) Rococo::Throw(0, "CodeBuilder::AssignVariableToVariable: target was blank");
	
		MemberDef sourceDef;
		if (!TryGetVariableByName(OUT sourceDef, source))
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("Could not resolve %s"), source);
		}

		MemberDef targetDef;
		if (!TryGetVariableByName(OUT targetDef, target))
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("Could not resolve %s"), target);
		}

		const IStructure* srcType = sourceDef.ResolvedType;
		const IStructure* trgType = targetDef.ResolvedType;

		if (srcType->VarType() == SexyVarType_Array)
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, "Array to array assignment must be handled at a higher level", target);
			return;
		}

		if (srcType != trgType)
		{
			if (!TryAssignClassInterfaceToInterface(source, target, srcType, trgType))
			{
				if (IsNullType(*srcType) && IsNullType(*trgType))
				{
					AssignInterfaceToInterface(*this, sourceDef, targetDef, source, target, isConstructingTarget);
					return;
				}
				else
				{
					Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, "Type mismatch trying to assign [%s %s] to [%s %s]", GetFriendlyName(*sourceDef.ResolvedType), source, GetFriendlyName(*targetDef.ResolvedType), target);
				}
			}
			else
			{
				return;
			}
		}

		if (sourceDef.CapturesLocalVariables && !targetDef.CapturesLocalVariables)
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, "Could not copy %s to %s. The target variable accepts regular function references, but not closures.", source, target);
		}

		if (targetDef.location == VARLOCATION_OUTPUT && trgType->VarType() == SexyVarType_Derivative && !IsNullType(*trgType))
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, "Do not know how to handle derivative type as output. %s %s", trgType->Name(), (cstr) target);
		}

		if (IsNullType(*trgType))
		{
			AssignInterfaceToInterface(*this, sourceDef, targetDef, source, target, isConstructingTarget);
			return;
		}

		if (sourceDef.Usage == ARGUMENTUSAGE_BYVALUE && targetDef.Usage == ARGUMENTUSAGE_BYVALUE)
		{
			size_t nBytesSource = sourceDef.AllocSize;
			if (nBytesSource == sizeof(uint32))
			{
				AssignVariableToVariableBothByValue(*this, sourceDef, targetDef, BITCOUNT_32);
			}
			else if (nBytesSource == sizeof(uint64))
			{
				AssignVariableToVariableBothByValue(*this, sourceDef, targetDef, BITCOUNT_64);
			}
			else
			{
				if (sourceDef.ResolvedType->VarType() != SexyVarType_Derivative)
				{
					Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, "Type mismatch trying to assign %s to %s. Unusual size of primitive arguments", source, target);
				}

				if (srcType->HasInterfaceMembers())
				{
					Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, "Could not assign %s to %s. Members contain interfaces.", source, target);
				}

				if (targetDef.IsParentValue ^ sourceDef.IsParentValue)
				{
					// N.B do not refactor into single VM instruction, as SF is not the same for source and target
					AssignVariableRefToTemp(source, 0, 0);
					AssignVariableRefToTemp(target, 1, 0);
					Assembler().Append_CopyMemory(VM::REGISTER_D5, VM::REGISTER_D4, nBytesSource);
					return;
				}

				UseStackFrameFor(*this, sourceDef);
				Assembler().Append_CopySFVariable(targetDef.SFOffset + targetDef.MemberOffset, sourceDef.SFOffset + sourceDef.MemberOffset, nBytesSource);
				RestoreStackFrameFor(*this, sourceDef);
			}
		}
		else if (sourceDef.Usage == ARGUMENTUSAGE_BYREFERENCE && targetDef.Usage == ARGUMENTUSAGE_BYREFERENCE)
		{
			if (targetDef.IsParentValue || sourceDef.IsParentValue)
			{
				AssignVariableToVariable_ByRefSlow(*this, sourceDef, targetDef, source, target);
			}
			else
			{
				AssignVariableToVariable_ByRef(*this, sourceDef, targetDef, source, target);
			}
		}
		else if (sourceDef.Usage == ARGUMENTUSAGE_BYREFERENCE && targetDef.Usage == ARGUMENTUSAGE_BYVALUE)
		{
			size_t nBytesSource = sourceDef.AllocSize;

			if (sourceDef.ResolvedType->VarType() != targetDef.ResolvedType->VarType())
			{
				Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, "Type mismatch %s to %s (%s vs %s)", source, target, sourceDef.ResolvedType->Name(), targetDef.ResolvedType->Name());
			}

			if (!(targetDef.IsParentValue ^ sourceDef.IsParentValue))
			{
				UseStackFrameFor(*this, sourceDef);
				if (sourceDef.ResolvedType->VarType() == SexyVarType_Derivative && (nBytesSource != 8 && nBytesSource != 4))
				{
					Assembler().Append_CopySFVariableFromRef(targetDef.SFOffset + targetDef.MemberOffset, sourceDef.SFOffset, sourceDef.MemberOffset, nBytesSource);
				}
				else
				{
					Assembler().Append_SetSFValueFromSFMemberRef(sourceDef.SFOffset, sourceDef.MemberOffset, targetDef.MemberOffset + targetDef.SFOffset, nBytesSource);
				}
				RestoreStackFrameFor(*this, sourceDef);
			}
			else
			{
				UseStackFrameFor(*this, sourceDef);
				Assembler().Append_GetStackFrameMemberPtr(VM::REGISTER_D4, sourceDef.SFOffset, sourceDef.MemberOffset);
				RestoreStackFrameFor(*this, sourceDef);
				UseStackFrameFor(*this, targetDef);
				Assembler().Append_GetStackFrameAddress(VM::REGISTER_D5, targetDef.SFOffset + targetDef.MemberOffset);
				RestoreStackFrameFor(*this, targetDef);

				Assembler().Append_CopyMemory(VM::REGISTER_D5, VM::REGISTER_D4, nBytesSource);	
			}
		}
		else // source is by value, target is by ref, oft used by set accessors
		{
			size_t nBytesSource = sourceDef.AllocSize;

			if (!(targetDef.IsParentValue ^ sourceDef.IsParentValue))
			{
				UseStackFrameFor(*this, sourceDef);
				Assembler().Append_SetSFMemberRefFromSFValue(targetDef.SFOffset, targetDef.MemberOffset, sourceDef.MemberOffset + sourceDef.SFOffset, nBytesSource);
				RestoreStackFrameFor(*this, sourceDef);
			}
			else
			{
				UseStackFrameFor(*this, sourceDef);
				Assembler().Append_GetStackFrameAddress(VM::REGISTER_D4, sourceDef.SFOffset + sourceDef.MemberOffset);
				RestoreStackFrameFor(*this, sourceDef);
				UseStackFrameFor(*this, targetDef);
				Assembler().Append_GetStackFrameMemberPtr(VM::REGISTER_D5, targetDef.SFOffset, targetDef.MemberOffset);
				RestoreStackFrameFor(*this, targetDef);

				Assembler().Append_CopyMemory(VM::REGISTER_D5, VM::REGISTER_D4, nBytesSource);
			}
		}
	}

	BITCOUNT GetBitCountFromStruct(const MemberDef& def)
	{
		if (def.ResolvedType->InterfaceCount() > 0)
		{
			return BITCOUNT_POINTER;
		}
		else if (def.ResolvedType->VarType() == SexyVarType_Array)
		{
			return BITCOUNT_POINTER;
		}

		switch (def.AllocSize)
		{
		default:
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("Cannot Get bitcount for struct. Unhandled size!"));
		case 4:
			return BITCOUNT_32;
		case 8:
			return BITCOUNT_64;
		}
	}

	void CodeBuilder::AssignTempToVariable(int srcIndex, cstr target)
	{
		if (target == nullptr || *target == 0) Rococo::Throw(0, "CodeBuilder::AssignTempToVariable: target was blank");

		MemberDef def;
		if (!TryGetVariableByName(OUT def, target))
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("Cannot find variable %s"), target);
		}
		
		BITCOUNT bitCount = GetBitCountFromStruct(def);

		UseStackFrameFor(*this, def);

		if ((def.ResolvedType->VarType() == SexyVarType_Array && !def.IsContained) 
			|| (def.ResolvedType->VarType() == SexyVarType_Map && !def.IsContained)
			|| (def.ResolvedType->VarType() == SexyVarType_List && !def.IsContained)
			|| def.location == VARLOCATION_OUTPUT
			|| def.Usage != ARGUMENTUSAGE_BYREFERENCE)
		{
			Assembler().Append_SetStackFrameValue(def.SFOffset + def.MemberOffset, VM::REGISTER_D4 + (VM::DINDEX)srcIndex, bitCount);
		}
		else
		{
			Assembler().Append_SetSFMemberByRefFromRegister((VM::DINDEX) srcIndex + VM::REGISTER_D4, def.SFOffset, def.MemberOffset, bitCount);
		}

		RestoreStackFrameFor(*this, def);
	}

	void CodeBuilder::AssignVariableAddressToTemp(cstr sourceVariable, int tempDepth)
	{
		if (sourceVariable == nullptr || *sourceVariable == 0) Rococo::Throw(0, "CodeBuilder::AssignVariableAddressToTemp: sourceVariable was blank");

		MemberDef def;
		if (!TryGetVariableByName(OUT def, sourceVariable))
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, "Cannot find variable %s", sourceVariable);
		}

		UseStackFrameFor(*this, def);

		if (def.location == VARLOCATION_OUTPUT || def.Usage != ARGUMENTUSAGE_BYREFERENCE)
		{
			Assembler().Append_GetStackFrameAddress(VM::REGISTER_D4 + (VM::DINDEX) tempDepth, def.SFOffset + def.MemberOffset);
		}
		else
		{
			Assembler().Append_GetStackFrameMemberPtr((VM::DINDEX) tempDepth + VM::REGISTER_D4, def.SFOffset, def.MemberOffset);
		}

		RestoreStackFrameFor(*this, def);
	}

	void CodeBuilder::AssignVariableToGlobal(const GlobalValue& g, const MemberDef& def)
	{
		BITCOUNT bitCount = GetBitCountFromStruct(def);

		UseStackFrameFor(*this, def);

		if (def.Usage == ARGUMENTUSAGE_BYREFERENCE)
		{
			Assembler().Append_GetStackFrameMember(VM::REGISTER_D4, def.SFOffset, def.MemberOffset, bitCount);
		}
		else // def.Usage is by value
		{
			Assembler().Append_GetStackFrameValue(def.SFOffset + def.MemberOffset, VM::REGISTER_D4, bitCount);
		}

		// D4 now contains the local variable value
		Assembler().Append_SetGlobal(bitCount, (int32) g.offset);

		RestoreStackFrameFor(*this, def);
	}

	void CodeBuilder::AssignVariableFromGlobal(const GlobalValue& g, const MemberDef& def)
	{
		BITCOUNT bitCount = GetBitCountFromStruct(def);

		UseStackFrameFor(*this, def);

		// D4 need to contain the global variable value
		Assembler().Append_GetGlobal(bitCount, (int32) g.offset);

		if (def.Usage == ARGUMENTUSAGE_BYREFERENCE)
		{
			Assembler().Append_SetSFMemberByRefFromRegister(VM::REGISTER_D4, def.SFOffset, def.MemberOffset, bitCount);
		}
		else // def.Usage is by value
		{
			Assembler().Append_SetStackFrameValue(def.SFOffset + def.MemberOffset, VM::REGISTER_D4, bitCount);
		}

		RestoreStackFrameFor(*this, def);
	}

	void CodeBuilder::AssignLiteralToGlobal(const GlobalValue& g, const VariantValue& value)
	{
		BITCOUNT bitCount = GetBitCount(g.type);
		Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, value, bitCount);
		Assembler().Append_SetGlobal(bitCount, (int32) g.offset);
	}

	void CodeBuilder::BinaryOperatorAdd(int a, int b, SexyVarType type)
	{
		switch(type)
		{
		case SexyVarType_Float32:
			Assembler().Append_FloatAdd(VM::REGISTER_D4+ (VM::DINDEX)a,VM::REGISTER_D4+ (VM::DINDEX)b, VM::FLOATSPEC_SINGLE);
			break;
		case SexyVarType_Float64:
			Assembler().Append_FloatAdd(VM::REGISTER_D4 + (VM::DINDEX)a,VM::REGISTER_D4 + (VM::DINDEX)b, VM::FLOATSPEC_DOUBLE);
			break;
		case SexyVarType_Int32:
			Assembler().Append_IntAdd(VM::REGISTER_D4 + (VM::DINDEX)a, BITCOUNT_32, VM::REGISTER_D4 + (VM::DINDEX)b);
			break;
		case SexyVarType_Int64:
			Assembler().Append_IntAdd(VM::REGISTER_D4 + (VM::DINDEX)a, BITCOUNT_64, VM::REGISTER_D4 + (VM::DINDEX)b);
			break;
		default:
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("Unhandled type for BinaryOperatorAdd"));
		}
	}

	void CodeBuilder::BinaryOperatorSubtract(int a, int b, SexyVarType type)
	{
		switch (type)
		{
		case SexyVarType_Float32:
			Assembler().Append_FloatSubtract(VM::REGISTER_D4 + (VM::DINDEX)a, VM::REGISTER_D4 + (VM::DINDEX)b, VM::FLOATSPEC_SINGLE);
			break;
		case SexyVarType_Float64:
			Assembler().Append_FloatSubtract(VM::REGISTER_D4 + (VM::DINDEX)a, VM::REGISTER_D4 + (VM::DINDEX)b, VM::FLOATSPEC_DOUBLE);
			break;
		case SexyVarType_Int32:
			Assembler().Append_IntSubtract(VM::REGISTER_D4 + (VM::DINDEX)a, BITCOUNT_32, VM::REGISTER_D4 + (VM::DINDEX)b);
			break;
		case SexyVarType_Int64:
			Assembler().Append_IntSubtract(VM::REGISTER_D4 + (VM::DINDEX)a, BITCOUNT_64, VM::REGISTER_D4 + (VM::DINDEX)b);
			break;
		default:
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("Unhandled type for BinaryOperatorSubtract"));
		}
	}

	void CodeBuilder::BinaryOperatorMultiply(int a, int b, SexyVarType type)
	{
		switch (type)
		{
		case SexyVarType_Float32:
			Assembler().Append_FloatMultiply(VM::REGISTER_D4 + (VM::DINDEX)a, VM::REGISTER_D4 + (VM::DINDEX)b, VM::FLOATSPEC_SINGLE);
			break;
		case SexyVarType_Float64:
			Assembler().Append_FloatMultiply(VM::REGISTER_D4 + (VM::DINDEX)a, VM::REGISTER_D4 + (VM::DINDEX)b, VM::FLOATSPEC_DOUBLE);
			break;
		case SexyVarType_Int32:
			Assembler().Append_IntMultiply(VM::REGISTER_D4 + (VM::DINDEX)a, BITCOUNT_32, VM::REGISTER_D4 + (VM::DINDEX)b);
			break;
		case SexyVarType_Int64:
			Assembler().Append_IntMultiply(VM::REGISTER_D4 + (VM::DINDEX)a, BITCOUNT_64, VM::REGISTER_D4 + (VM::DINDEX)b);
			break;
		default:
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("Unhandled type for BinaryOperatorMultiply"));
		}
	}

	void CodeBuilder::BinaryOperatorDivide(int a, int b, SexyVarType type)
	{
		switch (type)
		{
		case SexyVarType_Float32:
			Assembler().Append_FloatDivide(VM::REGISTER_D4 + (VM::DINDEX)a, VM::REGISTER_D4 + (VM::DINDEX)b, VM::FLOATSPEC_SINGLE);
			break;
		case SexyVarType_Float64:
			Assembler().Append_FloatDivide(VM::REGISTER_D4 + (VM::DINDEX)a, VM::REGISTER_D4 + (VM::DINDEX)b, VM::FLOATSPEC_DOUBLE);
			break;
		case SexyVarType_Int32:
			Assembler().Append_IntDivide(VM::REGISTER_D4 + (VM::DINDEX)a, BITCOUNT_32, VM::REGISTER_D4 + (VM::DINDEX)b);
			break;
		case SexyVarType_Int64:
			Assembler().Append_IntDivide(VM::REGISTER_D4 + (VM::DINDEX)a, BITCOUNT_64, VM::REGISTER_D4 + (VM::DINDEX)b);
			break;
		default:
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("Unhandled type for BinaryOperatorMultiply"));
		}
	}

	void AssignInterfaceToTemp(CodeBuilder& builder, const MemberDef& sourceDef, cstr source, int tempIndex, int /* memberOffsetCorrection */)
	{
		UNUSED(source);
		// Assumes sourceDef is an interface

		// Scenarios -> 
		
		//      a) source is an input interface, a pointer on the SF
		//      b) source is a temporary variable, a pointer on the SF
		//      c) source is a member of a structure on the stack, a pointer on the SF
		//      d) source is a member of a structure referenced by a pointer on the stack

		//      a, b and c all copy the value on the SF, d must dereference the pointer on the SF and copy from it

		UseStackFrameFor(builder, sourceDef);

		if (sourceDef.location == VARLOCATION_TEMP && sourceDef.Usage == ARGUMENTUSAGE_BYREFERENCE && sourceDef.IsContained)
		{
			// Assign from derefenced pointer to a deferenced pointer -> in this case a list node
			builder.Assembler().Append_GetStackFrameMemberPtrAndDeref(VM::REGISTER_D4 + (VM::DINDEX)tempIndex, sourceDef.SFOffset, sourceDef.MemberOffset);
		}
		else if (sourceDef.location != VARLOCATION_INPUT || sourceDef.Usage == ARGUMENTUSAGE_BYVALUE || !sourceDef.IsContained)
		{
			// Assign from a value
			builder.Assembler().Append_GetStackFrameValue(sourceDef.SFOffset + sourceDef.MemberOffset, (VM::DINDEX)tempIndex + VM::REGISTER_D4, BITCOUNT_POINTER);
		}
		else
		{
			// Assign from derefenced pointer to a deferenced pointer
			builder.Assembler().Append_GetStackFrameMemberPtrAndDeref(VM::REGISTER_D4 + (VM::DINDEX)tempIndex, sourceDef.SFOffset, sourceDef.MemberOffset);
		}

		RestoreStackFrameFor(builder, sourceDef);
	}

	void CodeBuilder::AssignVariableToTemp(cstr source, int tempIndex, int memberOffsetCorrection)
	{
		if (source == nullptr || *source == 0) Rococo::Throw(0, "CodeBuilder::AssignVariableToTemp: source was blank");

		MemberDef def;
		if (!TryGetVariableByName(OUT def, source))
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, "Unknown source variable: %s", source);
		}

		const IStructure* srcType = def.ResolvedType;

		char symbol[256];
		SafeFormat(symbol, 256, "D%d = %s", tempIndex + VM::REGISTER_D4, source);
		AddSymbol(symbol);

		if (srcType->InterfaceCount() > 0)
		{
			AssignInterfaceToTemp(*this, def, source, tempIndex, memberOffsetCorrection);
		}
		else
		{
			def.MemberOffset += memberOffsetCorrection;

			UseStackFrameFor(*this, def);

			if (def.Usage == ARGUMENTUSAGE_BYREFERENCE)
			{
				if (def.IsContained)
				{
					Assembler().Append_GetStackFrameMember(VM::REGISTER_D4 + (VM::DINDEX)tempIndex, def.SFOffset, def.MemberOffset, BITCOUNT_POINTER);
				}
				else if (srcType->VarType() == SexyVarType_Array || srcType->VarType() == SexyVarType_Map || srcType->VarType() == SexyVarType_List)
				{
					Assembler().Append_GetStackFrameValue(def.SFOffset, VM::REGISTER_D4 + (VM::DINDEX)tempIndex, BITCOUNT_POINTER);
				}
				else if (srcType->VarType() != SexyVarType_Derivative)
				{		
					BITCOUNT bits = GetBitCount(srcType->VarType());
					// Pointer to primitive. Currently these only exist as a result of (foreach x # array ...) operations
					Assembler().Append_GetStackFrameMember(VM::REGISTER_D4 + (VM::DINDEX)tempIndex, def.SFOffset, def.MemberOffset, bits);
				}
				else
				{
					Assembler().Append_GetStackFrameValue(def.SFOffset, VM::REGISTER_D4 + (VM::DINDEX)tempIndex, BITCOUNT_POINTER);
				}
			}
			else // def.Usage is by value
			{
				BITCOUNT bitCount = GetBitCountFromStruct(def);
				Assembler().Append_GetStackFrameValue(def.SFOffset + def.MemberOffset, VM::REGISTER_D4 + (VM::DINDEX)tempIndex, bitCount);
			}

			RestoreStackFrameFor(*this, def);
		}
	}

	bool IsContainerType(const IStructure& s) 
	{
		SexyVarType vType = s.VarType();

		switch (vType)
		{
		case SexyVarType_List:
		case SexyVarType_Map:
		case SexyVarType_Array:
			return true;
		}

		return false;
	}

	void CodeBuilder::AssignVariableRefToTemp(cstr source, int tempDepth, int offset)
	{
		if (source == nullptr || *source == 0) Rococo::Throw(0, "CodeBuilder::AssignVariableRefToTemp: source was blank");

		MemberDef def;
		if (!TryGetVariableByName(OUT def, source))
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("Unknown source variable: %s"), source);
		}

		UseStackFrameFor(*this, def);

		if (def.Usage == ARGUMENTUSAGE_BYVALUE)
		{
			if (def.ResolvedType->InterfaceCount() || IsContainerType(*def.ResolvedType))
			{
				Assembler().Append_GetStackFrameValue(def.SFOffset + def.MemberOffset + offset, VM::REGISTER_D4 + (VM::DINDEX)tempDepth, BITCOUNT_POINTER);
			}
			else
			{
				Assembler().Append_GetStackFrameAddress(VM::REGISTER_D4 + (VM::DINDEX)tempDepth, def.SFOffset + def.MemberOffset + offset);
			}
		}
		else
		{
			if (!def.IsContained)
			{
				Assembler().Append_GetStackFrameValue(def.SFOffset, VM::REGISTER_D4 + (VM::DINDEX)tempDepth, BITCOUNT_POINTER);
				Assembler().Append_IncrementPtr(VM::REGISTER_D4 + (VM::DINDEX)tempDepth, offset);
			}
			else
			{
				if (def.IsContained && (def.ResolvedType->InterfaceCount() > 0 || IsContainerType(*def.ResolvedType)))
				{
					Assembler().Append_GetStackFrameMemberPtrAndDeref(VM::REGISTER_D4 + (VM::DINDEX)tempDepth, def.SFOffset, def.MemberOffset);
				}
				else
				{
					Assembler().Append_GetStackFrameMemberPtr(VM::REGISTER_D4 + (VM::DINDEX)tempDepth, def.SFOffset, def.MemberOffset);
				}

				Assembler().Append_IncrementPtr(VM::REGISTER_D4 + (VM::DINDEX)tempDepth, offset);
			}
		}

		RestoreStackFrameFor(*this, def);
	}

	void CodeBuilder::PushVariableRef(cstr source, int interfaceIndex)
	{
		if (source == nullptr || *source == 0) Rococo::Throw(0, "CodeBuilder::PushVariableRef: source was blank");

		MemberDef def;
		if (!TryGetVariableByName(OUT def, source))
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("Unknown source variable: %s"), source);
		}

		UseStackFrameFor(*this, def);

		if (def.Usage == ARGUMENTUSAGE_BYVALUE)
		{
			int totalOffset = def.SFOffset + def.MemberOffset;
			if (IsNullType(*def.ResolvedType))
			{
				totalOffset += (sizeof(ObjectStub) - sizeof(void*));
			}
			else if (interfaceIndex >= 0)
			{
				totalOffset += sizeof(ObjectStub) + interfaceIndex * sizeof(size_t);
			}
			
			Assembler().Append_PushStackFrameAddress(totalOffset);
		}
		else // by reference
		{
			// The reference (a pointer) is on the stack at def.SFOffset, but the place in the object is offset by def.MemberOffset
			if (interfaceIndex >= 0)
			{
				int offset;
				if (IsNullType(*def.ResolvedType))
				{
					offset = 0;
				}
				else // concrete reference
				{
					offset =  sizeof(ObjectStub) - sizeof(void*);
				}

				offset += interfaceIndex * sizeof(void*);

				offset -= GetThisOffset();
				
				Assembler().Append_PushStackFrameMemberPtr(def.SFOffset, offset);
			}
			else
			{
				if (def.MemberOffset != 0)
				{
					Assembler().Append_PushStackFrameMemberPtr(def.SFOffset, def.MemberOffset);
				}
				else
				{
					Assembler().Append_PushStackVariable(def.SFOffset, BITCOUNT_POINTER);
				}
			}
		}	

		RestoreStackFrameFor(*this, def);
	}

	int32 SizeTToInt(size_t x, cstr source, int lineNumber)
	{
		if (x > 0x7FFFFFFF) Throw(ERRORCODE_COMPILE_ERRORS, ("Error converting size_t to positive int32 at %s(%d)"), source, lineNumber);
		return (int32) x;
	}

#define SIZET_TO_INT32(x) SizeTToInt(x, __SEXFUNCTION__, __LINE__)

	CONDITION NotSo(CONDITION x)
	{
		switch(x)
		{
		case CONDITION_IF_EQUAL: return CONDITION_IF_NOT_EQUAL;
		case CONDITION_IF_GREATER_THAN: return CONDITION_IF_LESS_OR_EQUAL;
		case CONDITION_IF_GREATER_OR_EQUAL: return CONDITION_IF_LESS_THAN;
		case CONDITION_IF_NOT_EQUAL: return CONDITION_IF_EQUAL;
		case CONDITION_IF_LESS_THAN: return CONDITION_IF_GREATER_OR_EQUAL;
		case CONDITION_IF_LESS_OR_EQUAL: return CONDITION_IF_GREATER_THAN;
		default:
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("Unhandled case type 0x%x"), x);
		}
	}

	void CodeBuilder::AppendConditional(CONDITION condition, ICompileSection& thenSection, ICompileSection& elseSection)
	{
		size_t ifPos = Assembler().WritePosition();
		Assembler().Append_BranchIf(NotSo(condition), 0);
		size_t ifLen = Assembler().WritePosition() - ifPos;

		int attempts = 5;
		do 
		{
			thenSection.Compile(*this, f.Object());

			size_t elsePos = Assembler().WritePosition();
			Assembler().Append_Branch(0);
			size_t elseLen = Assembler().WritePosition() - elsePos;
		
			size_t elseSectionPos = Assembler().WritePosition();
			elseSection.Compile(*this, f.Object());

			size_t endPos = Assembler().WritePosition();
			
			Assembler().SetWriteModeToOverwrite(elsePos);
			Assembler().Append_Branch(SIZET_TO_INT32(endPos - elsePos));			
			size_t afterElsePos = Assembler().WritePosition();
			size_t newElseLen = afterElsePos - elsePos;
		
			Assembler().SetWriteModeToOverwrite(ifPos);
			Assembler().Append_BranchIf(NotSo(condition), SIZET_TO_INT32(elseSectionPos - ifPos));
			size_t afterIfPos = Assembler().WritePosition();
			size_t newIfLen = afterIfPos - ifPos;

			attempts--;

			if (newIfLen > ifLen)
			{
				ifLen = newIfLen;
				Assembler().Revert(afterIfPos);
				continue;
			}

			if (newElseLen > elseLen)
			{
				elseLen = newElseLen;
				Assembler().Revert(afterIfPos);
				continue;
			}

			Assembler().SetWriteModeToAppend();
			break;
		}while(attempts > 0);

		if (attempts == 0)
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("Failed to evaluate conditional statement."));
		}
	}

	void CodeBuilder::AppendWhileDo(ICompileSection& loopCriterion, CONDITION condition, ICompileSection& loopBody, ICompileSection& finalSection)
	{
		size_t loopEnterPos = Assembler().WritePosition();
		Assembler().Append_Branch(0); // When overwritten, branches to the loop criterion code

		size_t loopFinallyTopPos = Assembler().WritePosition();
		Assembler().Append_Branch(0); // When overwritten branches to the finally block
		int branchAbortLengthGuess = 0;
		int branchToExitLengthGuess = 0;
		
		size_t loopAbortPos = Assembler().WritePosition();

		int attempts = 5;

		do
		{			
			Assembler().Append_Branch(branchAbortLengthGuess);
			size_t branchAbortLength = Assembler().WritePosition() - loopAbortPos;

			size_t loopStartPos = Assembler().WritePosition();
			loopCriterion.Compile(*this, f.Object());

			size_t branchToExitPos = Assembler().WritePosition();
			Assembler().Append_BranchIf(NotSo(condition), branchToExitLengthGuess);	
			size_t branchToExitLength = Assembler().WritePosition() - branchToExitPos;

			ControlFlowData cfd;
			cfd.ContinuePosition = loopFinallyTopPos;
			cfd.BreakPosition = loopAbortPos;

			loopBody.Compile(*this, f.Object(), &cfd);

			size_t lowerFinallyBottomPos = Assembler().WritePosition();

			finalSection.Compile(*this, f.Object(), &cfd);

			size_t loopEndPos = Assembler().WritePosition();
			Assembler().Append_Branch(SIZET_TO_INT32(loopStartPos) - SIZET_TO_INT32(loopEndPos));
			
			size_t loopExitPos = Assembler().WritePosition();

			Assembler().SetWriteModeToOverwrite(branchToExitPos);
			branchToExitLengthGuess = SIZET_TO_INT32(loopExitPos - branchToExitPos);
			Assembler().Append_BranchIf(NotSo(condition), branchToExitLengthGuess);
			size_t afterBranchPos = Assembler().WritePosition();
			size_t newBranchToExitLength = afterBranchPos - branchToExitPos;

			attempts--;

			if (newBranchToExitLength != branchToExitLength)
			{
				// TODO - investigate whether this ever gets called
				OS::TripDebugger();
				Assembler().Revert(loopAbortPos);
				continue;
			}

			Assembler().SetWriteModeToOverwrite(loopAbortPos);
			branchAbortLengthGuess = (int32)(loopExitPos - loopAbortPos);
			Assembler().Append_Branch(SIZET_TO_INT32(branchAbortLengthGuess));
			size_t afterAbortBranchPos = Assembler().WritePosition();
			size_t newAbortBranchLength = afterAbortBranchPos - loopAbortPos;

			if (newAbortBranchLength != branchAbortLength)
			{
				// TODO - investigate whether this ever gets called
				OS::TripDebugger();
				Assembler().Revert(loopAbortPos);
				continue;
			}

			Assembler().SetWriteModeToOverwrite(loopEnterPos);
			Assembler().Append_Branch((int32)(loopStartPos - loopEnterPos));
			Assembler().SetWriteModeToAppend();


			Assembler().SetWriteModeToOverwrite(loopFinallyTopPos);
			Assembler().Append_Branch((int32)(lowerFinallyBottomPos - loopFinallyTopPos));
			Assembler().SetWriteModeToAppend();
			break;
		} while (attempts > 0);

		if (attempts == 0)
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, "Failed to evaluate while...do statement.");
		}
	}

	void GetVariableByName(ICodeBuilder& builder, OUT MemberDef& def, cstr name)
	{
		if (!builder.TryGetVariableByName(def, name))
		{
			Throw(ERRORCODE_COMPILE_ERRORS, builder.Module().Name(), "Error, cannot find entry %s", name);
		}
	}

	void ValidateVariable(const Variable* instance, cstr instanceName, IModule& module)
	{
		if (instance == NULL)
		{
			Throw(ERRORCODE_COMPILE_ERRORS, module.Name(), "Error, cannot find instance %s", instanceName);
		}
	}

	void CodeBuilder::AppendDoWhile(ICompileSection& loopBody, ICompileSection& loopCriterion, CONDITION condition)
	{
		int branchBreakLengthGuess = 0;
		int branchContinueLengthGuess = 0;	
		int branchToStartGuess = 0;

		int attempts = 5;

		do
		{
			size_t loopEnterPos = Assembler().WritePosition();
			Assembler().Append_Branch(branchToStartGuess);
			size_t branchToStartLength = Assembler().WritePosition() - loopEnterPos;

			size_t loopBreakPos = Assembler().WritePosition();
			Assembler().Append_Branch(branchBreakLengthGuess);
			size_t branchBreakLength = Assembler().WritePosition() - loopBreakPos;

			size_t loopContinuePos = Assembler().WritePosition();
			Assembler().Append_Branch(branchContinueLengthGuess);
			size_t branchContinueLength = Assembler().WritePosition() - loopContinuePos;

			size_t loopStartPos = Assembler().WritePosition();

			ControlFlowData cfd;
			cfd.ContinuePosition = loopContinuePos;
			cfd.BreakPosition = loopBreakPos;
			loopBody.Compile(*this, f.Object(), &cfd);

			size_t criterionPos = Assembler().WritePosition();
			loopCriterion.Compile(*this, f.Object());

			Assembler().Append_NoOperation();

			size_t loopBackPos = Assembler().WritePosition();
			Assembler().Append_BranchIf(condition, SIZET_TO_INT32(loopStartPos) - SIZET_TO_INT32(loopBackPos));

			size_t loopExitPos = Assembler().WritePosition();

			branchBreakLengthGuess = (int32)(loopExitPos - loopBreakPos);
			Assembler().SetWriteModeToOverwrite(loopBreakPos);
			Assembler().Append_Branch(branchBreakLengthGuess);
			size_t newBranchBreakLength = Assembler().WritePosition() - loopBreakPos;

			branchContinueLengthGuess = (int32) (criterionPos - loopContinuePos);
			Assembler().SetWriteModeToOverwrite(loopContinuePos);
			Assembler().Append_Branch(branchContinueLengthGuess);
			size_t newBranchContinueLength = Assembler().WritePosition() - loopContinuePos;

			branchToStartGuess = (int32)(loopStartPos - loopEnterPos);
			Assembler().SetWriteModeToOverwrite(loopEnterPos);
			Assembler().Append_Branch(branchToStartGuess);
			size_t newBranchToStartLength = Assembler().WritePosition() - loopEnterPos;

			attempts--;
			
			Assembler().SetWriteModeToAppend();

			if (newBranchBreakLength != branchBreakLength)
			{
				Assembler().Revert(loopEnterPos);
				continue;
			}

			if (newBranchContinueLength != branchContinueLength)
			{
				Assembler().Revert(loopEnterPos);
				continue;
			}

			if (newBranchToStartLength != branchToStartLength)
			{
				Assembler().Revert(loopBreakPos);
				continue;
			}
			
			break;
		} while (attempts > 0);

		if (attempts == 0)
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, "Failed to evaluate do...while statement.");
		}
	}

	void CodeBuilder::LeaveSection()
	{
		// Erase labels in the section, as goto statements are only allowed to jump into siblings or ancestors
		auto i = mapLabelToPosition.begin();
		while (i != mapLabelToPosition.end())
		{
			if (i->second.sectionIndex == sectionIndex)
			{
				i = mapLabelToPosition.erase(i);
			}
			else
			{
				i++;
			}
		}

		sectionIndex--;

		for (auto& label : outstandingLabels)
		{
			// Once we leave a section that contained the goto statement, we are only permitted to search for labels in the ancestors and not other sections with the same depth
			label.sectionIndex = min(label.sectionIndex, sectionIndex);
		}

		if (!lambdaVariables.empty())
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, "Section closed with an outstanding lambda variable stack");
		}

		if (sectionIndex < 0)
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, "Section closed without a corresponding open");
		}

		sections.pop_back();
	}
}

namespace Rococo { namespace Compiler
{
	ICodeBuilder* CreateBuilder(IFunctionBuilder& f, bool _mayUseParentSF)
	{
		return new Anon::CodeBuilder(f, _mayUseParentSF);
	}
}} // Rococo::Compiler
