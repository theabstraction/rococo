namespace
{
	void NativeSysStringAreStringsEqual(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		fstring t;
		_offset += sizeof(t);

		ReadInput(t, _sf, -_offset);
		fstring s;
		_offset += sizeof(s);

		ReadInput(s, _sf, -_offset);
		boolean32 areEqual = AreStringsEqual(String, String);
		_offset += sizeof(areEqual);
		WriteOutput(areEqual, _sf, -_offset);
	}

	void NativeSysStringAreStringsEqualI(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		fstring t;
		_offset += sizeof(t);

		ReadInput(t, _sf, -_offset);
		fstring s;
		_offset += sizeof(s);

		ReadInput(s, _sf, -_offset);
		boolean32 areEqual = AreStringsEqualI(String, String);
		_offset += sizeof(areEqual);
		WriteOutput(areEqual, _sf, -_offset);
	}

	void NativeSysStringCompareStrings(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		fstring t;
		_offset += sizeof(t);

		ReadInput(t, _sf, -_offset);
		fstring s;
		_offset += sizeof(s);

		ReadInput(s, _sf, -_offset);
		int32 order = CompareStrings(String, String);
		_offset += sizeof(order);
		WriteOutput(order, _sf, -_offset);
	}

	void NativeSysStringCompareStringsI(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		fstring t;
		_offset += sizeof(t);

		ReadInput(t, _sf, -_offset);
		fstring s;
		_offset += sizeof(s);

		ReadInput(s, _sf, -_offset);
		int32 order = CompareStringsI(String, String);
		_offset += sizeof(order);
		WriteOutput(order, _sf, -_offset);
	}

	void NativeSysStringFindFirstSubstringIndex(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		fstring subset;
		_offset += sizeof(subset);

		ReadInput(subset, _sf, -_offset);
		fstring s;
		_offset += sizeof(s);

		ReadInput(s, _sf, -_offset);
		int32 charIndex = FindFirstSubstringIndex(String, String);
		_offset += sizeof(charIndex);
		WriteOutput(charIndex, _sf, -_offset);
	}

	void NativeSysStringFindLastSubstringIndex(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		fstring subset;
		_offset += sizeof(subset);

		ReadInput(subset, _sf, -_offset);
		fstring s;
		_offset += sizeof(s);

		ReadInput(s, _sf, -_offset);
		int32 charIndex = FindLastSubstringIndex(String, String);
		_offset += sizeof(charIndex);
		WriteOutput(charIndex, _sf, -_offset);
	}

	void NativeSysStringFindNextSubstringIndex(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		fstring subset;
		_offset += sizeof(subset);

		ReadInput(subset, _sf, -_offset);
		int32 startIndex;
		_offset += sizeof(startIndex);

		ReadInput(startIndex, _sf, -_offset);
		fstring s;
		_offset += sizeof(s);

		ReadInput(s, _sf, -_offset);
		int32 index = FindNextSubstringIndex(String, startIndex, String);
		_offset += sizeof(index);
		WriteOutput(index, _sf, -_offset);
	}

	void NativeSysStringFindFirstCharIndex(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 char;
		_offset += sizeof(char);

		ReadInput(char, _sf, -_offset);
		fstring s;
		_offset += sizeof(s);

		ReadInput(s, _sf, -_offset);
		int32 charIndex = FindFirstSubstringIndex(String, char);
		_offset += sizeof(charIndex);
		WriteOutput(charIndex, _sf, -_offset);
	}

	void NativeSysStringFindLastCharIndex(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 char;
		_offset += sizeof(char);

		ReadInput(char, _sf, -_offset);
		fstring s;
		_offset += sizeof(s);

		ReadInput(s, _sf, -_offset);
		int32 charIndex = FindLastSubstringIndex(String, char);
		_offset += sizeof(charIndex);
		WriteOutput(charIndex, _sf, -_offset);
	}

	void NativeSysStringFindNextCharIndex(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 char;
		_offset += sizeof(char);

		ReadInput(char, _sf, -_offset);
		int32 startIndex;
		_offset += sizeof(startIndex);

		ReadInput(startIndex, _sf, -_offset);
		fstring s;
		_offset += sizeof(s);

		ReadInput(s, _sf, -_offset);
		int32 index = FindNextSubstringIndex(String, startIndex, char);
		_offset += sizeof(index);
		WriteOutput(index, _sf, -_offset);
	}

}

namespace Sys { namespace String { 
	void AddNativeCalls_SysString(Sexy::Script::IPublicScriptSystem& ss, void* nullContext = nullptr)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Sys.String"));
		ss.AddNativeCall(ns, NativeSysStringAreStringsEqual, nullptr, SEXTEXT("AreStringsEqual(Sys.Type.IString s)(Sys.Type.IString t) -> (Bool areEqual)"));
		ss.AddNativeCall(ns, NativeSysStringAreStringsEqualI, nullptr, SEXTEXT("AreStringsEqualI(Sys.Type.IString s)(Sys.Type.IString t) -> (Bool areEqual)"));
		ss.AddNativeCall(ns, NativeSysStringCompareStrings, nullptr, SEXTEXT("CompareStrings(Sys.Type.IString s)(Sys.Type.IString t) -> (Int32 order)"));
		ss.AddNativeCall(ns, NativeSysStringCompareStringsI, nullptr, SEXTEXT("CompareStringsI(Sys.Type.IString s)(Sys.Type.IString t) -> (Int32 order)"));
		ss.AddNativeCall(ns, NativeSysStringFindFirstSubstringIndex, nullptr, SEXTEXT("FindFirstSubstringIndex(Sys.Type.IString s)(Sys.Type.IString subset) -> (Int32 charIndex)"));
		ss.AddNativeCall(ns, NativeSysStringFindLastSubstringIndex, nullptr, SEXTEXT("FindLastSubstringIndex(Sys.Type.IString s)(Sys.Type.IString subset) -> (Int32 charIndex)"));
		ss.AddNativeCall(ns, NativeSysStringFindNextSubstringIndex, nullptr, SEXTEXT("FindNextSubstringIndex(Sys.Type.IString s)(Int32 startIndex)(Sys.Type.IString subset) -> (Int32 index)"));
		ss.AddNativeCall(ns, NativeSysStringFindFirstCharIndex, nullptr, SEXTEXT("FindFirstCharIndex(Sys.Type.IString s)(Int32 char) -> (Int32 charIndex)"));
		ss.AddNativeCall(ns, NativeSysStringFindLastCharIndex, nullptr, SEXTEXT("FindLastCharIndex(Sys.Type.IString s)(Int32 char) -> (Int32 charIndex)"));
		ss.AddNativeCall(ns, NativeSysStringFindNextCharIndex, nullptr, SEXTEXT("FindNextCharIndex(Sys.Type.IString s)(Int32 startIndex)(Int32 char) -> (Int32 index)"));
	}
}}