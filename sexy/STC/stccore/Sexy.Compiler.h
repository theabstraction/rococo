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

#ifndef STC_H
# define STC_H

#ifndef SEXY_H
#error include "sexy.types.h" before including this file
#endif

#ifndef sexy_compiler_public_h
#error include "sexy.compiler.pubic.h"
#endif

# pragma pack(push,1)

// #define IS_COMPILER_DLL 1

#ifdef IS_COMPILER_DLL
# ifdef COMPILER_LIB
#  define COMPILER_API __declspec(dllexport)
# else
#  define COMPILER_API __declspec(dllimport)
# endif
#else
#  define COMPILER_API
#endif

namespace Sexy { namespace Compiler
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
	};

	ROCOCOAPI ICompileSection
	{
		virtual void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* controlFlowData = nullptr) = 0;
	};

	ROCOCOAPI IArgumentBuilder : public IArgument
	{
		virtual bool TryResolveArgument() = 0;
		virtual IFunctionBuilder& Parent() = 0;
	};

	ROCOCOAPI IFunctionBuilder : public IFunction
	{
		virtual IArgumentBuilder& AddInput(const NameString& name, const TypeString& type, void* userdata) = 0;
		virtual IArgumentBuilder& AddClosureInput(const NameString& name, const TypeString& type, void* userdata) = 0;
		virtual IArgumentBuilder& AddInput(const NameString& name, const IStructure& type, void* userdata) = 0;
		virtual IArgumentBuilder& AddInput(const NameString& name, const TypeString& genericType, const TypeString& genericArgType1, void* userdata) = 0;
		virtual IArgumentBuilder& AddInput(const NameString& name, const TypeString& genericType, const TypeString& genericArgType1, const TypeString& genericArgType2, void* userdata) = 0;
		virtual IArgumentBuilder& AddOutput(const NameString& name, const TypeString& type, void* userdata) = 0;
		virtual IArgumentBuilder& AddOutput(const NameString& name, const IStructure& type, void* userdata) = 0;
		virtual bool TryResolveArguments() = 0;		

		virtual ICodeBuilder& Builder() = 0;
		virtual IModuleBuilder& Module() = 0;
		virtual IFunctionBuilder* Parent() = 0;

		virtual IProgramObject& Object() = 0;

		virtual void SetType(const IStructure* type) = 0;
	};

	ROCOCOAPI IFunctionAliasBuilder : public IFunctionAlias
	{
		virtual IFunctionBuilder& GetFunction() = 0;
	};

	ROCOCOAPI IStructAliasBuilder : public IStructAlias
	{
		virtual IStructureBuilder& GetStructure() = 0;
	};

	ROCOCOAPI IFunctionEnumeratorBuilder : public IFunctionEnumerator
	{
		virtual IFunctionAliasBuilder& operator[](int index) = 0;
	};

	IFunctionBuilder* FindByName(IFunctionEnumeratorBuilder& e, csexstr publicName);
	
	ROCOCOAPI IMemberBuilder : public IMember
	{
		virtual IStructureBuilder* UnderlyingType() = 0;
		virtual IStructureBuilder* UnderlyingGenericArg1Type() = 0;
		virtual IStructureBuilder* UnderlyingGenericArg2Type() = 0;
	};
	
	ROCOCOAPI IStructureBuilder : public IStructure
	{
		virtual IModuleBuilder& Module() = 0;
		virtual void AddInterface(csexstr interfaceFullName) = 0;		
		virtual void AddMember(const NameString& name, const TypeString& type, csexstr genericArg1 = nullptr, csexstr genericArg2 = nullptr) = 0;
		virtual void AddPseudoMember(const NameString& name, const TypeString& type) = 0;
		virtual void Seal() = 0;
		virtual IMemberBuilder& GetMember(int index) = 0;
		virtual IInterfaceBuilder& GetInterface(int index) = 0;
		virtual void ExpandAllocSize(int minimumByteCount) = 0;
		virtual void Update() = 0;
		virtual void SetConstructor(const IFunction* cons) = 0;

		virtual IProgramObject& Object() = 0;
	};

	ROCOCOAPI IModuleBuilder : public IModule
	{
		virtual void ClearPrefixes() = 0;		
		virtual void Clear() = 0;
		virtual IStructure& DeclareClass(csexstr name, const StructurePrototype& prototype, const void* definition) = 0;
		virtual IFunctionBuilder& DeclareClosure(IFunctionBuilder& parent, bool mayUseParentSF, const void* definition) = 0;
		virtual IFunctionBuilder& DeclareFunction(const FunctionPrototype& prototype, const void* definition) = 0;	
		virtual IStructureBuilder& DeclareStructure(csexstr name, const StructurePrototype& prototype, const void* definition) = 0;
		virtual IFunctionBuilder* FindFunction(csexstr name) = 0;
		virtual IStructureBuilder* FindStructure(csexstr name) = 0;
		virtual IFunctionBuilder& GetFunction(int index) = 0;
		virtual IStructureBuilder& GetStructure(int index) = 0;
		virtual void UsePrefix(csexstr name) = 0;
		virtual INamespaceBuilder& GetPrefix(int index) = 0;
		virtual void IncVersion() = 0;

		virtual IProgramObject& Object() = 0;
	};

	ROCOCOAPI IInterfaceBuilder : public IInterface
	{
		virtual IAttributes& Attributes() = 0;
		virtual IStructureBuilder& NullObjectType() = 0;
		virtual void SetMethod(size_t index, csexstr name, size_t argCount, csexstr argNames[], const IStructure* types[], const IArchetype* archetypes[], const IStructure* genericArg1s[], const bool isOut[], const void* definition) = 0;
		virtual void PostCompile() = 0;	
		virtual void ExpandNullObjectAllocSize(int minimumByteCount) = 0;
	};

	ROCOCOAPI IFactoryBuilder : public IFactory
	{
		virtual void SetInline(IFunctionBuilder* f, IStructureBuilder* s) = 0; // Used in the compilation phase to set the inline implementation for the factory
		virtual IFunctionBuilder& Constructor() = 0;
		virtual IStructureBuilder* InlineClass() = 0; // if not NULL indicates the concrete class of the inline constructor
	};

	ROCOCOAPI IMacroBuilder : public IMacro
	{
		virtual IFunctionBuilder& Implementation() = 0;
	};

	ROCOCOAPI INamespaceBuilder : public INamespace
	{
		virtual INamespaceBuilder* Parent() = 0;
		virtual INamespaceBuilder& GetChild(size_t index) = 0;
		virtual INamespaceBuilder* FindSubspace(csexstr childName) = 0;
		virtual INamespaceBuilder& AddNamespace(csexstr childName, ADDNAMESPACEFLAGS flags) = 0;
		virtual IMacroBuilder* AddMacro(csexstr name, void* expression, IFunctionBuilder& f) = 0;
		virtual const IArchetype& AddArchetype(csexstr name, csexstr argNames[], const IStructure* stArray[], const IArchetype* archArray[], const IStructure* genericArg1s[], int numberOfOutputs, int numberOfInputs, const void* definition) = 0;
		virtual void Alias(IFunctionBuilder& f) = 0;
		virtual void Alias(csexstr publicName, IStructureBuilder& s) = 0;
		virtual void Alias(csexstr publicName, IFunctionBuilder& f) = 0;
		virtual void Clear() = 0;
		virtual IInterfaceBuilder* DeclareInterface(csexstr name, int methodCount, IStructureBuilder& nullObject, IInterfaceBuilder* base) = 0;
		
		virtual IInterfaceBuilder& GetInterface(int index) = 0;

		virtual IArchetype* FindArchetype(csexstr name) = 0;
		virtual IMacroBuilder* FindMacro(csexstr name) = 0;
		virtual IFactoryBuilder* FindFactory(csexstr name) = 0;
		virtual IInterfaceBuilder* FindInterface(csexstr name) = 0;
		virtual IFunctionBuilder* FindFunction(csexstr name) = 0;
		virtual IStructureBuilder* FindStructure(csexstr name) = 0;

		virtual IFactory& RegisterFactory(csexstr name, IFunctionBuilder& constructor, IInterfaceBuilder& interf, sexstring interfType) = 0;

		virtual IProgramObject& Object() = 0;
	};

	class CommonStructures;

	ROCOCOAPI IProgramObject : public IPublicProgramObject
	{
		virtual IModuleBuilder& AddModule(csexstr name) = 0;		
		virtual IModuleBuilder& GetModule(int index) = 0;
		virtual INamespaceBuilder& GetRootNamespace() = 0;
		virtual IModuleBuilder& IntrinsicModule() = 0;
		virtual IStructureBuilder& AddIntrinsicStruct(csexstr name, size_t sizeOfType, VARTYPE underlyingType, const IArchetype* archetype) = 0;
		virtual void ResolveNativeTypes() = 0;		
		virtual bool ResolveDefinitions() = 0;				
		virtual csexstr RegisterSymbol(csexstr text) = 0;		
		virtual CommonStructures& Common() = 0;
		virtual void InitCommon() = 0;
	};

	COMPILER_API IProgramObject* CreateProgramObject_1_0_0_0(const ProgramInitParameters& pip, ILog& log);

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

	COMPILER_API void ValidateNamespaceString(csexstr s, csexstr name, csexstr functionSymbol);

	// Recursive structure. Given a branch, its children are visited first and then it is evaluated
	ROCOCOAPI IBinaryExpression
	{
		// Evaluate and put the result/reference into the target register.
		virtual void EvaluateBranch(ICodeBuilder& builder, IProgramObject& object, int target) = 0;

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
		VARTYPE type;
		VariantValue initialValue;
	};

	struct IGlobalEnumerator
	{
		virtual void operator()(csexstr name, const GlobalValue& defaultValue) = 0;
	};

	ROCOCOAPI ICodeBuilder : public IFunctionCode
	{
		virtual IFunctionBuilder& Owner() = 0;
		
		virtual const bool NeedsParentsSF() const = 0;
		virtual void AddExpression(IBinaryExpression& tree) = 0;
		virtual void Begin() = 0;
		virtual void AddVariable(const NameString& name, const TypeString& type, void* userData) = 0;
		virtual void AddVariableRef(const NameString& name, const IStructure& type, void* userData) = 0;
		virtual void AddVariable(const NameString& name, const IStructure& type, void* userData) = 0;
		virtual void AddCatchVariable(csexstr name, void* userData) = 0;
		virtual void AssignLiteral(const NameString& name, csexstr literalValue) = 0;
		virtual void AssignPointer(const NameString& name, const void* ptr) = 0;
		virtual void AssignVariableToVariable(csexstr source, csexstr target) = 0;
		virtual void AssignVariableToTemp(csexstr source, int tempIndex, int memberOffsetCorrection = 0) = 0;
		virtual void AssignVariableRefToTemp(csexstr source, int tempDepth, int offset = 0) = 0;	
		virtual void AssignTempToVariable(int srcIndex, csexstr target) = 0;
		virtual void AssignVariableToGlobal(const GlobalValue& g, const MemberDef& def) = 0;
		virtual void AssignVariableFromGlobal(const GlobalValue& g, const MemberDef& def) = 0;
		virtual void AssignLiteralToGlobal(const GlobalValue& g, const VariantValue& value) = 0;
		virtual void Negate(int srcInvariantIndex, VARTYPE varType) = 0;
		virtual void BinaryOperatorAdd(int srcInvariantIndex, int trgInvariantIndex, VARTYPE type) = 0;
		virtual void BinaryOperatorSubtract(int srcInvariantIndex, int trgInvariantIndex, VARTYPE type) = 0;
		virtual void BinaryOperatorMultiply(int srcInvariantIndex, int trgInvariantIndex, VARTYPE type) = 0;
		virtual void BinaryOperatorDivide(int srcInvariantIndex, int trgInvariantIndex, VARTYPE type) = 0;
		virtual void AppendConditional(CONDITION condition, ICompileSection& thenSection, ICompileSection& elseSection) = 0;
		virtual void AppendWhileDo(ICompileSection& loopCriterion, CONDITION condition, ICompileSection& loopBody) = 0;
		virtual void AppendDoWhile(ICompileSection& loopBody, ICompileSection& loopCriterion, CONDITION condition) = 0;

		virtual void Append_InitializeVirtualTable(csexstr className) = 0;
		virtual void Append_InitializeVirtualTable(csexstr instanceName, const IStructure& classType) = 0;

		virtual void ArchiveRegister(int saveTempDepth, int restoreTempDepth, BITCOUNT bits, void* userData) = 0;
		virtual void AddArgVariable(csexstr desc, const TypeString& type, void* userData) = 0;
		virtual void AddArgVariable(csexstr desc, const IStructure& type, void* userData) = 0;

		virtual int SectionArgCount() const = 0;
		virtual void EnterSection() = 0;
		virtual void LeaveSection() = 0;
		virtual void End() = 0;
		virtual void Free() = 0;
		
		virtual VM::IAssembler& Assembler() = 0;
		virtual IModuleBuilder& Module() = 0;

		virtual void EnableClosures(csexstr targetVariable) = 0;

		virtual VARTYPE GetVarType(csexstr name) const = 0;
		virtual bool TryGetVariableByName(OUT MemberDef& def, csexstr name) const = 0;
		virtual const IStructure* GetVarStructure(csexstr varName) const = 0;
		virtual int GetVariableCount() const = 0;
		virtual void GetVariableByIndex(OUT MemberDef& def, csexstr& name, int index) const = 0;
		virtual int GetOffset(size_t variableIndex) const = 0;
		virtual void AssignClosureParentSF() = 0;

		virtual void PopControlFlowPoint() = 0;
		virtual void PushControlFlowPoint(const ControlFlowData& controlFlowData) = 0;
		virtual bool TryGetControlFlowPoint(OUT ControlFlowData& data) = 0;

		virtual void PopLastVariables(int count) = 0;
		virtual const StackRecoveryData& GetRequiredStackCorrection(size_t codeOffset) const = 0;
		virtual int GetDestructorFromInstancePos(int instancePosition) const = 0;
		virtual const IStructure& GetTypeFromInstancePos(int instancePosition) const = 0;

		virtual void NoteStackCorrection(int stackCorrection) = 0;
		virtual void NoteDestructorPosition(int instancePosition, const IStructure& type) = 0;	

		virtual void AddSymbol(csexstr text) = 0;
		virtual void MarkExpression(const void* sourceExpression) = 0;
		virtual void DeleteSymbols() = 0;

		virtual void PushVariable(const MemberDef& def) = 0;
		virtual void PushVariableRef(csexstr source, int interfaceIndex) = 0;

		virtual int GetThisOffset() const = 0;
		virtual void SetThisOffset(int offset) = 0;

		// Hack: allow variables to be defined with zero size. To be used in conjunction with references that refer to variables created at compile time
		// rather than at run-time. When the system sees a class instance named XXX and zero allocsize, it looks for _ref_XXX. The pseudo variable provides typeinfo.
		virtual void AddPseudoVariable(const NameString& ns, const IStructure& st) = 0;
	};

	enum { MAX_NAMESPACE_LEN = 256 };

	IStructureBuilder* FindMember(IStructureBuilder& s, csexstr name);
	IInterfaceBuilder* GetInterface(IProgramObject& object, csexstr fullyQualifiedName);
	IStructureBuilder* MatchStructure(ILog& logger, csexstr type, IModuleBuilder& module);
	INamespaceBuilder* MatchNamespace(IModuleBuilder& module, csexstr name);
}} // Sexy::Compiler

# ifdef WIN32
#  pragma pack(pop)
# endif

#endif // STC_H