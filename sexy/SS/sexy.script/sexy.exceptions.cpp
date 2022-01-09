#include "sexy.script.stdafx.h"
#include <Sexy.Compiler.h>
#include <sexy.vm.h>
#include <sexy.vm.cpu.h>

#include <vector>

using namespace Rococo::VM;

namespace Rococo::Script
{
	struct ExceptionHandler
	{
		ID_BYTECODE FunctionId;
		size_t Start;
		size_t End;
		size_t HandlerOffset;
	};

	typedef std::vector<ExceptionHandler> TExceptionHandlers;

	IFunctionBuilder* GetFunctionForAddress(const uint8* pc, IProgramObject& programObject);

	void DeconstructLocalObjects(IScriptSystem& ss, size_t functionOffset, const uint8* sf, IFunctionBuilder& f, REF int& totalStackCorrection);
	void DeconstructLocalObjects(IFunctionBuilder& f, const uint8* pc, const uint8* sf, IScriptSystem& ss, REF int& totalStackCorrection);

	const uint8* GetCallerSF(OUT const uint8*& returnAddress, const uint8* SF);

	const uint8* GetHandlerFor(const TExceptionHandlers& handlers, Compiler::IProgramObject& po, const uint8* pc);

	bool CatchException(const TExceptionHandlers& handlers, IScriptSystem& ss)
	{
		VM::CPU& cpu = ss.ProgramObject().VirtualMachine().Cpu();

		// LogCpu(ss.ProgramObject().Log(), cpu);

		const uint8* sf = cpu.SF();
		const uint8* returnAddress = cpu.PC();

		int totalStackCorrection = 0;

		while (true)
		{
			const uint8* pc = returnAddress;

			IFunctionBuilder* f = GetFunctionForAddress(pc, ss.ProgramObject());
			if (f == NULL)
			{
				return false;
			}

			cstr fname = f->Name();

			DeconstructLocalObjects(*f, pc, sf, ss, REF totalStackCorrection);

			const uint8* catchAddress = GetHandlerFor(handlers, ss.ProgramObject(), pc);
			if (catchAddress != NULL)
			{
				cpu.SetSF(sf);
				cpu.SetPC(catchAddress);
				cpu.D[VM::REGISTER_SP].uint8PtrValue -= totalStackCorrection;
				return true;
			}

			totalStackCorrection += 2 * sizeof(size_t); // the return address and the old stack frame are popped off the stack

			const uint8* oldSf = GetCallerSF(OUT returnAddress, sf);

			if (oldSf >= sf) return false; // If sf is NULL, the execution stub, then oldSf will also be null
			sf = oldSf;
		}
	}

}