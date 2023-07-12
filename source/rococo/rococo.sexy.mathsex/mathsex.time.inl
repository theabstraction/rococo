namespace
{
	void NativeSysTimeCpuHz(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int64 hz = Rococo::Time::TickHz();
		_offset += sizeof(hz);
		WriteOutput(hz, _sf, -_offset);
	}

	void NativeSysTimeCpuTicks(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int64 ticks = Rococo::Time::TickCount();
		_offset += sizeof(ticks);
		WriteOutput(ticks, _sf, -_offset);
	}

}

namespace Sys { namespace Time { 
	void AddNativeCalls_SysTime(Rococo::Script::IPublicScriptSystem& ss, void* nullContext = nullptr)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Sys.Time"));
		ss.AddNativeCall(ns, NativeSysTimeCpuHz, nullptr, ("CpuHz -> (Int64 hz)"));
		ss.AddNativeCall(ns, NativeSysTimeCpuTicks, nullptr, ("CpuTicks -> (Int64 ticks)"));
	}
}}