// svmhost.cpp : Defines the entry point for the console application.

#include "Sexy.VM.Test.StdAfx.h"
#include "Sexy.VM.h"
#include "Sexy.VM.CPU.h"

#include <vector>
#include <intrin.h>

#define validate(_Expression) if (!(_Expression)) { ShowFailure(#_Expression, __FILE__, __LINE__); Abort(); }

#include "sexy.lib.util.h"
#include "sexy.lib.vm.h"

using namespace Sexy;
using namespace Sexy::VM;

namespace
{
	const float PIf = 3.1415926535897932384626433832795f;

	void Disassemble(IAssembler& a);

	void Abort()
	{
		if (IsDebuggerPresent())
			__debugbreak();
		else
			exit(-1); 
	}

	void ShowFailure(const char* expression, const char* filename, int lineNumber)
	{
		printf("Validation failed in %s[%d]: %s\r\n", filename, lineNumber, expression);
	}

	int64 TimerTicks()
	{
		LARGE_INTEGER ticks;
		QueryPerformanceCounter(&ticks);
		return ticks.QuadPart;
	}

	int64 TimerHz()
	{
		LARGE_INTEGER hz;
		QueryPerformanceFrequency(&hz);
		return hz.QuadPart;
	}

	void Run(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		ID_BYTECODE id = pm.AddBytecode();
		pm.UpdateBytecode(id, a);
		vm.SetProgram(&pm);
		vm.InitCpu();
		vm.Execute(ExecutionFlags(false, false));
	}

	void RunProtected(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		ID_BYTECODE id = pm.AddBytecode();
		pm.UpdateBytecode(id, a);
		vm.SetProgram(&pm);
		vm.InitCpu();
		vm.Execute(ExecutionFlags(true, true));
	}

	void TestSetRegisterImmediate(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		VariantValue v;
		v.uint32Value = 0x12345678;
		a.Append_SetRegisterImmediate(REGISTER_D4, v, BITCOUNT_32);

		v.uint64Value = 0xFCDEBA9010203040;
		a.Append_SetRegisterImmediate(REGISTER_D5, v, BITCOUNT_64);

		a.Append_Exit(REGISTER_D4);
		Run(a,vm,pm);

		CPU cpu = vm.Cpu();

		validate(vm.ExitCode() == (int32) 0x12345678);
		validate(cpu.D[REGISTER_D4].uint32Value == 0x12345678);
		validate(cpu.D[REGISTER_D5].uint64Value == 0xFCDEBA9010203040);
	}

	void TestNoOp(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		a.Append_NoOperation();
		a.Append_Exit(REGISTER_D4);
		Run(a,vm,pm);

		CPU cpu = vm.Cpu();

		validate(cpu.PC() == cpu.ProgramStart + 1);
	}

	void TestTerminate(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		a.Append_Exit(REGISTER_D4);
		Run(a,vm,pm);

		CPU cpu = vm.Cpu();

		validate(cpu.PC() == cpu.ProgramStart);
		validate(vm.ExitCode() == cpu.D[REGISTER_D4].int32Value);
	}

	void TestTest1(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		VariantValue v;
		v.uint32Value = 0x12345678;
		a.Append_SetRegisterImmediate(REGISTER_D4, v, BITCOUNT_32);

		a.Append_Test(REGISTER_D4, BITCOUNT_32);
		a.Append_Exit(REGISTER_D5);
		Run(a,vm,pm);

		CPU cpu = vm.Cpu();

		validate(cpu.PC() == cpu.ProgramStart + 8);
		validate(!IsNegSet(cpu));
		validate(!IsEquiSet(cpu));
	}

	void TestTest4(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		VariantValue v;
		v.int64Value = 0xF000123400001234;
		a.Append_SetRegisterImmediate(REGISTER_D4, v, BITCOUNT_64);

		a.Append_Test(REGISTER_D4, BITCOUNT_64);
		a.Append_Exit(REGISTER_D5);
		Run(a,vm, pm);

		CPU cpu = vm.Cpu();

		validate(cpu.PC() == cpu.ProgramStart + 12);
		validate(IsNegSet(cpu));
		validate(!IsEquiSet(cpu));
	}

	void TestBranch1(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		VariantValue v;
		v.int32Value = 0x5678ABCD;
		a.Append_SetRegisterImmediate(REGISTER_D4, v, BITCOUNT_32);

		a.Append_Branch(6+5);

		v.int32Value = 0xF0001234;
		a.Append_SetRegisterImmediate(REGISTER_D4, v, BITCOUNT_32);

		a.Append_Exit(REGISTER_D4);
		Run(a,vm, pm);

		validate(vm.ExitCode() == 0x5678ABCD);
	}

	void TestBranch3(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		VariantValue v;
		v.int32Value = 0x5678ABCD;
		a.Append_SetRegisterImmediate(REGISTER_D4, v, BITCOUNT_32);

		a.Append_Test(REGISTER_D4, BITCOUNT_32);
		a.Append_BranchIf(CONDITION_IF_GREATER_OR_EQUAL, 6+5);

		v.int32Value = 0xF0001234;
		a.Append_SetRegisterImmediate(REGISTER_D4, v, BITCOUNT_32);

		a.Append_Exit(REGISTER_D4);
		Run(a,vm, pm);

		validate(vm.ExitCode() == 0x5678ABCD);
	}

	void TestBranch4(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		VariantValue v;
		v.int32Value = 0x5678ABCD;
		a.Append_SetRegisterImmediate(REGISTER_D4, v, BITCOUNT_32);

		a.Append_Test(REGISTER_D4, BITCOUNT_32);
		a.Append_BranchIf(CONDITION_IF_GREATER_THAN, 6+5);

		v.int32Value = 0xF0001234;
		a.Append_SetRegisterImmediate(REGISTER_D4, v, BITCOUNT_32);

		a.Append_Exit(REGISTER_D4);
		Run(a,vm, pm);

		validate(vm.ExitCode() == 0x5678ABCD);
	}

	void TestBranch5(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		VariantValue v;
		v.int32Value = 0x5678ABCD;
		a.Append_SetRegisterImmediate(REGISTER_D4, v, BITCOUNT_32);

		a.Append_Test(REGISTER_D4, BITCOUNT_32);
		a.Append_BranchIf(CONDITION_IF_LESS_OR_EQUAL, 6);

		v.int32Value = 0xF0001234;
		a.Append_SetRegisterImmediate(REGISTER_D4, v, BITCOUNT_32);

		a.Append_Exit(REGISTER_D4);
		Run(a,vm, pm);

		validate(vm.ExitCode() == 0xF0001234);
	}

	void TestBranch6(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		VariantValue v;
		v.int32Value = 0x5678ABCD;
		a.Append_SetRegisterImmediate(REGISTER_D4, v, BITCOUNT_32);

		a.Append_Test(REGISTER_D4, BITCOUNT_32);
		a.Append_BranchIf(CONDITION_IF_LESS_THAN, 6);

		v.int32Value = 0xF0001234;
		a.Append_SetRegisterImmediate(REGISTER_D4, v, BITCOUNT_32);

		a.Append_Exit(REGISTER_D4);
		Run(a,vm, pm);

		validate(vm.ExitCode() == 0xF0001234);
	}

	void TestBranch7(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		VariantValue v;
		v.int32Value = 0x5678ABCD;
		a.Append_SetRegisterImmediate(REGISTER_D4, v, BITCOUNT_32);

		a.Append_Test(REGISTER_D4, BITCOUNT_32);
		a.Append_BranchIf(CONDITION_IF_NOT_EQUAL, 6+5);

		v.int32Value = 0xF0001234;
		a.Append_SetRegisterImmediate(REGISTER_D4, v, BITCOUNT_32);

		a.Append_Exit(REGISTER_D4);
		Run(a,vm, pm);

		validate(vm.ExitCode() == 0x5678ABCD);
	}

	void TestMoveRegister(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		VariantValue v;
		v.int32Value = 0x71632422;
		a.Append_SetRegisterImmediate(REGISTER_D4, v, BITCOUNT_32);
		a.Append_MoveRegister(REGISTER_D4, REGISTER_D5, BITCOUNT_32);
		a.Append_Exit(REGISTER_D5);

		Run(a,vm, pm);

		validate(vm.ExitCode() == 0x71632422);
	}

	void TestPoke(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		int32 magic[3] = {-1,-1,-1};
		VariantValue v;
		v.vPtrValue = magic;
		a.Append_SetRegisterImmediate(REGISTER_D4, v, BITCOUNT_POINTER); // Register 4 now points to our magic array

		v.int32Value = 1;
		a.Append_SetRegisterImmediate(REGISTER_D5, v, BITCOUNT_32); // Register 4 now points to our magic array
		a.Append_Poke(REGISTER_D5, BITCOUNT_32, REGISTER_D4, 0); // lookup 32bits of (Register4) + 8 bytes, and store result in 5

		v.int32Value = 2;
		a.Append_SetRegisterImmediate(REGISTER_D5, v, BITCOUNT_32); // Register 4 now points to our magic array
		a.Append_Poke(REGISTER_D5, BITCOUNT_32, REGISTER_D4, 4); // lookup 32bits of (Register4) + 8 bytes, and store result in 5

		a.Append_Exit(REGISTER_D5);

		Run(a,vm, pm);

		validate(magic[0] == 1 && magic[1] == 2 && magic[2] == -1);
	}

	void TestPushIndirect(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		int32 magic[3] = {5,7,11};
		VariantValue v;
		v.vPtrValue = magic;
		a.Append_SetRegisterImmediate(REGISTER_D4, v, BITCOUNT_POINTER); // Register4 now points to our magic array

		int32 miniStack[4] = {0};
		v.vPtrValue = miniStack;
		a.Append_SetRegisterImmediate(REGISTER_D5, v, BITCOUNT_POINTER); // Register5 now points to our ministack

		a.Append_PushIndirect(REGISTER_D4, REGISTER_D5, 4);
		a.Append_PushIndirect(REGISTER_D4, REGISTER_D5, 4);
		a.Append_PushIndirect(REGISTER_D4, REGISTER_D5, 4);

		a.Append_Exit(REGISTER_D7);

		Run(a,vm, pm);

		validate(miniStack[0] == 5 && miniStack[1] == 5 && miniStack[2] == 5 && miniStack[3] == 0);
	}

	void TestPushRegister(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		VariantValue v;
		v.int32Value = 0xFEEDBEEF;
		a.Append_SetRegisterImmediate(REGISTER_D4, v, BITCOUNT_32);

		int32 miniStack[4] = {0};
		v.vPtrValue = miniStack;
		a.Append_SetRegisterImmediate(REGISTER_SP, v, BITCOUNT_POINTER); // Register5 now points to our ministack

		a.Append_PushRegister(REGISTER_D4, BITCOUNT_32);
		a.Append_PushRegister(REGISTER_D4, BITCOUNT_32);
		a.Append_PushRegister(REGISTER_D4, BITCOUNT_32);

		a.Append_Exit(REGISTER_D7);

		Run(a,vm, pm);

		validate(miniStack[0] == 0xFEEDBEEF && miniStack[1] == 0xFEEDBEEF && miniStack[2] == 0xFEEDBEEF && miniStack[3] == 0);

		vm.InitCpu();
	}

	void TestAddImmediate(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		VariantValue v;
		v.int32Value = 1750;
		a.Append_SetRegisterImmediate(REGISTER_D4, v, BITCOUNT_32);

		v.int32Value = 56;
		a.Append_AddImmediate(REGISTER_D4, BITCOUNT_32, REGISTER_D5, v);

		a.Append_Exit(REGISTER_D5);

		Run(a,vm, pm);

		validate(vm.ExitCode() == 1806);
	}

	void TestSubtractImmediate(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		VariantValue v;
		v.int32Value = 1806;
		a.Append_SetRegisterImmediate(REGISTER_D4, v, BITCOUNT_32);

		v.int32Value = 56;
		a.Append_SubtractImmediate(REGISTER_D4, BITCOUNT_32, REGISTER_D5, v);

		a.Append_Exit(REGISTER_D5);

		Run(a,vm, pm);

		validate(vm.ExitCode() == 1750);
	}

	void TestStackFrameImmediate(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		a.Append_SetRegisterImmediate(REGISTER_D4, VariantValue(), BITCOUNT_32);
		int32 callPos = (int32) a.WritePosition();
		a.Append_CallByRegister(REGISTER_D4);
		a.Append_Exit(REGISTER_D5);
		VariantValue v;
		v.int32Value = 0x76543210;
		int32 fnAddress = (int32) a.WritePosition();

		VariantValue sfv;
		sfv.uint32Value = 0xdeadfeed;
		a.Append_SetStackFrameImmediate(4, sfv, BITCOUNT_32);
		sfv.uint64Value = 0xdef0c00cfee1fee4;
		a.Append_SetStackFrameImmediate(8, sfv, BITCOUNT_64);
		a.Append_Yield();
		
		a.Append_SetRegisterImmediate(REGISTER_D5, v, BITCOUNT_32);
		a.Append_Return();

		a.SetWriteModeToOverwrite(0);
		VariantValue fnPos;
		fnPos.int32Value = fnAddress - callPos;
		a.Append_SetRegisterImmediate(REGISTER_D4, fnPos, BITCOUNT_32);

		ID_BYTECODE id = pm.AddBytecode();
		pm.UpdateBytecode(id, a);
		vm.SetProgram(&pm);


		vm.InitCpu();
		validate(vm.Execute(ExecutionFlags(false, true)) == EXECUTERESULT_YIELDED);

		validate(*(vm.Cpu().D[REGISTER_SF].uint32PtrValue + 1) == 0xdeadfeed);
		validate(*(vm.Cpu().D[REGISTER_SF].uint64PtrValue + 1) == 0xdef0c00cfee1fee4);
		
		vm.ContinueExecution(ExecutionFlags(false, true));
		validate(vm.ExitCode() == 0x76543210);
	}

	void TestLogicalAND(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		VariantValue v;
		v.int32Value = 0xFF0000FF;
		a.Append_SetRegisterImmediate(REGISTER_D5, v, BITCOUNT_32);

		v.int32Value = 0x0FF00FF0;
		a.Append_SetRegisterImmediate(REGISTER_D6, v, BITCOUNT_32);
		a.Append_LogicalAND(REGISTER_D5, BITCOUNT_32, REGISTER_D6);
		a.Append_Exit(REGISTER_D4);

		Run(a, vm, pm);

		validate(vm.ExitCode() == 0x0F0000F0);
	}

	void TestLogicalOR(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		VariantValue v;
		v.int32Value = 0xFF0000FF;
		a.Append_SetRegisterImmediate(REGISTER_D5, v, BITCOUNT_32);

		v.int32Value = 0x0FF00FF0;
		a.Append_SetRegisterImmediate(REGISTER_D6, v, BITCOUNT_32);
		a.Append_LogicalOR(REGISTER_D5, BITCOUNT_32, REGISTER_D6);
		a.Append_Exit(REGISTER_D4);

		Run(a, vm, pm);

		validate(vm.ExitCode() == 0xFFF00FFF);
	}

	void TestLogicalXOR(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		VariantValue v;
		v.int32Value = 0xFF0000FF;
		a.Append_SetRegisterImmediate(REGISTER_D5, v, BITCOUNT_32);

		v.int32Value = 0x0FF00FF0;
		a.Append_SetRegisterImmediate(REGISTER_D6, v, BITCOUNT_32);
		a.Append_LogicalXOR(REGISTER_D5, BITCOUNT_32, REGISTER_D6);
		a.Append_Exit(REGISTER_D4);

		Run(a, vm, pm);

		validate(vm.ExitCode() == 0xF0F00F0F);
	}

	void TestLogicalNOT(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		VariantValue v;
		v.int32Value = 0xFF0000FF;
		a.Append_SetRegisterImmediate(REGISTER_D4, v, BITCOUNT_32);

		a.Append_LogicalNOT(REGISTER_D4, BITCOUNT_32, REGISTER_D5);
		a.Append_Exit(REGISTER_D5);

		Run(a, vm, pm);

		validate(vm.ExitCode() == 0x00FFFF00);
	}

	void TestShiftLeft(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		VariantValue v;
		v.int32Value = 0x01234567;
		a.Append_SetRegisterImmediate(REGISTER_D4, v, BITCOUNT_32);
		a.Append_ShiftLeft(REGISTER_D4, BITCOUNT_32, 4);
		a.Append_Exit(REGISTER_D5);

		Run(a, vm, pm);

		validate(vm.ExitCode() == 0x12345670);
	}

	void TestShiftRight(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		VariantValue v;
		v.int32Value = 0x01234567;
		a.Append_SetRegisterImmediate(REGISTER_D4, v, BITCOUNT_32);
		a.Append_ShiftRight(REGISTER_D4, BITCOUNT_32, 4);
		a.Append_Exit(REGISTER_D5);

		Run(a, vm, pm);

		validate(vm.ExitCode() == 0x00123456);
	}

	void TestPop(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		VariantValue v;
		v.int32Value = 0x89789766;
		a.Append_SetRegisterImmediate(REGISTER_D4, v, BITCOUNT_32);
		a.Append_PushRegister(REGISTER_D4, BITCOUNT_32);
		a.Append_Pop(4);
		a.Append_Exit(REGISTER_D5);
		Run(a,vm, pm);
	}

	void TestStackAlloc(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		a.Append_StackAlloc(475);
		a.Append_StackAlloc(-325);
		a.Append_Exit(REGISTER_D5);
		Run(a,vm, pm);
	}

	void TestIntAdd(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		VariantValue v;
		v.int32Value = 58;
		a.Append_SetRegisterImmediate(REGISTER_D5, v, BITCOUNT_32);

		v.int32Value = 12;
		a.Append_SetRegisterImmediate(REGISTER_D6, v, BITCOUNT_32);

		a.Append_IntAdd(REGISTER_D5, BITCOUNT_32, REGISTER_D6);
		a.Append_Exit(REGISTER_D4);
		Run(a,vm, pm);

		validate(vm.ExitCode() == 70);
	}

	void TestIntSubtract(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		VariantValue v;
		v.int32Value = 58;
		a.Append_SetRegisterImmediate(REGISTER_D5, v, BITCOUNT_32);

		v.int32Value = 12;
		a.Append_SetRegisterImmediate(REGISTER_D6, v, BITCOUNT_32);

		a.Append_IntSubtract(REGISTER_D5, BITCOUNT_32, REGISTER_D6);
		a.Append_Exit(REGISTER_D4);
		Run(a,vm, pm);

		validate(vm.ExitCode() == 46);
	}

	void TestIntMultiply(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		VariantValue v;
		v.int32Value = 11;
		a.Append_SetRegisterImmediate(REGISTER_D5, v, BITCOUNT_32);

		v.int32Value = 12;
		a.Append_SetRegisterImmediate(REGISTER_D6, v, BITCOUNT_32);

		a.Append_IntMultiply(REGISTER_D5, BITCOUNT_32, REGISTER_D6);
		a.Append_Exit(REGISTER_D4);
		Run(a,vm, pm);

		validate(vm.ExitCode() == 132);
	}

	void TestIntDivide(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		VariantValue numerator;
		numerator.int32Value = 75;
		a.Append_SetRegisterImmediate(REGISTER_D5, numerator, BITCOUNT_32);

		VariantValue denominator;
		denominator.int32Value = 12;
		a.Append_SetRegisterImmediate(REGISTER_D6, denominator, BITCOUNT_32);

		a.Append_IntDivide(REGISTER_D5, BITCOUNT_32, REGISTER_D6);
		a.Append_Exit(REGISTER_D4);
		Run(a,vm, pm);

		validate(vm.ExitCode() == 6);
		validate(vm.Cpu().D[REGISTER_D7].int32Value == 3);
	}

	void TestIntNegate(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		VariantValue v;
		v.int32Value = 75;
		a.Append_SetRegisterImmediate(REGISTER_D4, v, BITCOUNT_32);
		a.Append_IntNegate(REGISTER_D4, BITCOUNT_32, REGISTER_D5);
		a.Append_Exit(REGISTER_D5);
		Run(a,vm, pm);

		validate(vm.ExitCode() == -75);
	}

	void TestFloatAdd(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		VariantValue v;
		v.floatValue = 3.0f;
		a.Append_SetRegisterImmediate(REGISTER_D5, v, BITCOUNT_32);

		v.floatValue = 4.0f;
		a.Append_SetRegisterImmediate(REGISTER_D6, v, BITCOUNT_32);

		a.Append_FloatAdd(REGISTER_D5, REGISTER_D6, FLOATSPEC_SINGLE);
		a.Append_Exit(REGISTER_D4);

		Run(a,vm, pm);

		validate(vm.Cpu().D[REGISTER_D4].floatValue == 7.0f);
	}

	void TestFloatSubtract(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		VariantValue v;
		v.floatValue = 3.0f;
		a.Append_SetRegisterImmediate(REGISTER_D5, v, BITCOUNT_32);

		v.floatValue = 4.0f;
		a.Append_SetRegisterImmediate(REGISTER_D6, v, BITCOUNT_32);

		a.Append_FloatSubtract(REGISTER_D5, REGISTER_D6, FLOATSPEC_SINGLE);
		a.Append_Exit(REGISTER_D4);

		Run(a,vm, pm);

		validate(vm.Cpu().D[REGISTER_D4].floatValue == -1.0f);
	}

	void validateWithinDelta(float x, float delta)
	{
		validate(x > -delta && x < delta );
	}

	void TestFloatMultiply(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		VariantValue v;
		v.floatValue = 3.0f;
		a.Append_SetRegisterImmediate(REGISTER_D5, v, BITCOUNT_32);

		v.floatValue = 4.0f;
		a.Append_SetRegisterImmediate(REGISTER_D6, v, BITCOUNT_32);

		a.Append_FloatMultiply(REGISTER_D5, REGISTER_D6, FLOATSPEC_SINGLE);
		a.Append_Exit(REGISTER_D4);

		Run(a,vm, pm);

		validate(vm.Cpu().D[REGISTER_D4].floatValue == 12.0f);
	}

	void TestFloatDivide(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		VariantValue v;
		v.floatValue = 3.0f;
		a.Append_SetRegisterImmediate(REGISTER_D5, v, BITCOUNT_32);

		v.floatValue = 4.0f;
		a.Append_SetRegisterImmediate(REGISTER_D6, v, BITCOUNT_32);

		a.Append_FloatDivide(REGISTER_D5, REGISTER_D6, FLOATSPEC_SINGLE);
		a.Append_Exit(REGISTER_D4);

		Run(a,vm, pm);

		validate(vm.Cpu().D[REGISTER_D4].floatValue == 0.75f);
	}

	void TestCallAndReturn(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		a.Append_SetRegisterImmediate(REGISTER_D4, VariantValue(), BITCOUNT_32);
		int callPos = (int32) a.WritePosition();
		a.Append_CallByRegister(REGISTER_D4);
		a.Append_Exit(REGISTER_D5);
		VariantValue v;
		v.int32Value = 0x76543210;
		int fnAddress = (int32) a.WritePosition();
		a.Append_SetRegisterImmediate(REGISTER_D5, v, BITCOUNT_32);
		a.Append_Return();

		a.SetWriteModeToOverwrite(0);
		VariantValue fnPos;
		fnPos.int32Value = fnAddress - callPos;
		a.Append_SetRegisterImmediate(REGISTER_D4, fnPos, BITCOUNT_32);

		Run(a,vm, pm);
		validate(vm.ExitCode() == 0x76543210);
	}

	void TestCallByIdIndirectAndReturn(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		a.Append_CallByIdIndirect(REGISTER_D4);
		a.Append_Exit(REGISTER_D5);

		ID_BYTECODE stubId = pm.AddBytecode();
		pm.UpdateBytecode(stubId, a);

		Disassemble(a);

		a.Clear();

		VariantValue v;
		v.int32Value = 0x76543210;
		size_t fnAddress = a.WritePosition();
		a.Append_SetRegisterImmediate(REGISTER_D5, v, BITCOUNT_32);
		a.Append_Return();

		ID_BYTECODE id = pm.AddBytecode();
		pm.UpdateBytecode(id, a);

		vm.SetProgram(&pm);

		vm.InitCpu();
		vm.InitPC();

		vm.Cpu().D[REGISTER_D4].byteCodeIdValue = id;

		vm.Execute(ExecutionFlags(true,true));

		validate(vm.ExitCode() == 0x76543210);
	}

	void TestCallByIdAndReturn(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		size_t callbyIdPos = a.WritePosition();
		a.Append_CallById(0);
		a.Append_Exit(REGISTER_D5);

		ID_BYTECODE stubId = pm.AddBytecode();
		pm.UpdateBytecode(stubId, a);

		Disassemble(a);

		a.Clear();

		VariantValue v;
		v.int32Value = 0x36543210;
		size_t fnAddress = a.WritePosition();
		a.Append_SetRegisterImmediate(REGISTER_D5, v, BITCOUNT_32);
		a.Append_Return();

		ID_BYTECODE id = pm.AddBytecode();
		pm.UpdateBytecode(id, a);

		Disassemble(a);

		vm.SetProgram(&pm);

		vm.InitCpu();
		vm.InitPC();

		a.Clear();
		a.Append_CallById(id);
		a.Append_Exit(REGISTER_D5);

		pm.UpdateBytecode(stubId, a);

		Disassemble(a);
		a.Clear();

		vm.Execute(ExecutionFlags(true,true));

		validate(vm.ExitCode() == 0x36543210);
	}

	void TestMemoryExceptions(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		VariantValue v;
		v.int32Value = 0x80000000;
		a.Append_SetRegisterImmediate(REGISTER_D4, v, BITCOUNT_32);
		a.Append_CallByRegister(REGISTER_D4);
		a.Append_Exit(REGISTER_D4);
		RunProtected(a,vm, pm);		
		validate(vm.Cpu().ExceptionCode == EXCEPTIONCODE_BAD_ADDRESS);
	}

	void TestOverwrite(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		VariantValue v;
		v.int32Value = 500;
		a.Append_SetRegisterImmediate(REGISTER_D5, v, BITCOUNT_32);
		size_t d4appendPos = a.WritePosition();
		a.Append_SetRegisterImmediate(REGISTER_D4, v, BITCOUNT_32);
		a.Append_Exit(REGISTER_D4);
		
		a.SetWriteModeToOverwrite(d4appendPos);
			v.int32Value = 350;
			a.Append_SetRegisterImmediate(REGISTER_D4, v, BITCOUNT_32);
		a.SetWriteModeToAppend();
	
		Run(a,vm, pm);

		validate(vm.ExitCode() == 350);
	}

	typedef void (*FN_TEST)(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm);

	void Wrap(const char* name, FN_TEST fnTest, IVirtualMachine& vm, IProgramMemory& pm)
	{
		pm.Clear();

		printf("<<<<<< %s\r\n", name);

		IAssembler* assembler = vm.Core().CreateAssembler();
		fnTest(*assembler,vm,pm);
		Disassemble(*assembler);
		assembler->Free();

		CPU cpu = vm.Cpu();

		validate(cpu.PC() >= cpu.ProgramStart && cpu.PC() < cpu.ProgramEnd);
		validate(cpu.SP() >= cpu.StackStart && cpu.SP() < cpu.StackEnd);

		printf("%s >>>>>>\r\n\r\n", name);
	}

#define WRAP(test, v, p) Wrap(#test, test, v, p)

	void WrapWithoutvalidate(FN_TEST fnTest, IVirtualMachine& vm, IProgramMemory& pm)
	{
		pm.Clear();
		IAssembler* assembler = vm.Core().CreateAssembler();
		fnTest(*assembler,vm,pm);
		assembler->Free();
	}

	void TestBranch(IVirtualMachine& vm, IProgramMemory& pm)
	{	
		WRAP(TestBranch1, vm, pm);
		WRAP(TestBranch3, vm, pm);
		WRAP(TestBranch4, vm, pm);
		WRAP(TestBranch5, vm, pm);
		WRAP(TestBranch6, vm, pm);
		WRAP(TestBranch7, vm, pm);
	}

	void Disassemble(IAssembler& a)
	{
		IDisassembler* dis = a.Core().CreateDisassembler();

		size_t programLength;
		const uint8* code = a.Program(OUT programLength);
		size_t i = 0;
		while(i < programLength)
		{
			IDisassembler::Rep rep;
			dis->Disassemble(code + i, OUT rep);

			PrintToStandardOutput(SEXTEXT("%s %s\r\n"), rep.OpcodeText, rep.ArgText);

			validate (rep.ByteCount != 0);
			i += rep.ByteCount;
		}

		dis->Free();
	}

	void TestSymbols(IVirtualMachine& vm, IProgramMemory& pm)
	{
		ISymbols* s = vm.Core().CreateSymbolTable();
		const char* code = "I said\nThat you\nCould come\nand eat\n";
		ISourceFile* testSrc = s->AddSourceImage("test.txt", code, -1);

		FileData fd;
		fd.Source = testSrc;
		fd.Pos.Y = 3;
		fd.Pos.X = 1;
		s->Add(72, IN fd);

		FileData result;
		validate(!s->TryGetSymbol(14, OUT result));
		validate(result.Source == NULL);
		validate(s->TryGetSymbol(72, OUT result));
		validate(result.Source == testSrc);

		size_t offset = result.Source->GetCodeOffset(result.Pos);
		validate(strncmp(code+offset, "ould", 4) == 0);

		s->Clear();
		validate(!s->TryGetSymbol(72, OUT result));

		s->Free();
	}

	void TestDebugger(IVirtualMachine& vm, IProgramMemory& pm)
	{
		printf("<<<<< TestDebugger\r\n");
		IAssembler* a = vm.Core().CreateAssembler();

		int32 setD4Pos = (int32) a->WritePosition();
		a->Append_SetRegisterImmediate(REGISTER_D4, VariantValue(), BITCOUNT_32);
		int32 callPos = (int32) a->WritePosition();
		a->Append_CallByRegister(REGISTER_D4);
		int32 exitPos = (int32) a->WritePosition();
		a->Append_Exit(REGISTER_D5);

		int32 fnPos = (int32) a->WritePosition();

		VariantValue v;
		v.int32Value = 0x76543210;
		a->Append_SetRegisterImmediate(REGISTER_D5, v, BITCOUNT_32);
		a->Append_Return();
		
		VariantValue toFnPos;
		toFnPos.int32Value = fnPos - callPos;
		a->SetWriteModeToOverwrite(setD4Pos);
		a->Append_SetRegisterImmediate(REGISTER_D4, toFnPos, BITCOUNT_32);
		a->SetWriteModeToAppend();

		Disassemble(*a);

		ID_BYTECODE id = pm.AddBytecode();
		pm.UpdateBytecode(id, *a);
		vm.SetProgram(&pm);

		vm.InitPC();
		vm.InitCpu();
		vm.SetBreakpoint(callPos);
		vm.Debug();

		CPU cpu = vm.Cpu();
		validate(cpu.PC() == cpu.ProgramStart + callPos);

		vm.StepOver();
		cpu = vm.Cpu();
		validate(cpu.PC() == cpu.ProgramStart + exitPos);

		vm.StepInto();

		validate(vm.ExitCode() == 0x76543210);

		vm.InitPC();
		vm.InitCpu();
		vm.Debug();
		cpu = vm.Cpu();
		validate(cpu.PC() == cpu.ProgramStart + callPos);

		vm.StepInto(); // Takes us to EnterStackFrame
		vm.StepInto(); // Takes us into call
		vm.StepOut(); // Takes us to exit

		cpu = vm.Cpu();
		validate(cpu.PC() == cpu.ProgramStart + exitPos);

		a->Free();

		printf("TestDebugger >>>>>>\r\n\r\n");
	}

	void TestPerformance(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		VariantValue v;
		v.int32Value = 1000 * 1000;
		a.Append_SetRegisterImmediate(REGISTER_D4, v, BITCOUNT_32);

		int32 subtractPos = (int32) a.WritePosition();

		VariantValue one;
		one.int32Value = 1;
		a.Append_SubtractImmediate(REGISTER_D4, BITCOUNT_32, REGISTER_D4, one);
		a.Append_Test(REGISTER_D4, BITCOUNT_32);

		int32 branchPos = (int32) a.WritePosition();
		a.Append_BranchIf(CONDITION_IF_NOT_EQUAL, subtractPos - branchPos);
		a.Append_Exit(REGISTER_D4);

		int64 startTime = TimerTicks();
		RunProtected(a,vm,pm);
		int64 endTime = TimerTicks();

		int64 dt = endTime - startTime;

		double DT = (double) dt;
		double DTHZ = (double) TimerHz();
		double timespan = DT / DTHZ;

		double ips = 3000000.0 / timespan;

		printf("............................................Mips: %lg subtract-test-branch\r\n", ips / 1000000.0);

		validate(vm.ExitCode() == 0);
	}

	void TestCallback(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		struct ANON {	static void SetD4To40(VariantValue* registers, void* context)
		{
			registers[REGISTER_D4].int32Value = 40;
		}};

		ID_API_CALLBACK ID_SETD4To40 = vm.Core().RegisterCallback(ANON::SetD4To40, NULL, SEXTEXT("SetD4To40"));

		a.Append_Invoke(ID_SETD4To40);
		a.Append_Exit(REGISTER_D4);

		Run(a,vm, pm);

		validate(vm.ExitCode() == 40);
	}

	void TestYieldResume(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		VariantValue v;
		v.int32Value = 765;
		a.Append_SetRegisterImmediate(REGISTER_D4, v, BITCOUNT_32);
		a.Append_Yield();
		v.int32Value = 760;
		a.Append_SetRegisterImmediate(REGISTER_D4, v, BITCOUNT_32);
		a.Append_Exit(REGISTER_D4);

		ID_BYTECODE id = pm.AddBytecode();
		pm.UpdateBytecode(id, a);
		vm.SetProgram(&pm);
		vm.InitCpu();
		validate(vm.Execute(ExecutionFlags(true, true)) == EXECUTERESULT_YIELDED);
		validate(vm.Cpu().D[REGISTER_D4].int32Value == 765);
		validate(vm.ContinueExecution(ExecutionFlags(false,true)) == EXECUTERESULT_TERMINATED);

		validate(vm.Cpu().D[REGISTER_D4].int32Value == 760);
	}

	void TestCallbackBy(IAssembler& a, IVirtualMachine& vm, IProgramMemory& pm)
	{
		struct ANON {	static void SetD5To720(VariantValue* registers, void* context)
		{
			registers[REGISTER_D5].int32Value = 720;
		}};

		ID_API_CALLBACK ID_SETD5To720 = vm.Core().RegisterCallback(ANON::SetD5To720, NULL, SEXTEXT("ID_SETD5To720"));

		VariantValue v;
		v.apiValue = ID_SETD5To720;
		a.Append_SetRegisterImmediate(REGISTER_D6, v, BITCOUNT_ID_API);
		a.Append_InvokeBy(REGISTER_D6);
		a.Append_Exit(REGISTER_D5);

		Run(a,vm, pm);

		validate(vm.ExitCode() == 720);
	}

	void TestStackTrace(IVirtualMachine& vm, IProgramMemory& pm)
	{
		printf("<<<<< Test Stack Trace\r\n");

		IAssembler* a = vm.Core().CreateAssembler();

		size_t callPos = a->WritePosition();
		a->Append_Call(0); // Function call is here
		size_t afterCallPos = a->WritePosition();
		a->Append_Exit(REGISTER_D5);
		size_t funcPos = a->WritePosition();

		VariantValue v;
		v.int32Value = 0x12345621;
		a->Append_SetRegisterImmediate(REGISTER_D5, v, BITCOUNT_32); // Function begins here
		a->Append_Return();

		a->SetWriteModeToOverwrite(callPos);
		a->Append_Call((int)funcPos);
		a->SetWriteModeToAppend();

		Disassemble(*a);

		ID_BYTECODE id = pm.AddBytecode();
		pm.UpdateBytecode(id, *a);
		vm.SetProgram(&pm);
		vm.InitCpu();
		vm.Execute(ExecutionFlags(false, false));

		validate(v.int32Value == vm.ExitCode());

		vm.SetBreakpoint(funcPos);
		vm.InitCpu();
		vm.Debug();

		CPU cpu = vm.Cpu();
		validate(cpu.PC() == cpu.ProgramStart + funcPos);
	
		typedef std::vector<StackTraceItem> TStackItems;

		struct ANON {	static void OnStackTrace(void* context, const StackTraceItem& item)
		{
			TStackItems* items = (TStackItems*) context;
			items->push_back(item);
		}};
		
		TStackItems items;
		vm.GetStackTrace(ANON::OnStackTrace, &items);

		validate(items.size() == 2);
		validate(items[0].Level == 0);
		validate(items[0].FunctionStart ==  cpu.ProgramStart + funcPos);
		validate(items[1].Level == 1);
		validate(items[1].FunctionStart ==  cpu.ProgramStart + afterCallPos);
		validate(items[1].StackFrame == NULL);

		a->Free();

		printf("Test Stack Trace >>>>>>\r\n\r\n");
	}

	void PresentApp(IVirtualMachine& vm)
	{
		IProgramMemory* pm = vm.Core().CreateProgramMemory(32768);

		WRAP(TestTerminate, vm, *pm);
		WRAP(TestNoOp, vm, *pm);
		WRAP(TestSetRegisterImmediate, vm, *pm);
		WRAP(TestTest1, vm, *pm);
		WRAP(TestTest4, vm, *pm);
		WRAP(TestMoveRegister, vm, *pm);
		WRAP(TestPoke, vm, *pm);
		WRAP(TestPushIndirect, vm, *pm);
		WRAP(TestPushRegister, vm, *pm);
		WRAP(TestAddImmediate, vm, *pm);
		WRAP(TestSubtractImmediate, vm, *pm);
		WRAP(TestLogicalAND, vm, *pm);
		WRAP(TestLogicalOR, vm, *pm);
		WRAP(TestLogicalXOR, vm, *pm);
		WRAP(TestLogicalNOT, vm, *pm);
		WRAP(TestShiftLeft, vm, *pm);
		WRAP(TestShiftRight, vm, *pm);
		WRAP(TestPop, vm, *pm);
		WRAP(TestStackAlloc, vm, *pm);
		WRAP(TestIntAdd, vm, *pm);
		WRAP(TestIntSubtract, vm, *pm);
		WRAP(TestIntMultiply, vm, *pm);
		WRAP(TestIntDivide, vm, *pm);
		WRAP(TestIntNegate, vm, *pm);

		WRAP(TestFloatAdd, vm, *pm),
		WRAP(TestFloatSubtract, vm, *pm),
		WRAP(TestFloatMultiply, vm, *pm),
		WRAP(TestFloatDivide, vm, *pm),
		
		WRAP(TestCallAndReturn, vm, *pm);
		WRAP(TestCallByIdIndirectAndReturn, vm, *pm);
		WRAP(TestCallByIdAndReturn, vm, *pm);
		WrapWithoutvalidate(TestMemoryExceptions, vm, *pm);
		WRAP(TestOverwrite, vm, *pm);
		TestBranch(vm, *pm);

		TestSymbols(vm, *pm);
		pm->Clear();
		TestDebugger(vm, *pm);
		pm->Clear();		
		TestStackTrace(vm, *pm);

		WRAP(TestCallback, vm, *pm);
		WRAP(TestCallbackBy, vm, *pm);
		WRAP(TestYieldResume, vm, *pm);

		WRAP(TestStackFrameImmediate, vm, *pm);
		
		WRAP(TestPerformance, vm, *pm);
	}

	void LogError(int error, const char* format, ...)
	{
		va_list args;
		va_start(args,format);
		vprintf(format, args);
		va_end(args);

		char errBuffer[256];
		if (0 == FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, errBuffer, 256, NULL))
		{
			printf("\r\nCode %d(0x%X)", error, error);		
		}
		else
		{
			printf("\r\nCode %d(0x%X): %s", error, error, errBuffer);
		}
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	CoreSpec spec;
	spec.SizeOfStruct = sizeof(CoreSpec);
	spec.Reserved = 0;
	spec.Version = CORE_LIB_VERSION;
	AutoFree<ICore> core = CreateSVMCore(&spec);
	if (core == NULL)
	{
		int exitCode = GetLastError();
		LogError(exitCode, "Error creating SVM core object");
		return exitCode;
	}
	else
	{
		struct CLogger: public ILog
		{
			void Write(csexstr text)
			{
				WriteToStandardOutput(SEXTEXT("%s\n"), text);
			}

			void OnUnhandledException(int errorCode, csexstr exceptionType, csexstr message, void* exceptionInstance) 
			{
				PrintToStandardOutput(SEXTEXT("%s: code %d\nMessage: %s\n"), exceptionType, errorCode, message);
			}

			void OnJITCompileException(Sex::ParseException& ex)
			{
			}
		} logger;

		core->SetLogger(&logger);

		try
		{
			AutoFree<IVirtualMachine> vm(core->CreateVirtualMachine());
			PresentApp(*vm);
		}
		catch (IException& ex)
		{
			PrintToStandardOutput(SEXTEXT("%s: code %d\n"), ex.Message(), ex.ErrorCode());
		}
	}

	return NO_ERROR;
}

