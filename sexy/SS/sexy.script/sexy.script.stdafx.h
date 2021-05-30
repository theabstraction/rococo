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

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define THIS_IS_THE_SEXY_CORE_LIBRARY
#define IS_SCRIPT_DLL 1

#include <sexy.types.h>
#include <sexy.vm.h>
#include <sexy.vm.cpu.h>
#include <sexy.script.h>
#define ROCOCO_USE_SAFE_V_FORMAT
#include "sexy.strings.h"
#include "sexy.stdstrings.h"
#include "sexy.compiler.h"
#include <rococo.hashtable.h>
#include <vector>

namespace Rococo
{
    [[nodiscard]] uint32 FastHash(cstr text);

   namespace Compiler
   {
      class STCException;
      struct IStructure;
      struct ICodeBuilder;
      struct INamespaceBuilder;
      struct IProgramObject;

      cstr GetTypeName(const IStructure& s);
   }

   namespace Sex
   {
      using namespace Rococo::Compiler;

      void AssertQualifiedIdentifier(cr_sex e);
      void AssertTypeIdentifier(cr_sex e);
      void AssertTypeIdentifier(cr_sex src, cstr name);
      void AssertLocalIdentifier(cr_sex e);

      void Throw(cr_sex e, const fstring& f);
      void ThrowTypeMismatch(cr_sex s, const IStructure& a, const IStructure& b, cstr extra);

      void AssertAtomicMatch(cr_sex s, cstr value);
      void ThrowNamespaceConflict(cr_sex s, const INamespace& n1, const INamespace& n2, cstr type, cstr token);
      void ThrowTokenNotFound(cr_sex s, cstr item, cstr repository, cstr type);
      INamespace& AssertGetNamespace(IProgramObject& object, cr_sex s, cstr fullName);
   }

   namespace Script
   {
      using namespace Rococo::Compiler;
      using namespace Rococo::Sex;

      extern const char* const THIS_POINTER_TOKEN;

      struct IScriptSystem;

      typedef stringmap<cstr> TMapMethodToMember;

      class CScript;

      void AddMember(IStructureBuilder& s, cr_sex field);
      IInterfaceBuilder* MatchInterface(cr_sex typeExpr, IModuleBuilder& module);

	  struct IStringPool
	  {
		  virtual void Free() = 0;
		  virtual AllocatorBinding* GetBinding() = 0;
		  virtual void SetStringBuilderType(const IStructure* typeFastStringBuilder) = 0;
		  virtual const IStructure* FastStringBuilderType() const = 0;
	  };

	  IStringPool* NewStringPool();

      class CCompileEnvironment
      {
      private:
         const IStructure* arrayStruct;
         const IStructure* listStruct;
         const IStructure* mapStruct;
         const TMapMethodToMember& methodMap;

      public: // member variables made public to enhance speed in debug mode
         CScript& Script;
         ICodeBuilder& Builder;
         INamespaceBuilder& RootNS;
         IScriptSystem& SS;
         IProgramObject& Object;
		 const IFactory* factory;

         CCompileEnvironment(CScript& script, ICodeBuilder& builder, const IFactory* factory = nullptr);

         const IStructure& StructArray();
         const IStructure& StructList();
         const IStructure& StructMap();

         cstr MapMethodToMember(cstr method)
         {
            auto i = methodMap.find(method);
            return i == methodMap.end() ? NULL : i->second;
         }
      };

      void InitClassMembers(CCompileEnvironment& ce, cstr id);
      void StreamSTCEX(StringBuilder& sb, const Compiler::STCException& ex);
      bool TryCompileFunctionCallAndReturnValue(CCompileEnvironment& ce, cr_sex s, VARTYPE type, const IStructure* derivedType, const IArchetype* returnArchetype);

      class CScripts;

      struct CBindFnDefToExpression
      {
         const ISExpression* FnDef;
         IFunctionBuilder* Fn;
      };
      typedef stringmap<CBindFnDefToExpression> TFunctionDefinitions;

      struct CBindStructDefToExpression
      {
         const ISExpression* StructDef;
         IStructureBuilder* Struct;
      };
      typedef std::vector<CBindStructDefToExpression> TStructureDefinitions;

      struct CClosureDef
      {
         const ISExpression* ClosureExpr;
         IFunctionBuilder* Closure;
         CClosureDef(cr_sex closureExpr, IFunctionBuilder& closure) : ClosureExpr(&closureExpr), Closure(&closure) {}
      };

      typedef std::vector<CClosureDef> TClosures;

      struct CNullDef
      {
         IInterfaceBuilder* Interface;
         const ISExpression* Source;
         IStructureBuilder* NullObject;
         INamespaceBuilder* NS;
      };

      typedef std::vector<CNullDef> TNullObjectDefs;
      typedef std::vector<CStringConstant*> TStringConstants;
      typedef std::vector<IMacroBuilder*> TMacros;

      struct BuilderAndNameKey
      {
         ICodeBuilder* Builder;
         HString Name;
      };

      struct hashBuilderAndNameKey
      {
         size_t operator()(const BuilderAndNameKey& s) const
         {
            return Rococo::FastHash(s.Name) ^ (size_t)s.Builder;
         }
      };

      inline bool operator == (const BuilderAndNameKey& a, const BuilderAndNameKey& b)
      {
         return a.Builder == b.Builder && a.Name == b.Name;
      }

      struct ArrayDef
      {
         cr_sex SexDef;
         const IStructure& ElementType;

         ArrayDef(cr_sex sexDef, const IStructure& elementType) : SexDef(sexDef), ElementType(elementType) {}
      };

      struct ListDef
      {
         cr_sex SexDef;
         const IStructure& ElementType;

         ListDef(cr_sex sexDef, const IStructure& elementType) : SexDef(sexDef), ElementType(elementType) {}
      };

      struct MapDef
      {
         cr_sex SexDef;
         const IStructure& KeyType;
         const IStructure& ValueType;

         MapDef(cr_sex sexDef, const IStructure& keyType, const IStructure& valueType) : SexDef(sexDef), KeyType(keyType), ValueType(valueType) {}
         MapDef(const MapDef& s) : SexDef(s.SexDef), KeyType(s.KeyType), ValueType(s.ValueType) {}
      };

      struct NodeDef
      {
         cr_sex SexDef;
         const IStructure& ElementType;

         NodeDef(cr_sex sexDef, const IStructure& elementType) : SexDef(sexDef), ElementType(elementType) {}
      };

      struct MapNodeDef
      {
         cr_sex SexDef;
         MapDef mapdef;
         stdstring mapName;
         MapNodeDef(cr_sex _sexDef, const MapDef& _mapdef, cstr _mapName) : SexDef(_sexDef), mapdef(_mapdef), mapName(_mapName) {}
      };


      typedef std::unordered_map<BuilderAndNameKey, ArrayDef, hashBuilderAndNameKey> TMapNameToArrayDef;
      typedef std::unordered_map<BuilderAndNameKey, ListDef, hashBuilderAndNameKey> TMapNameToListDef;
      typedef std::unordered_map<BuilderAndNameKey, NodeDef, hashBuilderAndNameKey> TMapNameToNodeDef;
      typedef std::unordered_map<BuilderAndNameKey, MapDef, hashBuilderAndNameKey> TMapNameToMapDef;
      typedef std::unordered_map<BuilderAndNameKey, MapNodeDef, hashBuilderAndNameKey> TMapNameToMapNodeDef;
      typedef stringmap<GlobalValue> TGlobalVariables;

	  struct TransformData
	  {
		  IExpressionTransform* transform;
	  };
	  typedef std::unordered_map<const ISExpression*, TransformData> TTransformMap;
      typedef std::vector<const ISExpression*> TExceptionBlocks;

      struct CBindNSExpressionToModule
      {
         const ISExpression* E;
         CScript* Module;
         INamespace* NS;
         bool isDefault = false;
      };

      inline bool operator < (const CBindNSExpressionToModule& a, const CBindNSExpressionToModule& b)
      {
         return a.E->String()->Length < b.E->String()->Length;
      }

      typedef std::vector<CBindNSExpressionToModule> TNamespaceDefinitions;

      class CScript
      {
      private:
         ISParserTree& tree;
         IProgramObject& programObject;
         IModuleBuilder& module;
         bool isDirty;
         CScripts& scripts;
         TFunctionDefinitions localFunctions;
         TStructureDefinitions localStructures;
         TClosures closureJobs;
         TNullObjectDefs nullDefs;
         TStringConstants stringConstants;
         TMacros macros;

         TGlobalVariables globalVariables;
         TExceptionBlocks exceptionBlocks;
         cstr contextName;

         TMapNameToArrayDef mapNameToArrayDef;
         TMapNameToListDef mapNameToListDef;
         TMapNameToNodeDef mapNameToNodeDef;
         TMapNameToMapDef mapNameToMapDef;
         TMapNameToMapNodeDef mapNameToMapNodeDef;
		 TTransformMap mapExpressionToTransform;

      public:
         CScript(ISParserTree& _tree, IProgramObject& _programObject, CScripts& _scripts);
         GlobalValue* GetGlobalValue(cstr name);
         void EnumerateGlobals(IGlobalEnumerator& cb);
         const ArrayDef* GetElementTypeForArrayVariable(ICodeBuilder& builder, cstr arrayName);
         const ListDef* GetListDef(ICodeBuilder& builder, cstr listName);
         const NodeDef* GetNodeDef(ICodeBuilder& builder, cstr listName);
         const MapDef* GetMapDef(ICodeBuilder& builder, cstr listName);
         const MapNodeDef* GetMapNodeDef(ICodeBuilder& builder, cstr name);
         void AddArrayDef(ICodeBuilder& builder, cstr arrayName, const IStructure& elementType, cr_sex s);
         void AddListDef(ICodeBuilder& builder, cstr name, const IStructure& elementType, cr_sex s);
         void AddMapDef(ICodeBuilder& builder, cstr name, const IStructure& keyType, const IStructure& valueType, cr_sex s);
         void AddMapNodeDef(ICodeBuilder& builder, const MapDef& mapDef, cstr mapName, cstr nodeName, cr_sex s);
         void AddNodeDef(ICodeBuilder& builder, cstr nodeName, const IStructure& elementType, cr_sex s);
		 ISExpressionBuilder* CreateMacroTransform(cr_sex src);
		 const ISExpression* GetTransform(cr_sex src);

         const bool IsIStringInlined() const;

         IProgramObject& Object() { return programObject; }
         IScriptSystem& System();

         CStringConstant* CreateStringConstant(int length, cstr pointer, const ISExpression* srcExpression);

         void RegisterContext(cstr _name) { contextName = _name; }
         cstr GetContext() const { return contextName; }

         const ISExpression* GetTryCatchExpression() const
         {
            return exceptionBlocks.empty() ? NULL : exceptionBlocks.back();
         }

         void PushTryCatchBlock(cr_sex s)
         {
            exceptionBlocks.push_back(&s);
         }

         void PopTryCatchBlock()
         {
            exceptionBlocks.pop_back();
         }

         void AddCatchHandler(CScript& script, ID_BYTECODE id, size_t start, size_t end, size_t handlerOffset);

         ~CScript();

         IModuleBuilder& ProgramModule() { return module; }
         TFunctionDefinitions& LocalFunctions() { return localFunctions; }
         TNullObjectDefs& NullObjects() { return nullDefs; }
         const ISParserTree& Tree() const { return tree; }
         ISParserTree& Tree() { return tree; }

         void Clear();
         void CompileNullObjects();
         void CompileNextClosures();
         void CompileLocalFunctions();
         void CompileJITStubs();
         void AppendCompiledNamespaces(TNamespaceDefinitions& nsDefs);
         void AddEnumeratedVirtualMethod(IN cr_sex virtualMethodExpr, IN cstr methodName, REF IInterfaceBuilder& inter, IN size_t vmIndex);
         void AddVirtualMethod(IN cr_sex virtualMethodExpr, REF IInterfaceBuilder& inter, IN size_t vmIndex);
         void AddVirtualMethod(IN const IArchetype& archetype, REF IInterfaceBuilder& inter, IN size_t vmIndex, cr_sex def);
         void ComputeGlobal(cr_sex globalDef, int& globalBaseIndex);
         void ComputeGlobals(int& globalBaseIndex);
         void ComputeInterfaces();
         void ComputeInterfacePrototypes();
         void ComputePrefixes();
         void ComputeStructureNames();
         void ComputeFunctionNames();
         void ComputeStructureFields();
         void ComputeFunctionArgs();
         void CompileFactoryJITStubs();
         void CompileMacroJITStubs();
         void ValidateTopLevel();
         void ComputeArchetypes();
         void ComputeArchetypeNames();
         void AddArchetype(cr_sex e);
         void AddArchetypeName(cr_sex e);
         void PostClosure(cr_sex s, IFunctionBuilder& closure);
         void AddInterfacePrototype(cr_sex s, bool isInterfaceDefinedFromClassMethods);
         void ValidateConcreteClasses();
         void ValidateConstructors();
         void DeclareFactory(cr_sex factoryDef);
         void DeclareMacros();
         IFunctionBuilder& GetNullFunction(const IArchetype& archetype);
      };

      typedef std::unordered_map<void*, void*> TAllocationMap;

      void GetAtomicValue(CCompileEnvironment& ce, cr_sex parent, cstr id, VARTYPE type);
	  void AppendDeconstructTailVariables(CCompileEnvironment& ce, cr_sex sequence, bool expire, int tailCount);
      void AppendDeconstruct(CCompileEnvironment& ce, cr_sex sequence, bool expireVariables);
      void AddInterfaceVariable(CCompileEnvironment& ce, const NameString& ns, const IStructure& ts);
      void AddVariableAndSymbol(CCompileEnvironment& ce, cstr type, cstr name);
      void ValidateUnusedVariable(cr_sex identifierExpr, ICodeBuilder& builder);
      void AssertGetVariable(OUT MemberDef& def, cstr name, CCompileEnvironment& ce, cr_sex exceptionSource);
      void CompileAsPopOutFromArray(CCompileEnvironment& ce, cr_sex s, cstr instanceName, VARTYPE requiredType);
      void CompileArraySet(CCompileEnvironment& ce, cr_sex s);
      void CompileGetArrayElement(CCompileEnvironment& ce, cr_sex s, cstr instanceName, VARTYPE varType, const IStructure* structType);
      void CompileGetArraySubelement(CCompileEnvironment& ce, cr_sex indexExpr, cr_sex subItemName, cstr instanceName, VARTYPE type, const IStructure* structType);

      bool TryCompileAsArrayCall(CCompileEnvironment& ce, cr_sex s, cstr instanceName, cstr methodName);
      bool TryCompileAsListCall(CCompileEnvironment& ce, cr_sex s, cstr instanceName, cstr methodName);
      bool TryCompileAsNodeCall(CCompileEnvironment& ce, cr_sex s, cstr instanceName, cstr methodName);
      bool TryCompileAsMapCall(CCompileEnvironment& ce, cr_sex s, cstr mapName, cstr methodName);
      bool TryCompileAsMapNodeCall(CCompileEnvironment& ce, cr_sex s, cstr name, cstr methodName);
      bool TryCompileAsInlineMapAndReturnValue(CCompileEnvironment& ce, cr_sex s, cstr instance, cstr methodName, VARTYPE returnType, const IStructure& instanceStruct, OUT VARTYPE& outputType);

      void ConstructMemberByRef(CCompileEnvironment& ce, cr_sex args, int tempDepth, const IStructure& type, int offset);
      cstr GetTypeName(VARTYPE type);

      VARTYPE GetAtomicValueAnyNumeric(CCompileEnvironment& ce, cr_sex parent, cstr id, int tempdepth);
      void AssignVariableToVariable(CCompileEnvironment& ce, cr_sex exceptionSource, cstr lhs, cstr rhs);
      CStringConstant* CreateStringConstant(CScript& script, int length, cstr s, const ISExpression* srcExpression);
      IFunctionBuilder& DeclareFunction(IModuleBuilder& module, cr_sex source, FunctionPrototype& prototype);
      cstr GetContext(const CScript& script);
      const ISExpression* GetTryCatchExpression(CScript& script);
      bool TryCompileBooleanExpression(CCompileEnvironment& ce, cr_sex s, bool isExpected, bool& negate);
      void CompileExpressionSequence(CCompileEnvironment& ce, int start, int end, cr_sex sequence);
      void CompileExpression(CCompileEnvironment& ce, cr_sex s);
      void ValidateArchetypeMatchesArchetype(cr_sex s, const IArchetype& f, const IArchetype& archetype, cstr source);
      int GetIndexOf(int start, cr_sex s, cstr text);
      void CompileExceptionBlock(CCompileEnvironment& ce, cr_sex s);
      void CompileThrow(CCompileEnvironment& ce, cr_sex s);
      void CompileNextClosures(CScript& script);
      void GetVariableByName(ICodeBuilder& builder, OUT MemberDef& def, cstr name, cr_sex src);
      void AddCatchHandler(CScript& script, ID_BYTECODE id, size_t start, size_t end);
      IModuleBuilder& GetModule(CScript& script);
      void AppendVirtualCallAssembly(cstr instanceName, int interfaceIndex, int methodIndex, ICodeBuilder& builder, const IInterface& interf, cr_sex s, cstr localName);
      int GetFirstOutputOffset(ICodeBuilder& builder);
      IModule& GetSysTypeMemoModule(CScript& script);
      INamespaceBuilder& GetNamespaceByFQN(CCompileEnvironment& ce, cstr ns, cr_sex s);
      void MarkStackRollback(CCompileEnvironment& ce, cr_sex invokeExpression);
      int PushInput(CCompileEnvironment& ce, cr_sex s, int index, const IStructure& inputStruct, const IArchetype* archetype, cstr inputName, const IStructure* genericArg1);
      const IArgument& GetInput(cr_sex s, IFunction& f, int index);
      const IArchetype* GetArchetype(cr_sex s, IFunction& f, int index);
      void PushVariableRef(cr_sex s, ICodeBuilder& builder, const MemberDef& def, cstr name, int interfaceIndex);
      int GetCommonInterfaceIndex(const IStructure& object, const IStructure& argType);

      IFunctionBuilder& MustMatchFunction(IModuleBuilder& module, cr_sex s, cstr name);
      IInterfaceBuilder* MatchInterface(cr_sex typeExpr, IModuleBuilder& module);
      IStructureBuilder* MatchStructure(cr_sex typeExpr, IModuleBuilder& module);
      IFunctionBuilder* MatchFunction(cr_sex nameExpr, IModuleBuilder& module);

      bool TryCompileAssignArchetype(CCompileEnvironment& ce, cr_sex s, const IStructure& elementType, bool allowClosures);
      bool TryCompileArithmeticExpression(CCompileEnvironment& ce, cr_sex s, bool expected, VARTYPE type);

      void CompileFunctionCallAndReturnValue(CCompileEnvironment& ce, cr_sex s, IFunction& callee, VARTYPE returnType, const IArchetype* returnArchetype, const IStructure* returnTypeStruct);
      bool TryCompileMacroInvocation(CCompileEnvironment& ce, cr_sex s, sexstring token);
      IProgramObject& GetProgramObject(CScript& script);
      void CompileJITStub(IFunctionBuilder& f, cr_sex fdef, CScript& script, IScriptSystem& ss);
      void CompileJITStub(IFactoryBuilder* f, cr_sex fdef, CScript& script, IScriptSystem& ss);
      void CompileJITStub(IMacroBuilder* m, CScript& script, IScriptSystem& ss);

      IScriptSystem& GetSystem(CScript& script);
      IScriptSystem& GetSystem(CScripts& scripts);

      IFunctionBuilder& GetNullFunction(CScript& script, const IArchetype& archetype);

      GlobalValue* GetGlobalValue(CScript& script, cstr buffer);

	  struct ArrayCallbacks
	  {
          ID_API_CALLBACK ArrayAssign;
		  ID_API_CALLBACK ArrayClear;
		  ID_API_CALLBACK ArrayPushAndGetRef;
		  ID_API_CALLBACK ArrayPush32;
		  ID_API_CALLBACK ArrayPush64;
		  ID_API_CALLBACK ArrayPushInterface;
		  ID_API_CALLBACK ArrayPushByRef;
		  ID_API_CALLBACK ArrayGet32;
		  ID_API_CALLBACK ArrayGet64;
		  ID_API_CALLBACK ArrayGetMember32;
		  ID_API_CALLBACK ArrayGetMember64;
		  ID_API_CALLBACK ArrayGetByRef;
		  ID_API_CALLBACK ArraySet32;
		  ID_API_CALLBACK ArraySet64;
		  ID_API_CALLBACK ArraySetByRef;
		  ID_API_CALLBACK ArrayInit;
          ID_API_CALLBACK ArrayDelete;
		  ID_API_CALLBACK ArrayPop;
		  ID_API_CALLBACK ArrayPopOut32;
		  ID_API_CALLBACK ArrayPopOut64;
		  ID_API_CALLBACK ArrayLock;
		  ID_API_CALLBACK ArrayUnlock;
		  ID_API_CALLBACK ArrayGetRefUnchecked;
		  ID_API_CALLBACK ArrayDestructElements;
		  ID_API_CALLBACK ArrayGetInterfaceUnchecked;
          ID_API_CALLBACK ArrayGetLength;
          ID_API_CALLBACK ArrayGetLastIndex; // D13 points to array, D11 gives last index (-1 for empty arrays)
          ID_API_CALLBACK ArrayReturnLength;
          ID_API_CALLBACK ArrayReturnCapacity;
	  };

      struct ListCallbacks
      {
         ID_API_CALLBACK ListInit;
         ID_API_CALLBACK ListAppend;
         ID_API_CALLBACK ListAppendAndGetRef;
         ID_API_CALLBACK ListAppend32;
         ID_API_CALLBACK ListAppend64;
		 ID_API_CALLBACK ListAppendInterface;
         ID_API_CALLBACK ListClear;
         ID_API_CALLBACK ListPrepend;
         ID_API_CALLBACK ListPrependAndGetRef;
         ID_API_CALLBACK ListPrepend32;
         ID_API_CALLBACK ListPrepend64;
		 ID_API_CALLBACK ListPrependInterface;
         ID_API_CALLBACK ListGetTail;
         ID_API_CALLBACK ListGetHead;
         ID_API_CALLBACK NodeGet32;
         ID_API_CALLBACK NodeGet64;
		 ID_API_CALLBACK NodeGetInterface;
         ID_API_CALLBACK NodeGetElementRef;
         ID_API_CALLBACK NodeNext;
         ID_API_CALLBACK NodePrevious;
         ID_API_CALLBACK NodeAppend;
		 ID_API_CALLBACK NodeAppendInterface;
         ID_API_CALLBACK NodeAppend32;
         ID_API_CALLBACK NodeAppend64;
         ID_API_CALLBACK NodePrepend;
		 ID_API_CALLBACK NodePrependInterface;
         ID_API_CALLBACK NodePrepend32;
         ID_API_CALLBACK NodePrepend64;
         ID_API_CALLBACK NodePop;
         ID_API_CALLBACK NodeEnumNext;
         ID_API_CALLBACK NodeHasNext;
         ID_API_CALLBACK NodeHasPrevious;
         ID_API_CALLBACK NodeReleaseRef;
		 ID_API_CALLBACK NodeGoNext;
		 ID_API_CALLBACK NodeGoPrevious;
      };

      struct MapCallbacks
      {
         ID_API_CALLBACK MapInit;
         ID_API_CALLBACK MapInsert32;
         ID_API_CALLBACK MapInsert64;
         ID_API_CALLBACK MapInsertValueByRef;
		 ID_API_CALLBACK MapInsertInterface;
         ID_API_CALLBACK MapInsertAndGetRef;
         ID_API_CALLBACK MapClear;
         ID_API_CALLBACK MapTryGet;
         ID_API_CALLBACK MapNodeGet32;
         ID_API_CALLBACK MapNodeGet64;
         ID_API_CALLBACK MapNodeGetRef;
         ID_API_CALLBACK MapNodePop;
         ID_API_CALLBACK MapGetHead;
         ID_API_CALLBACK NodeEnumNext;
         ID_API_CALLBACK MapNodeReleaseRef;
      };

      const ArrayCallbacks& GetArrayCallbacks(CCompileEnvironment& ce);
      const ListCallbacks& GetListCallbacks(CCompileEnvironment& ce);
      const MapCallbacks& GetMapCallbacks(CCompileEnvironment& ce);

      const IStructure& GetElementTypeForArrayVariable(CCompileEnvironment& ce, cr_sex src, cstr arrayName);
      const IStructure& GetListDef(CCompileEnvironment& ce, cr_sex src, cstr name);
      const MapDef GetMapDef(CCompileEnvironment& ce, cr_sex src, cstr name);
      const IStructure& GetNodeDef(CCompileEnvironment& ce, cr_sex src, cstr name);
      const MapNodeDef& GetMapNodeDef(CCompileEnvironment& ce, cr_sex src, cstr name);

      void AddArrayDef(CScript& script, ICodeBuilder& builder, cstr arrayName, const IStructure& elementType, cr_sex def);
      void AddListDef(CScript& script, ICodeBuilder& builder, cstr listName, const IStructure& elementType, cr_sex def);
      void AddMapDef(CScript& script, ICodeBuilder& builder, cstr name, const IStructure& keyType, const IStructure& valueType, cr_sex def);
      void AddNodeDef(CScript& script, ICodeBuilder& builder, cstr name, const IStructure& elementType, cr_sex s);
      void AddMapNodeDef(CScript& script, ICodeBuilder& builder, const MapDef& def, cstr nodeName, cstr mapName, cr_sex s);
      bool TryCompileClosureDef(CCompileEnvironment& ce, cr_sex s, const IArchetype& closureArchetype, bool mayUseParentSF);

      bool IsIStringInlined(CScript& script);
      bool IsIStringInlined(CScripts& scripts);
      void AddSymbol(ICodeBuilder& builder, cstr format, ...);

      void AddInterfaceVariable(CCompileEnvironment& ce, const NameString& ns, const IStructure& ts);
      void AddVariable(CCompileEnvironment& ce, const NameString& ns, const IStructure& typeStruct);
      void AddArgVariable(cstr desc, CCompileEnvironment& ce, const TypeString& type);
      void AddArgVariable(cstr desc, CCompileEnvironment& ce, const IStructure& type);
      void AddVariable(CCompileEnvironment& ce, const NameString& ns, const TypeString& ts);
      int PushInputs(CCompileEnvironment& ce, cr_sex s, const IArchetype& callee, bool isImplicitInput, int firstArgIndex);
      int CompileInstancePointerArg(CCompileEnvironment& ce, cstr classInstance);
      void AppendFunctionCallAssembly(CCompileEnvironment& ce, const IFunction& callee);
      void RepairStack(CCompileEnvironment& ce, cr_sex s, const IArchetype& callee, int extraArgs = 0);
      void CompileConstructFromFactory(CCompileEnvironment& ce, const IStructure& nullType, cstr id, cr_sex args);
      void AppendInvoke(CCompileEnvironment& ce, ID_API_CALLBACK callback, cr_sex s);
      void AddVariableRef(CCompileEnvironment& ce, const NameString& ns, const IStructure& typeStruct);
      void AssignTempToVariableRef(CCompileEnvironment& ce, int tempDepth, cstr name);
      IArchetype* MatchArchetype(cr_sex typeExpr, IModuleBuilder& module);
      bool IsAtomicMatch(cr_sex s, cstr value);
      bool TryCompileAsLateFactoryCall(CCompileEnvironment& ce, const MemberDef& targetDef, cr_sex directive);
      bool RequiresDestruction(const IStructure& s);
      int GetInterfaceOffset(int index);
      const IStructure& GetClass(cr_sex classExpr, CScript& script);
      const IStructure& GetThisInterfaceRefDef(OUT MemberDef& def, ICodeBuilder& builder, cr_sex s);
      IFunction& GetConstructor(IModuleBuilder& module, cr_sex typeExpr);
      const IFunction& GetConstructor(const IStructure& st, cr_sex s);
      IFunction& GetFunctionByFQN(CCompileEnvironment& ce, cr_sex s, cstr name);
      void CompileAsListNodeDeclaration(CCompileEnvironment& ce, cstr nodeName, cr_sex source);
      void CompileAsMapNodeDeclaration(CCompileEnvironment& ce, cstr nodeName, cr_sex source);
      void CompileIfThenElse(CCompileEnvironment& ce, cr_sex s);
      void CompileWhileLoop(CCompileEnvironment& ce, cr_sex s);
      void CompileBreak(CCompileEnvironment& ce, cr_sex s);
      void CompileContinue(CCompileEnvironment& ce, cr_sex s);
      void CompileDoWhile(CCompileEnvironment& ce, cr_sex s);
      void CompileForEach(CCompileEnvironment& ce, cr_sex s);
      void CompileArrayDeclaration(CCompileEnvironment& ce, cr_sex s);
      void CompileListDeclaration(CCompileEnvironment& ce, cr_sex s);
      void CompileMapDeclaration(CCompileEnvironment& ce, cr_sex s);
      void CallMacro(CCompileEnvironment& ce, const IFunction& f, cr_sex s);
      bool TryCompileAsPlainFunctionCall(CCompileEnvironment& ce, cr_sex s);
      bool TryCompileAsDerivativeFunctionCall(CCompileEnvironment& ce, cr_sex s);
      int GetOutputSFOffset(CCompileEnvironment& ce, int inputStackAllocCount, int outputStackAllocCount);
      void PopOutputs(CCompileEnvironment& ce, cr_sex invocation, const IArchetype& callee, int outputOffset, bool isVirtualCall);
   }
}