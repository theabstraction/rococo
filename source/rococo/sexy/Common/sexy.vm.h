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

#ifndef SVM_H
#define SVM_H

#ifndef SEXY_H
#error include "sexy.types.h" before including this file
#endif


#ifdef _WIN32
# pragma once
# define IMPORT_FROM_DLL extern "C" __declspec(dllimport)
# define EXPORT_FROM_DLL extern "C" __declspec(dllexport)
# define WIN32_API __stdcall
#endif

#ifndef IN
# define IN
# define OUT
# define REF
#endif

#ifdef _WIN32
#  pragma pack(push, 1)
# endif

namespace Rococo { namespace VM
{
	enum { CORE_LIB_VERSION = 0x00010001 };
		
	typedef uint8 DINDEX;

	enum REGISTER
	{
		REGISTER_PC = 0,	
		REGISTER_SP = 1,
		REGISTER_SF = 2,
		REGISTER_SR = 3,
		REGISTER_D4 = 4,
		REGISTER_D5 = 5,
		REGISTER_D6 = 6,
		REGISTER_D7 = 7,
		REGISTER_D8 = 8,
		REGISTER_D9 = 9,
		REGISTER_D10 = 10,
		REGISTER_D11 = 11,
		REGISTER_D12 = 12,
		REGISTER_D13 = 13,
		REGISTER_D14 = 14,
		REGISTER_D15 = 15
	};

	enum FLOATSPEC
	{
		FLOATSPEC_SINGLE = 0,
		FLOATSPEC_DOUBLE = 1
	};

	struct CoreSpec
	{
		uint32 Version;
		uint32 SizeOfStruct;
		uint32 Reserved;
	};

	struct ICore;
	struct IVirtualMachine;
	struct IAssembler;
	struct IDisassembler;
	struct ISymbols;
	struct IProgramMemory;

	typedef void (CALLTYPE_C *FN_API_CALLBACK)(VariantValue* registers, void* context);

	ROCOCO_INTERFACE ICore
	{
		virtual void Free() = 0;
		virtual void Log(cstr text) = 0;
		virtual void SetLogger(ILog* logger) = 0;
		virtual IVirtualMachine* CreateVirtualMachine() = 0;
		virtual IProgramMemory* CreateProgramMemory(size_t capacity) = 0;
		virtual IAssembler* CreateAssembler() = 0;
		virtual IDisassembler* CreateDisassembler() = 0;
		virtual ISymbols* CreateSymbolTable() = 0;

		virtual cstr GetCallbackSymbolName(ID_API_CALLBACK id) = 0;
		virtual bool TryInvoke(ID_API_CALLBACK id, VariantValue* registers) = 0;
		virtual ID_API_CALLBACK RegisterCallback(FN_API_CALLBACK callback, void* context, cstr symbol) = 0;
		virtual void UnregisterCallback(ID_API_CALLBACK id) = 0;
	};

	struct CPU;

	typedef void (*FN_BREAKPOINT_CALLBACK)(void* context, size_t offset);

	struct StackTraceItem
	{
		const uint8* FunctionStart;
		const uint8* StackFrame;
		int Level;
	} TIGHTLY_PACKED;

	typedef void (*FN_OnStackTrace)(void* context, const StackTraceItem& item);

	struct IDebugger;

	ROCOCO_INTERFACE IStepCallback
	{
		virtual void OnStep(IDebugger& debugger) = 0;
		virtual void StepNext() = 0;
	};

	ROCOCO_INTERFACE IDebugger: IStepCallback
	{		
		virtual void ClearBreakpoint(size_t offset) = 0;
		
		virtual EXECUTERESULT Debug() = 0;
		virtual void EnumBreakpoints(FN_BREAKPOINT_CALLBACK fnCallback, void* context) = 0;
		virtual void GetStackTrace(FN_OnStackTrace fnCallback, void* context) = 0;
		virtual void SetBreakpoint(size_t offset) = 0;
		virtual void StepNextSymbol(ISymbols& symbols) = 0;
		virtual void StepInto(bool ignoreCallbacks = false) = 0;
		virtual void StepOver() = 0;
		virtual void StepOut() = 0;

		virtual void SetStepCallback(IStepCallback* callback) = 0;
	};

	struct ExecutionFlags
	{
		ExecutionFlags(): ThrowToQuit(false), RunProtected(true), CorrectSF(true) {  }
		ExecutionFlags(bool throwToQuit, bool runProtected, bool correctSF = true):
			ThrowToQuit(throwToQuit), RunProtected(runProtected), CorrectSF(correctSF) {}

		// We have two algorithms for execution in our VM, either use an exception to terminate, which yields a faster VM execution, but slower termination
		// or use a flag to terminate, which slows execution, as it requires a test after each instruction, but is fast to terminate.
		bool ThrowToQuit;
		
		
		// Captures memory exceptions generated by the virtual machine and sets the cpu.ExceptionCode when the function returns
		// If you handle your own exceptions, then you may need to use SetStatus to properly terminate the VM ready for the step by step debugger.
		bool RunProtected;
		
		// Sets SP to SF on execution
		bool CorrectSF;
	};

   ROCOCO_INTERFACE ITraceOutput
   {
      virtual void Report() = 0;
   };

   struct WaitArgs
   {
	   IVirtualMachine* vm;
	   int64 nextWakeTime;
   };

	ROCOCO_INTERFACE IVirtualMachine : IDebugger
	{
		virtual IVirtualMachine* Clone(CPU& _cpu) = 0;
		virtual CPU& Cpu() = 0;
		virtual ICore& Core() = 0;
		
		virtual void GetLastFlags(ExecutionFlags& flags) const = 0;

		virtual EXECUTERESULT Execute(const ExecutionFlags& ef, ITraceOutput* tracer = nullptr) = 0;
		virtual EXECUTERESULT ContinueExecution(const ExecutionFlags& ef, ITraceOutput* tracer = nullptr) = 0;

		virtual void NotifyWaitEvent(int64 nextWakeTime) = 0; // Potentially tell the OS to sleep for a few 
		virtual void SetWaitHandler(IEventCallback<WaitArgs>* waitHandler) = 0; // Handled by the calling envrionment to sleep for a few
		virtual void Pause() = 0;
		virtual void InitCpu() = 0;
		virtual void InitPC() = 0;
		virtual bool IsRunning() const = 0;
		virtual int ExitCode() const = 0;
		virtual void Free() = 0;
		virtual void SetGlobalData(const uint8* globalData, size_t nBytes) = 0;
		virtual void SetProgram(IProgramMemory* program) = 0;
		virtual void SetStackSize(size_t nBytes) = 0;
		virtual void SetStatus(EXECUTERESULT status) = 0;
		
		virtual EXECUTERESULT Status() const = 0;

		virtual EXECUTERESULT ExecuteFunction(ID_BYTECODE codeId) = 0;

		// The current implementation of this is slow. Consider a change so a stub is used to invoke the function.
		// The stub should have an exit OPCODE following the function invocation that terminates the exection of the virtual machine
		virtual void ExecuteFunctionUntilReturn(ID_BYTECODE codeId) = 0;
		virtual EXECUTERESULT ExecuteFunctionProtected(ID_BYTECODE codeId) = 0;

		virtual float PopFloat32() = 0;
		virtual void Push(float value) = 0;

		virtual double PopFloat64() = 0;
		virtual void Push(double value) = 0;

		virtual int32 PopInt32() = 0;
		virtual void Push(int32 value) = 0;		

		virtual int64 PopInt64() = 0;
		virtual void Push(int64 value) = 0;		

		virtual void* PopPointer() = 0;
		virtual void Push(void* ptr) = 0;

		virtual void PushBlob(void* ptr, int nBytes) = 0;
		virtual void YieldExecution() = 0;
		virtual void Throw() = 0;
	};

	class OutputStack
	{	
		const uint8* sp;

	public:
		OutputStack(const uint8* _sp): sp(_sp)
		{
		}

		template<class T> const T GetNext()
		{
			T* t = (T*) sp;
			sp += sizeof(T);
			return *t;
		}
	};

	ROCOCO_INTERFACE IRefCounted
	{
		virtual void AddRef() = 0;
		virtual void Release() = 0;
	};

	ROCOCO_INTERFACE IProgramMemory : public IRefCounted
	{
		virtual void Clear() = 0;
		virtual uint8* StartOfMemory() = 0;
		virtual uint8* EndOfMemory() = 0;
		virtual const uint8* StartOfMemory() const = 0;
		virtual const uint8* EndOfMemory() const = 0;

		virtual ID_BYTECODE AddBytecode() = 0;
		virtual void UnloadBytecode(ID_BYTECODE id) = 0;
		virtual bool UpdateBytecode(ID_BYTECODE id, const IAssembler& assember) = 0;
		virtual size_t GetFunctionAddress(ID_BYTECODE) const = 0;
		virtual size_t GetFunctionLength(ID_BYTECODE id) const = 0;
		virtual ID_BYTECODE GetFunctionContaingAddress(size_t pcOffset) const = 0;
	};

	ROCOCO_INTERFACE IAssemblerBuilder
	{
		virtual void Append_AddImmediate(DINDEX source,  BITCOUNT bits, DINDEX target, const VariantValue& v) = 0;
		virtual void Append_BooleanNot(DINDEX target) = 0;
		virtual void Append_CallById(ID_BYTECODE id) = 0;
		virtual void Append_CallByIdIndirect(DINDEX Di) = 0;
		virtual void Append_CallByRegister(DINDEX offsetRegister) = 0;
		virtual void Append_Call(int offsetFromPCstart) = 0;
		virtual void Append_CopySFVariable(int targetOffset, int sourceOffset, size_t nBytes) = 0;
		virtual void Append_CopyMemory(DINDEX Dtarget, DINDEX source, size_t nBytes) = 0;		
		virtual void Append_Exit(DINDEX registerContainingExitCode) = 0;
		virtual void Append_IncrementPtr(DINDEX DsourceAndTarget, int32 value) = 0;
		virtual void Append_IntAdd(DINDEX Da, BITCOUNT bits, DINDEX Db) = 0;
		virtual void Append_IntMultiply(DINDEX Da, BITCOUNT bits, DINDEX Db) = 0;
		virtual void Append_IntDivide(DINDEX Dnumerator, BITCOUNT bits, DINDEX Ddenominator) = 0;
		virtual void Append_FloatNegate32(DINDEX src) = 0;
		virtual void Append_FloatNegate64(DINDEX src) = 0;
		virtual void Append_IntNegate(DINDEX reg, BITCOUNT bits) = 0;
		virtual void Append_IntSubtract(DINDEX Da, BITCOUNT bits, DINDEX Db) = 0;
		virtual void Append_FloatAdd(DINDEX Da, DINDEX Db, FLOATSPEC spec) = 0;
		virtual void Append_FloatSubtract(DINDEX Da, DINDEX Db, FLOATSPEC spec) = 0;
		virtual void Append_FloatMultiply(DINDEX Da, DINDEX Db, FLOATSPEC spec) = 0;
		virtual void Append_FloatDivide(DINDEX Da, DINDEX Db, FLOATSPEC spec) = 0;
		virtual void Append_LogicalAND(DINDEX source,  BITCOUNT bits, DINDEX target) = 0;
		virtual void Append_LogicalOR(DINDEX source,  BITCOUNT bits, DINDEX target) = 0;
		virtual void Append_LogicalXOR(DINDEX source,  BITCOUNT bits, DINDEX target) = 0;
		virtual void Append_LogicalNOT(DINDEX source,  BITCOUNT bits, DINDEX target) = 0;
		virtual void Append_MoveRegister(DINDEX source, DINDEX target, BITCOUNT bits) = 0;
		virtual void Append_Invoke(ID_API_CALLBACK callback) = 0;
		virtual void Append_InvokeBy(DINDEX index) = 0;
		virtual void Append_NoOperation() = 0;
		virtual void Append_PushAddress(DINDEX source, DINDEX offsetRegister) = 0;
		virtual void Append_Poke(DINDEX source, BITCOUNT bits, DINDEX target, int32 offset) = 0;
		virtual void Append_Pop(uint8 nBytes) = 0;
		virtual void Append_PushIndirect(DINDEX source, DINDEX target, size_t nBytes) = 0;
		virtual void Append_PushRegister(DINDEX source, BITCOUNT bits) = 0;
		virtual void Append_PushLiteral(BITCOUNT bits, const VariantValue& value) = 0;
		virtual void Append_PushStackVariable(int sfOffset, BITCOUNT bitcount) = 0;
		virtual void Append_PushStackFrameMemberPtr(int sfOffsetToStruct, int memberOffset) = 0;
		virtual void Append_PushStackFrameAddress(int offset) = 0;
		virtual void Append_Return() = 0;
		virtual void Append_RestoreRegister(DINDEX Di, BITCOUNT bits) = 0;
		virtual void Append_Yield() = 0;
		virtual void Append_SaveRegister(DINDEX Di, BITCOUNT bits) = 0;
		virtual void Append_SetRegisterImmediate(DINDEX Di, const VariantValue& v, BITCOUNT bits) = 0;
		virtual void Append_SetStackFrameImmediate(int32 offset, const VariantValue& v, BITCOUNT bits) = 0;
		virtual void Append_GetStackFrameValue(int32 offset, DINDEX Dtarget, BITCOUNT bits) = 0;
		virtual void Append_GetStackFrameValueAndExtendToPointer(int32 offset, DINDEX target) = 0;
		virtual void Append_SetStackFrameValue(int32 offset, DINDEX Dsource, BITCOUNT bits) = 0;
		virtual void Append_SetSFValueFromSFValue(int32 targetOffset, int32 srcOffset, BITCOUNT bits) = 0;
		virtual void Append_ShiftLeft(DINDEX Di, BITCOUNT bitCount, int8 shiftCount) = 0;
		virtual void Append_ShiftRight(DINDEX Di, BITCOUNT bitCount, int8 shiftCount) = 0;
		virtual void Append_StackAlloc(int32 nBytes) = 0;
		virtual void Append_SubtractImmediate(DINDEX source,  BITCOUNT bits, DINDEX target, const VariantValue& v) = 0;

		virtual void Append_Test(DINDEX Di, BITCOUNT bits) = 0;				
		virtual void Append_Branch(int PCoffset) = 0;
		virtual void Append_BranchIf(CONDITION cse, int PCoffset) = 0;		
		virtual void Append_SetIf(CONDITION cse, DINDEX Di, BITCOUNT bits) = 0;
		virtual void Append_SwapRegister(DINDEX a, DINDEX b) = 0;
		virtual void Append_GetStackFrameMemberPtr(DINDEX Dtarget, int SFoffset, int memberOffset) = 0;
		virtual void Append_GetStackFrameMemberPtrAndDeref(DINDEX Dtarget, int SFoffset, int memberOffset) = 0;
		virtual void Append_GetStackFrameMember(DINDEX Dtarget, int SFoffset, int memberOffset, BITCOUNT bits) = 0;
		virtual void Append_GetStackFrameAddress(DINDEX Dtarget, int offset) = 0;
		virtual void Append_CallVirtualFunctionViaRefOnStack(int32 SFoffsetToInterface, int32 vTableOffset, int32 instanceToInterfaceOffset = 0) = 0;
		virtual void Append_CallVirtualFunctionViaMemberOffsetOnStack(int32 SFoffsetToInterface, int32 memberOffset, int32 vTableOffset) = 0;
		virtual void Append_CallVirtualFunctionByAddress(int32 SFoffsetToInterfaceValue, int32 vTableOffset) = 0;
		virtual void Append_Dereference_D4() = 0;

		virtual void Append_SetSFMemberRefFromSFValue(int32 targetSFOffset, int32 targetMemberOffset, int32 SFSourceValueOffset, size_t nBytesSource) = 0;
		virtual void Append_SetSFValueFromSFMemberRef(int32 sourceSFOffset, int32 sourceMemberOffset, int32 SFTargetValueOffset, size_t nBytesSource) = 0;

		struct Args_SetMemberRefFromSFMemberByRef
		{
			int8 opcode;
			int32 sourceSFOffset;
			int32 sourceMemberOffset;
			int32 targetSFOffset;
			int32 targetMemberOffset;
		};

		virtual void Append_SetStackFramePtrFromD5(int SFoffset, int memberOffset) = 0;
		virtual void Append_SetSFMemberRefFromSFMemberByRef(int32 sourceSFOffset, int32 sourceMemberOffset, int32 targetSFOffset, int32 targetMemberOffset, size_t nBytesSource) = 0;
		virtual void Append_CopySFVariableFromRef(int32 targetSFOffset, int32 sourceSFOffset, int32 sourceMemberOffset, size_t nBytesSource) = 0;
		virtual void Append_SetSFMemberByRefFromRegister(DINDEX Dsource, int32 sfOffset, int32 memberOffset, BITCOUNT bitcount) = 0;
		virtual void Append_TripDebugger() = 0;

		virtual void Append_GetGlobal(BITCOUNT bits, int32 offset) = 0;
		virtual void Append_SetGlobal(BITCOUNT bits, int32 offset) = 0;
	};

	ROCOCO_INTERFACE IAssembler : IAssemblerBuilder
	{
		virtual void Clear() = 0; // Clear the program
		virtual ICore& Core() = 0; // Retrieve the SVM core object
		virtual void Free() = 0; // Deconstruct and free memory associated with this instance
		virtual const unsigned char* Program(OUT size_t& length) const = 0; // return program and length
		virtual size_t WritePosition() const = 0; // Returns the write cursor for the next instructions. 
		virtual void SetWriteModeToOverwrite(size_t position) = 0; // Set the append point. Used for overwriting placeholders, such as branches. Cannot grow the program in this mode
		virtual void SetWriteModeToAppend() = 0; // Set the assemble to append assembled instructions at the end of the current generated program, growing it.
		virtual void Revert(size_t position) = 0; // Drop everything after the position, and set the write position to position, and set write mode to append
	};

	ROCOCO_INTERFACE ISourceFile
	{
		virtual const char* FileName() const = 0;
		virtual size_t GetCodeOffset(const Vec2i& pos) const = 0;
	};

	struct FileData
	{
		const ISourceFile* Source;
		Vec2i Pos;				
	} TIGHTLY_PACKED;

	ROCOCO_INTERFACE ISymbols
	{
		virtual void Clear() = 0;
		virtual void Free() = 0;
		virtual ISourceFile* AddSourceImage(const char* sourceName, const char* sourceCode, size_t codeLenBytes) = 0;
		virtual void Add(size_t index, const FileData& fp) = 0;
		virtual bool TryGetSymbol(size_t index, OUT FileData& fd) const = 0;
	};
	
	ROCOCO_INTERFACE IDisassembler
	{
		enum {MAX_ARG_LEN = 64, MAX_FULL_TEXT_LEN = 128};

		struct Arg
		{
			char ArgText[MAX_ARG_LEN];
		};

		struct Rep
		{
			uint32 ByteCount;
			char OpcodeText[MAX_ARG_LEN];			
			uint32 NumberOfArgs;
			Arg Args[6];			
			char ArgText[MAX_FULL_TEXT_LEN];
		};

		virtual void Disassemble(const unsigned char* PC, OUT Rep& rep) = 0;
		virtual void Free() = 0;
	};

	class InvalidInstructionException
	{
	public:
		int reserved;
	};

	typedef Rococo::VM::ICore* (CALLTYPE_C *FN_CreateSVMCore)(const Rococo::VM::CoreSpec* spec);
}}

// #define IS_VM_DLL 1

# ifdef SVM_CORE_MODULE
#  ifdef IS_VM_DLL
#    define SVMLIB __declspec(dllexport) 
#  else
#    define SVMLIB
#  endif
# else
#  ifdef IS_VM_DLL
#    define SVMLIB __declspec(dllimport)
#  else
#    define SVMLIB
#  endif
  extern "C" SVMLIB Rococo::VM::ICore* CreateSVMCore(const Rococo::VM::CoreSpec* spec); 
# endif

# ifdef _WIN32
#  pragma pack(pop)
# endif

#endif