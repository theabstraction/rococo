namespace Rococo
{
	namespace Script
	{
		template<typename DISPATCH_ARGS> void DispatchToSexyClosure(NativeCallEnvironment& nce, DISPATCH_ARGS& args, const ArchetypeCallback& target)
		{
		   auto& ss = nce.ss;
		   VM::IVirtualMachine& vm(ss.PublicProgramObject().VirtualMachine());
		   VM::CPU& cpu(vm.Cpu());
		   VM::IProgramMemory& program(ss.PublicProgramObject().ProgramMemory());

		   const uint8* context = cpu.SF();
		   const uint8 *returnAddress = cpu.PC();
		   const uint8* sp = cpu.SP();

		   cpu.Push(target);
		   cpu.Push(&args);

		   cpu.SetPC(cpu.ProgramStart);
		   cpu.D[5].byteCodeIdValue = target.byteCodeId;

		   VM::ExecutionFlags currentFlags;
		   vm.GetLastFlags(currentFlags);

		   VM::ExecutionFlags flags{ currentFlags.ThrowToQuit, true, true };
		   EXECUTERESULT er = vm.Execute(flags, nullptr);

		   if (er == EXECUTERESULT_TERMINATED)
		   {
			   vm.SetStatus(EXECUTERESULT_RUNNING);

			   cpu.SetPC(returnAddress);
			   cpu.SetSF(context);
			   cpu.D[VM::REGISTER_SP].uint8PtrValue = (uint8*)sp;
		   }
		}
	} // Script
} // Rococo