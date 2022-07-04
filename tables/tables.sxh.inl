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

// BennyHill generated Sexy native functions for Rococo::Science::Materials::IPeriodicTable_Sexy 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoScienceMaterialsIPeriodicTable_SexyGetRow(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Science::Materials::PeriodicTableRow* row;
		_offset += sizeof(row);
		ReadInput(row, _sf, -_offset);

		int32 index;
		_offset += sizeof(index);
		ReadInput(index, _sf, -_offset);

		Rococo::Science::Materials::IPeriodicTable_Sexy* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetRow(index, *row);
	}
	void NativeRococoScienceMaterialsIPeriodicTable_SexyNumberOfRows(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Science::Materials::IPeriodicTable_Sexy* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 numberOfRows = _pObject->NumberOfRows();
		_offset += sizeof(numberOfRows);
		WriteOutput(numberOfRows, _sf, -_offset);
	}

	void NativeGetHandleForRococoScienceMaterialsGetPeriodicTable(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		IInstallation* nceContext = reinterpret_cast<IInstallation*>(_nce.context);
		// Uses: Rococo::Science::Materials::IPeriodicTable_Sexy* FactoryConstructRococoScienceMaterialsGetPeriodicTable(IInstallation* _context);
		Rococo::Science::Materials::IPeriodicTable_Sexy* pObject = FactoryConstructRococoScienceMaterialsGetPeriodicTable(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo::Science::Materials
{
	void AddNativeCalls_RococoScienceMaterialsIPeriodicTable_Sexy(Rococo::Script::IPublicScriptSystem& ss, IInstallation* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Science.Materials.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoScienceMaterialsGetPeriodicTable, _nceContext, ("GetHandleForIPeriodicTable0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoScienceMaterialsIPeriodicTable_SexyGetRow, nullptr, ("IPeriodicTableGetRow (Pointer hObject)(Int32 index)(Rococo.Science.Materials.PeriodicTableRow row) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoScienceMaterialsIPeriodicTable_SexyNumberOfRows, nullptr, ("IPeriodicTableNumberOfRows (Pointer hObject) -> (Int32 numberOfRows)"), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::Strings::ILocalizedText_Sexy 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoStringsILocalizedText_SexyGetRow(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Strings::LocalizedTextRowSexy* row;
		_offset += sizeof(row);
		ReadInput(row, _sf, -_offset);

		int32 index;
		_offset += sizeof(index);
		ReadInput(index, _sf, -_offset);

		Rococo::Strings::ILocalizedText_Sexy* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetRow(index, *row);
	}
	void NativeRococoStringsILocalizedText_SexyNumberOfRows(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Strings::ILocalizedText_Sexy* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 numberOfRows = _pObject->NumberOfRows();
		_offset += sizeof(numberOfRows);
		WriteOutput(numberOfRows, _sf, -_offset);
	}

	void NativeGetHandleForRococoStringsLocalizedText(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		IInstallation* nceContext = reinterpret_cast<IInstallation*>(_nce.context);
		// Uses: Rococo::Strings::ILocalizedText_Sexy* FactoryConstructRococoStringsLocalizedText(IInstallation* _context);
		Rococo::Strings::ILocalizedText_Sexy* pObject = FactoryConstructRococoStringsLocalizedText(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo::Strings
{
	void AddNativeCalls_RococoStringsILocalizedText_Sexy(Rococo::Script::IPublicScriptSystem& ss, IInstallation* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Strings.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoStringsLocalizedText, _nceContext, ("GetHandleForILocalizedText0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoStringsILocalizedText_SexyGetRow, nullptr, ("ILocalizedTextGetRow (Pointer hObject)(Int32 index)(Rococo.Strings.LocalizedTextRow row) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoStringsILocalizedText_SexyNumberOfRows, nullptr, ("ILocalizedTextNumberOfRows (Pointer hObject) -> (Int32 numberOfRows)"), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::Quotes::IQuotes_Sexy 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoQuotesIQuotes_SexyGetRow(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Quotes::QuotesRowSexy* row;
		_offset += sizeof(row);
		ReadInput(row, _sf, -_offset);

		int32 index;
		_offset += sizeof(index);
		ReadInput(index, _sf, -_offset);

		Rococo::Quotes::IQuotes_Sexy* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetRow(index, *row);
	}
	void NativeRococoQuotesIQuotes_SexyNumberOfRows(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Quotes::IQuotes_Sexy* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 numberOfRows = _pObject->NumberOfRows();
		_offset += sizeof(numberOfRows);
		WriteOutput(numberOfRows, _sf, -_offset);
	}

	void NativeGetHandleForRococoQuotesGetQuoteTable(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		IInstallation* nceContext = reinterpret_cast<IInstallation*>(_nce.context);
		// Uses: Rococo::Quotes::IQuotes_Sexy* FactoryConstructRococoQuotesGetQuoteTable(IInstallation* _context);
		Rococo::Quotes::IQuotes_Sexy* pObject = FactoryConstructRococoQuotesGetQuoteTable(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo::Quotes
{
	void AddNativeCalls_RococoQuotesIQuotes_Sexy(Rococo::Script::IPublicScriptSystem& ss, IInstallation* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Quotes.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoQuotesGetQuoteTable, _nceContext, ("GetHandleForIQuotes0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoQuotesIQuotes_SexyGetRow, nullptr, ("IQuotesGetRow (Pointer hObject)(Int32 index)(Rococo.Quotes.QuotesRow row) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoQuotesIQuotes_SexyNumberOfRows, nullptr, ("IQuotesNumberOfRows (Pointer hObject) -> (Int32 numberOfRows)"), __FILE__, __LINE__);
	}
}
