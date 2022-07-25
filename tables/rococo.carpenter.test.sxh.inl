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
// BennyHill generated Sexy native functions for Rococo::Test::UserDemo::IUsers_Sexy 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoTestUserDemoIUsers_SexyGetRow(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Test::UserDemo::UsersRowSexy* row;
		_offset += sizeof(row);
		ReadInput(row, _sf, -_offset);

		int32 index;
		_offset += sizeof(index);
		ReadInput(index, _sf, -_offset);

		Rococo::Test::UserDemo::IUsers_Sexy* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetRow(index, *row);
	}
	void NativeRococoTestUserDemoIUsers_SexyNumberOfRows(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Test::UserDemo::IUsers_Sexy* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 numberOfRows = _pObject->NumberOfRows();
		_offset += sizeof(numberOfRows);
		WriteOutput(numberOfRows, _sf, -_offset);
	}

	void NativeGetHandleForRococoTestUserDemoGetUserTable(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		IInstallation* nceContext = reinterpret_cast<IInstallation*>(_nce.context);
		// Uses: Rococo::Test::UserDemo::IUsers_Sexy* FactoryConstructRococoTestUserDemoGetUserTable(IInstallation* _context);
		Rococo::Test::UserDemo::IUsers_Sexy* pObject = FactoryConstructRococoTestUserDemoGetUserTable(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo::Test::UserDemo
{
	void AddNativeCalls_RococoTestUserDemoIUsers_Sexy(Rococo::Script::IPublicScriptSystem& ss, IInstallation* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Test.UserDemo.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoTestUserDemoGetUserTable, _nceContext, ("GetHandleForIUsers0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoTestUserDemoIUsers_SexyGetRow, nullptr, ("IUsersGetRow (Pointer hObject)(Int32 index)(Rococo.Test.UserDemo.UsersRow row) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoTestUserDemoIUsers_SexyNumberOfRows, nullptr, ("IUsersNumberOfRows (Pointer hObject) -> (Int32 numberOfRows)"), __FILE__, __LINE__);
	}
}
