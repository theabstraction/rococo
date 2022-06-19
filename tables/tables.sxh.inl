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


		auto [output_0, output_1] = TryParseElementName(s);
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


		auto [output_0, output_1] = TryParseElementSymbol(s);
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


		auto [output_0, output_1] = TryParseElementType(s);
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
