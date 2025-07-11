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

#pragma once

#ifndef STC_H
# define STC_H

#ifndef SEXY_H
#error include "sexy.types.h" before including this file
#endif

#ifndef sexy_compiler_public_h
#error include "sexy.compiler.pubic.h"
#endif

#ifdef _WIN32
# pragma pack(push,1)
#endif

namespace Rococo::Script
{
	struct NativeCallSecurity;
}

// justification: "No problem with an interface deriving from another and adding methods"
#pragma warning(disable : 4263)
#pragma warning(disable : 4264)

namespace Rococo::Compiler
{
	struct ICodeBuilder;
	struct IFunctionBuilder;
	struct IModuleBuilder;
	struct INamespaceBuilder;
	struct IProgramObject;
	struct IStructureBuilder;
	struct IInterfaceBuilder;

	struct ControlFlowData
	{
		size_t BreakPosition;
		size_t ContinuePosition;
		mutable size_t SectionIndex;
	};

	ROCOCO_INTERFACE ICompileSection
	{
		virtual void Compile(ICodeBuilder & builder, IProgramObject & object, ControlFlowData * controlFlowData = nullptr) = 0;
	};

	ROCOCO_INTERFACE IArgumentBuilder : public IArgument
	{
		virtual bool TryResolveArgument() = 0;
		virtual IFunctionBuilder& Parent() = 0;
	};

	ROCOCO_INTERFACE IFunctionBuilder : public IFunction
	{
		virtual void Free() = 0;
		virtual void AddDefaultToCurrentArgument(cstr defaultValueString) = 0;
		virtual IArgumentBuilder& AddInput(const NameString& name, const TypeString& type, void* userdata) = 0;
		virtual IArgumentBuilder& AddClosureInput(const NameString& name, const TypeString& type, void* userdata) = 0;
		virtual IArgumentBuilder& AddInput(const NameString& name, const IStructure& type, void* userdata) = 0;
		virtual IArgumentBuilder& AddInput(const NameString& name, const TypeString& genericType, const TypeString& genericArgType1, void* userdata) = 0;
		virtual IArgumentBuilder& AddInput(const NameString& name, const TypeString& genericType, const TypeString& genericArgType1, const TypeString& genericArgType2, void* userdata) = 0;
		virtual IArgumentBuilder& AddOutput(const NameString& name, const TypeString& type, void* userdata) = 0;
		virtual IArgumentBuilder& AddOutput(const NameString& name, const IStructure& type, void* userdata) = 0;
		virtual IArgumentBuilder& AddArrayOutput(const NameString& name, const TypeString& genericType, void* userdata) = 0;
		virtual bool TryResolveArguments() = 0;
		virtual void ValidateArguments() = 0;

		virtual ICodeBuilder& Builder() = 0;
		virtual IModuleBuilder& Module() = 0;
		virtual IFunctionBuilder* Parent() = 0;

		virtual IProgramObject& Object() = 0;

		virtual void SetType(const IStructure* type) = 0;

		virtual void SetProxy(ID_BYTECODE id) = 0;

		virtual void AddSecurity(const Rococo::Script::NativeSecurityHandler& security) = 0;
	};

	ROCOCO_INTERFACE IFunctionAliasBuilder : public IFunctionAlias
	{
		virtual IFunctionBuilder & GetFunction() = 0;
	};

	ROCOCO_INTERFACE IStructAliasBuilder : public IStructAlias
	{
		virtual IStructureBuilder & GetStructure() = 0;
	};

	ROCOCO_INTERFACE IFunctionEnumeratorBuilder : public IFunctionEnumerator
	{
		virtual IFunctionAliasBuilder & operator[](int index) = 0;
	};

	SEXYUTIL_API IFunctionBuilder* FindByName(IFunctionEnumeratorBuilder& e, cstr publicName);

	ROCOCO_INTERFACE IMemberLife
	{
		virtual void Release(uint8 * pInstance) = 0;
	};

	ROCOCO_INTERFACE IMemberLifeSupervisor : IMemberLife
	{
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE IMemberBuilder : public IMember
	{
		virtual IStructureBuilder * UnderlyingType() = 0;
		virtual IStructureBuilder* UnderlyingGenericArg1Type() = 0;
		virtual IStructureBuilder* UnderlyingGenericArg2Type() = 0;
		virtual void SetLifeTimeManager(IMemberLife* life) = 0;
		virtual void Release(uint8* pInstance) = 0;
	};

	ROCOCO_INTERFACE IStructureBuilder : public IStructure
	{
		virtual void Free() = 0;
		virtual IModuleBuilder& Module() = 0;
		virtual void AddAttribute(const Rococo::Sex::ISExpression& sourceDef, bool isCustom) = 0;
		virtual void AddInterface(cstr interfaceFullName) = 0;
		virtual IMemberBuilder& AddMember(const NameString& name, const TypeString& type, cstr genericArg1 = nullptr, cstr genericArg2 = nullptr) = 0;
		virtual void AddInterfaceMember(const NameString& name, const TypeString& type) = 0;
		virtual void Seal() = 0;
		virtual IMemberBuilder& GetMember(int index) = 0;
		virtual IInterfaceBuilder& GetInterface(int index) = 0;
		virtual void ExpandAllocSize(int minimumByteCount) = 0;
		virtual void FillVirtualTables() = 0;
		virtual void MakeStrong() = 0;
		virtual void Update() = 0;
		virtual void SetConstructor(const IFunction* cons) = 0;

		virtual IProgramObject& Object() = 0;
	};

	ROCOCO_INTERFACE IModuleBuilder : public IModule
	{
		virtual void ClearPrefixes() = 0;
		virtual void Clear() = 0;
		virtual IStructure& DeclareClass(cstr name, const StructurePrototype& prototype, const Sex::ISExpression*) = 0;
		virtual IFunctionBuilder& DeclareClosure(IFunctionBuilder& parent, bool mayUseParentSF, const Sex::ISExpression*) = 0;
		virtual IFunctionBuilder& DeclareFunction(const FunctionPrototype& prototype, const Sex::ISExpression*, int popBytes = 0) = 0;
		virtual IStructureBuilder& DeclareStrongType(cstr name, SexyVarType underlyingType) = 0;
		virtual IStructureBuilder& DeclareStructure(cstr name, const StructurePrototype& prototype, const Sex::ISExpression*) = 0;
		virtual IFunctionBuilder* FindFunction(cstr name) = 0;
		virtual IStructureBuilder* FindStructure(cstr name) = 0;
		virtual IFunctionBuilder& GetFunction(int index) = 0;
		virtual IStructureBuilder& GetStructure(int index) = 0;
		virtual void UsePrefix(cstr name) = 0;
		virtual INamespaceBuilder& GetPrefix(int index) = 0;
		virtual void IncVersion() = 0;
		virtual void SetDefaultNamespace(const INamespace* ns) = 0;
		virtual void SetPackage(IPackage* package, cstr namespaceString) = 0;

		virtual IProgramObject& Object() = 0;
	};

	ROCOCO_INTERFACE IInterfaceBuilder : public IInterface
	{
		virtual IAttributes & Attributes() = 0;
		virtual IStructureBuilder& NullObjectType() = 0;
		virtual void SetMethod(size_t index, cstr name, size_t argCount, cstr argNames[], const IStructure* types[], const IArchetype* archetypes[], const IStructure* genericArg1s[], const bool isOut[], const Sex::ISExpression*) = 0;
		virtual void PostCompile() = 0;
		virtual void ExpandNullObjectAllocSize(int minimumByteCount) = 0;
	};

	ROCOCO_INTERFACE IFactoryBuilder : public IFactory
	{
		virtual void SetInline(IFunctionBuilder * f, IStructureBuilder * s) = 0; // Used in the compilation phase to set the inline implementation for the factory
		virtual IFunctionBuilder& Constructor() = 0;
		virtual IStructureBuilder* InlineClass() = 0; // if not NULL indicates the concrete class of the inline constructor
	};

	ROCOCO_INTERFACE IMacroBuilder : public IMacro
	{
		virtual IFunctionBuilder & Implementation() = 0;
	};

	ROCOCO_INTERFACE INamespaceBuilder : public INamespace
	{
		virtual INamespaceBuilder * Parent() = 0;
		virtual INamespaceBuilder& GetChild(size_t index) = 0;
		virtual INamespaceBuilder* FindSubspace(cstr childName) = 0;
		virtual INamespaceBuilder& AddNamespace(cstr childName, ADDNAMESPACEFLAGS flags) = 0;
		virtual IMacroBuilder* AddMacro(cstr name, void* expression, IFunctionBuilder& f) = 0;
		virtual const IArchetype& AddArchetype(cstr name, cstr argNames[], const IStructure* stArray[], const IArchetype* archArray[], const IStructure* genericArg1s[], int numberOfOutputs, int numberOfInputs, const void* definition) = 0;
		virtual void Alias(IFunctionBuilder& f) = 0;
		virtual void Alias(cstr publicName, IStructureBuilder& s) = 0;
		virtual void Alias(cstr publicName, IFunctionBuilder& f) = 0;
		virtual void Clear() = 0;
		virtual IInterfaceBuilder* DeclareInterface(cstr name, int methodCount, IStructureBuilder& nullObject, IInterfaceBuilder* base) = 0;

		virtual IInterfaceBuilder& GetInterface(int index) = 0;

		virtual IArchetype* FindArchetype(cstr name) = 0;
		virtual IMacroBuilder* FindMacro(cstr name) = 0;
		virtual IFactoryBuilder* FindFactory(cstr name) = 0;
		virtual IInterfaceBuilder* FindInterface(cstr name) = 0;
		virtual IFunctionBuilder* FindFunction(cstr name) = 0;
		virtual IStructureBuilder* FindStructure(cstr name) = 0;

		virtual IFactory& RegisterFactory(cstr name, IFunctionBuilder& constructor, IInterfaceBuilder& interf, sexstring interfType) = 0;

		virtual IProgramObject& Object() = 0;
	};

	class CommonStructures;

	struct CallbackIds
	{
		ID_API_CALLBACK IdAllocate;
		ID_API_CALLBACK IdGetAllocSize;
		ID_API_CALLBACK IdAddRef;
		ID_API_CALLBACK IdReleaseRef;
		ID_API_CALLBACK IdUpdateRefsOnSourceAndTarget;
	};

	enum class EWarningLevel
	{
		Always,
		Never
	};

	ROCOCO_INTERFACE IProgramObject : public IPublicProgramObject
	{
		virtual IModuleBuilder & AddModule(cstr name) = 0;
		virtual IModuleBuilder& GetModule(int index) = 0;
		virtual INamespaceBuilder& GetRootNamespace() = 0;
		virtual IModuleBuilder& IntrinsicModule() = 0;
		virtual IStructureBuilder& AddIntrinsicStruct(cstr name, size_t sizeOfType, SexyVarType underlyingType, const IArchetype* archetype) = 0;
		virtual void ResolveNativeTypes(const void** pSrcErr) = 0;
		virtual bool ResolveDefinitions(const void** pSrcErr) = 0;
		virtual cstr RegisterSymbol(cstr text) = 0;
		virtual CommonStructures& Common() = 0;
		virtual void InitCommon() = 0;
		virtual const CallbackIds& GetCallbackIds() const = 0;
		virtual void ClearCustomAllocators() = 0;
		virtual IScriptObjectAllocator& GetDefaultObjectAllocator() = 0;
		virtual EWarningLevel GetWarningLevel() const = 0;
		virtual void SetWarningLevel(EWarningLevel level) = 0;
	};

	IProgramObject* CreateProgramObject_1_0_0_0(const ProgramInitParameters& pip, ILog& log);

	class CProgramObjectProxy
	{
	private:
		IProgramObject* instance;

	public:
		CProgramObjectProxy(const ProgramInitParameters& pip, ILog& log)
		{
			instance = CreateProgramObject_1_0_0_0(pip, log);
		}

		~CProgramObjectProxy()
		{
			instance->Free();
		}

		IProgramObject& operator()() { return *instance; }
	};

	void ValidateNamespaceString(cstr s, cstr name, cstr functionSymbol);

	// Recursive structure. Given a branch, its children are visited first and then it is evaluated
	ROCOCO_INTERFACE IBinaryExpression
	{
		// Evaluate and put the result/reference into the target register.
		virtual void EvaluateBranch(ICodeBuilder & builder, IProgramObject & object, int target) = 0;

	// Get the left half of the expression. If NULL, then the expression is atomic
	virtual IBinaryExpression* GetLeft() = 0;

	// Get the right half of the expression. If NULL, then the expression is not binary
	virtual IBinaryExpression* GetRight() = 0;
	};

	struct StackRecoveryData
	{
		int TotalDisplacement;
		int InstancePosStart;
		int InstancePosCount;
	};

	struct GlobalValue
	{
		size_t offset;
		SexyVarType type;
		VariantValue initialValue;
	};

	struct IGlobalEnumerator
	{
		virtual void operator()(cstr name, const GlobalValue& defaultValue) = 0;
	};

	inline ObjectStub* InterfaceToInstance(InterfacePointer i)
	{
		auto* p = ((uint8*)i) + (*i)->OffsetToInstance;
		auto* obj = (ObjectStub*)p;
		return obj;
	}

	ROCOCO_INTERFACE ICodeBuilder : public IFunctionCode
	{
	    virtual void Free() = 0;
		virtual IFunctionBuilder& Owner() = 0;

		virtual const bool NeedsParentsSF() const = 0;
		virtual void AddExpression(IBinaryExpression& tree) = 0;
		virtual void Begin() = 0;
		virtual void AddVariable(const NameString& name, const TypeString& type, void* userData) = 0;
		virtual void AddVariableRef(const NameString& name, const IStructure& type, void* userData) = 0;
		virtual void AddVariable(const NameString& name, const IStructure& type, void* userData) = 0;
		virtual void Append_GetAllocSize() = 0;

		/* decrement reference count to an object with interface or instance pointed to by D4. If it hits zero non null-objects will be freed */
		virtual void Append_DecRef() = 0;

		/* increment reference count to an object with interface or instance pointed to by D4. */
		virtual void Append_IncRef() = 0;

		virtual void Append_IncDerivativeRefs(cstr value) = 0;

		/* D4 = source, D5 = target
		   if source and target are different, decrement target refcount and increment source refcount */
		virtual void Append_UpdateRefsOnSourceAndTarget() = 0;

		virtual void AddDestructors(size_t startPosition, size_t endPosition) = 0;

		// Finds the program counter delta for the label position. Returns (size_t) -1 if no label has been identified to the end point of the current compilation state
		virtual size_t GetLabelPosition(cstr labelName) = 0;

		/* AddDynamicAllocateObject -> takes sizeof(obj) in D4 (int32),
		   returns pointer to object in D4 (vPtr)
		*/
		virtual void AddDynamicAllocateObject(const IStructure& structType, const IInterface& interface) = 0;
		virtual void AssignLiteral(const NameString& name, cstr literalValue) = 0;
		virtual void AssignPointer(const NameString& name, const void* ptr) = 0;
		virtual void AssignVariableToVariable(cstr source, cstr target, bool isConstructingTarget = false) = 0;
		virtual void AssignVariableToTemp(cstr source, int tempIndex, int memberOffsetCorrection = 0) = 0;
		virtual void AssignVariableRefToTemp(cstr source, int tempDepth, int offset = 0) = 0;
		virtual void AssignTempToVariable(int srcIndex, cstr target) = 0;
		virtual void AssignVariableAddressToTemp(cstr sourceVariable, int tempDepth) = 0;
		virtual void AssignVariableToGlobal(const GlobalValue& g, const MemberDef& def) = 0;
		virtual void AssignVariableFromGlobal(const GlobalValue& g, const MemberDef& def) = 0;
		virtual void AssignLiteralToGlobal(const GlobalValue& g, const VariantValue& value) = 0;
		virtual void BinaryOperatorAdd(int srcInvariantIndex, int trgInvariantIndex, SexyVarType type) = 0;
		virtual void BinaryOperatorSubtract(int srcInvariantIndex, int trgInvariantIndex, SexyVarType type) = 0;
		virtual void BinaryOperatorMultiply(int srcInvariantIndex, int trgInvariantIndex, SexyVarType type) = 0;
		virtual void BinaryOperatorDivide(int srcInvariantIndex, int trgInvariantIndex, SexyVarType type) = 0;
		virtual void AppendConditional(CONDITION condition, ICompileSection& thenSection, ICompileSection& elseSection) = 0;
		virtual void AppendWhileDo(ICompileSection& loopCriterion, CONDITION condition, ICompileSection& loopBody, ICompileSection& finalSection) = 0;
		virtual void AppendDoWhile(ICompileSection& loopBody, ICompileSection& loopCriterion, CONDITION condition) = 0;

		virtual void ArchiveRegister(int saveTempDepth, int restoreTempDepth, BITCOUNT bits, void* userData) = 0;
		virtual void AddArgVariable(cstr desc, const TypeString& type, void* userData) = 0;
		virtual void AddArgVariable(cstr desc, const IStructure& type, void* userData) = 0;

		virtual int SectionArgCount() const = 0;
		virtual int SectionArgCount(size_t index) const = 0;
		virtual void EnterSection() = 0;
		virtual void LeaveSection() = 0;
		virtual void End() = 0;

		virtual VM::IAssembler& Assembler() = 0;
		virtual IModuleBuilder& Module() = 0;

		virtual void EnableClosures(cstr targetVariable) = 0;

		virtual SexyVarType GetVarType(cstr name) const = 0;
		virtual bool TryGetVariableByName(OUT MemberDef& def, cstr name) const = 0;
		virtual const IStructure* GetVarStructure(cstr varName) const = 0;
		virtual int GetVariableCount() const = 0;
		virtual void GetVariableByIndex(OUT MemberDef& def, cstr& name, int index) const = 0;
		virtual int GetOffset(size_t variableIndex) const = 0;
		virtual void AssignClosureParentSFtoD6() = 0;

		virtual void MarkGoto(size_t gotoPosition, cstr labelName) = 0;
		virtual void MarkLabel(cstr labelName) = 0;

		virtual void PopControlFlowPoint() = 0;
		virtual void PushControlFlowPoint(const ControlFlowData& controlFlowData) = 0;
		virtual bool TryGetControlFlowPoint(OUT ControlFlowData& data) = 0;

		virtual void PopLastVariables(int count, bool expireVariables) = 0;
		virtual const StackRecoveryData& GetRequiredStackCorrection(size_t codeOffset) const = 0;
		virtual int GetDestructorFromInstancePos(int instancePosition) const = 0;
		virtual const IStructure& GetTypeFromInstancePos(int instancePosition) const = 0;

		virtual void NoteStackCorrection(int stackCorrection) = 0;
		virtual void NoteDestructorPosition(int instancePosition, const IStructure& type) = 0;

		virtual void AddSymbol(cstr text) = 0;
		virtual void MarkExpression(const void* sourceExpression) = 0;
		virtual void DeleteSymbols() = 0;

		virtual void PushVariable(const MemberDef& def) = 0;

		// If interfaceIndex is -1, will attempt to push a struct pointer
		virtual void PushVariableRef(cstr source, int interfaceIndex) = 0;

		virtual int GetThisOffset() const = 0;
		virtual void SetThisOffset(int offset) = 0;

		// Hack: allow variables to be defined with zero size. To be used in conjunction with references that refer to variables created at compile time
		// rather than at run-time. When the system sees a class instance named XXX and zero allocsize, it looks for _ref_XXX. The pseudo variable provides typeinfo.
		virtual void AddInterfaceVariable(const NameString& ns, const IStructure& st, void* userData) = 0;

		virtual void PreventMoreVariablesUntil(cstr label) = 0;

		virtual uint64 NextId() = 0;

		// Store a lambda variable name for later. Used by the compiler to store the names of temporary lambda references on the stack ahead of invocation
		virtual void PushLambdaVar(cstr variableName) = 0;

		// Retrieves lambda variable name and removes it from the lambda name stack. Names are thus removed in reverse order of the push operations
		virtual void PopLambdaVar(OUT char result[MAX_FQ_NAME_LEN]) = 0;
	};

	SEXYUTIL_API IStructureBuilder* FindMember(IStructureBuilder& s, cstr name);
	SEXYUTIL_API IInterfaceBuilder* GetInterface(IProgramObject& object, cstr fullyQualifiedName);
	SEXYUTIL_API IStructureBuilder* MatchStructure(ILog& logger, cstr type, IModuleBuilder& module);
	SEXYUTIL_API INamespaceBuilder* MatchNamespace(IModuleBuilder& module, cstr name);
} // Rococo::Compiler

#pragma warning(default : 4263)
#pragma warning(default : 4264)

#ifdef _WIN32
#  pragma pack(pop)
#endif

#endif // STC_H