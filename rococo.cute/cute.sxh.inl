// BennyHill generated Sexy native functions for Rococo::Cute::IMenu 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoCuteIMenuAddItem(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _key;
		ReadInput(_key, _sf, -_offset);
		fstring key { _key->buffer, _key->length };


		_offset += sizeof(IString*);
		IString* _text;
		ReadInput(_text, _sf, -_offset);
		fstring text { _text->buffer, _text->length };


		Rococo::Cute::IMenu* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddItem(text, key);
	}
	void NativeRococoCuteIMenuSubMenu(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		Rococo::Cute::IMenu* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::Cute::IMenu* subMenu = _pObject->SubMenu(name);
		_offset += sizeof(CReflectedClass*);
		auto& _subMenuStruct = Rococo::Helpers::GetDefaultProxy(("Rococo.Cute"),("IMenu"), ("ProxyIMenu"), _nce.ss);
		CReflectedClass* _sxysubMenu = _nce.ss.Represent(_subMenuStruct, subMenu);
		WriteOutput(&_sxysubMenu->header._vTables[0], _sf, -_offset);
	}

}

namespace Rococo { namespace Cute { 
	void AddNativeCalls_RococoCuteIMenu(Rococo::Script::IPublicScriptSystem& ss, Rococo::Cute::IMenu* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Cute.Native"));
		ss.AddNativeCall(ns, NativeRococoCuteIMenuAddItem, nullptr, ("IMenuAddItem (Pointer hObject)(Sys.Type.IString text)(Sys.Type.IString key) -> "));
		ss.AddNativeCall(ns, NativeRococoCuteIMenuSubMenu, nullptr, ("IMenuSubMenu (Pointer hObject)(Sys.Type.IString name) -> (Rococo.Cute.IMenu subMenu)"));
	}
}}
// BennyHill generated Sexy native functions for Rococo::Cute::IMasterWindow 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoCuteIMasterWindowMenu(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Cute::IMasterWindow* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::Cute::IMenu* menu = _pObject->Menu();
		_offset += sizeof(CReflectedClass*);
		auto& _menuStruct = Rococo::Helpers::GetDefaultProxy(("Rococo.Cute"),("IMenu"), ("ProxyIMenu"), _nce.ss);
		CReflectedClass* _sxymenu = _nce.ss.Represent(_menuStruct, menu);
		WriteOutput(&_sxymenu->header._vTables[0], _sf, -_offset);
	}
	void NativeRococoCuteIMasterWindowGetWindowHandle(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Cute::IMasterWindow* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		size_t hWnd = _pObject->GetWindowHandle();
		_offset += sizeof(hWnd);
		WriteOutput(hWnd, _sf, -_offset);
	}

	void NativeGetHandleForRococoCuteMasterWindow(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 dy;
		_offset += sizeof(dy);
		ReadInput(dy, _sf, -_offset);

		int32 dx;
		_offset += sizeof(dx);
		ReadInput(dx, _sf, -_offset);

		int32 y;
		_offset += sizeof(y);
		ReadInput(y, _sf, -_offset);

		int32 x;
		_offset += sizeof(x);
		ReadInput(x, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _title;
		ReadInput(_title, _sf, -_offset);
		fstring title { _title->buffer, _title->length };


		Rococo::Cute::IMasterWindowFactory* nceContext = reinterpret_cast<Rococo::Cute::IMasterWindowFactory*>(_nce.context);
		// Uses: Rococo::Cute::IMasterWindow* FactoryConstructRococoCuteMasterWindow(Rococo::Cute::IMasterWindowFactory* _context, const fstring& _title, int32 _x, int32 _y, int32 _dx, int32 _dy);
		Rococo::Cute::IMasterWindow* pObject = FactoryConstructRococoCuteMasterWindow(nceContext, title, x, y, dx, dy);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo { namespace Cute { 
	void AddNativeCalls_RococoCuteIMasterWindow(Rococo::Script::IPublicScriptSystem& ss, Rococo::Cute::IMasterWindowFactory* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Cute.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForRococoCuteMasterWindow, _nceContext, ("GetHandleForIMasterWindow0 (Sys.Type.IString title)(Int32 x)(Int32 y)(Int32 dx)(Int32 dy) -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeRococoCuteIMasterWindowMenu, nullptr, ("IMasterWindowMenu (Pointer hObject) -> (Rococo.Cute.IMenu menu)"));
		ss.AddNativeCall(ns, NativeRococoCuteIMasterWindowGetWindowHandle, nullptr, ("IMasterWindowGetWindowHandle (Pointer hObject) -> (Pointer hWnd)"));
	}
}}
