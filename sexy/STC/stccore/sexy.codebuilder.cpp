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

#include "Sexy.Compiler.StdAfx.h"
#include "Sexy.Validators.h"
#include "Sexy.VM.h"
#include "sexy.compiler.helpers.h"
#include "sexy.strings.h"
#include "sexy.stdstrings.h"

using namespace Sexy;
using namespace Sexy::Compiler;
using namespace Sexy::Parse;

namespace
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
			pcEnd(-1),
			primaryOffset(_primaryOffset),
			tempDepthOnRelease(-1),
			canCaptureClosureData(arg.IsClosureInput())
		{
			location = arg.Direction() == ARGDIRECTION_INPUT ? VARLOCATION_INPUT : VARLOCATION_OUTPUT;
		}

		Variable(size_t _pcStart, const NameString& name, const IStructure& resolvedType, int32 _sectionIndex, void* _userData, int _allocSize, VARLOCATION _location, bool asRef = false, int _tempDepthOnRelease = -1):
			name(name.c_str()),
			usage(asRef ? ARGUMENTUSAGE_BYREFERENCE : ARGUMENTUSAGE_BYVALUE),
			sectionIndex(_sectionIndex),
			resolvedType(resolvedType),
			userData(_userData),
			allocSize(_allocSize),
			pcStart(_pcStart),
			pcEnd(-1),
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

		csexstr Name() const { return name.c_str(); }
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

	typedef std::vector<Variable*> TVariables;
	typedef std::vector<ControlFlowData> TControlStack;
	typedef std::unordered_map<size_t,StackRecoveryData> TMapCodeOffsetToStackCorrection;
	typedef std::vector<int> TInstancePositions;
	typedef std::list<int> TSectionStack;

	typedef std::unordered_map<size_t,SymbolValue> TPCSymbols;

	typedef std::vector<const IStructure*> TTypeVector;

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
		
		Variable* TryGetVariableByName(csexstr name, OUT int32& offsetCorrect);		
		const Variable* TryGetVariableByName(csexstr name, OUT int32& offsetCorrect) const;

		bool IsVariableDefinedAtLevel(int32 sectionIndex, csexstr name);
		
		void AllocateLocalVariable(Variable& v, REF size_t& stackExpansion);
		void GetCodeSection(OUT CodeSection& section) const { section.Start = functionStartPosition; section.End = functionEndPosition; section.Id = byteCodeId; }
		void BuildInputAndOutputOffsets();

		Variable& DemandVariableByName(csexstr name, csexstr clue, OUT int32& offsetCorrect);		
		int AddVariableToVariables(Variable* v);

		int nextId;
	public:
		CodeBuilder(IFunctionBuilder& _f, bool _mayUseParentsSF);
		~CodeBuilder(void);

		Sexy::VM::IAssembler& Assembler() { return *assembler; }

		IModuleBuilder& Module() { return f.Module(); }
		const IModule& Module() const { return f.Module(); }

		virtual int SectionArgCount() const { return sections.back(); }

		virtual IFunctionBuilder& Owner() { return f; }
		virtual const IFunction& Owner() const { return f; }
		virtual void AddExpression(IBinaryExpression& tree);
		virtual void Begin();
		virtual void EnterSection();
		virtual void Append_InitializeVirtualTable(csexstr className);
		virtual void Append_InitializeVirtualTable(csexstr instanceName, const IStructure& classType);
		virtual void AddVariable(const NameString& name, const TypeString& type, void* userData);
		virtual void AddVariable(const NameString& name, const IStructure& type, void* userData);
		virtual void AddVariableRef(const NameString& name, const IStructure& type, void* userData);
		virtual void AddPseudoVariable(const NameString& ns, const IStructure& st);
		virtual void AddCatchVariable(csexstr name, void* userData);
		virtual void AssignLiteral(const NameString& name, csexstr valueLiteral);
		virtual void AssignPointer(const NameString& name, const void* ptr);
		virtual void AssignVariableToVariable(csexstr source, csexstr value);
		virtual void AssignVariableToTemp(csexstr source, int tempIndex, int offsetCorrection);
		virtual void AssignVariableRefToTemp(csexstr source, int tempDepth, int offset);
		virtual void AssignVariableToGlobal(const GlobalValue& g, const MemberDef& def);
		virtual void AssignVariableFromGlobal(const GlobalValue& g, const MemberDef& def);
		virtual void AssignLiteralToGlobal(const GlobalValue& g, const VariantValue& value);
		virtual void AssignTempToVariable(int srcIndex, csexstr target);
		virtual void BinaryOperatorAdd(int srcInvariantIndex, int trgMutatingIndex, VARTYPE type);
		virtual void BinaryOperatorSubtract(int srcInvariantIndex, int trgMutatingIndex, VARTYPE type);
		virtual void BinaryOperatorMultiply(int srcInvariantIndex, int trgInvariantIndex, VARTYPE type);
		virtual void BinaryOperatorDivide(int srcInvariantIndex, int trgInvariantIndex, VARTYPE type);
		virtual void AppendConditional(CONDITION condition, ICompileSection& thenSection, ICompileSection& elseSection);
		virtual void AppendDoWhile(ICompileSection& loopBody, ICompileSection& loopCriterion, CONDITION condition);
		virtual void AppendWhileDo(ICompileSection& loopCriterion, CONDITION condition, ICompileSection& loopBody);
		virtual void Negate(int srcInvariantIndex, VARTYPE varType);
		virtual void LeaveSection();
		virtual void End();
		virtual VARTYPE GetVarType(csexstr name) const;
		virtual const IStructure* GetVarStructure(csexstr name) const;
		virtual void AssignClosureParentSF();
		virtual void EnableClosures(csexstr targetVariable);
		virtual void Free() { delete this; }
		virtual bool TryGetVariableByName(OUT MemberDef& def, csexstr name) const;
		virtual int GetVariableCount() const;
		virtual void GetVariableByIndex(OUT MemberDef& def, csexstr& name, int index) const;
		virtual void PopControlFlowPoint();
		virtual void PushControlFlowPoint(const ControlFlowData& controlFlowData);
		virtual bool TryGetControlFlowPoint(OUT ControlFlowData& data);
		virtual const bool NeedsParentsSF() const { return codeReferencesParentsSF; }

		virtual void PopLastVariables(int count);
		virtual const StackRecoveryData& GetRequiredStackCorrection(size_t codeOffset) const;
		virtual void NoteStackCorrection(int stackCorrection);
		virtual void NoteDestructorPosition(int instancePosition, const IStructure& type);
		virtual int GetDestructorFromInstancePos(int instancePosition) const
		{
			if (instancePosition < 0 || instancePosition >= (int) destructorPositions.size())
			{
				Throw(ERRORCODE_BAD_ARGUMENT, SEXTEXT("GetDestructorFromInstancePos"), SEXTEXT("Bad instancePosition: %d"), instancePosition);
			}
			return destructorPositions[instancePosition]; 
		}
		virtual const IStructure& GetTypeFromInstancePos(int instancePosition) const { return *posToType[instancePosition]; }

		virtual void AddSymbol(csexstr text);
		virtual void MarkExpression(const void* sourceExpression);
		virtual void DeleteSymbols();
		virtual SymbolValue GetSymbol(size_t pcAddressOffset) const;

		virtual int GetLocalVariableSymbolCount() const;
		virtual void GetLocalVariableSymbolByIndex(OUT MemberDef& def, OUT csexstr& name, IN int index) const;
		virtual bool GetLocalVariableSymbolByName(OUT MemberDef& def, OUT csexstr name, IN size_t pcAddress) const;

		virtual void PushVariable(const MemberDef& def);
		virtual void PushVariableRef(csexstr source, int interfaceIndex);

		virtual int GetOffset(size_t argIndex) const;

		virtual int GetThisOffset() const;
		virtual void SetThisOffset(int offset);

		virtual void ArchiveRegister(int saveTempDepth, int restoreTempDepth, BITCOUNT bits, void* userData);
		virtual void AddArgVariable(csexstr desc, const TypeString& typeName, void* userData);
		virtual void AddArgVariable(csexstr desc, const IStructure& type, void* userData);
	};

	CodeBuilder::CodeBuilder(IFunctionBuilder& _f, bool _mayUseParentsSF):
		f(_f),
		sectionIndex(0),
		allocatedIndexLevel(-1),
		functionStartPosition(-1),
		functionEndPosition(-1),
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

		for(auto i = expiredVariables.begin(); i != expiredVariables.end(); ++i)
		{
			Variable* v = *i;
			delete v;
		}
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
		StringPrint(name, SEXTEXT("_archive_D%d_%d"), saveTempDepth + 4, nextId++);

		const IStructure& type =  (bits == BITCOUNT_32) ? Module().Object().Common().TypeInt32() :  Module().Object().Common().TypeInt64();

		Variable *v = new Variable(Assembler().WritePosition(), NameString::From(name), type, sectionIndex, userData, -1, VARLOCATION_TEMP, false, restoreTempDepth);
		int dx = AddVariableToVariables(v);
		if (dx >= 0) Assembler().Append_PushRegister(saveTempDepth + 4, bits);
	}

	void CodeBuilder::AddArgVariable(csexstr desc, const IStructure& type, void* userData)
	{
		TokenBuffer name;
		StringPrint(name, SEXTEXT("_arg_%s_%d"), desc, nextId++);

		const IStructure* argType = NULL;

		if (type.VarType() == VARTYPE_Derivative)
		{
			// Derivative types are passed by ref, os the arg type is pointer
			argType = &Module().Object().Common().TypePointer();
		}
		else
		{
			argType = &type;
		}

		Variable *v = new Variable(Assembler().WritePosition(), NameString::From(name), *argType, sectionIndex, userData, -1, VARLOCATION_TEMP, false);
		int dx = AddVariableToVariables(v);			
	}

	void CodeBuilder::AddArgVariable(csexstr desc, const TypeString& typeName, void* userData)
	{
		const IStructure& type = *MatchStructure(Module().Object().Log(), typeName.c_str(), Module());

		if (&type != NULL)
		{
			AddArgVariable(desc, type, userData);	
		}
		else
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Unknown type [%s]"), typeName);
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
		const IStructure& s = *def.ResolvedType;
		BITCOUNT bc = GetBitCount(s.VarType());
		if (s.VarType() != VARTYPE_Derivative)
		{
			// Primitives are pushed directly

			if (s.VarType() == VARTYPE_Closure)
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
		else // structures get passed by reference
		{
			if (def.Usage == ARGUMENTUSAGE_BYVALUE)
			{
				int totalOffset = def.SFOffset;
				if (def.ResolvedType->Prototype().IsClass)
				{
					if (IsNullType(*def.ResolvedType))
					{
						// Push the only interface of the null object
						totalOffset += 2 * sizeof(size_t);
					}
				}
				if (totalOffset != 0)
				{
					Assembler().Append_PushStackFrameAddress(totalOffset);
				}
				else
				{
					Assembler().Append_PushRegister(VM::REGISTER_SF, BITCOUNT_POINTER);
				}
			}
			else
			{
				Assembler().Append_GetStackFrameAddress(VM::REGISTER_D4, def.SFOffset);
				Assembler().Append_PushRegister(VM::REGISTER_D4, BITCOUNT_POINTER);
			}		
		}
	}

	void CodeBuilder::AddSymbol(csexstr text)
	{
		size_t address = assembler->WritePosition();

		csexstr symbolText = Module().Object().RegisterSymbol(text);

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
			symbol.Text = SEXTEXT("");

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
			nullSym.Text = SEXTEXT("");
			return nullSym;
		}
	}

	int CodeBuilder::GetLocalVariableSymbolCount() const
	{
		return (int) (variables.size() + expiredVariables.size());
	}

	void CodeBuilder::GetLocalVariableSymbolByIndex(OUT MemberDef& def, OUT csexstr& name, IN int index) const
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

	bool CodeBuilder::GetLocalVariableSymbolByName(OUT MemberDef& def, csexstr _name, size_t pcAddress) const
	{
		int totalVarCount = GetLocalVariableSymbolCount();
		for(int i = 0; i < totalVarCount; ++i)
		{
			csexstr name;
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

	void CodeBuilder::PopLastVariables(int count)
	{
		int bytesToFree = 0;

		for (int i = count; i > 0; i--)
		{
			if (variables.empty()) Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Serious algorithmic error in compile. An attempt was made to deconstruct a temp variable that does not exist"));
			
			Variable* v = variables.back();
			if (v->Offset() < 0)	Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Serious algorithmic error in compile. An attempt was made to deconstruct an argument to a function inside the function"));

			int vOffset = v->Offset();
			
			v->SetPCEnd(Assembler().WritePosition());
			variables.pop_back();
			expiredVariables.push_back(v);

			int tempDepth = v->TempDepthOnRelease();
			if (tempDepth >= 0)
			{
				if (bytesToFree > 0)
				{
					Assembler().Append_StackAlloc(-bytesToFree);
					bytesToFree = 0;
				}

				BITCOUNT bits = v->ResolvedType().SizeOfStruct() == 4 ? BITCOUNT_32 : BITCOUNT_64;
				Assembler().Append_RestoreRegister(tempDepth + VM::REGISTER_D4, bits);
			}
			else
			{
				int allocSize = v->AllocSize();
				
				if (allocSize == 0)
				{
					// Pseudo variable
				}
				else
				{
					bytesToFree += allocSize;
				}
			}
						
			nextOffset = vOffset;			
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

	VARTYPE GetMemberType(const IStructure& s, csexstr name)
	{
		NamespaceSplitter splitter(name);
		csexstr head, body;
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
				return VARTYPE_Bad;
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
				return VARTYPE_Bad;
			}
		}
	}

	int CodeBuilder::GetOffset(size_t argIndex) const
	{
		if (argIndex >= variables.size())
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("The variable index was out of range."));
		}

		const Variable* v = variables[variables.size() - argIndex - 1];
		return v->Offset();
	}

	const IStructure* CodeBuilder::GetVarStructure(csexstr varName) const
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

	VARTYPE CodeBuilder::GetVarType(csexstr name) const
	{
		MemberDef def;
		if (TryGetVariableByName(OUT def, name))
		{
			return def.ResolvedType->VarType();
		}

		return VARTYPE_Bad;

// 		NamespaceSplitter splitter(name);
// 		csexstr head, body;
// 		if (!splitter.SplitHead(OUT head, OUT body))
// 		{
// 			const Variable* v = GetVariableByName(name);
// 			return v == NULL ? VARTYPE_Bad : v->ResolvedType().VarType();
// 		}
// 		else
// 		{
// 			const Variable* v = GetVariableByName(head);
// 			if (v == NULL)
// 			{
// 				return VARTYPE_Bad;
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
	
	Variable* CodeBuilder::TryGetVariableByName(csexstr name, OUT int32& offsetCorrect)
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

	bool GetMemberVariable(OUT MemberDef& def, const Variable& v, const IStructure& struc, csexstr memberName)
	{
		NamespaceSplitter splitter(memberName);
		csexstr head, body;

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

	void CodeBuilder::EnableClosures(csexstr targetVariable)
	{
		int offsetCorrection;
		Variable * v = TryGetVariableByName(targetVariable, OUT offsetCorrection);
		if (v == nullptr)
		{
			Throw(ERRORCODE_BAD_ARGUMENT, __SEXFUNCTION__, SEXTEXT("Cannot find variable %s"), targetVariable);
		}

		if (v->ResolvedType().VarType() != VARTYPE_Closure)
		{
			Throw(ERRORCODE_BAD_ARGUMENT, __SEXFUNCTION__, SEXTEXT("Variable %s is not a closure"), targetVariable);
		}

		v->AllowCaptureClosure();
	}

	bool CodeBuilder::TryGetVariableByName(OUT MemberDef& def, csexstr name) const
	{
		NamespaceSplitter splitter(name);
		csexstr head, body;

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
				v = parentBuilder.TryGetVariableByName(name, offsetCorrection);
				if (v != NULL)
				{
					codeReferencesParentsSF = true;
					def.CapturesLocalVariables = v->CanCaptureClosures();
					def.IsParentValue = true;
					def.Userdata = v->Userdata();
					def.AllocSize = v->AllocSize();
					def.MemberOffset = offsetCorrection;
					def.location = v->Location();
					def.IsContained = false;
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

	void CodeBuilder::GetVariableByIndex(OUT MemberDef& def, csexstr& name, int index) const
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

	const Variable* CodeBuilder::TryGetVariableByName(csexstr name, OUT int32& offsetCorrect) const
	{		
		if (variables.empty()) return NULL;

		if (AreEqual(name, SEXTEXT("this")))
		{
			Variable* vThis = variables[0];

			if (Owner().IsVirtualMethod())
			{
				offsetCorrect = -thisOffset;
			}
			else
			{
				offsetCorrect = 0;
			}

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

	Variable& CodeBuilder::DemandVariableByName(csexstr name, csexstr clue, OUT int32& offsetCorrect)
	{
		Variable* v = CodeBuilder::TryGetVariableByName(name, OUT offsetCorrect);
		if (v == NULL) Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Could not find %s variable [%s]"), clue, name);
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

	bool CodeBuilder::IsVariableDefinedAtLevel(int32 sectionIndex, csexstr name)
	{
		for(TVariables::const_reverse_iterator i = variables.rbegin(); i != variables.rend(); ++i)
		{
			Variable* v = *i;
			if (v->SectionIndex() >= sectionIndex)
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
		size_t x = 2*sizeof(void*);
		int32 sfOffset = -(int32)x;

		for(int32 i = ArgCount(f)-1; i >= 0; --i)
		{
			const IArgument& arg = f.Arg(i);
			if (arg.ResolvedType() == NULL)
			{
				sexstringstream streamer;
				streamer << SEXTEXT("Could not resolve type (Arg #") << i << SEXTEXT(") ") << arg.TypeString() << SEXTEXT(" ") << arg.Name() << std::ends; 
				Throw(ERRORCODE_NULL_POINTER, __SEXFUNCTION__, streamer.str().c_str());
			}

			int varOffset = 0;
			if (i == ArgCount(f)-1)
			{
				varOffset = thisOffset;
			}
			Variable* v = new Variable(arg, /* section index */ 0, arg.Userdata(), varOffset);

			// Derivative types are always passed by reference (a pointer)
			int32 dx =  (v->ResolvedType().VarType() == VARTYPE_Derivative) ? (int32) sizeof(size_t) : (int32) v->AllocSize();			
			sfOffset -= dx;
			v->SetStackPosition(sfOffset);
			variables.push_back(v);
		}
	}

	void CodeBuilder::AssignClosureParentSF()
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

		AssignClosureParentSF();
	}

	void CodeBuilder::End()
	{
		sectionIndex--;
		if (sectionIndex > 0)
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Section not closed"));
		}

		int endOfTemps = 0;

		int nTemps = GetVariableCount() - ArgCount(Owner());
		if (nTemps > 0)
		{
			Variable* finalVar = variables.back();
			endOfTemps = finalVar->Offset() + finalVar->AllocSize();
		}

		int returnAddressOffset = (int) sizeof(size_t);

		if (endOfTemps > 0)
		{
			Assembler().Append_StackAlloc(-endOfTemps);
		}
		
		functionEndPosition = Assembler().WritePosition();

		TokenBuffer symbol;
		StringPrint(symbol, SEXTEXT("%d bytes"), functionEndPosition);
		AddSymbol(symbol);
		Assembler().Append_Return();		

		if (codeReferencesParentsSF && !mayUseParentsSF)
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("The closure body required access to the parent's stackframe, which is prohibited in its context"));
		}

		if (!f.Object().ProgramMemory().UpdateBytecode(byteCodeId, Assembler()))
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Insufficient program memory. Either reduce program size or increase MaxProgramBytes in ProgramInitParameters"));
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
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Variable [%s] already defined in this scope"), name.c_str());
		}

		if (&type != NULL)
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
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Could not add variable [%s]"), name.c_str());
		}
	}

	void CodeBuilder::AddVariable(const Sexy::Compiler::NameString& name, const Sexy::Compiler::TypeString& type, void* userData)
	{
		if (IsVariableDefinedAtLevel(sectionIndex, name.c_str()))
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Variable [%s] already defined in this scope"), name.c_str());
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
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Could not add variable [%s]"), name.c_str());
		}
	}

	void CodeBuilder::AddPseudoVariable(const NameString& name, const IStructure& type)
	{
		if (IsVariableDefinedAtLevel(sectionIndex, name.c_str()))
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Variable [%s] already defined in this scope"), name.c_str());
		}

		if (&type != NULL)
		{
			Variable *v = new Variable(Assembler().WritePosition(), name, type, sectionIndex, NULL, 0, VARLOCATION_NONE);
			variables.push_back(v);
			v->SetStackPosition(nextOffset);
		}
		else
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Could not add variable [%s]"), name.c_str());
		}
	}

	void CodeBuilder::AddVariable(const Sexy::Compiler::NameString& name, const IStructure& type, void* userData)
	{
		if (IsVariableDefinedAtLevel(sectionIndex, name.c_str()))
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Variable [%s] already defined in this scope"), name.c_str());
		}

		if (&type != NULL)
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
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Could not add variable [%s]"), name.c_str());
		}
	}

	void CodeBuilder::AddCatchVariable(csexstr name, void* userData)
	{
		if (IsVariableDefinedAtLevel(sectionIndex, name))
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Variable [%s] already defined in this scope"), name);
		}

		const IInterface& interf = Module().Object().Common().SysTypeIException();
		if (&interf == NULL)
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Cannot find IException in intrinsics module"), name);
		}

		const IStructure& nullObjectType = interf.NullObjectType();

		Variable *v = new Variable(Assembler().WritePosition(), NameString::From(name), nullObjectType, sectionIndex, userData, -1, VARLOCATION_TEMP);
		variables.push_back(v);

		int32 dx = v->AllocSize();

		int offset = nextOffset; // SF takes us just beyond the return address of the caller
		nextOffset += dx;

		v->SetStackPosition(offset);		
	}

	void CodeBuilder::AssignLiteral(const Sexy::Compiler::NameString& name, csexstr literalValue)
	{
		REQUIRE_NOT_NULL(literalValue);

		MemberDef def;
		if (!TryGetVariableByName(OUT def, name.c_str()))
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Could not assign '%s' to [%s]. Could not find the variable."), literalValue, name.c_str());
		}
		
		VariantValue value;

		VARTYPE vType = def.ResolvedType->VarType();
		if (vType == VARTYPE_Derivative)
		{
			if (def.ResolvedType->InterfaceCount() == 0)
			{
				Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Could not assign '%s' to [%s]. The variable is not of atomic type and has no interface."), literalValue, name.c_str());
			}

			const IInterface& interf = def.ResolvedType->GetInterface(0);

			if (!AreEqual(SEXTEXT("0"), literalValue))
			{
				Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Could not assign a non zero literal to interface [%s]"), name.c_str());
			}

			value.vPtrValue =  interf.UniversalNullInstance();
		}
		else if (vType == VARTYPE_Pointer)
		{
			if (!AreEqual(SEXTEXT("0"), literalValue))
			{
				Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Could not assign a non zero literal to pointer [%s]"), name.c_str());
			}

			value.vPtrValue = NULL;
		}
		else
		{
			PARSERESULT result = TryParse(OUT value, vType, literalValue);
			if (result != PARSERESULT_GOOD)
			{
				Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Cannot parse literal value '%s' to [%s]"), literalValue, name.c_str());
			}
		}

		if (def.Usage == ARGUMENTUSAGE_BYVALUE)
		{
			if (def.IsParentValue) { Assembler().Append_SwapRegister(VM::REGISTER_SF, VM::REGISTER_D6);	}
			Assembler().Append_SetStackFrameImmediate(def.SFOffset + def.MemberOffset, value, GetBitCount(vType));
			if (def.IsParentValue) {	Assembler().Append_SwapRegister(VM::REGISTER_SF, VM::REGISTER_D6);	}
		}
		else //ByRef
		{
			Assembler().Append_SetRegisterImmediate(VM::REGISTER_D5, value, GetBitCount(vType));
			Assembler().Append_SetSFMemberByRefFromRegister(VM::REGISTER_D5, def.SFOffset, def.MemberOffset, GetBitCount(vType));
		}
	}

	void CodeBuilder::AssignPointer(const NameString& name, const void* ptr)
	{
		MemberDef def;
		if (!TryGetVariableByName(OUT def, name.c_str()))
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Could not assign '%p' to [%s]. Could not find the variable."), ptr, name.c_str());
		}
		
		VariantValue value;

		VARTYPE vType = def.ResolvedType->VarType();
		if (vType != VARTYPE_Pointer)
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Could not assign '%p' to [%s]. Variable must of type Pointer."), ptr, name.c_str());
		}

		value.vPtrValue = (void*) ptr;
		
		if (def.Usage == ARGUMENTUSAGE_BYVALUE)
		{
			if (def.IsParentValue) { Assembler().Append_SwapRegister(VM::REGISTER_SF, VM::REGISTER_D6);	}
			Assembler().Append_SetStackFrameImmediate(def.SFOffset + def.MemberOffset, value, GetBitCount(vType));
			if (def.IsParentValue) {	Assembler().Append_SwapRegister(VM::REGISTER_SF, VM::REGISTER_D6);	}
		}
		else //ByRef
		{
			Assembler().Append_SetRegisterImmediate(VM::REGISTER_D5, value, GetBitCount(vType));
			Assembler().Append_SetSFMemberByRefFromRegister(VM::REGISTER_D5, def.SFOffset, def.MemberOffset, GetBitCount(vType));
		}
	}

	void AssignVariableToVariable_ByRef(ICodeBuilder& builder, const MemberDef& sourceDef, const MemberDef& targetDef, csexstr source, csexstr target)
	{
		const IStructure& s = *sourceDef.ResolvedType;
		const IStructure& t = *targetDef.ResolvedType;
		VM::IAssembler& assembler = builder.Assembler();

		if (sourceDef.location != VARLOCATION_OUTPUT && targetDef.location != VARLOCATION_OUTPUT)
		{
			if (&s != &t)
			{
				Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Cannot copy %s %s to %s %s. They must be the same type"), GetFriendlyName(s), source, GetFriendlyName(t), target);
			}
			if (s.Prototype().IsClass)
			{
				if (targetDef.AllocSize != 0)
				{
					Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Cannot copy %s %s. A class type can only be copied to a reference to class type"), GetFriendlyName(s), source);
				}

				TokenBuffer refName;
				GetRefName(refName, target);

				MemberDef refDef;
				if (!builder.TryGetVariableByName(refDef, refName))
				{
					Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Cannot copy %s %s. No associated reference to %s."), GetFriendlyName(s), source, target);
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
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Cannot return a reference of %s to %s. The source would be freed before the function returns"), source, target);
		}	
		else if (sourceDef.location == VARLOCATION_OUTPUT)
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Cannot copy a value from %s. It is an output value"), target);
		}	
		else // Copy a reference of the input to the output
		{
			if (targetDef.MemberOffset != 0)
			{
				Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Did not expect a member offset in the output %s"), target);
			}

			TokenBuffer sourceRef;
			GetRefName(sourceRef, source);

			MemberDef refDef;
			if (!builder.TryGetVariableByName(OUT refDef, sourceRef))
			{
				Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Could not find the reference for %s for variable. %s"), (csexstr) sourceRef, (csexstr) source);
			}

			assembler.Append_SetSFValueFromSFMemberRef(refDef.SFOffset, refDef.MemberOffset, targetDef.SFOffset, sizeof(size_t));
		}
	}

	void AssignVariableToVariableBothByValue(VM::IAssembler& assembler, const MemberDef& src, const MemberDef& dest, BITCOUNT bitcount)
	{
		if (src.IsParentValue) 
		{
			if (dest.IsParentValue)
			{
				assembler.Append_SwapRegister(VM::REGISTER_SF, VM::REGISTER_D6);
				assembler.Append_SetSFValueFromSFValue(dest.SFOffset + dest.MemberOffset, src.SFOffset + src.MemberOffset, bitcount);
				assembler.Append_SwapRegister(VM::REGISTER_SF, VM::REGISTER_D6);
			}
			else
			{
				assembler.Append_SwapRegister(VM::REGISTER_SF, VM::REGISTER_D6);
				assembler.Append_GetStackFrameValue(src.SFOffset + src.MemberOffset, VM::REGISTER_D4, bitcount);
				assembler.Append_SwapRegister(VM::REGISTER_SF, VM::REGISTER_D6);

				assembler.Append_SetStackFrameValue(dest.SFOffset + dest.MemberOffset, VM::REGISTER_D4, bitcount);
			}
		}
		else
		{
			if (dest.IsParentValue)
			{
				assembler.Append_GetStackFrameValue(src.SFOffset + src.MemberOffset, VM::REGISTER_D4, bitcount);
				assembler.Append_SwapRegister(VM::REGISTER_SF, VM::REGISTER_D6);
				assembler.Append_SetStackFrameValue(dest.SFOffset + dest.MemberOffset, VM::REGISTER_D4, bitcount);
				assembler.Append_SwapRegister(VM::REGISTER_SF, VM::REGISTER_D6);
			}
			else
			{
				assembler.Append_SetSFValueFromSFValue(dest.SFOffset + dest.MemberOffset, src.SFOffset + src.MemberOffset, bitcount);
			}
		}
	}

	void CodeBuilder::AssignVariableToVariable(csexstr source, csexstr target)
	{
		REQUIRE_NOT_BLANK(source);
		REQUIRE_NOT_BLANK(target);
	
		MemberDef sourceDef;
		if (!TryGetVariableByName(OUT sourceDef, source))
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Could not resolve %s"), source);
		}

		MemberDef targetDef;
		if (!TryGetVariableByName(OUT targetDef, target))
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Could not resolve %s"), target);
		}

		const IStructure* srcType = sourceDef.ResolvedType;
		const IStructure* trgType = targetDef.ResolvedType;

		if (srcType != trgType)
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Type mismatch trying to assign [%s %s] to [%s %s]"), GetFriendlyName(*sourceDef.ResolvedType), source, GetFriendlyName(*targetDef.ResolvedType), target);
		}

		if (sourceDef.CapturesLocalVariables && !targetDef.CapturesLocalVariables)
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Could not copy %s to %s. The target variable accepts regular function references, but not closures."), source, target);
		}

		if (targetDef.location == VARLOCATION_OUTPUT)
		{
			if (IsNullType(*trgType))
			{				
			}
			else if (trgType->VarType() == VARTYPE_Derivative)
			{
				Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Do not know how to handle a concrete type as output. %s %s"), trgType->Name(), (csexstr) target);
			}
		}

		if (sourceDef.Usage == ARGUMENTUSAGE_BYVALUE && targetDef.Usage == ARGUMENTUSAGE_BYVALUE)
		{
			size_t nBytesSource = sourceDef.AllocSize;
			if (nBytesSource == sizeof(uint32))
			{
				AssignVariableToVariableBothByValue(*assembler, sourceDef, targetDef, BITCOUNT_32);
			}
			else if (nBytesSource == sizeof(uint64))
			{
				AssignVariableToVariableBothByValue(*assembler, sourceDef, targetDef, BITCOUNT_64);
			}
			else
			{
				if (sourceDef.ResolvedType->VarType() != VARTYPE_Derivative)
				{
					Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Type mismatch trying to assign %s to %s. Unusual size of primitive arguments"), source, target);
				}

				if (targetDef.IsParentValue || sourceDef.IsParentValue)
				{
					Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Cannot handle this case for a closure upvalue"));
				}
				Assembler().Append_CopySFVariable(targetDef.SFOffset + targetDef.MemberOffset, sourceDef.SFOffset + sourceDef.MemberOffset, nBytesSource);
			}
		}
		else if (sourceDef.Usage == ARGUMENTUSAGE_BYREFERENCE && targetDef.Usage == ARGUMENTUSAGE_BYREFERENCE)
		{
			size_t nBytesSource = sourceDef.AllocSize;

			if (targetDef.IsParentValue || sourceDef.IsParentValue)
			{
				Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Cannot handle this case for a closure upvalue"));
			}

			AssignVariableToVariable_ByRef(*this, sourceDef, targetDef, source, target);
		}
		else if (sourceDef.Usage == ARGUMENTUSAGE_BYREFERENCE && targetDef.Usage == ARGUMENTUSAGE_BYVALUE)
		{
			size_t nBytesSource = sourceDef.AllocSize;

			if (sourceDef.ResolvedType->VarType() == VARTYPE_Derivative)
			{
				Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Error assigning %s to %s. The source was of derivative type"), source, target);
			}

			if (sourceDef.ResolvedType->VarType() != targetDef.ResolvedType->VarType())
			{
				Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Type mismatch %s to %s (%s vs %s)"), source, target, sourceDef.ResolvedType->Name(), targetDef.ResolvedType->Name());
			}

			if (targetDef.IsParentValue || sourceDef.IsParentValue)
			{
				Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Cannot handle this case for a closure upvalue"));
			}

			Assembler().Append_SetSFValueFromSFMemberRef(sourceDef.SFOffset, sourceDef.MemberOffset, targetDef.MemberOffset + targetDef.SFOffset, nBytesSource);	
		}
		else // source is by value, target is by ref, oft used by set accessors
		{
			size_t nBytesSource = sourceDef.AllocSize;

			if (srcType->VarType() == VARTYPE_Derivative)
			{
				Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Cannot assign %s to %s. The source was of derived type"), source, target);
			}

			if (targetDef.IsParentValue || sourceDef.IsParentValue)
			{
				Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Cannot handle this case for a closure upvalue"));
			}

			Assembler().Append_SetSFMemberRefFromSFValue(targetDef.SFOffset, targetDef.MemberOffset, sourceDef.MemberOffset + sourceDef.SFOffset, nBytesSource);			
		}
	}

	void CodeBuilder::AssignTempToVariable(int srcIndex, csexstr target)
	{
		REQUIRE_NOT_BLANK(target);
		MemberDef def;
		if (!TryGetVariableByName(OUT def, target))
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Cannot find variable %s"), target);
		}
		
		BITCOUNT bitCount;

		if (def.Usage == ARGUMENTUSAGE_BYREFERENCE && 1 == 0)
		{
			bitCount = BITCOUNT_POINTER;
		}
		else
		{
			switch(def.AllocSize)
			{
			default:			
				Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Cannot assign from datatype. Unhandled size!"));
			case 4:
				bitCount = BITCOUNT_32;
				break;
			case 8:
				bitCount = BITCOUNT_64;
				break;
				break;
			}		
		}
		
		if (def.Usage == ARGUMENTUSAGE_BYREFERENCE)
		{
			if (def.IsParentValue) Assembler().Append_SwapRegister(VM::REGISTER_SF, VM::REGISTER_D6);

			if (srcIndex < 1) Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Cannot assign from datatype. Bad src index."));
			Assembler().Append_SetSFMemberByRefFromRegister(srcIndex + VM::REGISTER_D4, def.SFOffset, def.MemberOffset, bitCount);			
			if (def.IsParentValue) Assembler().Append_SwapRegister(VM::REGISTER_SF, VM::REGISTER_D6);
		}
		else
		{
			if (def.IsParentValue) Assembler().Append_SwapRegister(VM::REGISTER_SF, VM::REGISTER_D6);
			Assembler().Append_SetStackFrameValue(def.SFOffset + def.MemberOffset, VM::REGISTER_D4 + srcIndex, bitCount);
			if (def.IsParentValue) Assembler().Append_SwapRegister(VM::REGISTER_SF, VM::REGISTER_D6);
		}
	}

	void CodeBuilder::AssignVariableToGlobal(const GlobalValue& g, const MemberDef& def)
	{
		BITCOUNT bitCount;
		switch (def.AllocSize)
		{
		default:
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Cannot assign from datatype. Unhandled size!"));
		case 4:
			bitCount = BITCOUNT_32;
			break;
		case 8:
			bitCount = BITCOUNT_64;
			break;
		}

		if (def.IsParentValue) { Assembler().Append_SwapRegister(VM::REGISTER_SF, VM::REGISTER_D6); }

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

		if (def.IsParentValue) { Assembler().Append_SwapRegister(VM::REGISTER_SF, VM::REGISTER_D6); }
	}

	void CodeBuilder::AssignVariableFromGlobal(const GlobalValue& g, const MemberDef& def)
	{
		BITCOUNT bitCount;
		switch (def.AllocSize)
		{
		default:
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Cannot assign from datatype. Unhandled size!"));
		case 4:
			bitCount = BITCOUNT_32;
			break;
		case 8:
			bitCount = BITCOUNT_64;
			break;
		}

		if (def.IsParentValue) { Assembler().Append_SwapRegister(VM::REGISTER_SF, VM::REGISTER_D6); }
		
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

		if (def.IsParentValue) { Assembler().Append_SwapRegister(VM::REGISTER_SF, VM::REGISTER_D6); }
	}

	void CodeBuilder::AssignLiteralToGlobal(const GlobalValue& g, const VariantValue& value)
	{
		BITCOUNT bitCount = GetBitCount(g.type);
		Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, value, bitCount);
		Assembler().Append_SetGlobal(bitCount, (int32) g.offset);
	}

	void CodeBuilder::Negate(int a, VARTYPE varType)
	{
		switch(varType)
		{
		case VARTYPE_Float32:
			{
				VariantValue v;
				v.floatValue = 0;
				Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4+a+2, v, BITCOUNT_32);
				Assembler().Append_FloatSubtract(VM::REGISTER_D4+a+2, VM::REGISTER_D4+a, VM::FLOATSPEC_SINGLE);
			}
			break;
		case VARTYPE_Float64:
			{
				VariantValue v;
				v.doubleValue = 0;
				Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4+a+2, v, BITCOUNT_64);
				Assembler().Append_FloatSubtract(VM::REGISTER_D4+a+2, VM::REGISTER_D4+a, VM::FLOATSPEC_DOUBLE);
			}
			break;
		case VARTYPE_Int32:
			Assembler().Append_IntNegate(VM::REGISTER_D4+a, BITCOUNT_32, VM::REGISTER_D4+a+1);
			break;
		case VARTYPE_Int64:
			Assembler().Append_IntNegate(VM::REGISTER_D4+a, BITCOUNT_64, VM::REGISTER_D4+a+1);
			break;
		default:
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Unhandled type for Negate"));
		}
	}

	void CodeBuilder::BinaryOperatorAdd(int a, int b, VARTYPE type)
	{
		switch(type)
		{
		case VARTYPE_Float32:
			Assembler().Append_FloatAdd(VM::REGISTER_D4+a,VM::REGISTER_D4+b, VM::FLOATSPEC_SINGLE);
			break;
		case VARTYPE_Float64:
			Assembler().Append_FloatAdd(VM::REGISTER_D4+a,VM::REGISTER_D4+b, VM::FLOATSPEC_DOUBLE);
			break;
		case VARTYPE_Int32:
			Assembler().Append_IntAdd(VM::REGISTER_D4+a, BITCOUNT_32, VM::REGISTER_D4+b);
			break;
		case VARTYPE_Int64:
			Assembler().Append_IntAdd(VM::REGISTER_D4+a, BITCOUNT_64, VM::REGISTER_D4+b);
			break;
		default:
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Unhandled type for BinaryOperatorAdd"));
		}
	}

	void CodeBuilder::BinaryOperatorSubtract(int a, int b, VARTYPE type)
	{
		switch(type)
		{
		case VARTYPE_Float32:
			Assembler().Append_FloatSubtract(VM::REGISTER_D4+a,VM::REGISTER_D4+b, VM::FLOATSPEC_SINGLE);
			break;
		case VARTYPE_Float64:
			Assembler().Append_FloatSubtract(VM::REGISTER_D4+a,VM::REGISTER_D4+b, VM::FLOATSPEC_DOUBLE);
			break;
		case VARTYPE_Int32:
			Assembler().Append_IntSubtract(VM::REGISTER_D4+a, BITCOUNT_32, VM::REGISTER_D4+b);
			break;
		case VARTYPE_Int64:
			Assembler().Append_IntSubtract(VM::REGISTER_D4+a, BITCOUNT_64, VM::REGISTER_D4+b);
			break;
		default:
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Unhandled type for BinaryOperatorSubtract"));
		}
	}

	void CodeBuilder::BinaryOperatorMultiply(int a, int b, VARTYPE type)
	{
		switch(type)
		{
		case VARTYPE_Float32:
			Assembler().Append_FloatMultiply(VM::REGISTER_D4+a,VM::REGISTER_D4+b, VM::FLOATSPEC_SINGLE);
			break;
		case VARTYPE_Float64:
			Assembler().Append_FloatMultiply(VM::REGISTER_D4+a,VM::REGISTER_D4+b, VM::FLOATSPEC_DOUBLE);
			break;
		case VARTYPE_Int32:
			Assembler().Append_IntMultiply(VM::REGISTER_D4+a, BITCOUNT_32, VM::REGISTER_D4+b);
			break;
		case VARTYPE_Int64:
			Assembler().Append_IntMultiply(VM::REGISTER_D4+a, BITCOUNT_64, VM::REGISTER_D4+b);
			break;
		default:
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Unhandled type for BinaryOperatorMultiply"));
		}
	}

	void CodeBuilder::BinaryOperatorDivide(int a, int b, VARTYPE type)
	{
		switch(type)
		{
		case VARTYPE_Float32:
			Assembler().Append_FloatDivide(VM::REGISTER_D4+a,VM::REGISTER_D4+b, VM::FLOATSPEC_SINGLE);
			break;
		case VARTYPE_Float64:
			Assembler().Append_FloatDivide(VM::REGISTER_D4+a,VM::REGISTER_D4+b, VM::FLOATSPEC_DOUBLE);
			break;
		case VARTYPE_Int32:
			Assembler().Append_IntDivide(VM::REGISTER_D4+a, BITCOUNT_32, VM::REGISTER_D4+b);
			break;
		case VARTYPE_Int64:
			Assembler().Append_IntDivide(VM::REGISTER_D4+a, BITCOUNT_64, VM::REGISTER_D4+b);
			break;
		default:
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Unhandled type for BinaryOperatorMultiply"));
		}
	}

	BITCOUNT GetBitCountFromStruct(size_t sizeOfStruct)
	{
		switch(sizeOfStruct)
		{
		default:
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Cannot Get bitcount for struct. Unhandled size!"));
		case 4:
			return BITCOUNT_32;
		case 8:
			return BITCOUNT_64;
		}		
	}

	void CodeBuilder::AssignVariableToTemp(csexstr source, int tempIndex, int memberOffsetCorrection)
	{
		REQUIRE_NOT_BLANK(source);
		MemberDef def;
		if (!TryGetVariableByName(OUT def, source))
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Unknown source variable: %s"), source);
		}

		def.MemberOffset += memberOffsetCorrection;

		BITCOUNT bitCount = GetBitCountFromStruct(def.AllocSize);

		if (def.IsParentValue)	{	Assembler().Append_SwapRegister(VM::REGISTER_SF, VM::REGISTER_D6); }

		if (def.Usage == ARGUMENTUSAGE_BYREFERENCE)
		{
			Assembler().Append_GetStackFrameMember(VM::REGISTER_D4 + tempIndex, def.SFOffset, def.MemberOffset, bitCount);
		}
		else // def.Usage is by value
		{
			Assembler().Append_GetStackFrameValue(def.SFOffset + def.MemberOffset, VM::REGISTER_D4 + tempIndex, bitCount);			
		}

		if (def.IsParentValue)	{	Assembler().Append_SwapRegister(VM::REGISTER_SF, VM::REGISTER_D6); }
	}

	void CodeBuilder::AssignVariableRefToTemp(csexstr source, int tempDepth, int offset)
	{
		REQUIRE_NOT_BLANK(source);

		MemberDef def;
		if (!TryGetVariableByName(OUT def, source))
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Unknown source variable: %s"), source);
		}

		if (def.IsParentValue) Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Cannot handle upvalue refs"));
		
		if (def.Usage == ARGUMENTUSAGE_BYVALUE)
		{
			Assembler().Append_GetStackFrameAddress(VM::REGISTER_D4 + tempDepth, def.SFOffset + def.MemberOffset + offset);
		}
		else
		{
			Assembler().Append_GetStackFrameMemberPtr(VM::REGISTER_D4 + tempDepth, def.SFOffset,  def.MemberOffset);
			Assembler().Append_IncrementPtr(VM::REGISTER_D4 + tempDepth , offset);
		}
	}

	void CodeBuilder::PushVariableRef(csexstr source, int interfaceIndex)
	{
		REQUIRE_NOT_BLANK(source);

		MemberDef def;
		if (!TryGetVariableByName(OUT def, source))
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Unknown source variable: %s"), source);
		}

		if (def.IsParentValue) { Assembler().Append_SwapRegister(VM::REGISTER_SF, VM::REGISTER_D6);	}

		if (def.Usage == ARGUMENTUSAGE_BYVALUE)
		{
			int totalOffset = def.SFOffset + def.MemberOffset;
			if (IsNullType(*def.ResolvedType))
			{
				totalOffset += (sizeof(ObjectStub) - sizeof(void*));
			}
			else if (interfaceIndex > 0)
			{
				totalOffset += (sizeof(ObjectStub) - sizeof(void*) + (interfaceIndex - 1) * sizeof(size_t));
			}
			
			Assembler().Append_PushStackFrameAddress(totalOffset);
		}
		else // by reference
		{
			// The reference (a pointer) is on the stack at def.SFOffset, but the place in the object is offset by def.MemberOffset
			if (interfaceIndex > 0)
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

				offset += (interfaceIndex-1) * sizeof(void*);
				
				Assembler().Append_GetStackFrameMemberPtr(VM::REGISTER_D4, def.SFOffset, offset);			
				Assembler().Append_PushRegister(VM::REGISTER_D4, BITCOUNT_POINTER);
			}
			else
			{
				if (def.MemberOffset != 0)
				{
					Assembler().Append_GetStackFrameMemberPtr(VM::REGISTER_D4, def.SFOffset, def.MemberOffset);
					Assembler().Append_PushRegister(VM::REGISTER_D4, BITCOUNT_POINTER);
				}
				else
				{
					Assembler().Append_PushStackVariable(def.SFOffset, BITCOUNT_POINTER);
				}
			}
		}	

		if (def.IsParentValue) { Assembler().Append_SwapRegister(VM::REGISTER_SF, VM::REGISTER_D6);	}
	}

	int32 SizeTToInt(size_t x, csexstr source, int lineNumber)
	{
		if (x > 0x7FFFFFFF) Throw(ERRORCODE_COMPILE_ERRORS, SEXTEXT("Error converting size_t to positive int32 at %s(%d)"), source, lineNumber);
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
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Unhandled case type 0x%x"), x);
			return CONDITION_IF_EQUAL;
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

			attempts--;
		}while(attempts > 0);

		if (attempts == 0)
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Failed to evaluate conditional statement."));
		}
	}

	void CodeBuilder::AppendWhileDo(ICompileSection& loopCriterion, CONDITION condition, ICompileSection& loopBody)
	{
		size_t loopEnterPos = Assembler().WritePosition();
		Assembler().Append_Branch(0);

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
			cfd.ContinuePosition = loopStartPos;
			cfd.BreakPosition = loopAbortPos;

			loopBody.Compile(*this, f.Object(), &cfd);

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
				Assembler().Revert(loopAbortPos);
				continue;
			}

			Assembler().SetWriteModeToOverwrite(loopEnterPos);
			Assembler().Append_Branch((int32)(loopStartPos - loopEnterPos));
			Assembler().SetWriteModeToAppend();
			break;
		} while (attempts > 0);

		if (attempts == 0)
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Failed to evaluate while...do statement."));
		}
	}

	void GetVariableByName(ICodeBuilder& builder, OUT MemberDef& def, csexstr name)
	{
		if (!builder.TryGetVariableByName(def, name))
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Error, cannot find entry ") << name << std::ends;
			Throw(ERRORCODE_COMPILE_ERRORS, builder.Module().Name(), streamer.str().c_str());
		}
	}

	void ValidateVariable(const Variable* instance, csexstr instanceName, IModule& module)
	{
		if (instance == NULL)
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Error, cannot find instance ") << instanceName << std::ends;
			Throw(ERRORCODE_COMPILE_ERRORS, module.Name(), streamer.str().c_str());
		}
	}

	void InitVirtualTable(CodeBuilder& builder, const MemberDef& def, csexstr instanceName, const IStructure& classType)
	{
		int sfOffset = 0;

		if (def.Usage == ARGUMENTUSAGE_BYREFERENCE)
		{
			// Put the stack frame into D4 and make the address of the object the stack
			sfOffset = 0;
			builder.Assembler().Append_MoveRegister(VM::REGISTER_SF, VM::REGISTER_D4, BITCOUNT_POINTER);
			builder.Assembler().Append_GetStackFrameValue(def.SFOffset, VM::REGISTER_SF, BITCOUNT_POINTER);
		}
		else
		{
			sfOffset = def.SFOffset + def.MemberOffset;
		}

		TokenBuffer vTableSymbol;
		StringPrint(vTableSymbol, SEXTEXT("%s._typeInfo"), classType.Name());
		builder.AddSymbol(vTableSymbol);

		VariantValue v;
		v.uint8PtrValue = (uint8*) classType.GetVirtualTable(0);
		builder.Assembler().Append_SetStackFrameImmediate(sfOffset, v, BITCOUNT_POINTER);
		
		StringPrint(vTableSymbol, SEXTEXT("%s._allocSize"), classType.Name());
		builder.AddSymbol(vTableSymbol);

		v.int32Value = classType.SizeOfStruct();
		builder.Assembler().Append_SetStackFrameImmediate(sfOffset + sizeof(size_t), v, BITCOUNT_32);

		for(int i = 0; i < classType.InterfaceCount(); i++)
		{
			const IInterface& interf = classType.GetInterface(i);
			StringPrint(vTableSymbol, SEXTEXT("%s.%s._vTable"), classType.Name(), interf.Name());
			builder.AddSymbol(vTableSymbol);

			v.uint8PtrValue = (uint8*) classType.GetVirtualTable(i+1);
			builder.Assembler().Append_SetStackFrameImmediate(sfOffset + sizeof(int32) + sizeof(size_t) * (i+1), v, BITCOUNT_POINTER);
		}		
		
		if (def.Usage == ARGUMENTUSAGE_BYREFERENCE)
		{
			// Restore the stack frame
			builder.Assembler().Append_MoveRegister(VM::REGISTER_D4, VM::REGISTER_SF, BITCOUNT_POINTER);
		}		
	}

	void CodeBuilder::Append_InitializeVirtualTable(csexstr instanceName)
	{
		MemberDef def;
		if (!TryGetVariableByName(OUT def, instanceName))
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Error, cannot find instance ") << instanceName << std::ends;
			Throw(ERRORCODE_COMPILE_ERRORS, Module().Name(), streamer.str().c_str());
		}

		if (def.ResolvedType->Prototype().IsClass)
		{
			InitVirtualTable(*this, def, instanceName, *def.ResolvedType);
		}
	}

	void CodeBuilder::Append_InitializeVirtualTable(csexstr instanceName, const IStructure& classType)
	{
		MemberDef def;
		if (!TryGetVariableByName(OUT def, instanceName))
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Error, cannot find instance ") << instanceName << std::ends;
			Throw(ERRORCODE_COMPILE_ERRORS, classType.Module().Name(), streamer.str().c_str());
		}

		if (def.ResolvedType->Prototype().IsClass)
		{
			InitVirtualTable(*this, def, instanceName, classType);
		}
	}

	void CodeBuilder::AppendDoWhile(ICompileSection& loopBody, ICompileSection& loopCriterion, CONDITION condition)
	{
		int branchBreakLengthGuess = 0;
		int branchContinueLengthGuess = 0;
		int branchToExitLengthGuess = 0;		
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
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Failed to evaluate do...while statement."));
		}
	}

	void CodeBuilder::LeaveSection()
	{
		sectionIndex--;
		if (sectionIndex < 0)
		{
			Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Section closed without a corresponding open"));
		}

		sections.pop_back();
	}
}

namespace Sexy { namespace Compiler
{
	ICodeBuilder* CreateBuilder(IFunctionBuilder& f, bool _mayUseParentSF)
	{
		return new CodeBuilder(f, _mayUseParentSF);
	}
}} // Sexy::Compiler