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

#include "Sexy.VM.StdAfx.h"
#include "Sexy.VM.CPU.h"
#include <math.h>
#include "Sexy.VM.OS.h"

#include <excpt.h>

using namespace Sexy;
using namespace Sexy::VM;

#define ActivateInstruction(x) s_instructionTable[Opcodes::##x] = &CVirtualMachine::OnOpcode##x;
#define OPCODE_CALLBACK_CONVENTION _fastcall
#define OPCODE_CALLBACK(x) void OPCODE_CALLBACK_CONVENTION OnOpcode##x()

static_assert(sizeof(Sexy::VariantValue) == 8, "The codebase assumes that sizeof(Sexy::VariantValue) is 8. Major changes are needed if this is not so.");

namespace
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
		case Opcodes::CallVirtualFunctionByValue:
			return true;
		default:
			return false;
		}
	}

	class CVirtualMachine: public IVirtualMachine, public IStepCallback
	{
	private:
		volatile EXECUTERESULT status;		

		uint8* stack;
		size_t stackSize;
		std::vector<uint8> globalData;

		TMemory breakpoints;
		int exitCode;	
		bool throwToQuit;
		ICore& core;

		IProgramMemory* program;
		
		CPU& cpu;

		typedef void (OPCODE_CALLBACK_CONVENTION CVirtualMachine::*FN_VM)();

		static FN_VM* s_instructionTable; 

		int isBeingStepped;

		IStepCallback* stepCallback;

	public:
		CVirtualMachine(ICore& _core, CPU& _cpu) : cpu(_cpu), core(_core), program(nullptr), stack(nullptr), isBeingStepped(0)
		{
			SetStackSize(64 * 1024);

			stepCallback = this;
			
			if (s_instructionTable != NULL) return;

			s_instructionTable = new FN_VM[Opcodes::MAX_OPCODES];
			for(uint32 i = 0; i < Opcodes::MAX_OPCODES; ++i)
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
			ActivateInstruction(CallVirtualFunctionByValue);
			ActivateInstruction(CallVirtualFunctionByAddress);
			ActivateInstruction(CopySFMemory);
			ActivateInstruction(CopySFMemoryNear);
			ActivateInstruction(CopyMemory);
			ActivateInstruction(CopyMemoryBig);
			ActivateInstruction(Copy32Bits);
			ActivateInstruction(Copy64Bits);
			ActivateInstruction(Invoke);
			ActivateInstruction(InvokeBy);
			ActivateInstruction(Return);
			ActivateInstruction(Yield);
			ActivateInstruction(PushIndirect);
			ActivateInstruction(PushRegister64);
			ActivateInstruction(PushRegister32);
			ActivateInstruction(PushAddress);
			ActivateInstruction(PushImmediate32);
			ActivateInstruction(PushStackVariable32);
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
			ActivateInstruction(SetSFValueFromSFMemberByRef32);
			ActivateInstruction(SetSFMemberByRefFromRegister32);

			ActivateInstruction(RestoreRegister32);
			ActivateInstruction(SaveRegister32);			
			ActivateInstruction(RestoreRegister64);
			ActivateInstruction(SaveRegister64);	
			ActivateInstruction(TripDebugger);

			ActivateInstruction(GetGlobal);
			ActivateInstruction(SetGlobal);

			ActivateInstruction(GetStackFrameValueAndExtendToPointer);
		}

		IVirtualMachine* Clone(CPU& _cpu)
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

		void OnStep(IDebugger& debugger)
		{
		}

		void StepNext()
		{
		}

		bool RouteSysMessages()
		{
			return VM::OS::RouteSysMessages();
		}

		void SetStepCallback(IStepCallback* stepCallback)
		{
			this->stepCallback = stepCallback != NULL ? stepCallback : this;
		}

		virtual void Pause()
		{
			if (status == EXECUTERESULT_RUNNING) status = EXECUTERESULT_YIELDED;
		}

		virtual bool IsRunning() const
		{
			return status == EXECUTERESULT_RUNNING;
		}

		virtual void ClearBreakpoint(size_t offset)
		{
			if (offset < breakpoints.size())
			{
				breakpoints[offset] = 0;
			}
		}

		virtual void SetBreakpoint(size_t offset)
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

		virtual EXECUTERESULT Debug()
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
			return VM::OS::ExecuteProtected(ANON::ProtectedDebug, this, OUT cpu.ExceptionCode);
		}

		virtual void Throw()
		{
			status = EXECUTERESULT_THROWN;
			TerminateByIllegal(0);
		}

		virtual void GetStackTrace(FN_OnStackTrace fnCallback, void* context)
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

		virtual void EnumBreakpoints(FN_BREAKPOINT_CALLBACK fnCallback, void* context)
		{		
			for(size_t i = 0; i < breakpoints.size(); ++i)
			{
				if (breakpoints[i] != 0)
				{
					fnCallback(context, i);
				}
			}
		}

		EXECUTERESULT Continue(const ExecutionFlags& ef)
		{
			if (status == EXECUTERESULT_YIELDED) status = EXECUTERESULT_RUNNING;

			if (ef.RunProtected)
			{
				struct ANON
				{
					static EXECUTERESULT ProtectedContinue(void* context, bool throwToQuit)
					{
						CVirtualMachine* vm = (CVirtualMachine*) context;
						return vm->ProtectedContinue(throwToQuit);
					}
				};
				return VM::OS::ExecuteProtected(ANON::ProtectedContinue, this, OUT cpu.ExceptionCode, ef.ThrowToQuit);
			}
			else
			{
				cpu.ExceptionCode = EXCEPTIONCODE_DISABLED;
				return ProtectedContinue(ef.ThrowToQuit);
			}
		}

		void InitCpu()
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

		void InitPC()
		{
			cpu.D[REGISTER_PC].uint8PtrValue = cpu.ProgramStart;
		}

		EXECUTERESULT ExecuteFunction(ID_BYTECODE codeId)
		{
			const uint8* context = cpu.SF();
			cpu.Push(context);

			const uint8 *returnAddress = cpu.PC(); 
			cpu.Push(returnAddress);

			// Then make the new stack frame equal to the stack pointer
			cpu.D[REGISTER_SF].charPtrValue = cpu.D[REGISTER_SP].charPtrValue;

			size_t functionStart = program->GetFunctionAddress(codeId);
			cpu.SetPC(cpu.ProgramStart + functionStart);

			__try
			{
				while(cpu.SF() > context && status == EXECUTERESULT_RUNNING)
				{
					Advance();
				}
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
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

		virtual EXECUTERESULT ExecuteFunctionProtected(ID_BYTECODE codeId)
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
			VM::OS::BreakOnThrow();
			
			this->exitCode = exitCode;
			if (throwToQuit) throw IllegalException();
		}

		virtual EXECUTERESULT ContinueExecution(const ExecutionFlags& ef)
		{
			return Continue(ef);
		}

		virtual void StepNextSymbol(ISymbols& symbols)
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

			FileData start;
			if (!symbols.TryGetSymbol(cpu.PC() - cpu.ProgramStart, OUT start))
			{
				StepInto(true);
				return;
			}

			if (status == EXECUTERESULT_YIELDED || status == EXECUTERESULT_BREAKPOINT) 
				status = EXECUTERESULT_RUNNING;

			while(status == EXECUTERESULT_RUNNING)
			{
				FileData fd;
				if (symbols.TryGetSymbol(cpu.PC() - cpu.ProgramStart, OUT fd))
				{
					if (fd.Source != start.Source || fd.Pos.X != start.Pos.X || fd.Pos.Y != start.Pos.Y)
					{
						break;
					}
				}

				status = VM::OS::ExecuteProtected(ANON::ProtectedAdvance, this, OUT cpu.ExceptionCode);
			}
		}

		virtual void StepInto(bool ignoreCallbacks)
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
				status = VM::OS::ExecuteProtected(ANON::ProtectedAdvance, this, OUT cpu.ExceptionCode);
				isBeingStepped--;
			}
		}

		virtual void StepOver()
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
				status = VM::OS::ExecuteProtected(ANON::ProtectedRunUntilReturn, this, OUT cpu.ExceptionCode);
			}
		}

		virtual void StepOut()
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
				status = VM::OS::ExecuteProtected(ANON::ProtectedStepOut, this, OUT cpu.ExceptionCode);
			}
		}

		virtual CPU& Cpu() { return cpu; }
		virtual int ExitCode() const { return exitCode; }
		virtual ICore& Core()	{ return core;	}
		virtual void Free();

		virtual void SetStackSize(size_t nBytes)
		{ 
			if (stack != NULL)
			{
				VM::OS::FreeAlignedMemory(stack, stackSize);
				stack = NULL;
			}
			
			stackSize = nBytes;

			if (stack == NULL && stackSize > 0)
			{
				stack = (uint8*) VM::OS::AllocAlignedMemory(stackSize); 
			}

			// N.B in 64-mode we need to convert parentSF to and from 32-bit uint32. This can only work if the maximum stack value is less than 32-bits
			enum { MAX_STACK_POINTER = 0x20000000 };
			const uint8* maxPtr = (const uint8*) MAX_STACK_POINTER;
			if (false && stack + nBytes > maxPtr)
			{
				VM::OS::FreeAlignedMemory(stack, stackSize);

				Sexy::OS::OSException ex;
				ex.exceptionNumber = 0;
				StringPrint(ex.message, 256, SEXTEXT("The SexyVM stack end %p exceeded the maximum pointer value of %p"), stack, maxPtr);

				stack = NULL;

				throw ex;
			}
		}

		virtual void SetGlobalData(const uint8* globalData, size_t nBytes)
		{
			this->globalData.resize(nBytes);
			memcpy(&this->globalData[0], globalData, nBytes);
		}

		virtual void SetStatus(EXECUTERESULT status)
		{
			this->status = status;
		}

		virtual EXECUTERESULT Status() const
		{
			return status;
		}

		inline const Ins* NextInstruction()
		{
			return reinterpret_cast<const Ins*>(cpu.PC());
		}

		virtual void SetProgram(IProgramMemory* program)
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

		virtual EXECUTERESULT Execute(const ExecutionFlags& ef)
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

			return Continue(ef);
		}
		
		void Advance()
		{
			const uint8* pc = cpu.PC();
			Opcodes::OPCODE i = (Opcodes::OPCODE) *pc;
			FN_VM fn = s_instructionTable[i];
			(this->*fn)();
		}

		EXECUTERESULT ProtectedContinue(bool throwToQuit)
		{
			status = EXECUTERESULT_RUNNING;
			this->throwToQuit = throwToQuit;
			
			if (throwToQuit)
			{
				try
				{	
					while(true)
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
			target.int64Value = *(int64*) pMember;
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
			VariantValue& target = cpu.D[I->Opmod1];
			target.int32Value++;
			cpu.AdvancePC(2);
		}

		OPCODE_CALLBACK(Decrement32)
		{
			const Ins* I = NextInstruction();
			VariantValue& target = cpu.D[I->Opmod1];
			target.int32Value--;
			cpu.AdvancePC(2);
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
			VariantValue& remainder = cpu.D[I->Opmod2+1];

			quotient.int32Value = A.int32Value / B.int32Value;
			remainder.int32Value = A.int32Value % B.int32Value;
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

		OPCODE_CALLBACK(PushStackVariable32)
		{
			const Ins* I = NextInstruction();
			int32 offset = (int32) (int8) I->Opmod1;
			const uint8* sfItem = cpu.SF() + offset;
			const int32* pArg = (const int32*) sfItem;
			cpu.Push(*pArg);
			cpu.AdvancePC(2);
		}

		OPCODE_CALLBACK(PushStackAddress)
		{
			const Ins* I = NextInstruction();
			int32 offset = (int32) (int8) I->Opmod1;
			const uint8* sfItem = cpu.SF() + offset;
			cpu.Push(sfItem);
			cpu.AdvancePC(2);
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

			const Ins* I = NextInstruction();
			const VariantValue& offsetRegister = cpu.D[I->Opmod1];

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

		OPCODE_CALLBACK(CallVirtualFunctionByValue)
		{
			const Ins* I = NextInstruction();
			const int32* offsetArray = (const int32*) (cpu.PC() + 1);

			int32 sfOffset = *offsetArray;
			int32 methodIndex = offsetArray[1];
			int32 memberOffset = offsetArray[2];

			const uint8* sfItem = cpu.SF() + sfOffset;
			const uint8** pInterface = (const uint8**) sfItem;

			const void** pVTable = (const void**) (*pInterface + memberOffset);

			cpu.Push(pVTable);

			cpu.Push(cpu.D[REGISTER_SF].vPtrValue);

			const uint8 *returnAddress = cpu.PC() + 13; 
			cpu.Push(returnAddress);						

			// Then make the new stack frame equal to the stack pointer
			cpu.D[REGISTER_SF].charPtrValue = cpu.D[REGISTER_SP].charPtrValue;
								
			const uint8* vTable = (const uint8*) *pVTable;

			const ID_BYTECODE* pFunction = (const ID_BYTECODE*) (vTable + methodIndex);
			ID_BYTECODE id = *pFunction;
			size_t functionStart = program->GetFunctionAddress(id);
			cpu.SetPC(cpu.ProgramStart + functionStart);
		}

		OPCODE_CALLBACK(CallVirtualFunctionByAddress)
		{
			const Ins* I = NextInstruction();
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
			const Ins* I = NextInstruction();
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
			uint8* target = cpu.D[I->Opmod1].uint8PtrValue;
			const uint8* source = cpu.D[I->Opmod2].uint8PtrValue;
			
			*(int32*) target = *(const int32*) source;

			cpu.AdvancePC(3);			
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

			const Ins* I = NextInstruction();
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
			DINDEX index = I->Opmod1;
			uint32* value = (uint32*)(I->ToPC() + 2);
			cpu.D[index].uint32Value = *value;
			cpu.AdvancePC(6);
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
			cpu.D[REGISTER_SR].uint32Value = 0;
			VariantValue& v = cpu.D[I->Opmod1];
			cpu.SR() |= v.int32Value ? 0 : STATUSBIT_EQUIVALENCE;
			cpu.SR() |= v.int32Value < 0 ? STATUSBIT_NEGATIVE : 0;
			cpu.AdvancePC(2);
		}

		OPCODE_CALLBACK(BranchIfGTE)
		{
			const Ins* I = NextInstruction();	
			int* pOffset = (int*) (((uint8*) I) + 1);
			int32 offset = (IsEquiSet(cpu) || !IsNegSet(cpu)) ? *pOffset : 5;
			cpu.AdvancePC(offset);
		}

		OPCODE_CALLBACK(BranchIfGT)
		{
			const Ins* I = NextInstruction();	
			int* pOffset = (int*) (((uint8*) I) + 1);
			int32 offset = (!IsEquiSet(cpu) && !IsNegSet(cpu)) ? *pOffset : 5;
			cpu.AdvancePC(offset);
		}

		OPCODE_CALLBACK(BranchIfLT)
		{
			const Ins* I = NextInstruction();	
			int* pOffset = (int*) (((uint8*) I) + 1);
			int32 offset = (!IsEquiSet(cpu) && IsNegSet(cpu)) ? *pOffset : 5;
			cpu.AdvancePC(offset);
		}

		OPCODE_CALLBACK(BranchIfNE)
		{
			const Ins* I = NextInstruction();		
			int* pOffset = (int*) (((uint8*) I) + 1);
			int32 offset = (!IsEquiSet(cpu)) ? *pOffset : 5;
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
			DINDEX index = I->Opmod1;

			uint64* value = (uint64*)(I->ToPC() + 2);
			cpu.D[index].uint64Value = *value;
			cpu.AdvancePC(10);
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
			cpu.D[REGISTER_SR].uint32Value = 0;
			VariantValue& v = cpu.D[I->Opmod1];
			cpu.SR() |= v.int64Value ? 0 : STATUSBIT_EQUIVALENCE;
			cpu.SR() |= v.int64Value >= 0 ? 0 : STATUSBIT_NEGATIVE;
			cpu.AdvancePC(2);
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
			VariantValue& remainder = cpu.D[I->Opmod2+1];

			quotient.int64Value = A.int64Value / B.int64Value;
			remainder.int64Value = A.int64Value % B.int64Value;
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

		OPCODE_CALLBACK(TripDebugger)
		{
			const Ins* I = NextInstruction();
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

		virtual int64 PopInt64()
		{
			return cpu.Pop<int64>();
		}

		virtual double PopFloat64()
		{
			return cpu.Pop<float64>();
		}

		virtual void Push(double value)
		{
			cpu.Push(value);
		}

		virtual void Push(int64 value)
		{
			cpu.Push(value);
		}

		virtual float PopFloat32()
		{
			return cpu.Pop<float32>();
		}

		virtual int32 PopInt32()
		{
			return cpu.Pop<int32>();
		}

		virtual void* PopPointer()
		{
			return cpu.Pop<void*>();
		}

		virtual void PushBlob(void* ptr, int nBytes)
		{			
			uint8* sp = (uint8*) cpu.SP();
			memcpy(sp, ptr, nBytes);
			sp += nBytes;
			cpu.D[REGISTER_SP].uint8PtrValue = sp;
		}

		virtual void Push(float value)
		{
			cpu.Push(value);
		}

		virtual void Push(int32 value)
		{
			cpu.Push(value);
		}
		
		virtual void Push(void* ptr)
		{
			cpu.Push(ptr);
		}
	};

	CVirtualMachine::FN_VM* CVirtualMachine::s_instructionTable = NULL;  

	ptrdiff_t GetCpuToVMOffset()
	{
		size_t padding = (16 - sizeof(CPU) % 16) & (size_t)0xF;
		return padding + sizeof(CPU);
	}

	void CVirtualMachine::Free()
	{ 
		this->~CVirtualMachine();

		uint8* mem = (uint8*) this;
		
		uint8* pCpu = mem - GetCpuToVMOffset();
		CPU& cpu = (CPU&) *pCpu; // no destructor on this
		_aligned_free(pCpu);
	}
}

namespace Sexy { namespace VM 
{
	IVirtualMachine* CreateVirtualMachine(ICore& core)
	{
		enum { CACHE_LINE_ALIGN = 128 };
		uint8* mem = (uint8*)_aligned_malloc(GetCpuToVMOffset() + sizeof(CVirtualMachine), CACHE_LINE_ALIGN);

		CPU* cpu = new (mem) CPU();
		return new (mem + GetCpuToVMOffset()) CVirtualMachine(core, *cpu);
	}
}} // Sexy::VM