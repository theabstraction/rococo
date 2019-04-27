namespace
{
	void NativeMHostOSIsKeyPressed(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 vKeyCode;
		_offset += sizeof(vKeyCode);
		ReadInput(vKeyCode, _sf, -_offset);

		MHost::OS::KeyState* keys;
		_offset += sizeof(keys);
		ReadInput(keys, _sf, -_offset);

		boolean32 isPressed = MHost::OS::IsKeyPressed(*keys, vKeyCode);
		_offset += sizeof(isPressed);
		WriteOutput(isPressed, _sf, -_offset);
	}

}

namespace MHost { namespace OS { 
	void AddNativeCalls_MHostOS(Rococo::Script::IPublicScriptSystem& ss, void* nullContext = nullptr)
	{
		const INamespace& ns = ss.AddNativeNamespace(("MHost.OS"));
		ss.AddNativeCall(ns, NativeMHostOSIsKeyPressed, nullptr, ("IsKeyPressed(MHost.OS.KeyState keys)(Int32 vKeyCode) -> (Bool isPressed)"));
	}
}}