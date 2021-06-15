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

#include "sexy.vm.stdafx.h"
#include "sexy.vm.cpu.h"
#include <math.h>
#include "sexy.vm.os.h"
#include <rococo.api.h>

#include <rococo.allocators.h> // provides _aligned_malloc

#ifdef _WIN32
# include <excpt.h>
#else
# include <stdlib.h> // for posix_memalign and free
#endif

#ifdef _WIN32
# define PROTECT  __try
# define CATCH  __except(1)
#else
namespace Rococo
{
   struct SignalException
   {
      int dummy;
   };
}
# define PROTECT try
# define CATCH  catch(SignalException& ex)
#endif

using namespace Rococo;
using namespace Rococo::VM;
using namespace Rococo::Memory;

#define MERGE_TOKENS(a, b) a##b
#define FoobarActivateInstruction(x) s_instructionTable[MERGE_TOKENS(Opcodes::,x) ] = &CVirtualMachine::MERGE_TOKENS(OnOpcode,x)
#define ActivateInstruction(x) s_instructionTable[Opcodes::x] = &CVirtualMachine::MERGE_TOKENS(OnOpcode,x)
#define OPCODE_CALLBACK_CONVENTION // Since we switched to 64-bit, there is no special fastcall
#define OPCODE_CALLBACK(x) void OPCODE_CALLBACK_CONVENTION OnOpcode##x()

static_assert(sizeof(Rococo::VariantValue) == 8, "The codebase assumes that sizeof(Rococo::VariantValue) is 8. Major changes are needed if this is not so.");

namespace Anon
{
	class TerminateException
	{
	private:
		uint32 Reserved;
	};

	class IllegalException
	{
	private:
		uint32 Reserved;
	};

	class YieldException
	{
	private:
		uint32 reserved;
	};

	typedef std::vector<unsigned char> TMemory;

	bool IsOpcodeCall(Opcodes::OPCODE opcode)
	{
		switch(opcode)
		{
		case Opcodes::Call:
		case Opcodes::CallBy:
		case Opcodes::CallById:
		case Opcodes::CallByIdIndirect:
		case Opcodes::CallVirtualFunctionByAddress:
		case Opcodes::CallVitualFunctionViaRefOnStack:
			return true;
		default:
			return false;
		}
	}

	class CVirtualMachine final
		: public IVirtualMachine, public IStepCallback
	{
	private:	
		CPU& cpu;
		uint8* stack{ nullptr };
		size_t stackSize{ 0 };

		volatile EXECUTERESULT status;
		int exitCode{ 0 };
		bool throwToQuit{ true };
		ICore& core;

		IProgramMemory* program{ nullptr };

		typedef void (OPCODE_CALLBACK_CONVENTION CVirtualMachine::*FN_VM)();

		static FN_VM* s_instructionTable; 

		int isBeingStepped{ 0 };

		IStepCallback* stepCallback;

		std::vector<uint8> globalData;

		TMemory breakpoints;

	public:
		CVirtualMachine(ICore& _core, CPU& _cpu) :
			core(_core), cpu(_cpu)
		{
			SetStackSize(64 * 1024);

			stepCallback = this;

			if (s_instructionTable != NULL) return;

			s_instructionTable = new FN_VM[Opcodes::MAX_OPCODES];
			for (uint32 i = 0; i < Opcodes::MAX_OPCODES; ++i)
			{
				s_instructionTable[i] = &CVirtualMachine::OnOpcodeBadInstruction;
			}

			ActivateInstruction(AddQuick32);
			ActivateInstruction(Increment32);
			ActivateInstruction(Decrement32);

			ActivateInstruction(SetRegisterImmediate64);
			ActivateInstruction(SetRegisterImmediate32);

			ActivateInstruction(StackAllocBig);
			ActivateInstruction(StackAlloc);

			ActivateInstruction(Swap);

			ActivateInstruction(BooleanNot);
			ActivateInstruction(SetSFValueFromSFValue32);
			ActivateInstruction(Test32);
			ActivateInstruction(BranchIfGTE);
			ActivateInstruction(BranchIfGT);
			ActivateInstruction(BranchIfLT);
			ActivateInstruction(BranchIfNE);
			ActivateInstruction(Test64);
			ActivateInstruction(Branch);
			ActivateInstruction(BranchIf);
			ActivateInstruction(SetIf32);
			ActivateInstruction(SetIf64);
			ActivateInstruction(Move32);
			ActivateInstruction(Move64);
			ActivateInstruction(Poke64);
			ActivateInstruction(Poke32);
			ActivateInstruction(Call);
			ActivateInstruction(CallBy);
			ActivateInstruction(CallById);
			ActivateInstruction(CallByIdIndirect);
			ActivateInstruction(CallVitualFunctionViaRefOnStack);
			ActivateInstruction(CallVitualFunctionViaMemberOffsetOnStack);
			ActivateInstruction(CallVirtualFunctionByAddress);
			ActivateInstruction(CopySFMemory);
			ActivateInstruction(CopySFMemoryNear);
			ActivateInstruction(CopyMemory);
			ActivateInstruction(CopyMemoryBig);
			ActivateInstruction(Copy32Bits);
			ActivateInstruction(Copy64Bits);
			ActivateInstruction(CopySFVariableFromRef);
			ActivateInstruction(Invoke);
			ActivateInstruction(InvokeBy);
			ActivateInstruction(Return);
			ActivateInstruction(Yield);
			ActivateInstruction(PushIndirect);
			ActivateInstruction(PushRegister64);
			ActivateInstruction(PushRegister32);
			ActivateInstruction(PushAddress);
			ActivateInstruction(PushImmediate32);
			ActivateInstruction(PushImmediate64);
			ActivateInstruction(PushStackVariable32);
			ActivateInstruction(PushStackVariable64);
			ActivateInstruction(PushStackFrameMemberPtr);
			ActivateInstruction(PushStackAddress);
			ActivateInstruction(Pop);
			ActivateInstruction(AddImmediate);

			ActivateInstruction(LogicalAND32);
			ActivateInstruction(LogicalOR32);
			ActivateInstruction(LogicalXOR32);
			ActivateInstruction(LogicalNOT32);
			ActivateInstruction(LogicalAND64);
			ActivateInstruction(LogicalOR64);
			ActivateInstruction(LogicalXOR64);
			ActivateInstruction(LogicalNOT64);

			ActivateInstruction(ShiftLeft32);
			ActivateInstruction(ShiftLeft64);
			ActivateInstruction(ShiftRight32);
			ActivateInstruction(ShiftRight64);

			ActivateInstruction(IntAdd64);
			ActivateInstruction(IntAdd32);
			ActivateInstruction(IntSubtract64);
			ActivateInstruction(IntSubtract32);
			ActivateInstruction(IntMultiply64);
			ActivateInstruction(IntMultiply32);
			ActivateInstruction(IntDivide64);
			ActivateInstruction(IntDivide32);
			ActivateInstruction(IntNegate64);
			ActivateInstruction(IntNegate32);

			ActivateInstruction(FloatAdd);
			ActivateInstruction(FloatSubtract);
			ActivateInstruction(FloatMultiply);
			ActivateInstruction(FloatDivide);

			ActivateInstruction(DereferenceD4);

			ActivateInstruction(DoubleAdd);
			ActivateInstruction(DoubleSubtract);
			ActivateInstruction(DoubleMultiply);
			ActivateInstruction(DoubleDivide);

			ActivateInstruction(IncrementPtr);
			ActivateInstruction(IncrementPtrBig);

			ActivateInstruction(Exit);
			ActivateInstruction(NoOperation);

			ActivateInstruction(SetStackFrameImmediate32);
			ActivateInstruction(SetStackFrameImmediate64);
			ActivateInstruction(SetStackFrameImmediateFar);

			ActivateInstruction(GetStackFrameValue32);
			ActivateInstruction(GetStackFrameValue64);
			ActivateInstruction(GetStackFrameValueFar);
			ActivateInstruction(GetStackFrameMemberPtr);
			ActivateInstruction(GetStackFrameMemberPtrFar);
			ActivateInstruction(GetStackFrameAddress);
			ActivateInstruction(GetStackFrameMember32);
			ActivateInstruction(GetStackFrameMember64);

			ActivateInstruction(SetStackFrameValue32);
			ActivateInstruction(SetStackFrameValue64);
			ActivateInstruction(SetStackFrameValueFar);
			ActivateInstruction(SetSFMemberByRefFromSFByValue32);
			ActivateInstruction(SetSFMemberByRefFromSFByValue64);
			ActivateInstruction(SetSFValueFromSFMemberByRef32);
			ActivateInstruction(SetSFMemberByRefFromRegister32);
			ActivateInstruction(SetSFMemberByRefFromRegisterLong);

			ActivateInstruction(RestoreRegister32);
			ActivateInstruction(SaveRegister32);
			ActivateInstruction(RestoreRegister64);
			ActivateInstruction(SaveRegister64);
			ActivateInstruction(Debug);

			ActivateInstruction(GetGlobal);
			ActivateInstruction(SetGlobal);

			ActivateInstruction(GetStackFrameValueAndExtendToPointer);
			ActivateInstruction(GetStackFrameMemberPtrAndDeref);

			ActivateInstruction(SetSFValueFromSFValueLong);
			ActivateInstruction(SetSFMemberRefFromSFValue);

			ActivateInstruction(SetSFValueFromSFMemberByRef);

			ActivateInstruction(SetSFMemberPtrFromD5);

			static_assert(sizeof(VariantValue) == sizeof(size_t), "Bad packing size");
			static_assert(BITCOUNT_POINTER == sizeof(size_t) * 8, "Bad BITCOUNT_POINTER");
		}

		IVirtualMachine* Clone(CPU& _cpu) override
		{
			/*
			auto clone = new CVirtualMachine(core, _cpu);
			if (!globalData.empty()) clone->SetGlobalData(&globalData[0], globalData.size());

			clone->SetStackSize(stackSize);
			clone->SetProgram(program);
			clone->InitCpu();

			clone->cpu.D[VM::REGISTER_D5] = cpu.D[VM::REGISTER_D5];
			return clone; 
			*/

			return nullptr; // Currently does not work with JIT compilation. May work later on when we allow total compilation
		}

		~CVirtualMachine()
		{
			if (program != NULL) program->Release();
			SetStackSize(0);
		}

		void OnStep(IDebugger& debugger) override
		{
		}

		void StepNext() override
		{
		}

		void SetStepCallback(IStepCallback* stepCallback) override
		{
			this->stepCallback = stepCallback != NULL ? stepCallback : this;
		}

		void Pause() override
		{
			if (status == EXECUTERESULT_RUNNING) status = EXECUTERESULT_YIELDED;
		}

		bool IsRunning() const override
		{
			return status == EXECUTERESULT_RUNNING;
		}

		void ClearBreakpoint(size_t offset) override
		{
			if (offset < breakpoints.size())
			{
				breakpoints[offset] = 0;
			}
		}

		void SetBreakpoint(size_t offset) override
		{
			if (offset < breakpoints.size())
			{
				breakpoints[offset] = 1;
			}
		}

		bool IsAtBreakpoint()
		{
			size_t offset = cpu.PC() - cpu.ProgramStart;
			return breakpoints[offset] != 0;
		}

		EXECUTERESULT Debug() override
		{
			struct ANON
			{
				static EXECUTERESULT ProtectedDebug(void* context)
				{
					CVirtualMachine* vm = (CVirtualMachine*) context;

					while(vm->status == EXECUTERESULT_RUNNING && !vm->IsAtBreakpoint())
					{
						vm->Advance();
					}

					return vm->status;
				}
			};

			status = EXECUTERESULT_RUNNING;
			return VM::OS::ExecuteProtected(*this, ANON::ProtectedDebug, this, OUT cpu.ExceptionCode);
		}

		void Throw() override
		{
			status = EXECUTERESULT_THROWN;
			TerminateByIllegal(0);
		}

		void GetStackTrace(FN_OnStackTrace fnCallback, void* context) override
		{
			uint8* SFlevel = (uint8*) cpu.SF();
			uint8* PClevel = (uint8*) cpu.PC();

			int level = 0;

			do 
			{
				StackTraceItem item;
				item.FunctionStart = PClevel;
				item.StackFrame = SFlevel;
				item.Level = level++;
				fnCallback(context, item);

				uint8** pRetAddr = (uint8**)(SFlevel - sizeof(void*));
				uint8** pLowerSF = (uint8**)(SFlevel - 2 * sizeof(void*));

				PClevel = *pRetAddr;
				if (PClevel < cpu.ProgramStart || PClevel >= cpu.ProgramEnd)
				{
					PClevel = NULL; // Although the PC was rubbish, we can still go back through the stack crawling for valid stack frames
				}

				if (*pLowerSF == NULL && PClevel != NULL)
				{
					// This indicates the entry point for the program
					item.FunctionStart = PClevel;
					item.StackFrame = NULL;
					item.Level = level++;
					fnCallback(context, item);
					break;
				}

				if (*pLowerSF == NULL || *pLowerSF >= SFlevel || *pLowerSF < cpu.StackStart)
				{
					break;
				}
			} while (true);
		}

		void EnumBreakpoints(FN_BREAKPOINT_CALLBACK fnCallback, void* context) override
		{		
			for(size_t i = 0; i < breakpoints.size(); ++i)
			{
				if (breakpoints[i] != 0)
				{
					fnCallback(context, i);
				}
			}
		}

      ITraceOutput* currentTracer = nullptr;

      struct TraceContext
      {
         ITraceOutput* previous;
         CVirtualMachine& vm;

         TraceContext(CVirtualMachine& _vm, ITraceOutput* tracer): vm(_vm)
         {
            previous = vm.currentTracer;
            vm.currentTracer = tracer;
         }

         ~TraceContext()
         {
            vm.currentTracer = previous;
         }
      };

		ExecutionFlags currentFlags{ false, false, false };
		void GetLastFlags(ExecutionFlags& flags) const override
		{
			flags = currentFlags;
		}

		EXECUTERESULT Continue(const ExecutionFlags& ef, ITraceOutput* tracer)
		{
			if (status == EXECUTERESULT_YIELDED) status = EXECUTERESULT_RUNNING;

			currentFlags = ef;

			TraceContext tc(*this, tracer);

			if (ef.RunProtected)
			{
				struct ANON
				{
					static EXECUTERESULT ProtectedContinue(void* context, bool throwToQuit)
					{
						CVirtualMachine* vm = (CVirtualMachine*)context;
						return vm->ProtectedContinue(throwToQuit);
					}
				};

				return VM::OS::ExecuteProtected(*this, ANON::ProtectedContinue, this, OUT cpu.ExceptionCode, ef.ThrowToQuit);
			}
			else
			{
				cpu.ExceptionCode = EXCEPTIONCODE_DISABLED;
				return ProtectedContinue(ef.ThrowToQuit);
			}
		}

		void InitCpu() override
		{
			cpu.StackStart = stack;
			cpu.StackEnd = stack + stackSize;
			cpu.D[REGISTER_SP].uint8PtrValue = cpu.StackStart;
			cpu.D[REGISTER_PC].uint8PtrValue = cpu.ProgramStart;
			cpu.D[REGISTER_SF].uint8PtrValue = NULL;
			cpu.ExceptionCode = EXCEPTIONCODE_NONE;
			cpu.Globals = globalData.empty() ? nullptr : &globalData[0];

			exitCode = 0;
			status = EXECUTERESULT_RUNNING;

			cpu.Push(NULL); // fake previous SF
			cpu.Push(NULL); // fake return address
		}

		void InitPC() override
		{
			cpu.D[REGISTER_PC].uint8PtrValue = cpu.ProgramStart;
		}

		EXECUTERESULT ExecuteFunction(ID_BYTECODE codeId) override
		{
			const uint8* context = cpu.SF();
			cpu.Push(context);

			const uint8 *returnAddress = cpu.PC(); 
			cpu.Push(returnAddress);

			// Then make the new stack frame equal to the stack pointer
			cpu.D[REGISTER_SF].charPtrValue = cpu.D[REGISTER_SP].charPtrValue;

			size_t functionStart = program->GetFunctionAddress(codeId);
			cpu.SetPC(cpu.ProgramStart + functionStart);

			PROTECT
			{
				while(cpu.SF() > context && status == EXECUTERESULT_RUNNING)
				{
					Advance();
				}
			}
			CATCH
			{
				return EXECUTERESULT_SEH;
			}

			if (status == EXECUTERESULT_RUNNING) status = EXECUTERESULT_RETURNED;

			return status;
		}

		void ExecuteFunctionStepped(ID_BYTECODE codeId)
		{
			const uint8* context = cpu.SF();
			cpu.Push(context);

			const uint8 *returnAddress = cpu.PC(); 
			cpu.Push(returnAddress);

			// Then make the new stack frame equal to the stack pointer
			cpu.D[REGISTER_SF].charPtrValue = cpu.D[REGISTER_SP].charPtrValue;

			size_t functionStart = program->GetFunctionAddress(codeId);
			cpu.SetPC(cpu.ProgramStart + functionStart);

			while(cpu.SF() > context && status == EXECUTERESULT_RUNNING)
			{
				Advance();
				stepCallback->OnStep(*this);
			}
		}

		virtual EXECUTERESULT ExecuteFunctionProtected(ID_BYTECODE codeId) override
		{
			try
			{
				status = EXECUTERESULT_RUNNING;
				if (!isBeingStepped)
				{
					ExecuteFunction(codeId);
				}
				else
				{
					ExecuteFunctionStepped(codeId);
				}
				
				return status;
			}
			catch (TerminateException&)
			{	
			}
			catch(YieldException&)
			{
			}
			catch(IllegalException&)
			{
				return EXECUTERESULT_ILLEGAL;
			}

			return status;
		}

		void Terminate(int exitCode)
		{
			this->exitCode = exitCode;
			this->status = EXECUTERESULT_TERMINATED;

			if (throwToQuit) throw TerminateException();
		}

		void TerminateByIllegal(int exitCode)
		{
			Rococo::OS::BreakOnThrow(Rococo::OS::BreakFlag_VM);
			if (this->status == EXECUTERESULT_RUNNING) this->status = EXECUTERESULT_TERMINATED;
			this->exitCode = exitCode;
			if (throwToQuit) throw IllegalException();
		}

		void YieldExecution() override
		{
			status = EXECUTERESULT_YIELDED;
			if (throwToQuit) throw YieldException();
		}

		EXECUTERESULT ContinueExecution(const ExecutionFlags& ef, ITraceOutput* tracer) override
		{
			return Continue(ef, tracer);
		}

		void StepNextSymbol(ISymbols& symbols) override
		{
			struct ANON
			{
				static EXECUTERESULT ProtectedAdvance(void* context)
				{
					CVirtualMachine* vm = (CVirtualMachine*) context;
					vm->Advance();
					return EXECUTERESULT_YIELDED;
				}
			};

			if (status == EXECUTERESULT_YIELDED || status == EXECUTERESULT_BREAKPOINT) 
				status = EXECUTERESULT_RUNNING;

			if (status == EXECUTERESULT_RUNNING) return;

			VM::FileData start;
			if (!symbols.TryGetSymbol(cpu.PC() - cpu.ProgramStart, OUT start))
			{
				StepInto(true);
				return;
			}

			if (status == EXECUTERESULT_YIELDED || status == EXECUTERESULT_BREAKPOINT) 
				status = EXECUTERESULT_RUNNING;

			while(status == EXECUTERESULT_RUNNING)
			{
				VM::FileData fd;
				if (symbols.TryGetSymbol(cpu.PC() - cpu.ProgramStart, OUT fd))
				{
					if (fd.Source != start.Source || fd.Pos.x != start.Pos.x || fd.Pos.y != start.Pos.y)
					{
						break;
					}
				}

				status = VM::OS::ExecuteProtected(*this, ANON::ProtectedAdvance, this, OUT cpu.ExceptionCode);
			}
		}

		void StepInto(bool ignoreCallbacks) override
		{
			if (!ignoreCallbacks && isBeingStepped)
			{
				stepCallback->StepNext();
				return;
			}

			struct ANON
			{
				static EXECUTERESULT ProtectedAdvance(void* context)
				{
					CVirtualMachine* vm = (CVirtualMachine*) context;
					vm->Advance();
					return EXECUTERESULT_YIELDED;
				}
			};

			if (status == EXECUTERESULT_YIELDED || status == EXECUTERESULT_BREAKPOINT) 
				status = EXECUTERESULT_RUNNING;

			if (status == EXECUTERESULT_RUNNING)
			{
				isBeingStepped++;
				status = VM::OS::ExecuteProtected(*this, ANON::ProtectedAdvance, this, OUT cpu.ExceptionCode);
				isBeingStepped--;
			}
		}

		void StepOver() override
		{
			const Ins* I = NextInstruction();
			if (!IsOpcodeCall((Opcodes::OPCODE) I->Opcode))
			{
				StepInto(true);
				return;
			}

			struct ANON
			{
				static EXECUTERESULT ProtectedRunUntilReturn(void* context)
				{
					CVirtualMachine* vm = (CVirtualMachine*) context;
					CPU& cpu = vm->Cpu();

					void* targetSP = cpu.D[REGISTER_SP].vPtrValue;

					vm->StepInto(true); // This should increase the targetSP as the function call dumped the return address onto the stack

					if (vm->status == EXECUTERESULT_YIELDED)
					{
						vm->status = EXECUTERESULT_RUNNING;
					}

					while(cpu.D[REGISTER_SP].vPtrValue > targetSP && vm->status == EXECUTERESULT_RUNNING)
					{
						vm->Advance();
					}

					return EXECUTERESULT_YIELDED;
				}
			};

			if (status == EXECUTERESULT_YIELDED || status == EXECUTERESULT_BREAKPOINT) 
				status = EXECUTERESULT_RUNNING;

			if (status == EXECUTERESULT_RUNNING)
			{
				status = VM::OS::ExecuteProtected(*this, ANON::ProtectedRunUntilReturn, this, OUT cpu.ExceptionCode);
			}
		}

		void StepOut() override
		{
			struct ANON
			{
				static EXECUTERESULT ProtectedStepOut(void* context)
				{
					CVirtualMachine* vm = (CVirtualMachine*) context;

					void* targetSF = vm->cpu.D[REGISTER_SF].vPtrValue;
					while(vm->cpu.D[REGISTER_SF].vPtrValue >= targetSF)
					{						
						vm->Advance();

						if (vm->status != EXECUTERESULT_RUNNING)
						{
							return vm->status;
						}						
					}

					return EXECUTERESULT_YIELDED;
				}
			};

			if (status == EXECUTERESULT_YIELDED || status == EXECUTERESULT_BREAKPOINT) 
				status = EXECUTERESULT_RUNNING;

			if (status == EXECUTERESULT_RUNNING)
			{
				status = VM::OS::ExecuteProtected(*this, ANON::ProtectedStepOut, this, OUT cpu.ExceptionCode);
			}
		}

		CPU& Cpu()  override { return cpu; }
		int ExitCode() const override  { return exitCode; }
		ICore& Core() override { return core;	}

		void SetStackSize(size_t nBytes) override
		{ 
			if (stack != NULL)
			{
				VM::OS::FreeAlignedMemory(stack, stackSize);
				stack = NULL;
			}
			
			stackSize = nBytes;

			if (stack == NULL && stackSize > 0)
			{
				stack = (uint8*) VM::OS::AllocAlignedMemory(stackSize, 2048); 
			}

			// N.B in 64-mode we need to convert parentSF to and from 32-bit uint32. This can only work if the maximum stack value is less than 32-bits
			enum { MAX_STACK_POINTER = 0x20000000 };
			const uint8* maxPtr = (const uint8*) MAX_STACK_POINTER;
			if (false && stack + nBytes > maxPtr)
			{
				VM::OS::FreeAlignedMemory(stack, stackSize);
            stack = NULL;
				Rococo::Throw(0, ("The SexyVM stack end %p exceeded the maximum pointer value of %p"), stack, maxPtr);
			}
		}

		void SetGlobalData(const uint8* globalData, size_t nBytes) override
		{
			this->globalData.resize(nBytes);
			memcpy(&this->globalData[0], globalData, nBytes);
		}

		IEventCallback<WaitArgs>* waitHandler = nullptr;

		void SetWaitHandler(IEventCallback<WaitArgs>* waitHandler) override
		{
			this->waitHandler = waitHandler;
		}

		void NotifyWaitEvent(int64 nextWaitTime) override
		{
			if (waitHandler)
			{
				WaitArgs args { this, nextWaitTime };
				waitHandler->OnEvent(args);
			}
		}

		void SetStatus(EXECUTERESULT status) override
		{
			this->status = status;
		}

		EXECUTERESULT Status() const override
		{
			return status;
		}

		inline const Ins* NextInstruction()
		{
			return reinterpret_cast<const Ins*>(cpu.PC());
		}

		void SetProgram(IProgramMemory* program) override
		{
			if (program == this->program)
			{
				return;
			}

			if (this->program != NULL)
			{
				this->program->Release();
			}

			if (program != NULL)
			{
				program->AddRef();
			}

			this->program = program;

			
			cpu.ProgramStart = program == NULL ? NULL : program->StartOfMemory();
			cpu.ProgramEnd = program == NULL ? NULL : program->EndOfMemory();

			breakpoints.resize(cpu.ProgramEnd - cpu.ProgramStart);
			memset(&breakpoints[0], 0, cpu.ProgramEnd - cpu.ProgramStart);
		}

		EXECUTERESULT Execute(const ExecutionFlags& ef, ITraceOutput* tracer) override
		{
			if (program == NULL)
			{
				exitCode = -1;
				return EXECUTERESULT_NO_PROGRAM;
			}

			// Args were pushed onto the stack prior to the execution
			if (ef.CorrectSF && cpu.SF() < cpu.SP())
			{
				cpu.SetSF(cpu.SP());
			}

			return Continue(ef, tracer);
		}
		
		void Advance()
		{
			const uint8* pc = cpu.PC();
			Opcodes::OPCODE i = (Opcodes::OPCODE) *pc;
			FN_VM fn = s_instructionTable[i];
			(this->*fn)();
		}

      EXECUTERESULT ProtectedContinueAndTrace(ITraceOutput& tracer)
      {
         this->throwToQuit = false;

         tracer.Report();

         while (status == EXECUTERESULT_RUNNING)
         {
            const uint8* pc = cpu.PC();
            Opcodes::OPCODE i = (Opcodes::OPCODE) *pc;
            FN_VM fn = s_instructionTable[i];
            (this->*fn)();

            tracer.Report();
         }

         return status;
      }

		EXECUTERESULT ProtectedContinue(bool throwToQuit)
		{
			status = EXECUTERESULT_RUNNING;

         if (currentTracer)
         {
            return ProtectedContinueAndTrace(*currentTracer);
         }

         this->throwToQuit = throwToQuit;
			
			if (throwToQuit)
			{
				try
				{	
					for(;;)
					{
						Advance();
					}
				}
				catch (TerminateException&)
				{	
				}
				catch(YieldException&)
				{
				}
				catch(IllegalException&)
				{
					return EXECUTERESULT_ILLEGAL;
				}
			}
			else
			{
				while(status == EXECUTERESULT_RUNNING)
				{
					Advance();
				}
			}
			
			return status;
		}

		OPCODE_CALLBACK(SetStackFrameImmediate32)
		{
			const Ins* I = NextInstruction();
			int32 offset = (int32)(int8) I->Opmod1;
			cpu.AdvancePC(2);
			uint32 value = *((uint32*) cpu.PC());
			uint8* target = cpu.D[REGISTER_SF].uint8PtrValue + offset;
			*((uint32*) target) = value;
			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(GetStackFrameValue32)
		{
			const Ins* I = NextInstruction();
			int32 offset = (int32)(int8) I->Opmod1;
			uint32* source = (uint32*)(cpu.D[REGISTER_SF].uint8PtrValue + offset);
			VariantValue& target = cpu.D[I->Opmod2];
			target.uint32Value = *source;
			cpu.AdvancePC(3);
		}

		OPCODE_CALLBACK(SetSFMemberByRefFromSFByValue32)
		{
			const Ins* I = NextInstruction();
			int32 targetSF = (int8) I->Opmod1;
			int32 targetMemberSF = (int8) I->Opmod2;
			int32 sourceSF = (int8) I->Opmod3;

			void** ppTarget = (void**)(cpu.SF() + targetSF);
			uint8* pMember = ((uint8*) *ppTarget) + targetMemberSF;

			const uint8* pValue = cpu.SF() + sourceSF;
			*((int32*) pMember) = *((const int32*) pValue);

			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(SetSFMemberByRefFromSFByValue64)
		{
			const Ins* I = NextInstruction();
			int32 targetSF = (int8)I->Opmod1;
			int32 targetMemberSF = (int8)I->Opmod2;
			int32 sourceSF = (int8)I->Opmod3;

			void** ppTarget = (void**)(cpu.SF() + targetSF);
			uint8* pMember = ((uint8*)*ppTarget) + targetMemberSF;

			const uint8* pValue = cpu.SF() + sourceSF;
			*((int64*)pMember) = *((const int64*)pValue);

			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(SetSFValueFromSFMemberByRef32)
		{
			const Ins* I = NextInstruction();
			int32 sourceSF = (int8) I->Opmod1;
			int32 sourceMemberSF = (int8) I->Opmod2;
			int32 targetSF = (int8) I->Opmod3;

			const void** ppSource = (const void**)(cpu.SF() + sourceSF);
			const uint8* pMember = ((const uint8*) *ppSource) + sourceMemberSF;

			uint8* pValue = (uint8*) cpu.SF() + targetSF;
			*((int32*) pValue) = *((const int32*) pMember);

			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(SetSFValueFromSFValue32)
		{
			const Ins* I = NextInstruction();
			int32 targetSF = (int8) I->Opmod1;
			int32 sourceSF = (int8) I->Opmod2;

			const uint32* source = (const uint32*) (cpu.SF() + sourceSF);
			uint32* target = (uint32*) (cpu.SF() + targetSF);

			*target = *source;

			cpu.AdvancePC(3);
		}

		OPCODE_CALLBACK(SetSFMemberByRefFromRegister32)
		{
			const Ins* I = NextInstruction();
			const VariantValue& src = cpu.D[I->Opmod1];
			int32 SFoffset = (int8) I->Opmod2;
			int32 memberOffset = (int8) I->Opmod3;

			const void** ppSource = (const void**)(cpu.SF() + SFoffset);
			uint8* pMember = ((uint8*) *ppSource) + memberOffset;

			*((int32*) pMember) = src.int32Value;

			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(SetSFMemberByRefFromRegisterLong)
		{
			const Ins* I = NextInstruction();
			const VariantValue& src = cpu.D[I->Opmod1];
			int bitcount = (int8) I->Opmod2;
			cpu.AdvancePC(3);

			int32 SFOffset = *(int32*)cpu.PC();

			cpu.AdvancePC(4);

			int32 memberOffset = *(int32*)cpu.PC();
			cpu.AdvancePC(4);

			auto* dest = (uint8**) (cpu.SF() + SFOffset) ;
			uint8* pMember = ((uint8*)*dest) + memberOffset;

			if (bitcount == BITCOUNT_32)
			{
				*((int32*)pMember) = src.int32Value;
			}
			else
			{
				*((int64*)pMember) = src.int64Value;
			}
		}

		OPCODE_CALLBACK(SetMemberRefFromSFMemberByRef64)
		{
			const Ins* I = NextInstruction();
			auto& args = *(IAssemblerBuilder::Args_SetMemberRefFromSFMemberByRef*) I;

			const uint8* pSourceObject = *(const uint8**)(cpu.SF() + args.sourceSFOffset);
			const uint8* pSourceRef = pSourceObject + args.sourceMemberOffset;
			const int64* pSource = (int64*)pSourceRef;

			uint8* pTargetObject = *(uint8**)(cpu.SF() + args.targetSFOffset);
			uint8* pTargerRef = pTargetObject + args.targetMemberOffset;
			int64* pTarget = (int64*)pTargerRef;

			*pTarget = *pSource;

			cpu.AdvancePC(sizeof(IAssemblerBuilder::Args_SetMemberRefFromSFMemberByRef));
		}

		OPCODE_CALLBACK(SetStackFrameValue32)
		{
			const Ins* I = NextInstruction();
			int32 offset = (int32)(int8) I->Opmod1;
			uint32* target = (uint32*)(cpu.D[REGISTER_SF].uint8PtrValue + offset);
			const VariantValue& source = cpu.D[I->Opmod2];
			*target = source.uint32Value;
			cpu.AdvancePC(3);
		}

		OPCODE_CALLBACK(GetStackFrameMemberPtr)
		{
			const Ins* I = NextInstruction();
			VariantValue& target = cpu.D[I->Opmod1];
			int32 SFoffset = (int32)(int8) I->Opmod2;
			int32 memberOffset = (int32)(int8) I->Opmod3;

			void** ppSource = (void**)(cpu.SF() + SFoffset);
			uint8* pMember = ((uint8*) *ppSource) + memberOffset;
			target.uint8PtrValue = pMember;

			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(GetStackFrameMemberPtrAndDeref)
		{
			const Ins* I = NextInstruction();

			auto* args = (ArgsGetStackFrameMemberPtrAndDeref*)I;

			VariantValue& target = cpu.D[args->dtarget];

			void** ppSource = (void**)(cpu.SF() + args->sfToStructRef);
			uint8* pMember = ((uint8*)*ppSource) + args->structToMemberOffset;
			target.uint8PtrValue = *(uint8**) pMember;

			cpu.AdvancePC(sizeof(ArgsGetStackFrameMemberPtrAndDeref));
		}

		OPCODE_CALLBACK(GetStackFrameMember32)
		{
			const Ins* I = NextInstruction();
			VariantValue& target = cpu.D[I->Opmod1];

			cpu.AdvancePC(2);
			int32 SFOffset = *(int32*) cpu.PC();

			cpu.AdvancePC(4);

			int32 memberOffset = *(int32*) cpu.PC();
			cpu.AdvancePC(4);

			void** ppSource = (void**)(cpu.SF() + SFOffset);
			uint8* pMember = ((uint8*) *ppSource) + memberOffset;
			target.int32Value = *(int32*) pMember;
		}

		OPCODE_CALLBACK(SetSFMemberPtrFromD5)
		{
			const Ins* I = NextInstruction();

			cpu.AdvancePC(1);
			int32 SFOffset = *(int32*)cpu.PC();

			cpu.AdvancePC(4);

			int32 memberOffset = *(int32*)cpu.PC();
			cpu.AdvancePC(4);

			void** ppSource = (void**)(cpu.SF() + SFOffset);
			void* pStructure = *ppSource;
			uint8* pMember = ((uint8*)pStructure) + memberOffset;
			*((uint64*) pMember) = cpu.D[5].uint64Value;
		}

		OPCODE_CALLBACK(GetStackFrameMember64)
		{
			const Ins* I = NextInstruction();
			VariantValue& target = cpu.D[I->Opmod1];

			cpu.AdvancePC(2);
			int32 SFOffset = *(int32*) cpu.PC();

			cpu.AdvancePC(4);

			int32 memberOffset = *(int32*) cpu.PC();
			cpu.AdvancePC(4);

			void** ppSource = (void**)(cpu.SF() + SFOffset);
			uint8* pMember = ((uint8*) *ppSource) + memberOffset;
			auto iValue = (int64*)pMember;
			target.int64Value = *iValue;
		}

		OPCODE_CALLBACK(GetStackFrameAddress)
		{
			const Ins* I = NextInstruction();
			VariantValue& target = cpu.D[I->Opmod1];
			int32 offset = *(int32*) (cpu.PC() + 2);			
			target.uint8PtrValue = (uint8*)cpu.SF() + offset;
			cpu.AdvancePC(6);
		}

		OPCODE_CALLBACK(GetStackFrameMemberPtrFar)
		{
			const Ins* I = NextInstruction();
			const uint8* pc = (const uint8*) I;

			VariantValue& target = cpu.D[I->Opmod1];

			int32 SFoffset = *(int32*) (pc+2);
			int32 memberOffset = *(int32*) (pc+6);

			void** ppSource = (void**)(cpu.SF() + SFoffset);
			uint8* pMember = ((uint8*) *ppSource) + memberOffset;
			target.uint8PtrValue = pMember;
			
			cpu.AdvancePC(10);
		}

		OPCODE_CALLBACK(BooleanNot)
		{
			const Ins* I = NextInstruction();
			cpu.D[I->Opmod1].int32Value = !cpu.D[I->Opmod1].int32Value;
			cpu.AdvancePC(2);
		}

		OPCODE_CALLBACK(Swap)
		{
			const Ins* I = NextInstruction();
			std::swap(cpu.D[I->Opmod1], cpu.D[I->Opmod2]);
			cpu.AdvancePC(3);
		}

		OPCODE_CALLBACK(StackAlloc)
		{
			const Ins* I = NextInstruction();
			cpu.D[REGISTER_SP].charPtrValue += I->Opmod1;
			cpu.AdvancePC(2);
		}

		OPCODE_CALLBACK(StackAllocBig)
		{
			int32 nBytes = *(int32*) (cpu.PC()+1);
			cpu.D[REGISTER_SP].charPtrValue += nBytes;
			cpu.AdvancePC(5);
		}

		OPCODE_CALLBACK(AddQuick32)
		{
			const Ins* I = NextInstruction();
			VariantValue& target = cpu.D[I->Opmod1];
			int8 value = (int8) I->Opmod2;
			int32 value32 = (int32) value;
			target.int32Value += value32;
			cpu.AdvancePC(3);
		}

		OPCODE_CALLBACK(Increment32)
		{
			const Ins* I = NextInstruction();
			auto* args = (ArgsOperateOnRegister*)I;
			VariantValue& target = cpu.D[args->reg];
			target.int32Value++;
			cpu.AdvancePC(sizeof(ArgsOperateOnRegister));
		}

		OPCODE_CALLBACK(Decrement32)
		{
			const Ins* I = NextInstruction();
			auto* args = (ArgsOperateOnRegister*)I;
			VariantValue& target = cpu.D[args->reg];
			target.int32Value--;
			cpu.AdvancePC(sizeof(ArgsOperateOnRegister));
		}

		OPCODE_CALLBACK(IntAdd32)
		{
			const Ins* I = NextInstruction();
			cpu.AdvancePC(3);
			const VariantValue& A = cpu.D[I->Opmod1];
			const VariantValue& B = cpu.D[I->Opmod2];
			VariantValue& C = cpu.D[I->Opmod1-1];
			C.int32Value = A.int32Value + B.int32Value;
		}

		OPCODE_CALLBACK(IntSubtract32)
		{
			const Ins* I = NextInstruction();
			cpu.AdvancePC(3);
			const VariantValue& A = cpu.D[I->Opmod1];
			const VariantValue& B = cpu.D[I->Opmod2];
			VariantValue& C = cpu.D[I->Opmod1-1];
			C.int32Value = A.int32Value - B.int32Value;
		}

		OPCODE_CALLBACK(IntMultiply32)
		{
			const Ins* I = NextInstruction();
			cpu.AdvancePC(3);
			const VariantValue& A = cpu.D[I->Opmod1];
			const VariantValue& B = cpu.D[I->Opmod2];
			VariantValue& C = cpu.D[I->Opmod1-1];
			C.int32Value = A.int32Value * B.int32Value;
		}

		OPCODE_CALLBACK(IntNegate32)
		{
			const Ins* I = NextInstruction();
			cpu.AdvancePC(3);
			const VariantValue& src = cpu.D[I->Opmod1];
			VariantValue& trg = cpu.D[I->Opmod2];
			trg.int32Value = -src.int32Value;
		}

		OPCODE_CALLBACK(IntDivide32)
		{
			const Ins* I = NextInstruction();
			cpu.AdvancePC(3);

			const VariantValue& A = cpu.D[I->Opmod1];
			const VariantValue& B = cpu.D[I->Opmod2];
			VariantValue& quotient = cpu.D[I->Opmod1-1];

			quotient.int32Value = A.int32Value / B.int32Value;
		}

		OPCODE_CALLBACK(LogicalAND32)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& A = cpu.D[I->Opmod1];
			const VariantValue& B = cpu.D[I->Opmod2];
			VariantValue& C = cpu.D[I->Opmod1-1];
			cpu.AdvancePC(3);
			C.int32Value = A.int32Value & B.int32Value;
		}

		OPCODE_CALLBACK(LogicalOR32)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& A = cpu.D[I->Opmod1];
			const VariantValue& B = cpu.D[I->Opmod2];
			VariantValue& C = cpu.D[I->Opmod1-1];
			cpu.AdvancePC(3);
			C.int32Value = A.int32Value | B.int32Value;
		}

		OPCODE_CALLBACK(LogicalXOR32)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& A = cpu.D[I->Opmod1];
			const VariantValue& B = cpu.D[I->Opmod2];
			VariantValue& C = cpu.D[I->Opmod1-1];
			cpu.AdvancePC(3);
			C.int32Value = A.int32Value ^ B.int32Value;
		}

		OPCODE_CALLBACK(LogicalNOT32)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& A = cpu.D[I->Opmod1];
			VariantValue& B = cpu.D[I->Opmod2];
			cpu.AdvancePC(3);
			B.int32Value = ~A.int32Value;
		}
		
		OPCODE_CALLBACK(Move32)
		{
			const Ins* I = NextInstruction();		
			cpu.D[I->Opmod2].int32Value = cpu.D[I->Opmod1].int32Value;
			cpu.AdvancePC(3);
		}

		OPCODE_CALLBACK(Poke32)
		{
			const Ins* I = NextInstruction();
			int offset = *((int32*) (cpu.PC() + 3));
			void* target = cpu.D[I->Opmod2].charPtrValue + offset;
			const VariantValue& source = cpu.D[I->Opmod1];
			*(int32*) target = source.int32Value;
			cpu.AdvancePC(7);
		}

		OPCODE_CALLBACK(Pop)
		{
			const Ins* I = NextInstruction();
			uint8 nBytes = I->Opmod1;
			cpu.D[REGISTER_SP].charPtrValue -= (size_t) nBytes;
			cpu.AdvancePC(2);
		}

		OPCODE_CALLBACK(DereferenceD4)
		{
			const Ins* I = NextInstruction();
			cpu.D[REGISTER_D4].int64Value = *cpu.D[REGISTER_D4].int64PtrValue;
			cpu.AdvancePC(1);	
		}

		OPCODE_CALLBACK(PushIndirect)
		{
			const Ins* I = NextInstruction();
			size_t nBytes = *((size_t*) (cpu.PC() + 3));
			char* target = cpu.D[I->Opmod2].charPtrValue;
			const char* source = cpu.D[I->Opmod1].charPtrValue;
			memcpy(target, source, nBytes);
			cpu.D[I->Opmod2].charPtrValue += nBytes;
			cpu.AdvancePC(3 + sizeof(size_t));
		}

		OPCODE_CALLBACK(PushRegister32)
		{
			const Ins* I = NextInstruction();
			char* target = cpu.D[VM::REGISTER_SP].charPtrValue;
			const VariantValue& v = cpu.D[I->Opmod1];
			*(int32*) target = v.int32Value;
			cpu.D[VM::REGISTER_SP].charPtrValue += 4;
			cpu.AdvancePC(2);
		}

		OPCODE_CALLBACK(PushAddress)
		{
			const Ins* I = NextInstruction();
			const VariantValue& source = cpu.D[I->Opmod1];
			const VariantValue& offset = cpu.D[I->Opmod2];
			const uint8* totalAddress = source.uint8PtrValue + (ptrdiff_t) offset.int32Value;
			cpu.Push(totalAddress);
			cpu.AdvancePC(3);
		}

		OPCODE_CALLBACK(PushImmediate32)
		{
			cpu.AdvancePC(1);
			const int32* pArg = (const int32*) cpu.PC();
			cpu.Push(*pArg);
			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(PushImmediate64)
		{
			cpu.AdvancePC(1);
			const int64* pArg = (const int64*)cpu.PC();
			cpu.Push(*pArg);
			cpu.AdvancePC(8);
		}

		OPCODE_CALLBACK(PushStackVariable32)
		{
			const Ins* I = NextInstruction();
			auto* args = (ArgsPushStackVariable*) I;
			const uint8* sfItem = cpu.SF() + args->sfOffset;
			const int32* pArg = (const int32*) sfItem;
			cpu.Push(*pArg);
			cpu.AdvancePC(sizeof(ArgsPushStackVariable));
		}

		OPCODE_CALLBACK(PushStackVariable64)
		{
			const Ins* I = NextInstruction();
			auto* args = (ArgsPushStackVariable*)I;
			const uint8* sfItem = cpu.SF() + args->sfOffset;
			const int64* pArg = (const int64*)sfItem;
			cpu.Push(*pArg);
			cpu.AdvancePC(sizeof(ArgsPushStackVariable));
		}

		OPCODE_CALLBACK(PushStackFrameMemberPtr)
		{
			const Ins* I = NextInstruction();
			auto* args = (ArgsPushStackFrameMemberPtr*)I;
			const uint8* sfItem = cpu.SF() + args->sfOffset;
			const uint8* pItem = *(const uint8**)sfItem;
			auto* pMember = pItem + args->memberOffset;
			cpu.Push(pMember);
			cpu.AdvancePC(sizeof(ArgsPushStackFrameMemberPtr));
		}

		OPCODE_CALLBACK(PushStackAddress)
		{
			const Ins* I = NextInstruction();

			auto* args = (ArgsPushStackVariable*)I;
			const uint8* sfItem = cpu.SF() + args->sfOffset;
			cpu.Push(sfItem);
			cpu.AdvancePC(sizeof(ArgsPushStackVariable));
		}

		OPCODE_CALLBACK(SaveRegister32)
		{
			const Ins* I = NextInstruction();
			int value = cpu.D[I->Opmod1].int32Value;
			cpu.Push(value);
			cpu.AdvancePC(2);
		}

		OPCODE_CALLBACK(RestoreRegister32)
		{
			const Ins* I = NextInstruction();
			int32& reg = cpu.D[I->Opmod1].int32Value;
			reg = cpu.Pop<int32>();
			cpu.AdvancePC(2);
		}

		OPCODE_CALLBACK(SaveRegister64)
		{
			const Ins* I = NextInstruction();
			const VariantValue& reg = cpu.D[I->Opmod1];
			cpu.Push(reg);
			cpu.AdvancePC(2);
		}

		OPCODE_CALLBACK(GetGlobal)
		{
			const Ins* I = NextInstruction();
			BITCOUNT bits = (BITCOUNT) I->Opmod1;
			cpu.AdvancePC(2);

			int32 offset = *(int32*) cpu.PC();
			const uint8* pData = cpu.Globals + offset;

			switch (bits)
			{
			case BITCOUNT_32:
				{
					const int32* pInt32Data = (const int32*)pData;
					cpu.D[VM::REGISTER_D4].int32Value = *pInt32Data;
				}
				break;
			case BITCOUNT_64:
				{
					const int64* pInt64Data = (const int64*)pData;
					cpu.D[VM::REGISTER_D4].int64Value = *pInt64Data;
				}
				break;
         default:
            TerminateByIllegal(-1);
            break;
			}

			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(SetGlobal)
		{
			const Ins* I = NextInstruction();
			BITCOUNT bits = (BITCOUNT) I->Opmod1;
			cpu.AdvancePC(2);

			int32 offset = *(int32*)cpu.PC();
			uint8* pData = cpu.Globals + offset;

			switch (bits)
			{
			case BITCOUNT_32:
				{
					int32* pInt32Data = (int32*)pData;
					*pInt32Data = cpu.D[VM::REGISTER_D4].int32Value;
				}
				break;
			case BITCOUNT_64:
				{
					int64* pInt64Data = (int64*)pData;
					*pInt64Data = cpu.D[VM::REGISTER_D4].int64Value;
				}
				break;
         default:
            TerminateByIllegal(-1);
            break;
			}

			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(RestoreRegister64)
		{
			const Ins* I = NextInstruction();
			VariantValue& reg = cpu.D[I->Opmod1];
			reg = cpu.Pop<VariantValue>();
			cpu.AdvancePC(2);
		}

		OPCODE_CALLBACK(CallBy)
		{
			cpu.Push(cpu.D[REGISTER_SF].vPtrValue);

			const uint8 *returnAddress = cpu.PC() + 2; 
			cpu.Push(returnAddress);

			// Then make the new stack frame equal to the stack pointer
			cpu.D[REGISTER_SF].charPtrValue = cpu.D[REGISTER_SP].charPtrValue;

			const Ins* I = NextInstruction();
			const VariantValue& offsetRegister = cpu.D[I->Opmod1];
			cpu.SetPC(cpu.PC() + offsetRegister.int32Value);
		}

		OPCODE_CALLBACK(CallById)
		{
			cpu.Push(cpu.D[REGISTER_SF].vPtrValue);

			const uint8 *returnAddress = cpu.PC() + 1 + sizeof(ID_BYTECODE); 
			cpu.Push(returnAddress);

			// Then make the new stack frame equal to the stack pointer
			cpu.D[REGISTER_SF].charPtrValue = cpu.D[REGISTER_SP].charPtrValue;

			ID_BYTECODE* pByteCodeId = (ID_BYTECODE*)(cpu.PC() + 1);
			size_t addressOffset = program->GetFunctionAddress(*pByteCodeId);
			cpu.SetPC(cpu.ProgramStart + addressOffset);
		}

		OPCODE_CALLBACK(CallByIdIndirect)
		{
			cpu.Push(cpu.D[REGISTER_SF].vPtrValue);

			const uint8 *returnAddress = cpu.PC() + 2; 
			cpu.Push(returnAddress);

			// Then make the new stack frame equal to the stack pointer
			cpu.D[REGISTER_SF].charPtrValue = cpu.D[REGISTER_SP].charPtrValue;

			const Ins* I = NextInstruction();
			const ID_BYTECODE id = cpu.D[I->Opmod1].int32Value;
			size_t functionStart = program->GetFunctionAddress(id);
			cpu.SetPC(cpu.ProgramStart + functionStart);
		}

		OPCODE_CALLBACK(CallVitualFunctionViaRefOnStack)
		{
			/* args:
			Opcodes::OPCODE opcode;
			int32 SFoffsetToInterfaceRef;
			int32 vTableOffset;
			int32 instanceToInterfaceOffset;
			*/

			struct VirtualTable
			{
				ptrdiff_t OffsetToInstance;
				ID_BYTECODE FirstMethodId;
			};

			const auto* args = (ArgsCallVirtualFunctionViaRefOnStack*) cpu.PC();

			const uint8* sfItem = cpu.SF() + args->SFoffsetToInterfaceRef;
			const uint8** pInstance = (const uint8**) sfItem;

			const VirtualTable** pVTable = (const VirtualTable**) (*pInstance + args->instanceToInterfaceOffset);

			cpu.Push(pVTable);

			cpu.Push(cpu.D[REGISTER_SF].vPtrValue);

			const uint8 *returnAddress = cpu.PC() + sizeof(ArgsCallVirtualFunctionViaRefOnStack);
			cpu.Push(returnAddress);						

			// Then make the new stack frame equal to the stack pointer
			cpu.D[REGISTER_SF].charPtrValue = cpu.D[REGISTER_SP].charPtrValue;
								
			const ID_BYTECODE * vTable = (const ID_BYTECODE*) *pVTable;

			const ID_BYTECODE id = vTable[args->vTableOffset];
			size_t functionStart = program->GetFunctionAddress(id);
			cpu.SetPC(cpu.ProgramStart + functionStart);
		}

		OPCODE_CALLBACK(CallVitualFunctionViaMemberOffsetOnStack)
		{
			struct VirtualTable
			{
				ptrdiff_t OffsetToInstance;
				ID_BYTECODE FirstMethodId;
			};

			const auto* args = (ArgsCallVirtualFunctionViaMemberOffsetOnStack*)cpu.PC();

			const uint8* sfItem = cpu.SF() + args->SFoffsetToStruct;
			const uint8* pStructure = *(const uint8**)sfItem;

			const uint8* pMember = pStructure + args->memberOffsetToInterfaceRef;

			const VirtualTable** pVTable = *(const VirtualTable***)(pMember);

			cpu.Push(pVTable);

			cpu.Push(cpu.D[REGISTER_SF].vPtrValue);

			const uint8 *returnAddress = cpu.PC() + sizeof(ArgsCallVirtualFunctionViaMemberOffsetOnStack);
			cpu.Push(returnAddress);

			// Then make the new stack frame equal to the stack pointer
			cpu.D[REGISTER_SF].charPtrValue = cpu.D[REGISTER_SP].charPtrValue;

			const ID_BYTECODE * vTable = (const ID_BYTECODE*)*pVTable;

			const ID_BYTECODE id = vTable[args->vTableOffset];
			size_t functionStart = program->GetFunctionAddress(id);
			cpu.SetPC(cpu.ProgramStart + functionStart);
		}

		OPCODE_CALLBACK(CallVirtualFunctionByAddress)
		{
			const int32* offsetArray = (const int32*) (cpu.PC() + 1);
			int32 sfOffset = *offsetArray;
			int32 methodIndex = offsetArray[1];

			const uint8* sfItem = cpu.SF() + sfOffset;
			const void** pVTable = (const void**) sfItem;

			cpu.Push(pVTable);

			cpu.Push(cpu.D[REGISTER_SF].vPtrValue);

			const uint8 *returnAddress = cpu.PC() + 9; 
			cpu.Push(returnAddress);

			// Then make the new stack frame equal to the stack pointer
			cpu.D[REGISTER_SF].charPtrValue = cpu.D[REGISTER_SP].charPtrValue;

			const uint8* vTable = (const uint8*) *pVTable;

			const ID_BYTECODE* pFunction = (const ID_BYTECODE*) (vTable + methodIndex);
			ID_BYTECODE id = *pFunction;
			size_t functionStart = program->GetFunctionAddress(id);
			cpu.SetPC(cpu.ProgramStart + functionStart);
		}

		struct MemCopyInfo
		{
			int TargetOffset;
			int SourceOffset;
			size_t ByteCount;
			uint8* SF;
		};

		void GetMemCopyInfoAndAdvance(OUT MemCopyInfo& info)
		{
			cpu.AdvancePC(1);
			const int32* pOffset = (const int32*) cpu.PC();
			info.TargetOffset = *pOffset;
			cpu.AdvancePC(4);
			pOffset = (const int32*) cpu.PC();
			info.SourceOffset = *pOffset;
			cpu.AdvancePC(4);

			const size_t* pByteCount = (const size_t*) cpu.PC();
			cpu.AdvancePC(sizeof(size_t));
			info.ByteCount = *pByteCount;

			info.SF = cpu.D[REGISTER_SF].uint8PtrValue;
		}

		void GetMemCopyNearInfoAndAdvance(OUT MemCopyInfo& info)
		{
			const Ins* I = NextInstruction();
			info.TargetOffset = (int32)(int8) I->Opmod1;
			info.SourceOffset = (int32)(int8) I->Opmod2;
			info.ByteCount = (size_t) I->Opmod3;			
			cpu.AdvancePC(4);

         info.SF = cpu.D[REGISTER_SF].uint8PtrValue;
		}

		uint8* LookupSFPtr(int offset)
		{
			uint8* ptr = cpu.D[REGISTER_SF].uint8PtrValue + offset;
			uint8** pPtr = (uint8**) ptr;
			return *pPtr;
		}

		OPCODE_CALLBACK(CopySFMemoryNear)
		{
			MemCopyInfo mci;
			GetMemCopyNearInfoAndAdvance(OUT mci);
			memcpy(mci.SF + mci.TargetOffset, mci.SF + mci.SourceOffset, mci.ByteCount);
		}

		OPCODE_CALLBACK(CopySFMemory)
		{
			MemCopyInfo mci;
			GetMemCopyInfoAndAdvance(OUT mci);
			memcpy(mci.SF + mci.TargetOffset, mci.SF + mci.SourceOffset, mci.ByteCount);
		}

		OPCODE_CALLBACK(Copy32Bits)
		{
			const Ins* I = NextInstruction();
			int32* target = cpu.D[I->Opmod1].int32PtrValue;
			const int32* source = cpu.D[I->Opmod2].int32PtrValue;
			
			*target = *source;

			cpu.AdvancePC(3);			
		}

		OPCODE_CALLBACK(CopySFVariableFromRef)
		{
			const Ins* I = NextInstruction();

#pragma pack(push,1)
			struct Args // This is how OPCODES should have been implemented. MAT
			{
				uint8 instruction;
				int32 targetSFOffset;
				int32 sourcePtrSFOffset;
				int32 sourceMemberOffset;
				size_t nBytesSource;
			};
#pragma pack(pop)

			const Args* args = (const Args*) I;
			cpu.AdvancePC(sizeof(Args));

			uint8* sf = cpu.SF();
			uint8* target = sf + args->targetSFOffset;
			const uint8** ppSource = (const uint8**) (sf + args->sourcePtrSFOffset);
			const uint8* source = (*ppSource) + args->sourceMemberOffset;
			memcpy(target, source, args->nBytesSource);
		}
	
		OPCODE_CALLBACK(CopyMemory)
		{
			const Ins* I = NextInstruction();
			uint8* target = cpu.D[I->Opmod1].uint8PtrValue;
			const uint8* source = cpu.D[I->Opmod2].uint8PtrValue;
			size_t byteCount = (size_t) I->Opmod3;
			cpu.AdvancePC(4);

			memcpy(target, source, byteCount);
		}

		OPCODE_CALLBACK(CopyMemoryBig)
		{
			const Ins* I = NextInstruction();
			uint8* target = cpu.D[I->Opmod1].uint8PtrValue;
			const uint8* source = cpu.D[I->Opmod2].uint8PtrValue;
			cpu.AdvancePC(3);

			const size_t* pByteCount = (const size_t*) cpu.PC();

			cpu.AdvancePC(sizeof(size_t));

			memcpy(target, source, *pByteCount);
		}

		OPCODE_CALLBACK(Call)
		{
			cpu.Push(cpu.D[REGISTER_SF].vPtrValue);

			const uint8 *returnAddress = cpu.PC() + 5; 
			cpu.Push(returnAddress);

			// Then make the new stack frame equal to the stack pointer
			cpu.D[REGISTER_SF].charPtrValue = cpu.D[REGISTER_SP].charPtrValue;

			int32* offsetPtr = (int32*) (cpu.PC() + 1);
			cpu.SetPC(cpu.PC() + *offsetPtr);
		}

		OPCODE_CALLBACK(Invoke)
		{
			ID_API_CALLBACK* pCallbackId = (ID_API_CALLBACK*) (cpu.PC() + 1);

			cpu.AdvancePC(sizeof(ID_API_CALLBACK)+1);

			if (!core.TryInvoke(*pCallbackId, cpu.D))
			{
				cpu.ExceptionCode = EXCEPTION_CODE_UNKNOWN_CALLBACK;
				TerminateByIllegal(-3);
			}			
		}

		OPCODE_CALLBACK(InvokeBy)
		{
			const Ins* I = NextInstruction();
			ID_API_CALLBACK id = cpu.D[I->Opmod1].apiValue;

			cpu.AdvancePC(2);

			if (!core.TryInvoke(id, cpu.D))
			{
				cpu.ExceptionCode = EXCEPTION_CODE_UNKNOWN_CALLBACK;
				TerminateByIllegal(-3);
			}			
		}

		OPCODE_CALLBACK(Return)
		{
			uint8* returnAddress = cpu.Pop<uint8*>();
			uint8* callingSF = cpu.Pop<uint8*>();

			cpu.D[REGISTER_SF].uint8PtrValue = callingSF;
			cpu.SetPC(returnAddress);	
		}

		OPCODE_CALLBACK(SetRegisterImmediate32)
		{
			const Ins* I = NextInstruction();	
			auto* args = (ArgsSetRegister32*) I;
			cpu.D[args->reg].int32Value = args->value;
			cpu.AdvancePC(sizeof(ArgsSetRegister32));
		}

		OPCODE_CALLBACK(ShiftLeft32)
		{
			const Ins* I = NextInstruction();
			const VariantValue& source = cpu.D[I->Opmod1];
			VariantValue& target = cpu.D[I->Opmod1+1];
			target.int32Value = source.int32Value << I->Opmod2;
			cpu.AdvancePC(3);
		}

		OPCODE_CALLBACK(ShiftLeft64)
		{
			const Ins* I = NextInstruction();
			const VariantValue& source = cpu.D[I->Opmod1];
			VariantValue& target = cpu.D[I->Opmod1+1];
			target.int64Value = source.int64Value << I->Opmod2;
			cpu.AdvancePC(3);
		}

		OPCODE_CALLBACK(ShiftRight32)
		{
			const Ins* I = NextInstruction();
			const VariantValue& source = cpu.D[I->Opmod1];
			VariantValue& target = cpu.D[I->Opmod1+1];
			target.int32Value = source.int32Value >> I->Opmod2;
			cpu.AdvancePC(3);
		}

		OPCODE_CALLBACK(ShiftRight64)
		{
			const Ins* I = NextInstruction();
			const VariantValue& source = cpu.D[I->Opmod1];
			VariantValue& target = cpu.D[I->Opmod1+1];
			target.int64Value = source.int64Value >> I->Opmod2;
			cpu.AdvancePC(3);
		}

		OPCODE_CALLBACK(Test32)
		{
			const Ins* I = NextInstruction();
			auto* args = (ArgsOperateOnRegister*)I;
			cpu.D[REGISTER_SR].uint32Value = 0;
			VariantValue& v = cpu.D[args->reg];
			cpu.SR() |= v.int32Value ? 0 : STATUSBIT_EQUIVALENCE;
			cpu.SR() |= v.int32Value < 0 ? STATUSBIT_NEGATIVE : 0;
			cpu.AdvancePC(sizeof(ArgsOperateOnRegister));
		}

		OPCODE_CALLBACK(BranchIfGTE)
		{
			const Ins* I = NextInstruction();	
			auto* args = (ArgsBranchIf*)I;
			int32 offset = (IsEquiSet(cpu) || !IsNegSet(cpu)) ? args->PCoffset : sizeof(ArgsBranchIf);
			cpu.AdvancePC(offset);
		}

		OPCODE_CALLBACK(BranchIfGT)
		{
			const Ins* I = NextInstruction();	
			auto* args = (ArgsBranchIf*)I;
			int32 offset = (!IsEquiSet(cpu) && !IsNegSet(cpu)) ? args->PCoffset : sizeof(ArgsBranchIf);
			cpu.AdvancePC(offset);
		}

		OPCODE_CALLBACK(BranchIfLT)
		{
			const Ins* I = NextInstruction();	
			auto* args = (ArgsBranchIf*)I;
			int32 offset = (!IsEquiSet(cpu) && IsNegSet(cpu)) ? args->PCoffset : sizeof(ArgsBranchIf);
			cpu.AdvancePC(offset);
		}

		OPCODE_CALLBACK(BranchIfNE)
		{
			const Ins* I = NextInstruction();		
			auto* args = (ArgsBranchIf*)I;
			int32 offset = (!IsEquiSet(cpu)) ? args->PCoffset : sizeof(ArgsBranchIf);
			cpu.AdvancePC(offset);
		}

		OPCODE_CALLBACK(FloatAdd)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& a = cpu.D[I->Opmod1];
			const VariantValue& b = cpu.D[I->Opmod2];
			VariantValue& c = cpu.D[I->Opmod1-1];

			c.floatValue = a.floatValue + b.floatValue;
			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(FloatSubtract)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& a = cpu.D[I->Opmod1];
			const VariantValue& b = cpu.D[I->Opmod2];
			VariantValue& c = cpu.D[I->Opmod1-1];
			c.floatValue = a.floatValue - b.floatValue;
			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(FloatMultiply)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& a = cpu.D[I->Opmod1];
			const VariantValue& b = cpu.D[I->Opmod2];
			VariantValue& c = cpu.D[I->Opmod1-1];
			c.floatValue = a.floatValue * b.floatValue;
			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(FloatDivide)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& a = cpu.D[I->Opmod1];
			const VariantValue& b = cpu.D[I->Opmod2];
			VariantValue& c = cpu.D[I->Opmod1-1];
			c.floatValue = a.floatValue / b.floatValue;
			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(FloatSin)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& src = cpu.D[I->Opmod1];
			VariantValue& target = cpu.D[I->Opmod2];
			target.floatValue = sinf(src.floatValue);
			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(FloatCos)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& src = cpu.D[I->Opmod1];
			VariantValue& target = cpu.D[I->Opmod2];
			target.floatValue = cosf(src.floatValue);
			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(FloatTan)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& src = cpu.D[I->Opmod1];
			VariantValue& target = cpu.D[I->Opmod2];

			target.floatValue = tanf(src.floatValue);
			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(FloatArcSin)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& src = cpu.D[I->Opmod1];
			VariantValue& target = cpu.D[I->Opmod2];
			target.floatValue = asinf(src.floatValue);
			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(FloatArcCos)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& src = cpu.D[I->Opmod1];
			VariantValue& target = cpu.D[I->Opmod2];
			target.floatValue = acosf(src.floatValue);
			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(FloatArcTan)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& src = cpu.D[I->Opmod1];
			VariantValue& target = cpu.D[I->Opmod2];
			target.floatValue = atanf(src.floatValue);
			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(FloatArcTan2)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& srcX = cpu.D[I->Opmod1];
			const VariantValue& srcY = cpu.D[I->Opmod1+1];
			VariantValue& target = cpu.D[I->Opmod2];
			target.floatValue = atan2f(srcY.floatValue, srcX.floatValue);
			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(FloatSinh)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& src = cpu.D[I->Opmod1];
			VariantValue& target = cpu.D[I->Opmod2];
			target.floatValue = sinhf(src.floatValue);
			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(FloatCosh)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& src = cpu.D[I->Opmod1];
			VariantValue& target = cpu.D[I->Opmod2];
			target.floatValue = coshf(src.floatValue);
			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(FloatExp)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& src = cpu.D[I->Opmod1];
			VariantValue& target = cpu.D[I->Opmod2];
			target.floatValue = expf(src.floatValue);
			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(FloatLn)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& src = cpu.D[I->Opmod1];
			VariantValue& target = cpu.D[I->Opmod2];
			target.floatValue = logf(src.floatValue);
			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(IncrementPtr)
		{
			const Ins* I = NextInstruction();	
			VariantValue& d = cpu.D[I->Opmod1];
			int32 delta = (int32)(int8) I->Opmod2;
			d.uint8PtrValue += delta;
			cpu.AdvancePC(3);
		}

		OPCODE_CALLBACK(IncrementPtrBig)
		{
			const Ins* I = NextInstruction();	
			VariantValue& d = cpu.D[I->Opmod1];
			cpu.AdvancePC(2);

			const int32* pDelta = (const int32*) cpu.PC();
			d.uint8PtrValue += *pDelta;
			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(PushRegister64)
		{
			const Ins* I = NextInstruction();
			char* target = cpu.D[VM::REGISTER_SP].charPtrValue;

			const VariantValue& v = cpu.D[I->Opmod1];

			*(int64*) target = v.int64Value;
			cpu.D[VM::REGISTER_SP].charPtrValue += 8;
			
			cpu.AdvancePC(2);
		}

		OPCODE_CALLBACK(AddImmediate)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& source = cpu.D[I->Opmod1];
			VariantValue& target = cpu.D[I->Opmod3];

			BITCOUNT bc = (BITCOUNT) I->Opmod2;

			cpu.AdvancePC(4);

			const int8* arg = (const int8*) cpu.PC();

			switch(bc)
			{
			case BITCOUNT_32:
				target.int32Value = source.int32Value + *(const int32*)arg;
				cpu.AdvancePC(4);
				break;
			case BITCOUNT_64:
				target.int64Value = source.int64Value + *(const int64*)arg;
				cpu.AdvancePC(8);
				break;
         default:
            TerminateByIllegal(-1);
            break;
			}
		}

		OPCODE_CALLBACK(Branch)
		{
			const Ins* I = NextInstruction();
			const uint8* pPC = (const uint8*) I;
			const int32* pIntArg = (const int32*)(pPC + 1); // Look beyond the 2nd byte of the test instruction for the offset value
			int32 offset = *pIntArg;
			cpu.AdvancePC(offset);
		}

		OPCODE_CALLBACK(BranchIf)
		{
			const Ins* I = NextInstruction();
			
			switch((CONDITION) I->Opmod1)
			{
			case CONDITION_IF_EQUAL:
				if (IsEquiSet(cpu))
				{
					break;
				}
				else
				{
					cpu.AdvancePC(6);
					return;
				}
			case CONDITION_IF_GREATER_OR_EQUAL:
				if (IsEquiSet(cpu) || !IsNegSet(cpu))
				{
					break;
				}
				else
				{
					cpu.AdvancePC(6);
					return;
				}
			case CONDITION_IF_GREATER_THAN:
				if (!IsEquiSet(cpu) && !IsNegSet(cpu))
				{
					break;
				}
				else
				{
					cpu.AdvancePC(6);
					return;
				}
			case CONDITION_IF_LESS_OR_EQUAL:
				if (IsEquiSet(cpu) || IsNegSet(cpu))
				{
					break;
				}
				else
				{
					cpu.AdvancePC(6);
					return;
				}
			case CONDITION_IF_LESS_THAN:
				if (!IsEquiSet(cpu) && IsNegSet(cpu))
				{
					break;
				}
				else
				{
					cpu.AdvancePC(6);
					return;
				}
			case CONDITION_IF_NOT_EQUAL:
				if (!IsEquiSet(cpu))
				{
					break;
				}
				else
				{
					cpu.AdvancePC(6);
					return;
				}
			default:
				throw IllegalException();
			}

			const uint8* pPC = (const uint8*) I;
			const int32* pIntArg = (const int32*)(pPC + 2); // Look beyond the 2nd byte of the test instruction for the offset value
			int32 offset = *pIntArg;
			cpu.AdvancePC(offset);
		}

		OPCODE_CALLBACK(SetIf32)
		{
			const Ins* I = NextInstruction();
			CONDITION cond = (CONDITION) I->Opmod1;
			VariantValue& targetRegister = cpu.D[I->Opmod2];
			cpu.AdvancePC(3);

			const bool isEquivalent = targetRegister.int32Value == 0;
			const bool isNegative = targetRegister.int32Value < 0;

			switch(cond)
			{
			case CONDITION_IF_EQUAL:
				targetRegister.int32Value = isEquivalent ? 1 : 0;
				return;
			case CONDITION_IF_GREATER_OR_EQUAL:
				targetRegister.int32Value = (isEquivalent || !isNegative) ? 1 : 0;
				return;
			case CONDITION_IF_GREATER_THAN:
				targetRegister.int32Value = (!isEquivalent && !isNegative) ? 1 : 0;
				return;
			case CONDITION_IF_LESS_OR_EQUAL:
				targetRegister.int32Value = (isEquivalent || isNegative) ? 1 : 0;
				return;
			case CONDITION_IF_LESS_THAN:
				targetRegister.int32Value = (!isEquivalent && isNegative) ? 1 : 0;
				return;
			case CONDITION_IF_NOT_EQUAL:
				targetRegister.int32Value = !isEquivalent  ? 1 : 0;
				return;
			default:
				throw IllegalException();
			}
		}

		OPCODE_CALLBACK(SetIf64)
		{
			const Ins* I = NextInstruction();
			CONDITION cond = (CONDITION) I->Opmod1;
			VariantValue& targetRegister = cpu.D[I->Opmod2];
			cpu.AdvancePC(3);

			const bool isEquivalent = targetRegister.int64Value == 0;
			const bool isNegative = targetRegister.int64Value < 0;

			switch(cond)
			{
			case CONDITION_IF_EQUAL:
				targetRegister.int32Value = isEquivalent ? 1 : 0;
				return;
			case CONDITION_IF_GREATER_OR_EQUAL:
				targetRegister.int32Value = (isEquivalent || !isNegative) ? 1 : 0;
				return;
			case CONDITION_IF_GREATER_THAN:
				targetRegister.int32Value = (!isEquivalent && !isNegative) ? 1 : 0;
				return;
			case CONDITION_IF_LESS_OR_EQUAL:
				targetRegister.int32Value = (isEquivalent || isNegative) ? 1 : 0;
				return;
			case CONDITION_IF_LESS_THAN:
				targetRegister.int32Value = (!isEquivalent && isNegative) ? 1 : 0;
				return;
			case CONDITION_IF_NOT_EQUAL:
				targetRegister.int32Value = !isEquivalent  ? 1 : 0;
				return;
			default:
				throw IllegalException();
			}
		}

		OPCODE_CALLBACK(SetSFValueFromSFValueLong)
		{
			auto* args = (ArgsSetSFValueFromSFValue*) NextInstruction();
			const uint8* pSrc = cpu.SF() + args->sfSourceOffset;
			uint8* pTarget = cpu.SF() + args->sfTargetOffset;

			memcpy(pTarget, pSrc, args->byteCount);

			cpu.AdvancePC(sizeof(ArgsSetSFValueFromSFValue));
		}

		OPCODE_CALLBACK(SetSFMemberRefFromSFValue)
		{
			auto* args = (ArgsSetSFMemberRefFromSFValue*) NextInstruction();
			const uint8* src = cpu.SF() + args->SFSourceValueOffset;

			uint8* ppTargetStruct = cpu.SF() + args->targetSFOffset;
			uint8* pTargetStruct = *(uint8**)ppTargetStruct;

			uint8* pTarget = pTargetStruct + args->targetMemberOffset;

			memcpy(pTarget, src, args->nBytesSource);

			cpu.AdvancePC(sizeof(ArgsSetSFMemberRefFromSFValue));
		}

		OPCODE_CALLBACK(SetSFValueFromSFMemberByRef)
		{
			auto* args = (ArgsSetSFValueFromSFMemberRef*) NextInstruction();
			const uint8* ppStruct = cpu.SF() + args->srcSFOffset;
			const uint8* pStruct = *(const uint8**)ppStruct;
			const uint8* pMember = pStruct + args->srcMemberOffset;

			uint8* pTarget = cpu.SF() + args->targetSFOffset;

			memcpy(pTarget, pMember, args->nBytesSource);

			cpu.AdvancePC(sizeof(ArgsSetSFValueFromSFMemberRef));
		}

		OPCODE_CALLBACK(Copy64Bits)
		{
			const Ins* I = NextInstruction();
			uint8* target = cpu.D[I->Opmod1].uint8PtrValue;
			const uint8* source = cpu.D[I->Opmod2].uint8PtrValue;

			*(int64*) target = *(const int64*) source;

			cpu.AdvancePC(3);
		}

		OPCODE_CALLBACK(Poke64)
		{
			const Ins* I = NextInstruction();
			int offset = *((int32*) (cpu.PC() + 3));

			void* target = cpu.D[I->Opmod2].charPtrValue + offset;
			const VariantValue& source = cpu.D[I->Opmod1];

			*(int64*) target = source.int64Value;
			
			cpu.AdvancePC(7);
		}


		OPCODE_CALLBACK(SetRegisterImmediate64)
		{
			const Ins* I = NextInstruction();
			auto* args = (ArgsSetRegister64*)I;
			cpu.D[args->reg].int64Value = args->value;
			cpu.AdvancePC(sizeof(ArgsSetRegister64));
		}

		OPCODE_CALLBACK(Peek64)
		{
			const Ins* I = NextInstruction();
			int offset = *((int32*) (cpu.PC() + 3));
			VariantValue& target = cpu.D[I->Opmod2];
			const void* source = (const void*) (cpu.D[I->Opmod1].charPtrValue + offset);
			target.int64Value = *((const int64 *) source);
			cpu.AdvancePC(7);
		}

		OPCODE_CALLBACK(Test64)
		{
			const Ins* I = NextInstruction();
			auto* args = (ArgsOperateOnRegister*)I;
			cpu.D[REGISTER_SR].uint32Value = 0;
			VariantValue& v = cpu.D[args->reg];
			cpu.SR() |= v.int64Value ? 0 : STATUSBIT_EQUIVALENCE;
			cpu.SR() |= v.int64Value >= 0 ? 0 : STATUSBIT_NEGATIVE;
			cpu.AdvancePC(sizeof(ArgsOperateOnRegister));
		}
		
		OPCODE_CALLBACK(DoubleAdd)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& a = cpu.D[I->Opmod1];
			const VariantValue& b = cpu.D[I->Opmod2];
			VariantValue& c = cpu.D[I->Opmod1-1];
			c.doubleValue = a.doubleValue + b.doubleValue;
			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(DoubleSubtract)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& a = cpu.D[I->Opmod1];
			const VariantValue& b = cpu.D[I->Opmod2];
			VariantValue& c = cpu.D[I->Opmod1-1];
			c.doubleValue = a.doubleValue - b.doubleValue;
			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(DoubleMultiply)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& a = cpu.D[I->Opmod1];
			const VariantValue& b = cpu.D[I->Opmod2];
			VariantValue& c = cpu.D[I->Opmod1-1];
			c.doubleValue = a.doubleValue * b.doubleValue;
			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(DoubleDivide)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& a = cpu.D[I->Opmod1];
			const VariantValue& b = cpu.D[I->Opmod2];
			VariantValue& c = cpu.D[I->Opmod1-1];
			c.doubleValue = a.doubleValue / b.doubleValue;
			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(DoubleSin)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& src = cpu.D[I->Opmod1];
			VariantValue& target = cpu.D[I->Opmod2];
			target.doubleValue = sin(src.doubleValue);
			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(DoubleCos)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& src = cpu.D[I->Opmod1];
			VariantValue& target = cpu.D[I->Opmod2];
			target.doubleValue = cos(src.doubleValue);
			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(DoubleTan)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& src = cpu.D[I->Opmod1];
			VariantValue& target = cpu.D[I->Opmod2];
			target.doubleValue = tan(src.doubleValue);
			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(DoubleArcSin)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& src = cpu.D[I->Opmod1];
			VariantValue& target = cpu.D[I->Opmod2];
			target.doubleValue = asin(src.doubleValue);
			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(DoubleArcCos)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& src = cpu.D[I->Opmod1];
			VariantValue& target = cpu.D[I->Opmod2];
			target.doubleValue = acos(src.doubleValue);
			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(DoubleArcTan)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& src = cpu.D[I->Opmod1];
			VariantValue& target = cpu.D[I->Opmod2];
			target.doubleValue = atan(src.doubleValue);
			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(DoubleArcTan2)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& srcX = cpu.D[I->Opmod1];
			const VariantValue& srcY = cpu.D[I->Opmod1+1];
			VariantValue& target = cpu.D[I->Opmod2];
			target.doubleValue = atan2(srcY.doubleValue, srcX.doubleValue);
			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(DoubleSinh)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& src = cpu.D[I->Opmod1];
			VariantValue& target = cpu.D[I->Opmod2];
			target.doubleValue = sinh(src.doubleValue);
			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(DoubleCosh)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& src = cpu.D[I->Opmod1];
			VariantValue& target = cpu.D[I->Opmod2];
			target.doubleValue = cosh(src.doubleValue);
			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(DoubleExp)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& src = cpu.D[I->Opmod1];
			VariantValue& target = cpu.D[I->Opmod2];
			target.doubleValue = exp(src.doubleValue);
			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(DoubleLn)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& src = cpu.D[I->Opmod1];
			VariantValue& target = cpu.D[I->Opmod2];
			target.doubleValue = log(src.doubleValue);
			cpu.AdvancePC(4);
		}

		OPCODE_CALLBACK(LogicalAND64)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& A = cpu.D[I->Opmod1];
			const VariantValue& B = cpu.D[I->Opmod2];
			VariantValue& C = cpu.D[I->Opmod1-1];
			cpu.AdvancePC(3);
			C.int64Value = A.int64Value & B.int64Value;
		}

		OPCODE_CALLBACK(LogicalOR64)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& A = cpu.D[I->Opmod1];
			const VariantValue& B = cpu.D[I->Opmod2];
			VariantValue& C = cpu.D[I->Opmod1-1];
			cpu.AdvancePC(3);
			C.int64Value = A.int64Value | B.int64Value;
		}

		OPCODE_CALLBACK(LogicalXOR64)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& A = cpu.D[I->Opmod1];
			const VariantValue& B = cpu.D[I->Opmod2];
			VariantValue& C = cpu.D[I->Opmod1-1];
			cpu.AdvancePC(3);
			C.int64Value = A.int64Value ^ B.int64Value;
		}
		
		OPCODE_CALLBACK(LogicalNOT64)
		{
			const Ins* I = NextInstruction();	
			const VariantValue& A = cpu.D[I->Opmod1];
			VariantValue& B = cpu.D[I->Opmod2];
			cpu.AdvancePC(3);
			B.int64Value = ~A.int64Value;
		}

		OPCODE_CALLBACK(Move64)
		{
			const Ins* I = NextInstruction();		
			cpu.D[I->Opmod2].int64Value = cpu.D[I->Opmod1].int64Value;
			cpu.AdvancePC(3);
		}

		OPCODE_CALLBACK(IntAdd64)
		{
			const Ins* I = NextInstruction();
			cpu.AdvancePC(3);

			const VariantValue& A = cpu.D[I->Opmod1];
			const VariantValue& B = cpu.D[I->Opmod2];
			VariantValue& C = cpu.D[I->Opmod1-1];

			C.int64Value = A.int64Value + B.int64Value;
		}

		OPCODE_CALLBACK(IntNegate64)
		{
			const Ins* I = NextInstruction();
			cpu.AdvancePC(3);

			const VariantValue& src = cpu.D[I->Opmod1];
			VariantValue& trg = cpu.D[I->Opmod2];

			trg.int64Value = -src.int64Value;
		}

		OPCODE_CALLBACK(IntSubtract64)
		{
			const Ins* I = NextInstruction();
			cpu.AdvancePC(3);

			const VariantValue& A = cpu.D[I->Opmod1];
			const VariantValue& B = cpu.D[I->Opmod2];
			VariantValue& C = cpu.D[I->Opmod1-1];

			C.int64Value = A.int64Value - B.int64Value;
		}

		OPCODE_CALLBACK(IntMultiply64)
		{
			const Ins* I = NextInstruction();
			cpu.AdvancePC(3);

			const VariantValue& A = cpu.D[I->Opmod1];
			const VariantValue& B = cpu.D[I->Opmod2];
			VariantValue& C = cpu.D[I->Opmod1-1];

			C.int64Value = A.int64Value * B.int64Value;
		}

		OPCODE_CALLBACK(IntDivide64)
		{
			const Ins* I = NextInstruction();
			cpu.AdvancePC(3);

			const VariantValue& A = cpu.D[I->Opmod1];
			const VariantValue& B = cpu.D[I->Opmod2];
			VariantValue& quotient = cpu.D[I->Opmod1-1];

			quotient.int64Value = A.int64Value / B.int64Value;
		}

		OPCODE_CALLBACK(GetStackFrameValue64)
		{
			const Ins* I = NextInstruction();
			int32 offset = (int32)(int8) I->Opmod1;
			const uint64* source = (uint64*)(cpu.D[REGISTER_SF].uint8PtrValue + offset);
			VariantValue& target = cpu.D[I->Opmod2];
			target.uint64Value = *source;
			cpu.AdvancePC(3);
		}

		OPCODE_CALLBACK(SetStackFrameValue64)
		{
			const Ins* I = NextInstruction();
			int32 offset = (int32)(int8) I->Opmod1;
			uint64* target = (uint64*)(cpu.D[REGISTER_SF].uint8PtrValue + offset);
			const VariantValue& source = cpu.D[I->Opmod2];
			*target = source.uint64Value;
			cpu.AdvancePC(3);
		}

		OPCODE_CALLBACK(SetStackFrameImmediate64)
		{
			const Ins* I = NextInstruction();
			int32 offset = (int32)(int8) I->Opmod1;
			cpu.AdvancePC(2);
			uint64 value = *((uint64*) cpu.PC());
			uint8* target = cpu.D[REGISTER_SF].uint8PtrValue + offset;
			*((uint64*) target) = value;
			cpu.AdvancePC(8);
		}

		OPCODE_CALLBACK(GetStackFrameValueFar)
		{
			const Ins* I = NextInstruction();

			BITCOUNT bitCount = (BITCOUNT) I->Opmod1;
			VariantValue& target = cpu.D[I->Opmod2];
			cpu.AdvancePC(3);

			int32* pOffset = (int32*) (cpu.PC());
			const uint8* source = cpu.D[REGISTER_SF].uint8PtrValue + *pOffset;
			cpu.AdvancePC(4);

			switch(bitCount)
			{
			case BITCOUNT_32:
				target.uint32Value = *(uint32*) source;
				break;
			case BITCOUNT_64:
				target.uint64Value = *(uint64*) source;
				break;
			default:
				throw IllegalException();
			}
		}

		OPCODE_CALLBACK(SetStackFrameValueFar)
		{
			const Ins* I = NextInstruction();

			BITCOUNT bitCount = (BITCOUNT) I->Opmod1;
			const VariantValue& source = cpu.D[I->Opmod2];
			cpu.AdvancePC(3);

			int32* pOffset = (int32*) (cpu.PC());
			uint8* target = cpu.D[REGISTER_SF].uint8PtrValue + *pOffset;
			cpu.AdvancePC(4);

			switch(bitCount)
			{
			case BITCOUNT_32:
				*(uint32*) target = source.uint32Value;
				break;
			case BITCOUNT_64:
				*(uint64*) target = source.uint64Value;
				break;
			default:
				throw IllegalException();
			}
		}

		OPCODE_CALLBACK(SetStackFrameImmediateFar)
		{
			const Ins* I = NextInstruction();
			BITCOUNT bitCount = (BITCOUNT) I->Opmod1;
			cpu.AdvancePC(2);

			uint32 offset = *((uint32*) cpu.PC());

			cpu.AdvancePC(4);

			const uint8* source = cpu.PC();
			uint8* target = cpu.D[REGISTER_SF].uint8PtrValue + offset;
			
			switch(bitCount)
			{
			case BITCOUNT_32:
				*(uint32*)target = *(uint32*)source;
				cpu.AdvancePC(4);
				break;

			case BITCOUNT_64:
				*(uint64*)target = *(uint64*)source;
				cpu.AdvancePC(8);
				break;

			default:
				throw InvalidInstructionException();
			}
		}

		OPCODE_CALLBACK(Yield)
		{
			cpu.AdvancePC(1);
			status = EXECUTERESULT_YIELDED;
			if (throwToQuit) throw YieldException();
		}

		OPCODE_CALLBACK(Exit)
		{
			const Ins* I = NextInstruction();
			Terminate(cpu.D[I->Opmod1].int32Value);
		}
	
		OPCODE_CALLBACK(NoOperation)
		{
			cpu.AdvancePC(1);
		}	
		
		OPCODE_CALLBACK(BadInstruction)
		{
			TerminateByIllegal(-1);
		}

		OPCODE_CALLBACK(Debug)
		{
			cpu.AdvancePC(1);
			status = EXECUTERESULT_BREAKPOINT;
			if (throwToQuit) throw YieldException();
		}

		OPCODE_CALLBACK(GetStackFrameValueAndExtendToPointer)
		{
			const Ins* I = NextInstruction();
			uint8 target = I->Opmod1;
			cpu.AdvancePC(2);

			const int32* pOffset = (const int32*) cpu.PC();
			int32 offset = *pOffset;
			cpu.AdvancePC(4);

			const uint32* pValue = (const uint32*) (cpu.SF() + (ptrdiff_t) offset);

			cpu.D[target].sizetValue = (size_t) *pValue;
		}

		int64 PopInt64() override
		{
			return cpu.Pop<int64>();
		}

		double PopFloat64() override
		{
			return cpu.Pop<float64>();
		}

		void Push(double value) override
		{
			cpu.Push(value);
		}

		void Push(int64 value) override
		{
			cpu.Push(value);
		}

		float PopFloat32() override
		{
			return cpu.Pop<float32>();
		}

		int32 PopInt32() override
		{
			return cpu.Pop<int32>();
		}

		void* PopPointer() override
		{
			return cpu.Pop<void*>();
		}

		void PushBlob(void* ptr, int nBytes) override
		{			
			uint8* sp = (uint8*) cpu.SP();
			memcpy(sp, ptr, nBytes);
			sp += nBytes;
			cpu.D[REGISTER_SP].uint8PtrValue = sp;
		}

		void Push(float value) override
		{
			cpu.Push(value);
		}

		void Push(int32 value) override
		{
			cpu.Push(value);
		}
		
		void Push(void* ptr) override
		{
			cpu.Push(ptr);
		}

		void Free() override;
	};

	CVirtualMachine::FN_VM* CVirtualMachine::s_instructionTable = NULL;  

	ptrdiff_t GetCpuToVMOffset()
	{
		size_t padding = (16 - sizeof(CPU) % 16) & (size_t)0xF;
		return padding + sizeof(CPU);
	}

	void CVirtualMachine::Free()
	{ 
      // hack, wwe use in-place allocation to create the instance, so dont call delete. See: IVirtualMachine* CreateVirtualMachine(ICore& core)
		this->~CVirtualMachine();

		uint8* mem = (uint8*) this;
		
		uint8* pCpu = mem - GetCpuToVMOffset();

		Rococo::VM::OS::FreeAlignedMemory(pCpu, Anon::GetCpuToVMOffset() + sizeof(Anon::CVirtualMachine));
	}
}

namespace Rococo { namespace VM 
{
	IVirtualMachine* CreateVirtualMachine(ICore& core)
	{
		enum { CACHE_LINE_ALIGN = 128 };

		uint8* mem = (uint8*) Rococo::VM::OS::AllocAlignedMemory(Anon::GetCpuToVMOffset() + sizeof(Anon::CVirtualMachine), CACHE_LINE_ALIGN);
		CPU* cpu = new (mem) CPU();
		return new (mem + Anon::GetCpuToVMOffset()) Anon::CVirtualMachine(core, *cpu);
	}
}} // Rococo::VM
