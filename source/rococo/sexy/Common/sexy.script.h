
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
#ifndef SEXY_SCRIPT_H
#define SEXY_SCRIPT_H

#include "sexy.types.h" 
#include "sexy.compiler.public.h" 
#include "sexy.debug.types.h" 

#include "sexy.vm.h"
#include "sexy.vm.cpu.h"

// #define SEXY_ENABLE_EXTRA_STRING_SECURITY // uncomment to add in extra security code, which slows some strings ops down a bit

namespace Rococo
{
	DECLARE_ROCOCO_INTERFACE ILog;

	namespace Compiler
	{
		DECLARE_ROCOCO_INTERFACE IProgramObject;
		struct VirtualTable;
		DECLARE_ROCOCO_INTERFACE IStructure;
		DECLARE_ROCOCO_INTERFACE IPublicProgramObject;
		DECLARE_ROCOCO_INTERFACE IFunction;
		DECLARE_ROCOCO_INTERFACE IMemberLife;
		DECLARE_ROCOCO_INTERFACE IInterface;
	}

	namespace Sex
	{
		DECLARE_ROCOCO_INTERFACE ISParser;
		DECLARE_ROCOCO_INTERFACE ISParserTree;
		DECLARE_ROCOCO_INTERFACE ISExpression;
		DECLARE_ROCOCO_INTERFACE ISExpressionBuilder;

		typedef const ISExpression& cr_sex;
	}

	namespace VM
	{
		struct CPU;
	}
}

namespace Rococo {
	namespace Script
	{
		ROCOCO_INTERFACE IFreeable
		{
			virtual void Free() = 0;
		};

		struct IPublicScriptSystem;
		struct ArchetypeCallback;

		struct NativeCallEnvironment
		{
			IPublicScriptSystem& ss;
			const Compiler::IFunction& function;
			const  Compiler::IFunctionCode& code;
			VM::CPU& cpu;
			void* context;

			NativeCallEnvironment(IPublicScriptSystem& _ss, const  Compiler::IFunction& _function, VM::CPU& _cpu, void* _context);
		};

		typedef void (CALLTYPE_C *FN_NATIVE_CALL)(NativeCallEnvironment& e);

		const Rococo::Sex::ISExpression* GetSourceExpression(Rococo::Compiler::IPublicProgramObject& po, const Rococo::Compiler::IFunction& f, size_t pcOffset);

#ifdef _DEBUG
		inline void ValidateOutputIndex(int index, const Compiler::IFunctionCode& code)
		{
			if (index < 0 || index >= code.Owner().NumberOfOutputs())
			{
				ThrowBadNativeArg(index, code.Owner().Name(), ("Bad output argument"));
			}
		}

		inline void ValidateInputIndex(int index, const Compiler::IFunctionCode& code)
		{
			if (index < 0 || index >= code.Owner().NumberOfInputs())
			{
				ThrowBadNativeArg(index, code.Owner().Name(), ("Bad input argument"));
			}
		}
#else
		inline void ValidateOutputIndex(int, const Compiler::IFunctionCode&)  {}
		inline void ValidateInputIndex(int, const Compiler::IFunctionCode&) {}
#endif

		template<class T> void ReadInput(T& value, const uint8 *SF, ptrdiff_t offset)
		{
		//	static_assert(((sizeof T) % 4) == 0, "sizeof(T) needs to be a multiple of 4");
			const uint8* inputStart = SF + offset;
			uint8* readPos = (uint8*)inputStart;
			T* pValue = (T*)readPos;
			value = *pValue;
		}

		template<class T>void WriteOutput(T value, const uint8 *SF, ptrdiff_t offset)
		{
		//	static_assert(((sizeof T) % 4) == 0, "sizeof(T) needs to be a multiple of 4");
			const uint8* outputStart = SF + offset;
			uint8* writePos = (uint8*)outputStart;
			T* pValue = (T*)writePos;
			*pValue = value;
		}

		template<class T>void WriteOutput(int index, T value, NativeCallEnvironment& e)
		{
			static_assert((sizeof (T) % 4) == 0, "sizeof(T) needs to be a multiple of 4");
			ValidateOutputIndex(index, e.code);
			int offset = e.code.GetOffset(index);
			WriteOutput(value, e.cpu.SF(), offset);
		}

		template<class T> void ReadInput(int index, T& value, Compiler::IPublicProgramObject& po, const Compiler::IFunction& f)
		{
			static_assert((sizeof(T) % 4) == 0, "sizeof(T) needs to be a multiple of 4");
			ValidateInputIndex(index, f.Code());
			int offset = f.Code().GetOffset(index + f.NumberOfOutputs());
			ReadInput(value, po.VirtualMachine().Cpu().SF(), offset);
		}

		template<class T> void ReadInput(int index, T& value, NativeCallEnvironment& e)
		{
			static_assert((sizeof(T) % 4) == 0, "sizeof(T) needs to be a multiple of 4");
			ValidateInputIndex(index, e.code);
			int offset = e.code.GetOffset(index + e.function.NumberOfOutputs());
			ReadInput(value, e.cpu.SF(), offset);
		}

		struct IScriptSystem;

#pragma pack(push,1)
		struct CClassExpression
		{
			Compiler::ObjectStub Header;
			Sex::ISExpression* ExpressionPtr;
		};

		struct CClassExpressionBuilder
		{
			Compiler::ObjectStub Header;
			Sex::ISExpressionBuilder* BuilderPtr;
		};

		struct CScriptSystemClass
		{
			Compiler::ObjectStub header;
		};

		struct CReflectedClass
		{
			Compiler::ObjectStub header;
			void* context;
		};
#pragma pack(pop)

		enum ENUM_REPRESENT
		{
			ENUM_REPRESENT_BREAK,
			ENUM_REPRESENT_CONTINUE,
			ENUM_REPRESENT_DELETE
		};

		ROCOCO_INTERFACE IRepresentationEnumeratorCallback
		{
			virtual ENUM_REPRESENT OnRepresentation(CReflectedClass* rep) = 0;
		};

		ROCOCO_INTERFACE ISexyPackager
		{
			/*
				Add a package to the package set, the package and its pointer must
			    remain valid for the lifetime of the ISexyPackager.
				If the package hash code matches that of an existing package
				inside the ISexyPackager then the method returns false and
				nothing else happens. Otherwise the files and directories
				inside the package will be enumerated for later use and true is returned
			*/
			virtual bool RegisterNamespacesInPackage(IPackage* package)= 0;

			virtual void LoadSubpackages(cstr namespaceFilter, cstr packageName) = 0;

			// Returns a reference to the package that matches the prefix in the supplied id. If not found throws an exception.
			virtual IPackage& GetPackage(cstr packageId) = 0;
		};

		ROCOCO_INTERFACE ISexyPackagerSupervisor: ISexyPackager
		{
			virtual void Free() = 0;
		};

		struct ReflectionArguments
		{
			Rococo::Sex::cr_sex s;
			const Compiler::IStructure& lhsType;
			void* lhsData;
			cstr rhsName;
			const Compiler::IStructure& rhsType;
			void* rhsData;
			IPublicScriptSystem& ss;
		};

		typedef void (*FN_RAW_NATIVE_REFLECTION_CALL)(void* context, ReflectionArguments& args);

		struct RawReflectionBinding
		{
			void* context;
			FN_RAW_NATIVE_REFLECTION_CALL fnCall;
			ID_API_CALLBACK callbackId = 0;

			void* operator new(size_t nBytes);
			void operator delete(void* buffer);
		};

		template<class CONTEXT>
		class TReflectionCall
		{
		public:
			typedef void (*FN_NATIVE_REFLECTION_CALL)(CONTEXT* context, ReflectionArguments& args);

			static FN_RAW_NATIVE_REFLECTION_CALL ToRaw(FN_NATIVE_REFLECTION_CALL fnReflect)
			{
				return reinterpret_cast<FN_RAW_NATIVE_REFLECTION_CALL>(fnReflect);
			}
		};

#pragma pack(push,1)
		struct CStringConstant
		{
			Compiler::ObjectStub header;
			int32 length;
			cstr pointer;
			void* srcExpression;

			void* operator new(size_t nBytes);
			void operator delete(void* buffer);
		};

		struct ArrayImage
		{
			void* operator new(size_t nBytes);
			void operator delete(void* buffer);

			void* Start;
			int32 NumberOfElements;
			int32 ElementCapacity;
			const Compiler::IStructure* ElementType;
			int32 ElementLength;
			int32 LockNumber;
			int64 RefCount;
		};

		struct ListImage;

		struct ListNode
		{
			ListImage* Container;
			const Compiler::IStructure* ElementType;
			ListNode* Previous;
			ListNode* Next;
			int32 RefCount;
			char Element[4]; // N.B this is 16-bit aligned to the instance pointer in 32-bit and 64-bit mode
		};

#pragma pack(pop)
		struct NodeRef
		{
			ListNode* NodePtr;
		};

		struct OrphanedNodeList;

		struct ListImage
		{
			int64 refCount;
			int32 NumberOfElements;
			int32 LockNumber;
			const Compiler::IStructure* ElementType;
			ListNode* Head;
			ListNode* Tail;
			int32 ElementSize;
			OrphanedNodeList* OrphanedNodeList;
		};

		struct MapImage;

		struct NativeCallSecurity;

		ROCOCO_INTERFACE ISecuritySystem
		{
			virtual void ValidateSafeToRead(IPublicScriptSystem& ss, cstr pathname) = 0;
			virtual void ValidateSafeToWrite(IPublicScriptSystem& ss, cstr pathname) = 0;
		};
		
		ROCOCO_INTERFACE IPublicScriptSystem : public IFreeable
		{
			virtual void AddCommonSource(const char* dynamicLinkLibOfNativeCalls) = 0;
			virtual void AddNativeCall(const Compiler::INamespace& ns, FN_NATIVE_CALL callback, void* context, cstr archetype, cstr sourceFile, int lineNumber, bool checkName = true, int popBytes = 0) = 0; // Example: AddNativeCall(ns, ANON::CpuHz, NULL, "CpuHz -> (Int64 hz)");
			virtual void AddNativeCallSecurityForNS(const Compiler::INamespace& ns, const NativeCallSecurity& security) = 0;
			virtual void AddNativeCallSecurityForNS(const Compiler::INamespace& ns, cstr permittedPingPath) = 0;
			virtual const Compiler::INamespace& AddNativeNamespace(cstr name) = 0;
			virtual void AddNativeLibrary(const char *sexyLibraryFile) = 0;

			virtual void AddRawNativeReflectionCall(cstr functionName, FN_RAW_NATIVE_REFLECTION_CALL, void* context) = 0;

			template<class CONTEXT> void AddNativeReflectionCall(cstr functionName, typename TReflectionCall<CONTEXT>::FN_NATIVE_REFLECTION_CALL fnCall, CONTEXT* context)
			{
				AddRawNativeReflectionCall(functionName, TReflectionCall<CONTEXT>::ToRaw(fnCall), (void*)context);
			}

			virtual void* AlignedMalloc(int32 alignment, int32 capacity) = 0;
			virtual void AlignedFree(void* buffer) = 0;

			// Create an object, use AlignedFree to free up the memory (generally when reference count hits zero)
			virtual Compiler::ObjectStub* CreateScriptObject(cstr type, cstr sourceFile) = 0;

			// Create an object, the sizeOfObject can reserve more space than that specified by [compilersView], along C++ access to extra hidden fields. Use AlignedFree to free up the memory (generally when reference count hits zero)
			virtual Compiler::ObjectStub* CreateScriptObjectDirect(size_t sizeofObject, const Compiler::IStructure& compilersView) = 0;

			// Create a blank array with 0 capacity and no elements. The implementation should allocate memory, set the capacity > 0 and assign elements prior
			// to allowing the script to process the array. Use ss.AlignedMalloc(16, capacity * a->ElementLength); and assign to the Start variable to set the size.
			// Use ss.AlignedFree to clean up the memory (generally when reference count hits zero)
			virtual ArrayImage* CreateArrayImage(const Compiler::IStructure& type) = 0;

			// Create a blank map with no elements. The implementation should allocate memory with ss.AlignedMalloc(16, capacity * a->ElementLength); 
			// and use ss.AlignedFree to clean up the memory (generally when reference count hits zero)
			virtual MapImage* CreateMapImage(const Compiler::IStructure& keyType, const Compiler::IStructure& valueType) = 0;

			// Creates a blank linked list with no elements. The implementation should allocate memory with ss.AlignedMalloc(16, capacity * a->ElementLength); 
			// and use ss.AlignedFree to clean up the memory (generally when reference count hits zero)
			virtual ListImage* CreateListImage(const Compiler::IStructure& valueType) = 0;

			virtual Rococo::uint8* AppendListNode(ListImage& image) = 0;

			// Assumes that the string bound to the input pointer is immutable for the lifespan of the script and provides a CStringConstant for it that can be used by any script that assumes immutability
			virtual CStringConstant* ReflectImmutableStringPointer(const char* const s, int32 stringLength = -1) = 0;

			// Duplicates the null terminated string argument and provides a CStringConstant for it that can be used by any script that assumes immutability
			virtual CStringConstant* ReflectTransientStringByDuplication(cstr source, int32 stringLength = -1) = 0;

			virtual Compiler::FastStringBuilder* CreateAndPopulateFastStringBuilder(const fstring& text, int32 capacity) = 0;

			virtual void RegisterPackage(IPackage* package) = 0;

			/*
			  Find a registered package with [packageName] and attempt to load all sxy files that
			  match the search filter. If the filter ends with a * the string after the final dot 
			  is matched against all filenames in the package with the prefix before the dot.
			  So that 'Sys.Maths.I32.Do*', for example, would match Sys/Maths/I32/Double.sxy, but not
			  Sys/Dog.sxy nor Sys/Maths/I32/Triple.sxy. If the star is missed off, all files in the 
			  namespace and subspaces are matched so 'Sys.Math' would match Sys/Maths/I32/Double.sxy. 
			  If the [namespaceFilter] is blank all files in all namespaces are matched. If no matches 
			  occur an IException is thrown. Filename container paths that do not map to legal Sexy 
			  namespace strings by substituting slash for dot (/ -> .) are not enumerated and will not match. 
			  All matching files have are added to the module list and their default namespace
			  set to their subspace mapped from their file path prefix.
			  
			  E.g module Sys/Maths/I32/Double.sxy would have its default namespace set to Sys.Maths.I32
			 */
			virtual void LoadSubpackages(cstr namespaceFilter, cstr packageName) = 0;
			virtual Compiler::IModule* AddTree(Sex::ISParserTree& tree) = 0;
			virtual void BeginPartialCompilation(Strings::StringBuilder* declarationBuilder) = 0;
			virtual void Compile(Strings::StringBuilder* declarationBuilder = nullptr) = 0;
			virtual void PartialCompile(Strings::StringBuilder* declarationBuilder = nullptr) = 0;
			virtual void DispatchToSexyClosure(void* pArgBuffer, const ArchetypeCallback& target) = 0;
			virtual cstr GetSymbol(const void* ptr) const = 0;
			virtual Compiler::IPublicProgramObject& PublicProgramObject() = 0;
			virtual Sex::ISParser& SParser() = 0;
			virtual void ReleaseTree(Sex::ISParserTree* tree) = 0;
			virtual void ThrowNative(int errorNumber, cstr source, cstr message) = 0;
			virtual Sex::ISParserTree* GetSourceCode(const Compiler::IModule& module) const = 0;

			// Call this from within a C++ script function to pass the message up to the try catch block of the sexy script file.
			// Since it is the virtual machine that has its stack corrected, and not C++, you should immediately return from the calling function after invoking this method
			virtual void ThrowFromNativeCodeF(int32 errorCode, cstr format, ...) = 0;
			virtual int32 GetIntrinsicModuleCount() const = 0;
			virtual bool ValidateMemory() = 0;
			virtual void SetGlobalVariablesToDefaults() = 0;
			virtual const Compiler::IStructure* GetStringBuilderType() const = 0;

			// gets Expression of Sys.Type.Reflection.sxy. If not available throws an IException. Never returns null.
			virtual const Compiler::IStructure* GetExpressionType() const = 0;
			virtual const Compiler::IStructure* GetExpressionBuilderType() const = 0;

			virtual void CancelRepresentation(void* pSourceInstance) = 0;
			virtual void EnumRepresentations(IRepresentationEnumeratorCallback& callback) = 0;
			virtual CReflectedClass* GetRepresentation(void* pSourceInstance) = 0;
			virtual CReflectedClass* Represent(const Rococo::Compiler::IStructure& st, void* pSourceInstance) = 0;

			// Returns the interface pointer for the specified null object. If an interface is not matched the function throws an exception
			virtual Compiler::InterfacePointer GetUniversalNullObject(cstr instanceType, cstr instanceSource) = 0;

			// Returns the specified type, otherwise throws an exception
			virtual const Compiler::IStructure& GetTypeForSource(cstr concreteType, cstr sourceFile) = 0;

			virtual void SetCommandLine(int argc, char* argv[]) = 0;
			virtual cstr GetCommandLineArg(int argc) = 0;

			// Uses the cached native.hashes file to determine whether a file is suitable for Sexy. If not an exception is thrown.
			virtual void ValidateSecureFile(cstr fileId, const char* source, size_t length) = 0;

			// The security system is expected to be valid for the life time of the scrypt system
			virtual void SetSecurityHandler(ISecuritySystem& system) = 0;

			// Asks the host process whether it is acceptable for the script system to write the file with the given path name
			virtual void ValidateSafeToWrite(cstr pathname) = 0;

			// Asks the host process whether it is acceptable for the script system to read the file with the given path name
			virtual void ValidateSafeToRead(cstr pathname) = 0;
		};

		ROCOCO_INTERFACE IScriptSystemFactory : public IFreeable
		{
			virtual IPublicScriptSystem* CreateScriptSystem(const Rococo::Compiler::ProgramInitParameters& pip, ILog& logger) = 0;
		};

		struct ScriptCallbacks
		{
			ID_API_CALLBACK idThrowNullRef;
			ID_API_CALLBACK idTestD4neqD5_retBoolD7;
			ID_API_CALLBACK idYieldMicroseconds;
			ID_API_CALLBACK idDynamicDispatch;
			ID_API_CALLBACK idIsSameObject;
			ID_API_CALLBACK idIsDifferentObject;
			ID_API_CALLBACK idStringIndexToChar;
			ID_API_CALLBACK idTransformAt_D4D5retIExpressionBuilderD7;
			ID_API_CALLBACK idTransformParent_D4retIExpressionBuilderD7;
			ID_API_CALLBACK idInvokeMethodByName;
			ID_API_CALLBACK idVariableRefToType;
			ID_API_CALLBACK idJumpFromProxyToMethod;
		};

		struct MethodInfo
		{
			const Rococo::Compiler::IFunction* f;
			ptrdiff_t offset;
		};

		struct IIOSystem
		{
			virtual void Free() = 0;
		};

		ROCOCO_INTERFACE IScriptSystem : IPublicScriptSystem
		{
			virtual Compiler::IProgramObject& ProgramObject() = 0;

			virtual const CClassExpression* GetExpressionReflection(const Sex::ISExpression& s) = 0;
			virtual CScriptSystemClass* GetScriptSystemClass() = 0;
			virtual CReflectedClass* GetReflectedClass(void* ptr) = 0;
			virtual CReflectedClass* CreateReflectionClass(cstr className, void* context) = 0;
			virtual bool ConstructExpressionBuilder(CClassExpressionBuilder& builderContainer, Rococo::Sex::ISExpressionBuilder* builder) = 0;
			virtual const void* GetMethodMap() = 0;
			virtual int NextID() = 0;
			virtual const ScriptCallbacks& GetScriptCallbacks() = 0;
			virtual cstr GetPersistentString(cstr text, int textLength = -1) = 0;
			virtual const MethodInfo GetMethodByName(cstr methodName,  const Rococo::Compiler::IStructure& concreteClassType) = 0;
			
			virtual ID_API_CALLBACK GetIdSerializeCallback() const = 0;
			virtual ID_API_CALLBACK TryGetRawReflectionCallbackId(cstr functionId) const = 0;

			virtual Rococo::Sex::ISExpressionBuilder* CreateMacroTransform(Rococo::Sex::cr_sex src) = 0;
			virtual Rococo::Sex::ISExpressionBuilder* CreateMacroTransformClonedFromParent(Rococo::Sex::cr_sex sChild) = 0;
			virtual const  Rococo::Sex::ISExpression* GetTransform(Rococo::Sex::cr_sex src) = 0;
			virtual IIOSystem& IOSystem() = 0;

			virtual Compiler::IMemberLife* GetListLifetimeManager() = 0;
			virtual Compiler::IMemberLife* GetArrayLifetimeManager() = 0;
			virtual Compiler::IMemberLife* GetMapLifetimeManager() = 0;
		};

		ROCOCO_INTERFACE INativeLib
		{
			virtual void AddNativeCalls() = 0;
			virtual void ClearResources() = 0; // Used by coroutines lib
			virtual void Release() = 0;
		};

		typedef INativeLib* (*FN_CreateLib)(Rococo::Script::IScriptSystem& ss);

		ROCOCO_INTERFACE MemberEnumeratorCallback
		{
			virtual void OnListMember(IPublicScriptSystem& ss, cstr childName, const Rococo::Compiler::IMember& member, const ListImage* l, const Rococo::uint8* sfItem, int offset, int recurseDepth) = 0;
			virtual void OnMapMember(IPublicScriptSystem& ss, cstr childName, const Rococo::Compiler::IMember& member, const MapImage* m, const Rococo::uint8* sfItem, int offset, int recurseDepth) = 0;
			virtual void OnArrayMember(IPublicScriptSystem& ss, cstr childName, const Rococo::Compiler::IMember& member, const ArrayImage* array, const Rococo::uint8* sfItem, int offset, int recurseDepth) = 0;
			virtual void OnMember(IPublicScriptSystem& ss, cstr childName, const Rococo::Compiler::IMember& member, const Rococo::uint8* sfItem, int offset, int recurseDepth) = 0;
		};
}}

namespace Rococo::Variants
{
    inline VariantValue FromValue(int32 value)
    {
        VariantValue v;
        v.int32Value = value;
        return v;
    }

    inline VariantValue Zero() { return FromValue(0); }
    inline VariantValue ValueTrue() { return FromValue(1); }
    inline VariantValue ValueFalse() { return FromValue(0); }

    inline bool IsAssignableToBoolean(SexyVarType type)
    {
        return type == SexyVarType_Bool;
    }
}

#endif // SEXY_SCRIPT_H