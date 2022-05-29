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
		void* nceContext = reinterpret_cast<void*>(_nce.context);
		// Uses: Rococo::Science::Materials::IPeriodicTable_Sexy* FactoryConstructRococoScienceMaterialsGetPeriodicTable(void* _context);
		Rococo::Science::Materials::IPeriodicTable_Sexy* pObject = FactoryConstructRococoScienceMaterialsGetPeriodicTable(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo::Science::Materials
{
	void AddNativeCalls_RococoScienceMaterialsIPeriodicTable_Sexy(Rococo::Script::IPublicScriptSystem& ss, void* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Science.Materials.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForRococoScienceMaterialsGetPeriodicTable, _nceContext, ("GetHandleForIPeriodicTable0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoScienceMaterialsIPeriodicTable_SexyGetRow, nullptr, ("IPeriodicTableGetRow (Pointer hObject)(Int32 index)(Rococo.Science.Materials.PeriodicTableRow row) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoScienceMaterialsIPeriodicTable_SexyNumberOfRows, nullptr, ("IPeriodicTableNumberOfRows (Pointer hObject) -> (Int32 numberOfRows)"), __FILE__, __LINE__);
	}
}
