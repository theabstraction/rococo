namespace Rococo { namespace Cute { 
	bool TryParse(const Rococo::fstring& s, ColourTarget& value)
	{
		if (s ==  "ColourTarget_NormalBackground"_fstring)
		{
			value = ColourTarget_NormalBackground;
		}
		else if (s ==  "ColourTarget_HilightBackground"_fstring)
		{
			value = ColourTarget_HilightBackground;
		}
		else
		{
			return false;
		}

		return true;
	}

	bool TryShortParse(const Rococo::fstring& s, ColourTarget& value)
	{
		if (s ==  "NormalBackground"_fstring)
		{
			value = ColourTarget_NormalBackground;
		}
		else if (s ==  "HilightBackground"_fstring)
		{
			value = ColourTarget_HilightBackground;
		}
		else
		{
			return false;
		}

		return true;
	}
	fstring ToShortString(ColourTarget value)
	{
		switch(value)
		{
			case ColourTarget_NormalBackground:
				return "NormalBackground"_fstring;
			case ColourTarget_HilightBackground:
				return "HilightBackground"_fstring;
			default:
				return {"",0};
		}
	}
}}// Rococo.Cute.ColourTarget

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
	void NativeRococoCuteIMenuSetBackgroundColour(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb colour;
		_offset += sizeof(colour);
		ReadInput(colour, _sf, -_offset);

		Rococo::Cute::IMenu* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetBackgroundColour(colour);
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
		ss.AddNativeCall(ns, NativeRococoCuteIMenuSetBackgroundColour, nullptr, ("IMenuSetBackgroundColour (Pointer hObject)(Int32 colour) -> "));
		ss.AddNativeCall(ns, NativeRococoCuteIMenuSubMenu, nullptr, ("IMenuSubMenu (Pointer hObject)(Sys.Type.IString name) -> (Rococo.Cute.IMenu subMenu)"));
	}
}}
// BennyHill generated Sexy native functions for Rococo::Cute::ISplit 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoCuteISplitLo(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Cute::ISplit* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::Cute::IWindowBase* low = _pObject->Lo();
		_offset += sizeof(CReflectedClass*);
		auto& _lowStruct = Rococo::Helpers::GetDefaultProxy(("Rococo.Cute"),("IWindowBase"), ("ProxyIWindowBase"), _nce.ss);
		CReflectedClass* _sxylow = _nce.ss.Represent(_lowStruct, low);
		WriteOutput(&_sxylow->header._vTables[0], _sf, -_offset);
	}
	void NativeRococoCuteISplitHi(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Cute::ISplit* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::Cute::IWindowBase* high = _pObject->Hi();
		_offset += sizeof(CReflectedClass*);
		auto& _highStruct = Rococo::Helpers::GetDefaultProxy(("Rococo.Cute"),("IWindowBase"), ("ProxyIWindowBase"), _nce.ss);
		CReflectedClass* _sxyhigh = _nce.ss.Represent(_highStruct, high);
		WriteOutput(&_sxyhigh->header._vTables[0], _sf, -_offset);
	}
	void NativeRococoCuteISplitHandle(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Cute::ISplit* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		WindowRef hWnd = _pObject->Handle();
		_offset += sizeof(hWnd);
		WriteOutput(hWnd, _sf, -_offset);
	}

}

namespace Rococo { namespace Cute { 
	void AddNativeCalls_RococoCuteISplit(Rococo::Script::IPublicScriptSystem& ss, Rococo::Cute::ISplit* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Cute.Native"));
		ss.AddNativeCall(ns, NativeRococoCuteISplitLo, nullptr, ("ISplitLo (Pointer hObject) -> (Rococo.Cute.IWindowBase low)"));
		ss.AddNativeCall(ns, NativeRococoCuteISplitHi, nullptr, ("ISplitHi (Pointer hObject) -> (Rococo.Cute.IWindowBase high)"));
		ss.AddNativeCall(ns, NativeRococoCuteISplitHandle, nullptr, ("ISplitHandle (Pointer hObject) -> (Pointer hWnd)"));
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
	void NativeRococoCuteIMasterWindowSplitIntoLeftAndRight(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 draggable;
		_offset += sizeof(draggable);
		ReadInput(draggable, _sf, -_offset);

		int32 splitterWidth;
		_offset += sizeof(splitterWidth);
		ReadInput(splitterWidth, _sf, -_offset);

		int32 pixelSplit;
		_offset += sizeof(pixelSplit);
		ReadInput(pixelSplit, _sf, -_offset);

		Rococo::Cute::IMasterWindow* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::Cute::ISplit* split = _pObject->SplitIntoLeftAndRight(pixelSplit, splitterWidth, draggable);
		_offset += sizeof(CReflectedClass*);
		auto& _splitStruct = Rococo::Helpers::GetDefaultProxy(("Rococo.Cute"),("ISplit"), ("ProxyISplit"), _nce.ss);
		CReflectedClass* _sxysplit = _nce.ss.Represent(_splitStruct, split);
		WriteOutput(&_sxysplit->header._vTables[0], _sf, -_offset);
	}
	void NativeRococoCuteIMasterWindowSplitIntoTopAndBottom(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 draggable;
		_offset += sizeof(draggable);
		ReadInput(draggable, _sf, -_offset);

		int32 splitterHeight;
		_offset += sizeof(splitterHeight);
		ReadInput(splitterHeight, _sf, -_offset);

		int32 pixelSplit;
		_offset += sizeof(pixelSplit);
		ReadInput(pixelSplit, _sf, -_offset);

		Rococo::Cute::IMasterWindow* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::Cute::ISplit* split = _pObject->SplitIntoTopAndBottom(pixelSplit, splitterHeight, draggable);
		_offset += sizeof(CReflectedClass*);
		auto& _splitStruct = Rococo::Helpers::GetDefaultProxy(("Rococo.Cute"),("ISplit"), ("ProxyISplit"), _nce.ss);
		CReflectedClass* _sxysplit = _nce.ss.Represent(_splitStruct, split);
		WriteOutput(&_sxysplit->header._vTables[0], _sf, -_offset);
	}
	void NativeRococoCuteIMasterWindowSetMinimumSize(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 dy;
		_offset += sizeof(dy);
		ReadInput(dy, _sf, -_offset);

		int32 dx;
		_offset += sizeof(dx);
		ReadInput(dx, _sf, -_offset);

		Rococo::Cute::IMasterWindow* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetMinimumSize(dx, dy);
	}
	void NativeRococoCuteIMasterWindowSetMaximumSize(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 dy;
		_offset += sizeof(dy);
		ReadInput(dy, _sf, -_offset);

		int32 dx;
		_offset += sizeof(dx);
		ReadInput(dx, _sf, -_offset);

		Rococo::Cute::IMasterWindow* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetMaximumSize(dx, dy);
	}
	void NativeRococoCuteIMasterWindowHandle(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Cute::IMasterWindow* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		WindowRef hWnd = _pObject->Handle();
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
		ss.AddNativeCall(ns, NativeRococoCuteIMasterWindowSplitIntoLeftAndRight, nullptr, ("IMasterWindowSplitIntoLeftAndRight (Pointer hObject)(Int32 pixelSplit)(Int32 splitterWidth)(Bool draggable) -> (Rococo.Cute.ISplit split)"));
		ss.AddNativeCall(ns, NativeRococoCuteIMasterWindowSplitIntoTopAndBottom, nullptr, ("IMasterWindowSplitIntoTopAndBottom (Pointer hObject)(Int32 pixelSplit)(Int32 splitterHeight)(Bool draggable) -> (Rococo.Cute.ISplit split)"));
		ss.AddNativeCall(ns, NativeRococoCuteIMasterWindowSetMinimumSize, nullptr, ("IMasterWindowSetMinimumSize (Pointer hObject)(Int32 dx)(Int32 dy) -> "));
		ss.AddNativeCall(ns, NativeRococoCuteIMasterWindowSetMaximumSize, nullptr, ("IMasterWindowSetMaximumSize (Pointer hObject)(Int32 dx)(Int32 dy) -> "));
		ss.AddNativeCall(ns, NativeRococoCuteIMasterWindowHandle, nullptr, ("IMasterWindowHandle (Pointer hObject) -> (Pointer hWnd)"));
	}
}}
