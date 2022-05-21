// BennyHill generated Sexy native functions for Rococo::Science::Materials::IPeriodicTable 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoScienceMaterialsIPeriodicTableGetRow(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Science::Materials::ElementsTableRow* row;
		_offset += sizeof(row);
		ReadInput(row, _sf, -_offset);

		int32 index;
		_offset += sizeof(index);
		ReadInput(index, _sf, -_offset);

		Rococo::Science::Materials::IPeriodicTable* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetRow(index, *row);
	}
	void NativeRococoScienceMaterialsIPeriodicTableNumberOfRows(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Science::Materials::IPeriodicTable* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 numberOfRows = _pObject->NumberOfRows();
		_offset += sizeof(numberOfRows);
		WriteOutput(numberOfRows, _sf, -_offset);
	}

}

namespace Rococo { namespace Science { namespace Materials { 
	void AddNativeCalls_RococoScienceMaterialsIPeriodicTable(Rococo::Script::IPublicScriptSystem& ss, Rococo::Science::Materials::IPeriodicTable* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Science.Materials.Native"));
		ss.AddNativeCall(ns, NativeRococoScienceMaterialsIPeriodicTableGetRow, nullptr, ("IPeriodicTableGetRow (Pointer hObject)(Int32 index)(Rococo.Science.Materials.ElementsTableRow row) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoScienceMaterialsIPeriodicTableNumberOfRows, nullptr, ("IPeriodicTableNumberOfRows (Pointer hObject) -> (Int32 numberOfRows)"), __FILE__, __LINE__);
	}
}}}
