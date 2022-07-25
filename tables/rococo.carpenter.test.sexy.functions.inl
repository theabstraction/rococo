namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoScienceMaterialsElementNameAppendString(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(VirtualTable**);
		VirtualTable** sb;
		ReadInput(sb, _sf, -_offset);
		Rococo::Helpers::StringPopulator _sbPopulator(_nce, sb);
		Rococo::Science::Materials::ElementName value;
		_offset += sizeof(value);
		ReadInput(value, _sf, -_offset);

		int32 stringLength = Rococo::Science::Materials::AppendString(value, _sbPopulator);
		_offset += sizeof(stringLength);
		WriteOutput(stringLength, _sf, -_offset);
	}

	void NativeRococoScienceMaterialsTryParseElementName(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _s;
		ReadInput(_s, _sf, -_offset);
		fstring s { _s->buffer, _s->length };


		auto [output_0, output_1] = Rococo::Science::Materials::TryParseElementName(s);
		_offset += sizeof(boolean32);
		WriteOutput(output_0, _sf, -_offset);
		_offset += sizeof(int32);
		WriteOutput(output_1, _sf, -_offset);
	}

	void NativeRococoScienceMaterialsElementSymbolAppendString(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(VirtualTable**);
		VirtualTable** sb;
		ReadInput(sb, _sf, -_offset);
		Rococo::Helpers::StringPopulator _sbPopulator(_nce, sb);
		Rococo::Science::Materials::ElementSymbol value;
		_offset += sizeof(value);
		ReadInput(value, _sf, -_offset);

		int32 stringLength = Rococo::Science::Materials::AppendString(value, _sbPopulator);
		_offset += sizeof(stringLength);
		WriteOutput(stringLength, _sf, -_offset);
	}

	void NativeRococoScienceMaterialsTryParseElementSymbol(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _s;
		ReadInput(_s, _sf, -_offset);
		fstring s { _s->buffer, _s->length };


		auto [output_0, output_1] = Rococo::Science::Materials::TryParseElementSymbol(s);
		_offset += sizeof(boolean32);
		WriteOutput(output_0, _sf, -_offset);
		_offset += sizeof(int32);
		WriteOutput(output_1, _sf, -_offset);
	}

	void NativeRococoScienceMaterialsElementTypeAppendString(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(VirtualTable**);
		VirtualTable** sb;
		ReadInput(sb, _sf, -_offset);
		Rococo::Helpers::StringPopulator _sbPopulator(_nce, sb);
		Rococo::Science::Materials::ElementType value;
		_offset += sizeof(value);
		ReadInput(value, _sf, -_offset);

		int32 stringLength = Rococo::Science::Materials::AppendString(value, _sbPopulator);
		_offset += sizeof(stringLength);
		WriteOutput(stringLength, _sf, -_offset);
	}

	void NativeRococoScienceMaterialsTryParseElementType(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _s;
		ReadInput(_s, _sf, -_offset);
		fstring s { _s->buffer, _s->length };


		auto [output_0, output_1] = Rococo::Science::Materials::TryParseElementType(s);
		_offset += sizeof(boolean32);
		WriteOutput(output_0, _sf, -_offset);
		_offset += sizeof(int32);
		WriteOutput(output_1, _sf, -_offset);
	}

}

namespace Rococo::Science::Materials
{

	void AddNativeCalls_RococoScienceMaterials(Rococo::Script::IPublicScriptSystem& ss)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Science.Materials");
		ss.AddNativeCall(ns, NativeRococoScienceMaterialsElementNameAppendString, nullptr, ("ElementNameAppendString(Rococo.Science.Materials.ElementName value)(Sys.Type.IStringBuilder sb) -> (Int32 stringLength)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoScienceMaterialsTryParseElementName, nullptr, ("TryParseElementName(Sys.Type.IString s) -> (Bool wasFound)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoScienceMaterialsElementSymbolAppendString, nullptr, ("ElementSymbolAppendString(Rococo.Science.Materials.ElementSymbol value)(Sys.Type.IStringBuilder sb) -> (Int32 stringLength)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoScienceMaterialsTryParseElementSymbol, nullptr, ("TryParseElementSymbol(Sys.Type.IString s) -> (Bool wasFound)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoScienceMaterialsElementTypeAppendString, nullptr, ("ElementTypeAppendString(Rococo.Science.Materials.ElementType value)(Sys.Type.IStringBuilder sb) -> (Int32 stringLength)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoScienceMaterialsTryParseElementType, nullptr, ("TryParseElementType(Sys.Type.IString s) -> (Bool wasFound)"), __FILE__, __LINE__);
	}
}

namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoStringsTextIdAppendString(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(VirtualTable**);
		VirtualTable** sb;
		ReadInput(sb, _sf, -_offset);
		Rococo::Helpers::StringPopulator _sbPopulator(_nce, sb);
		Rococo::Strings::TextId value;
		_offset += sizeof(value);
		ReadInput(value, _sf, -_offset);

		int32 stringLength = Rococo::Strings::AppendString(value, _sbPopulator);
		_offset += sizeof(stringLength);
		WriteOutput(stringLength, _sf, -_offset);
	}

	void NativeRococoStringsTryParseTextId(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _s;
		ReadInput(_s, _sf, -_offset);
		fstring s { _s->buffer, _s->length };


		auto [output_0, output_1] = Rococo::Strings::TryParseTextId(s);
		_offset += sizeof(boolean32);
		WriteOutput(output_0, _sf, -_offset);
		_offset += sizeof(int32);
		WriteOutput(output_1, _sf, -_offset);
	}

}

namespace Rococo::Strings
{

	void AddNativeCalls_RococoStrings(Rococo::Script::IPublicScriptSystem& ss)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Strings");
		ss.AddNativeCall(ns, NativeRococoStringsTextIdAppendString, nullptr, ("TextIdAppendString(Rococo.Strings.TextId value)(Sys.Type.IStringBuilder sb) -> (Int32 stringLength)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoStringsTryParseTextId, nullptr, ("TryParseTextId(Sys.Type.IString s) -> (Bool wasFound)"), __FILE__, __LINE__);
	}
}

namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoQuotesQuoteIdAppendString(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(VirtualTable**);
		VirtualTable** sb;
		ReadInput(sb, _sf, -_offset);
		Rococo::Helpers::StringPopulator _sbPopulator(_nce, sb);
		Rococo::Quotes::QuoteId value;
		_offset += sizeof(value);
		ReadInput(value, _sf, -_offset);

		int32 stringLength = Rococo::Quotes::AppendString(value, _sbPopulator);
		_offset += sizeof(stringLength);
		WriteOutput(stringLength, _sf, -_offset);
	}

	void NativeRococoQuotesTryParseQuoteId(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _s;
		ReadInput(_s, _sf, -_offset);
		fstring s { _s->buffer, _s->length };


		auto [output_0, output_1] = Rococo::Quotes::TryParseQuoteId(s);
		_offset += sizeof(boolean32);
		WriteOutput(output_0, _sf, -_offset);
		_offset += sizeof(int32);
		WriteOutput(output_1, _sf, -_offset);
	}

}

namespace Rococo::Quotes
{

	void AddNativeCalls_RococoQuotes(Rococo::Script::IPublicScriptSystem& ss)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Quotes");
		ss.AddNativeCall(ns, NativeRococoQuotesQuoteIdAppendString, nullptr, ("QuoteIdAppendString(Rococo.Quotes.QuoteId value)(Sys.Type.IStringBuilder sb) -> (Int32 stringLength)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoQuotesTryParseQuoteId, nullptr, ("TryParseQuoteId(Sys.Type.IString s) -> (Bool wasFound)"), __FILE__, __LINE__);
	}
}

