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

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define THIS_IS_THE_SEXY_CORE_LIBRARY
#define IS_SCRIPT_DLL 1

#define SEXY_SPARSER_API

#include <sexy.types.h>
#include <sexy.vm.h>
#include <sexy.vm.cpu.h>
#include <sexy.script.h>
#define ROCOCO_USE_SAFE_V_FORMAT
#include "sexy.strings.h"
#include "sexy.stdstrings.h"
#include <sexy.unordered_map.h>
#include "..\STC\stccore\Sexy.Compiler.h"
#include <rococo.hashtable.h>
#include <sexy.vector.h>

namespace Rococo
{
    int CALLTYPE_C StringPrint(TokenBuffer& token, const char* format, ...);

    namespace Compiler
    {
        class STCException;
        struct IStructure;
        struct ICodeBuilder;
        struct INamespaceBuilder;
        struct IProgramObject;

        SCRIPTEXPORT_API cstr GetTypeName(const IStructure& s);
    }

    namespace Sex
    {
        void AssertQualifiedIdentifier(cr_sex e);
        void AssertTypeIdentifier(cr_sex e);
        void AssertTypeIdentifier(cr_sex src, cstr name);
        void AssertLocalIdentifier(cr_sex e);

        void Throw(cr_sex e, const fstring& f);
        void ThrowTypeMismatch(cr_sex s, const Compiler::IStructure& a, const Compiler::IStructure& b, cstr extra);

        void AssertAtomicMatch(cr_sex s, cstr value);
        [[noreturn]] void ThrowNamespaceConflict(cr_sex s, const Compiler::INamespace& n1, const Compiler::INamespace& n2, cstr type, cstr token);
        [[noreturn]] void ThrowTokenNotFound(cr_sex s, cstr item, cstr repository, cstr type);
        [[nodiscard]] Compiler::INamespace& AssertGetNamespace(Compiler::IProgramObject& object, cr_sex s, cstr fullName);
    }

    namespace Script
    {
        using namespace Rococo::Compiler;

        extern const char* const THIS_POINTER_TOKEN;

        struct IScriptSystem;

        typedef TSexyStringMap<cstr> TMapMethodToMember;

        class CScript;

        void AddMember(IStructureBuilder& s, Sex::cr_sex field, IScriptSystem& ss);
        IInterfaceBuilder* MatchInterface(Sex::cr_sex typeExpr, IModuleBuilder& module);

        DECLARE_ROCOCO_INTERFACE IStringPool: Rococo::Compiler::IFastStringBuilderControl
        {
            virtual void Free() = 0;
            virtual AllocatorBinding* GetBinding() = 0;
            virtual void SetStringBuilderType(const IStructure* typeFastStringBuilder) = 0;
            virtual const IStructure* FastStringBuilderType() const = 0;
            virtual FastStringBuilder* CreateAndInitFields(int32 capacity) = 0;
        };

        IStringPool* NewStringPool();

        class CCompileEnvironment
        {
        private:
            const IStructure* arrayStruct;
            const IStructure* listStruct;
            const IStructure* mapStruct;
            const IStructure* expressionInterface;
            const IStructure* expressionBuilderInterface;
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
            const IStructure& StructExpressionInterface();
            const IStructure& StructExpressionBuilderInterface();
            const IStructure& StructList();
            const IStructure& StructMap();

            cstr MapMethodToMember(cstr method)
            {
                auto i = methodMap.find(method);
                return i == methodMap.end() ? NULL : i->second;
            }
        };

        Rococo::Sex::ISExpressionProxy& CreateExpressionProxy(CCompileEnvironment& ce, Sex::cr_sex inner, int numberOfElements);
        void ValidateAssignment(Sex::cr_sex callDef);
        void InitClassMembers(CCompileEnvironment& ce, cstr id);
        void StreamSTCEX(Strings::StringBuilder& sb, const Compiler::STCException& ex);
        bool TryCompileFunctionCallAndReturnValue(CCompileEnvironment& ce, Sex::cr_sex s, VARTYPE type, const IStructure* derivedType, const IArchetype* returnArchetype);

        class CScripts;

        struct CBindFnDefToExpression
        {
            const Sex::ISExpression* FnDef;
            IFunctionBuilder* Fn;
        };
        typedef TSexyStringMap<CBindFnDefToExpression> TFunctionDefinitions;

        struct CBindStructDefToExpression
        {
            const Sex::ISExpression* StructDef;
            IStructureBuilder* Struct;
        };
        typedef TSexyVector<CBindStructDefToExpression> TStructureDefinitions;

        struct CClosureDef
        {
            const Sex::ISExpression* ClosureExpr;
            IFunctionBuilder* Closure;
            CClosureDef(Sex::cr_sex closureExpr, IFunctionBuilder& closure) : ClosureExpr(&closureExpr), Closure(&closure) {}
        };

        typedef TSexyVector<CClosureDef> TClosures;

        struct CNullDef
        {
            IInterfaceBuilder* Interface;
            const Sex::ISExpression* Source;
            IStructureBuilder* NullObject;
            INamespaceBuilder* NS;
        };

        typedef TSexyVector<CNullDef> TNullObjectDefs;
        typedef TSexyVector<CStringConstant*> TStringConstants;
        typedef TSexyVector<IMacroBuilder*> TMacros;

        struct BuilderAndNameKey
        {
            ICodeBuilder* Builder;
            Strings::HString Name;
        };

        struct hashBuilderAndNameKey
        {
            size_t operator()(const BuilderAndNameKey& s) const
            {
                return Rococo::Strings::FastHash(s.Name) ^ (size_t)s.Builder;
            }
        };

        inline bool operator == (const BuilderAndNameKey& a, const BuilderAndNameKey& b)
        {
            return a.Builder == b.Builder && a.Name == b.Name;
        }

        struct ArrayDef
        {
            Sex::cr_sex SexDef;
            const IStructure& ElementType;

            ArrayDef(Sex::cr_sex sexDef, const IStructure& elementType) : SexDef(sexDef), ElementType(elementType) {}
        };

        struct ListDef
        {
            Sex::cr_sex SexDef;
            const IStructure& ElementType;

            ListDef(Sex::cr_sex sexDef, const IStructure& elementType) : SexDef(sexDef), ElementType(elementType) {}
        };

        struct MapDef
        {
            Sex::cr_sex SexDef;
            const IStructure& KeyType;
            const IStructure& ValueType;

            MapDef(Sex::cr_sex sexDef, const IStructure& keyType, const IStructure& valueType) : SexDef(sexDef), KeyType(keyType), ValueType(valueType) {}
            MapDef(const MapDef& s) : SexDef(s.SexDef), KeyType(s.KeyType), ValueType(s.ValueType) {}
        };

        struct NodeDef
        {
            Sex::cr_sex SexDef;
            const IStructure& ElementType;

            NodeDef(Sex::cr_sex sexDef, const IStructure& elementType) : SexDef(sexDef), ElementType(elementType) {}
        };

        struct MapNodeDef
        {
            Sex::cr_sex SexDef;
            MapDef mapdef;
            stdstring mapName;
            MapNodeDef(Sex::cr_sex _sexDef, const MapDef& _mapdef, cstr _mapName) : SexDef(_sexDef), mapdef(_mapdef), mapName(_mapName) {}
        };


        typedef TSexyHashMap<BuilderAndNameKey, ArrayDef, hashBuilderAndNameKey> TMapNameToArrayDef;
        typedef TSexyHashMap<BuilderAndNameKey, ListDef, hashBuilderAndNameKey> TMapNameToListDef;
        typedef TSexyHashMap<BuilderAndNameKey, NodeDef, hashBuilderAndNameKey> TMapNameToNodeDef;
        typedef TSexyHashMap<BuilderAndNameKey, MapDef, hashBuilderAndNameKey> TMapNameToMapDef;
        typedef TSexyHashMap<BuilderAndNameKey, MapNodeDef, hashBuilderAndNameKey> TMapNameToMapNodeDef;
        typedef TSexyStringMap<GlobalValue> TGlobalVariables;

        struct TransformData
        {
            Sex::IExpressionTransform* transform;
        };
        typedef TSexyHashMap<const Sex::ISExpression*, TransformData> TTransformMap;
        typedef TSexyVector<const Sex::ISExpression*> TExceptionBlocks;

        struct CBindNSExpressionToModule
        {
            const Sex::ISExpression* E;
            CScript* Module;
            INamespace* NS;
            bool isDefault = false;
        };

        inline bool operator < (const CBindNSExpressionToModule& a, const CBindNSExpressionToModule& b)
        {
            return a.E->String()->Length < b.E->String()->Length;
        }

        typedef TSexyVector<CBindNSExpressionToModule> TNamespaceDefinitions;

        class CScript
        {
        private:
            Sex::ISParserTree& tree;
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

        public:
            CScript(Sex::ISParserTree& _tree, IProgramObject& _programObject, CScripts& _scripts);
            GlobalValue* GetGlobalValue(cstr name);

            void* operator new(size_t nBytes);
            void operator delete(void* buffer);

            void EnumerateGlobals(IGlobalEnumerator& cb);
            const ArrayDef* GetElementTypeForArrayVariable(ICodeBuilder& builder, cstr arrayName);
            const ListDef* GetListDef(ICodeBuilder& builder, cstr listName);
            const NodeDef* GetNodeDef(ICodeBuilder& builder, cstr listName);
            const MapDef* GetMapDef(ICodeBuilder& builder, cstr listName);
            const MapNodeDef* GetMapNodeDef(ICodeBuilder& builder, cstr name);
            void AddArrayDef(ICodeBuilder& builder, cstr arrayName, const IStructure& elementType, Sex::cr_sex s);
            void AddListDef(ICodeBuilder& builder, cstr name, const IStructure& elementType, Sex::cr_sex s);
            void AddMapDef(ICodeBuilder& builder, cstr name, const IStructure& keyType, const IStructure& valueType, Sex::cr_sex s);
            void AddMapNodeDef(ICodeBuilder& builder, const MapDef& mapDef, cstr mapName, cstr nodeName, Sex::cr_sex s);
            void AddNodeDef(ICodeBuilder& builder, cstr nodeName, const IStructure& elementType, Sex::cr_sex s);
            Sex::cr_sex GetActiveRoot();
            Sex::cr_sex GetActiveExpression(Sex::cr_sex s);
            void Invoke_S_Macro(Sex::cr_sex sDirective);
            void CompileTopLevelMacros();

            const bool IsIStringInlined() const;

            IProgramObject& Object() { return programObject; }
            IScriptSystem& System();

            CStringConstant* CreateStringConstant(int length, cstr pointer, const Sex::ISExpression* srcExpression);

            void RegisterContext(cstr _name) { contextName = _name; }
            cstr GetContext() const { return contextName; }

            bool HasCompiled() const
            {
                return !isDirty;
            }

            void MarkCompiled()
            {
                isDirty = false;
            }

            const Sex::ISExpression* GetTryCatchExpression() const
            {
                return exceptionBlocks.empty() ? NULL : exceptionBlocks.back();
            }

            void PushTryCatchBlock(Sex::cr_sex s)
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
            const Sex::ISParserTree& Tree() const { return tree; }
            Sex::ISParserTree& Tree() { return tree; }

            void Clear();
            void CompileNullObjects();
            void CompileNextClosures();
            void CompileLocalFunctions();
            void CompileJITStubs();
            void CompileVTables();
            void AppendCompiledNamespaces(TNamespaceDefinitions& nsDefs);
            void AddEnumeratedVirtualMethod(IN Sex::cr_sex virtualMethodExpr, IN cstr methodName, REF IInterfaceBuilder& inter, IN size_t vmIndex);
            void AddVirtualMethod(IN Sex::cr_sex virtualMethodExpr, REF IInterfaceBuilder& inter, IN size_t vmIndex);
            void AddVirtualMethod(IN const IArchetype& archetype, REF IInterfaceBuilder& inter, IN size_t vmIndex, Sex::cr_sex def);
            void ComputeGlobal(Sex::cr_sex globalDef, int& globalBaseIndex);
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
            void AddArchetype(Sex::cr_sex e);
            void AddArchetypeName(Sex::cr_sex e);
            void PostClosure(Sex::cr_sex s, IFunctionBuilder& closure);
            void AddInterfacePrototype(Sex::cr_sex s, bool isInterfaceDefinedFromClassMethods);
            void ValidateConcreteClasses();
            void ValidateConstructors();
            void DeclareFactory(Sex::cr_sex factoryDef);
            void DeclareMacros();
            IFunctionBuilder& GetNullFunction(const IArchetype& archetype);
        };

        typedef TSexyHashMap<void*, void*> TAllocationMap;

        void GetAtomicValue(CCompileEnvironment& ce, Sex::cr_sex parent, cstr id, VARTYPE type);
        void AppendDeconstructTailVariables(CCompileEnvironment& ce, Sex::cr_sex sequence, bool expire, int tailCount);
        void AppendDeconstruct(CCompileEnvironment& ce, Sex::cr_sex sequence, bool expireVariables);
        void AddInterfaceVariable(CCompileEnvironment& ce, const NameString& ns, const IStructure& ts);
        void AddVariableAndSymbol(CCompileEnvironment& ce, cstr type, cstr name);
        void ValidateUnusedVariable(Sex::cr_sex identifierExpr, ICodeBuilder& builder);
        void AssertGetVariable(OUT MemberDef& def, cstr name, CCompileEnvironment& ce, Sex::cr_sex exceptionSource);
        void CompileAsPopOutFromArray(CCompileEnvironment& ce, Sex::cr_sex s, cstr instanceName, VARTYPE requiredType);
        void CompileArraySet(CCompileEnvironment& ce, Sex::cr_sex s, cstr arrayName);
        void CompileGetArrayElement(CCompileEnvironment& ce, Sex::cr_sex s, cstr instanceName, VARTYPE varType, const IStructure* structType);
        void CompileGetArraySubelement(CCompileEnvironment& ce, Sex::cr_sex indexExpr, Sex::cr_sex subItemName, cstr instanceName, VARTYPE type, const IStructure* structType);

        bool TryCompileAsArrayCall(CCompileEnvironment& ce, Sex::cr_sex s, cstr instanceName, cstr methodName);
        bool TryCompileAsListCall(CCompileEnvironment& ce, Sex::cr_sex s, cstr instanceName, cstr methodName);
        bool TryCompileAsNodeCall(CCompileEnvironment& ce, Sex::cr_sex s, cstr instanceName, cstr methodName);
        bool TryCompileAsMapCall(CCompileEnvironment& ce, Sex::cr_sex s, cstr mapName, cstr methodName);
        bool TryCompileAsMapNodeCall(CCompileEnvironment& ce, Sex::cr_sex s, cstr name, cstr methodName);
        bool TryCompileAsInlineMapAndReturnValue(CCompileEnvironment& ce, Sex::cr_sex s, cstr instance, cstr methodName, VARTYPE returnType, const IStructure& instanceStruct, OUT VARTYPE& outputType);

        void ConstructMemberByRef(CCompileEnvironment& ce, Sex::cr_sex args, int tempDepth, const IStructure& type, int offset);
        SCRIPTEXPORT_API cstr GetTypeName(VARTYPE type);

        VARTYPE GetAtomicValueAnyNumeric(CCompileEnvironment& ce, Sex::cr_sex parent, cstr id, int tempdepth);
        void AssignVariableToVariable(CCompileEnvironment& ce, Sex::cr_sex exceptionSource, cstr lhs, cstr rhs);
        CStringConstant* CreateStringConstant(CScript& script, int length, cstr s, const Sex::ISExpression* srcExpression);
        IFunctionBuilder& DeclareFunction(IModuleBuilder& module, Sex::cr_sex source, FunctionPrototype& prototype);
        cstr GetContext(const CScript& script);
        const Sex::ISExpression* GetTryCatchExpression(CScript& script);
        bool TryCompileBooleanExpression(CCompileEnvironment& ce, Sex::cr_sex s, bool isExpected, bool& negate);
        void CompileExpressionSequence(CCompileEnvironment& ce, int start, int end, Sex::cr_sex sequence);
        void CompileExpression(CCompileEnvironment& ce, Sex::cr_sex s);
        void ValidateArchetypeMatchesArchetype(Sex::cr_sex s, const IArchetype& f, const IArchetype& archetype, cstr source);
        int GetIndexOf(int start, Sex::cr_sex s, cstr text);
        void CompileExceptionBlock(CCompileEnvironment& ce, Sex::cr_sex s);
        void CompileThrow(CCompileEnvironment& ce, Sex::cr_sex s);
        void CompileNextClosures(CScript& script);
        void GetVariableByName(ICodeBuilder& builder, OUT MemberDef& def, cstr name, Sex::cr_sex src);
        void AddCatchHandler(CScript& script, ID_BYTECODE id, size_t start, size_t end);
        IModuleBuilder& GetModule(CScript& script);
        void AppendVirtualCallAssembly(cstr instanceName, int interfaceIndex, int methodIndex, ICodeBuilder& builder, const IInterface& interf, Sex::cr_sex s, cstr localName);
        int GetFirstOutputOffset(ICodeBuilder& builder);
        IModule& GetSysTypeMemoModule(CScript& script);
        INamespaceBuilder& GetNamespaceByFQN(CCompileEnvironment& ce, cstr ns, Sex::cr_sex s);
        void MarkStackRollback(CCompileEnvironment& ce, Sex::cr_sex invokeExpression);
        int PushInput(CCompileEnvironment& ce, Sex::cr_sex s, int index, const IStructure& inputStruct, const IArchetype* archetype, cstr inputName, const IStructure* genericArg1, cstr defaultValue);
        const IArgument& GetInput(Sex::cr_sex s, IFunction& f, int index);
        const IArchetype* GetArchetype(Sex::cr_sex s, IFunction& f, int index);
        void PushVariableRef(Sex::cr_sex s, ICodeBuilder& builder, const MemberDef& def, cstr name, int interfaceIndex);
        int GetCommonInterfaceIndex(const IStructure& object, const IStructure& argType);

        IFunctionBuilder& MustMatchFunction(IModuleBuilder& module, Sex::cr_sex s, cstr name);
        IInterfaceBuilder* MatchInterface(Sex::cr_sex typeExpr, IModuleBuilder& module);
        IStructureBuilder* MatchStructure(Sex::cr_sex typeExpr, IModuleBuilder& module);
        IFunctionBuilder* MatchFunction(Sex::cr_sex nameExpr, IModuleBuilder& module);

        bool TryCompileAssignArchetype(CCompileEnvironment& ce, Sex::cr_sex s, const IStructure& elementType, bool allowClosures);
        bool TryCompileArithmeticExpression(CCompileEnvironment& ce, Sex::cr_sex s, bool expected, VARTYPE type);

        void CompileFunctionCallAndReturnValue(CCompileEnvironment& ce, Sex::cr_sex s, IFunction& callee, VARTYPE returnType, const IArchetype* returnArchetype, const IStructure* returnTypeStruct);
        bool TryCompileMacroInvocation(CCompileEnvironment& ce, Sex::cr_sex s, sexstring token);
        IProgramObject& GetProgramObject(CScript& script);
        void CompileJITStub(IFunctionBuilder& f, Sex::cr_sex fdef, CScript& script, IScriptSystem& ss);
        void CompileJITStub(IFactoryBuilder* f, Sex::cr_sex fdef, CScript& script, IScriptSystem& ss);
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
            ID_API_CALLBACK ArrayGetLastIndexToD12;
            ID_API_CALLBACK ArrayGetMember32;
            ID_API_CALLBACK ArrayGetMember64;
            ID_API_CALLBACK ArrayCopyByRef;
            ID_API_CALLBACK ArraySet32;
            ID_API_CALLBACK ArraySet64;
            ID_API_CALLBACK ArraySetByRef;
            ID_API_CALLBACK ArrayInit;
            ID_API_CALLBACK ArrayRelease;
            ID_API_CALLBACK ArrayPop;
            ID_API_CALLBACK ArrayPopOut32;
            ID_API_CALLBACK ArrayPopOut64;
            ID_API_CALLBACK ArrayLock;
            ID_API_CALLBACK ArrayUnlock;
            ID_API_CALLBACK ArrayGetRefUnchecked;
            ID_API_CALLBACK ArrayDestructElements;
            ID_API_CALLBACK ArrayGetInterfaceUnchecked;
            ID_API_CALLBACK ArrayGetInterfaceLockless;
            ID_API_CALLBACK ArrayGetLength;
            ID_API_CALLBACK ArrayGetLastIndex; // D13 points to array, D11 gives last index (-1 for empty arrays)
            ID_API_CALLBACK ArrayReturnLength;
            ID_API_CALLBACK ArrayReturnCapacity;
            ID_API_CALLBACK ArrayUpdateRefCounts;
        };

        struct ListCallbacks
        {
            ID_API_CALLBACK ListInit;
            ID_API_CALLBACK ListAppend;
            ID_API_CALLBACK ListAppendAndGetRef;
            ID_API_CALLBACK ListAppend32;
            ID_API_CALLBACK ListAppend64;
            ID_API_CALLBACK ListAppendInterface;
            ID_API_CALLBACK ListAssign;
            ID_API_CALLBACK ListClear;
            ID_API_CALLBACK ListRelease;
            ID_API_CALLBACK ListPrepend;
            ID_API_CALLBACK ListPrependAndGetRef;
            ID_API_CALLBACK ListPrepend32;
            ID_API_CALLBACK ListPrepend64;
            ID_API_CALLBACK ListPrependInterface;
            ID_API_CALLBACK ListGetTail;
            ID_API_CALLBACK ListGetHead;
            ID_API_CALLBACK ListGetHeadUnreferenced;
            ID_API_CALLBACK ListGetLength;
            ID_API_CALLBACK NodeGet32;
            ID_API_CALLBACK NodeGet64;
            ID_API_CALLBACK NodeGetKey32;
            ID_API_CALLBACK NodeGetKey64;
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
            ID_API_CALLBACK DoesMapNodeExist;
            ID_API_CALLBACK MapAssign;
            ID_API_CALLBACK MapInit;
            ID_API_CALLBACK MapInsert32;
            ID_API_CALLBACK MapInsert64;
            ID_API_CALLBACK MapInsertValueByRef;
            ID_API_CALLBACK MapInsertInterface;
            ID_API_CALLBACK MapInsertAndGetRef;
            ID_API_CALLBACK MapRelease;
            ID_API_CALLBACK MapTryGet;
            ID_API_CALLBACK MapNodeGet32;
            ID_API_CALLBACK MapNodeGet64;
            ID_API_CALLBACK MapNodeGetInterface;
            ID_API_CALLBACK MapNodeGetKey32;
            ID_API_CALLBACK MapNodeGetKey64;
            ID_API_CALLBACK MapNodeGetKeyIString;
            ID_API_CALLBACK MapNodeGetRef;
            ID_API_CALLBACK MapNodePop;
            ID_API_CALLBACK MapGetHead;
            ID_API_CALLBACK NodeEnumNext;
            ID_API_CALLBACK MapNodeReleaseRef;
            ID_API_CALLBACK MapGetLength;
            ID_API_CALLBACK MapUpdateRefCounts;
        };

        const ArrayCallbacks& GetArrayCallbacks(CCompileEnvironment& ce);
        const ListCallbacks& GetListCallbacks(CCompileEnvironment& ce);
        const MapCallbacks& GetMapCallbacks(CCompileEnvironment& ce);

        const IStructure& GetKeyTypeForMapVariable(CCompileEnvironment& ce, Sex::cr_sex src, cstr mapName);
        const IStructure& GetValueTypeForMapVariable(CCompileEnvironment& ce, Sex::cr_sex src, cstr mapName);
        const IStructure& GetElementTypeForArrayVariable(CCompileEnvironment& ce, Sex::cr_sex src, cstr arrayName);
        const IStructure& GetListDef(CCompileEnvironment& ce, Sex::cr_sex src, cstr name);
        const MapDef GetMapDef(CCompileEnvironment& ce, Sex::cr_sex src, cstr name);
        const IStructure& GetNodeDef(CCompileEnvironment& ce, Sex::cr_sex src, cstr name);
        const MapNodeDef& GetMapNodeDef(CCompileEnvironment& ce, Sex::cr_sex src, cstr name);

        void AddArrayDef(CScript& script, ICodeBuilder& builder, cstr arrayName, const IStructure& elementType, Sex::cr_sex def);
        void AddListDef(CScript& script, ICodeBuilder& builder, cstr listName, const IStructure& elementType, Sex::cr_sex def);
        void AddMapDef(CScript& script, ICodeBuilder& builder, cstr name, const IStructure& keyType, const IStructure& valueType, Sex::cr_sex def);
        void AddNodeDef(CScript& script, ICodeBuilder& builder, cstr name, const IStructure& elementType, Sex::cr_sex s);
        void AddMapNodeDef(CScript& script, ICodeBuilder& builder, const MapDef& def, cstr nodeName, cstr mapName, Sex::cr_sex s);
        bool TryCompileClosureDef(CCompileEnvironment& ce, Sex::cr_sex s, const IArchetype& closureArchetype, bool mayUseParentSF);

        bool IsIStringInlined(CScript& script);
        bool IsIStringInlined(CScripts& scripts);
        void AddSymbol(ICodeBuilder& builder, cstr format, ...);

        void AddInterfaceVariable(CCompileEnvironment& ce, const NameString& ns, const IStructure& ts);
        void AddVariable(CCompileEnvironment& ce, const NameString& ns, const IStructure& typeStruct);
        void AddArgVariable(cstr desc, CCompileEnvironment& ce, const TypeString& type);
        void AddArgVariable(cstr desc, CCompileEnvironment& ce, const IStructure& type);
        void AddVariable(CCompileEnvironment& ce, const NameString& ns, const TypeString& ts);
        int PushInputs(CCompileEnvironment& ce, Sex::cr_sex s, const IArchetype& callee, bool isImplicitInput, int firstArgIndex);
        int CompileInstancePointerArg(CCompileEnvironment& ce, cstr classInstance);
        void AppendFunctionCallAssembly(CCompileEnvironment& ce, const IFunction& callee);
        void RepairStack(CCompileEnvironment& ce, Sex::cr_sex s, const IArchetype& callee, int extraArgs = 0);
        void CompileConstructFromFactory(CCompileEnvironment& ce, const IStructure& nullType, cstr id, Sex::cr_sex args);
        void AppendInvoke(CCompileEnvironment& ce, ID_API_CALLBACK callback, Sex::cr_sex s);
        void AddVariableRef(CCompileEnvironment& ce, const NameString& ns, const IStructure& typeStruct);
        void AssignTempToVariableRef(CCompileEnvironment& ce, int tempDepth, cstr name);
        IArchetype* MatchArchetype(Sex::cr_sex typeExpr, IModuleBuilder& module);
        bool IsAtomicMatch(Sex::cr_sex s, cstr value);
        bool TryCompileAsLateFactoryCall(CCompileEnvironment& ce, const MemberDef& targetDef, Sex::cr_sex directive);
        bool RequiresDestruction(const IStructure& s);
        int GetInterfaceOffset(int index);
        const IStructure& GetClass(Sex::cr_sex classExpr, CScript& script);
        const IStructure& GetThisInterfaceRefDef(OUT MemberDef& def, ICodeBuilder& builder, Sex::cr_sex s);
        IFunction& GetConstructor(IModuleBuilder& module, Sex::cr_sex typeExpr);
        const IFunction& GetConstructor(const IStructure& st, Sex::cr_sex s);
        IFunction& GetFunctionByFQN(CCompileEnvironment& ce, Sex::cr_sex s, cstr name);
        void CompileAsListNodeDeclaration(CCompileEnvironment& ce, cstr nodeName, Sex::cr_sex source);
        void CompileAsMapNodeDeclaration(CCompileEnvironment& ce, cstr nodeName, Sex::cr_sex source);
        void CompileIfThenElse(CCompileEnvironment& ce, Sex::cr_sex s);
        void CompileForLoop(CCompileEnvironment& ce, Sex::cr_sex s);
        void CompileWhileLoop(CCompileEnvironment& ce, Sex::cr_sex s);
        void CompileBreak(CCompileEnvironment& ce, Sex::cr_sex s);
        void CompileContinue(CCompileEnvironment& ce, Sex::cr_sex s);
        void CompileDoWhile(CCompileEnvironment& ce, Sex::cr_sex s);
        void CompileForEach(CCompileEnvironment& ce, Sex::cr_sex s);
        void CompileGoto(CCompileEnvironment& ce, Sex::cr_sex s);
        void CompileLabel(CCompileEnvironment& ce, Sex::cr_sex s);
        void CompileArrayDeclaration(CCompileEnvironment& ce, Sex::cr_sex s);
        void CompileListDeclaration(CCompileEnvironment& ce, Sex::cr_sex s);
        void CompileMapDeclaration(CCompileEnvironment& ce, Sex::cr_sex s);
        void CallMacro(IScriptSystem& ss, const IFunction& f, Sex::cr_sex s);
        bool TryCompileAsPlainFunctionCall(CCompileEnvironment& ce, Sex::cr_sex s);
        bool TryCompileAsDerivativeFunctionCall(CCompileEnvironment& ce, Sex::cr_sex s);
        int GetOutputSFOffset(CCompileEnvironment& ce, int inputStackAllocCount, int outputStackAllocCount);
        void PopOutputs(CCompileEnvironment& ce, Sex::cr_sex invocation, const IArchetype& callee, int outputOffset, bool isVirtualCall);
    }
}