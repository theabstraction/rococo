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

namespace Rococo { namespace Graphics { 
	bool TryParse(const Rococo::fstring& s, OrientationFlags& value)
	{
		if (s ==  "OrientationFlags_None"_fstring)
		{
			value = OrientationFlags_None;
		}
		else if (s ==  "OrientationFlags_HasButtons"_fstring)
		{
			value = OrientationFlags_HasButtons;
		}
		else if (s ==  "OrientationFlags_HasLines"_fstring)
		{
			value = OrientationFlags_HasLines;
		}
		else if (s ==  "OrientationFlags_LinesAtRoot"_fstring)
		{
			value = OrientationFlags_LinesAtRoot;
		}
		else if (s ==  "OrientationFlags_EditLabels"_fstring)
		{
			value = OrientationFlags_EditLabels;
		}
		else
		{
			return false;
		}

		return true;
	}

	bool TryShortParse(const Rococo::fstring& s, OrientationFlags& value)
	{
		if (s ==  "None"_fstring)
		{
			value = OrientationFlags_None;
		}
		else if (s ==  "HasButtons"_fstring)
		{
			value = OrientationFlags_HasButtons;
		}
		else if (s ==  "HasLines"_fstring)
		{
			value = OrientationFlags_HasLines;
		}
		else if (s ==  "LinesAtRoot"_fstring)
		{
			value = OrientationFlags_LinesAtRoot;
		}
		else if (s ==  "EditLabels"_fstring)
		{
			value = OrientationFlags_EditLabels;
		}
		else
		{
			return false;
		}

		return true;
	}
	fstring ToShortString(OrientationFlags value)
	{
		switch(value)
		{
			case OrientationFlags_None:
				return "None"_fstring;
			case OrientationFlags_HasButtons:
				return "HasButtons"_fstring;
			case OrientationFlags_HasLines:
				return "HasLines"_fstring;
			case OrientationFlags_LinesAtRoot:
				return "LinesAtRoot"_fstring;
			case OrientationFlags_EditLabels:
				return "EditLabels"_fstring;
			default:
				return {"",0};
		}
	}
}}// Rococo.Graphics.OrientationFlags

// BennyHill generated Sexy native functions for Rococo::Cute::IWindowBase 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoCuteIWindowBaseHandle(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Cute::IWindowBase* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		WindowRef hWnd = _pObject->Handle();
		_offset += sizeof(hWnd);
		WriteOutput(hWnd, _sf, -_offset);
	}

}

namespace Rococo { namespace Cute { 
	void AddNativeCalls_RococoCuteIWindowBase(Rococo::Script::IPublicScriptSystem& ss, Rococo::Cute::IWindowBase* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Cute.Native"));
		ss.AddNativeCall(ns, NativeRococoCuteIWindowBaseHandle, nullptr, ("IWindowBaseHandle (Pointer hObject) -> (Pointer hWnd)"));
	}
}}
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
		WriteOutput(&_sxysubMenu->header.pVTables[0], _sf, -_offset);
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
// BennyHill generated Sexy native functions for Rococo::Cute::ITree 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoCuteITreeHandle(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Cute::ITree* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		WindowRef hWnd = _pObject->Handle();
		_offset += sizeof(hWnd);
		WriteOutput(hWnd, _sf, -_offset);
	}
	void NativeRococoCuteITreeSetPopulator(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _populatorId;
		ReadInput(_populatorId, _sf, -_offset);
		fstring populatorId { _populatorId->buffer, _populatorId->length };


		Rococo::Cute::ITree* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetPopulator(populatorId);
	}

}

namespace Rococo { namespace Cute { 
	void AddNativeCalls_RococoCuteITree(Rococo::Script::IPublicScriptSystem& ss, Rococo::Cute::ITree* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Cute.Native"));
		ss.AddNativeCall(ns, NativeRococoCuteITreeHandle, nullptr, ("ITreeHandle (Pointer hObject) -> (Pointer hWnd)"));
		ss.AddNativeCall(ns, NativeRococoCuteITreeSetPopulator, nullptr, ("ITreeSetPopulator (Pointer hObject)(Sys.Type.IString populatorId) -> "));
	}
}}
// BennyHill generated Sexy native functions for Rococo::Cute::IParentWindow 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoCuteIParentWindowHandle(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Cute::IParentWindow* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		WindowRef ref = _pObject->Handle();
		_offset += sizeof(ref);
		WriteOutput(ref, _sf, -_offset);
	}
	void NativeRococoCuteIParentWindowMenu(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Cute::IParentWindow* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::Cute::IMenu* menu = _pObject->Menu();
		_offset += sizeof(CReflectedClass*);
		auto& _menuStruct = Rococo::Helpers::GetDefaultProxy(("Rococo.Cute"),("IMenu"), ("ProxyIMenu"), _nce.ss);
		CReflectedClass* _sxymenu = _nce.ss.Represent(_menuStruct, menu);
		WriteOutput(&_sxymenu->header.pVTables[0], _sf, -_offset);
	}
	void NativeRococoCuteIParentWindowSplitIntoLeftAndRight(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 draggable;
		_offset += sizeof(draggable);
		ReadInput(draggable, _sf, -_offset);

		int32 splitterWidth;
		_offset += sizeof(splitterWidth);
		ReadInput(splitterWidth, _sf, -_offset);

		int32 maxRight;
		_offset += sizeof(maxRight);
		ReadInput(maxRight, _sf, -_offset);

		int32 minLeft;
		_offset += sizeof(minLeft);
		ReadInput(minLeft, _sf, -_offset);

		int32 pixelSplit;
		_offset += sizeof(pixelSplit);
		ReadInput(pixelSplit, _sf, -_offset);

		Rococo::Cute::IParentWindow* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::Cute::ISplit* split = _pObject->SplitIntoLeftAndRight(pixelSplit, minLeft, maxRight, splitterWidth, draggable);
		_offset += sizeof(CReflectedClass*);
		auto& _splitStruct = Rococo::Helpers::GetDefaultProxy(("Rococo.Cute"),("ISplit"), ("ProxyISplit"), _nce.ss);
		CReflectedClass* _sxysplit = _nce.ss.Represent(_splitStruct, split);
		WriteOutput(&_sxysplit->header.pVTables[0], _sf, -_offset);
	}
	void NativeRococoCuteIParentWindowSplitIntoTopAndBottom(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 draggable;
		_offset += sizeof(draggable);
		ReadInput(draggable, _sf, -_offset);

		int32 splitterHeight;
		_offset += sizeof(splitterHeight);
		ReadInput(splitterHeight, _sf, -_offset);

		int32 maxBottom;
		_offset += sizeof(maxBottom);
		ReadInput(maxBottom, _sf, -_offset);

		int32 minTop;
		_offset += sizeof(minTop);
		ReadInput(minTop, _sf, -_offset);

		int32 pixelSplit;
		_offset += sizeof(pixelSplit);
		ReadInput(pixelSplit, _sf, -_offset);

		Rococo::Cute::IParentWindow* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::Cute::ISplit* split = _pObject->SplitIntoTopAndBottom(pixelSplit, minTop, maxBottom, splitterHeight, draggable);
		_offset += sizeof(CReflectedClass*);
		auto& _splitStruct = Rococo::Helpers::GetDefaultProxy(("Rococo.Cute"),("ISplit"), ("ProxyISplit"), _nce.ss);
		CReflectedClass* _sxysplit = _nce.ss.Represent(_splitStruct, split);
		WriteOutput(&_sxysplit->header.pVTables[0], _sf, -_offset);
	}
	void NativeRococoCuteIParentWindowSetMinimumSize(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 dy;
		_offset += sizeof(dy);
		ReadInput(dy, _sf, -_offset);

		int32 dx;
		_offset += sizeof(dx);
		ReadInput(dx, _sf, -_offset);

		Rococo::Cute::IParentWindow* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetMinimumSize(dx, dy);
	}
	void NativeRococoCuteIParentWindowSetMaximumSize(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 dy;
		_offset += sizeof(dy);
		ReadInput(dy, _sf, -_offset);

		int32 dx;
		_offset += sizeof(dx);
		ReadInput(dx, _sf, -_offset);

		Rococo::Cute::IParentWindow* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetMaximumSize(dx, dy);
	}
	void NativeRococoCuteIParentWindowAddTree(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 createStyleFlags;
		_offset += sizeof(createStyleFlags);
		ReadInput(createStyleFlags, _sf, -_offset);

		Rococo::Cute::IParentWindow* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::Cute::ITree* tree = _pObject->AddTree(createStyleFlags);
		_offset += sizeof(CReflectedClass*);
		auto& _treeStruct = Rococo::Helpers::GetDefaultProxy(("Rococo.Cute"),("ITree"), ("ProxyITree"), _nce.ss);
		CReflectedClass* _sxytree = _nce.ss.Represent(_treeStruct, tree);
		WriteOutput(&_sxytree->header.pVTables[0], _sf, -_offset);
	}

}

namespace Rococo { namespace Cute { 
	void AddNativeCalls_RococoCuteIParentWindow(Rococo::Script::IPublicScriptSystem& ss, Rococo::Cute::IParentWindow* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Cute.Native"));
		ss.AddNativeCall(ns, NativeRococoCuteIParentWindowHandle, nullptr, ("IParentWindowHandle (Pointer hObject) -> (Pointer ref)"));
		ss.AddNativeCall(ns, NativeRococoCuteIParentWindowMenu, nullptr, ("IParentWindowMenu (Pointer hObject) -> (Rococo.Cute.IMenu menu)"));
		ss.AddNativeCall(ns, NativeRococoCuteIParentWindowSplitIntoLeftAndRight, nullptr, ("IParentWindowSplitIntoLeftAndRight (Pointer hObject)(Int32 pixelSplit)(Int32 minLeft)(Int32 maxRight)(Int32 splitterWidth)(Bool draggable) -> (Rococo.Cute.ISplit split)"));
		ss.AddNativeCall(ns, NativeRococoCuteIParentWindowSplitIntoTopAndBottom, nullptr, ("IParentWindowSplitIntoTopAndBottom (Pointer hObject)(Int32 pixelSplit)(Int32 minTop)(Int32 maxBottom)(Int32 splitterHeight)(Bool draggable) -> (Rococo.Cute.ISplit split)"));
		ss.AddNativeCall(ns, NativeRococoCuteIParentWindowSetMinimumSize, nullptr, ("IParentWindowSetMinimumSize (Pointer hObject)(Int32 dx)(Int32 dy) -> "));
		ss.AddNativeCall(ns, NativeRococoCuteIParentWindowSetMaximumSize, nullptr, ("IParentWindowSetMaximumSize (Pointer hObject)(Int32 dx)(Int32 dy) -> "));
		ss.AddNativeCall(ns, NativeRococoCuteIParentWindowAddTree, nullptr, ("IParentWindowAddTree (Pointer hObject)(Int32 createStyleFlags) -> (Rococo.Cute.ITree tree)"));
	}
}}
// BennyHill generated Sexy native functions for Rococo::Cute::ISplit 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

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
	void NativeRococoCuteISplitLo(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Cute::ISplit* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::Cute::IParentWindow* low = _pObject->Lo();
		_offset += sizeof(CReflectedClass*);
		auto& _lowStruct = Rococo::Helpers::GetDefaultProxy(("Rococo.Cute"),("IParentWindow"), ("ProxyIParentWindow"), _nce.ss);
		CReflectedClass* _sxylow = _nce.ss.Represent(_lowStruct, low);
		WriteOutput(&_sxylow->header.pVTables[0], _sf, -_offset);
	}
	void NativeRococoCuteISplitHi(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Cute::ISplit* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::Cute::IParentWindow* high = _pObject->Hi();
		_offset += sizeof(CReflectedClass*);
		auto& _highStruct = Rococo::Helpers::GetDefaultProxy(("Rococo.Cute"),("IParentWindow"), ("ProxyIParentWindow"), _nce.ss);
		CReflectedClass* _sxyhigh = _nce.ss.Represent(_highStruct, high);
		WriteOutput(&_sxyhigh->header.pVTables[0], _sf, -_offset);
	}

}

namespace Rococo { namespace Cute { 
	void AddNativeCalls_RococoCuteISplit(Rococo::Script::IPublicScriptSystem& ss, Rococo::Cute::ISplit* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Cute.Native"));
		ss.AddNativeCall(ns, NativeRococoCuteISplitHandle, nullptr, ("ISplitHandle (Pointer hObject) -> (Pointer hWnd)"));
		ss.AddNativeCall(ns, NativeRococoCuteISplitLo, nullptr, ("ISplitLo (Pointer hObject) -> (Rococo.Cute.IParentWindow low)"));
		ss.AddNativeCall(ns, NativeRococoCuteISplitHi, nullptr, ("ISplitHi (Pointer hObject) -> (Rococo.Cute.IParentWindow high)"));
	}
}}
// BennyHill generated Sexy native functions for Rococo::Cute::IMasterWindow 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoCuteIMasterWindowHandle(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Cute::IMasterWindow* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		WindowRef ref = _pObject->Handle();
		_offset += sizeof(ref);
		WriteOutput(ref, _sf, -_offset);
	}
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
		WriteOutput(&_sxymenu->header.pVTables[0], _sf, -_offset);
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

		int32 maxRight;
		_offset += sizeof(maxRight);
		ReadInput(maxRight, _sf, -_offset);

		int32 minLeft;
		_offset += sizeof(minLeft);
		ReadInput(minLeft, _sf, -_offset);

		int32 pixelSplit;
		_offset += sizeof(pixelSplit);
		ReadInput(pixelSplit, _sf, -_offset);

		Rococo::Cute::IMasterWindow* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::Cute::ISplit* split = _pObject->SplitIntoLeftAndRight(pixelSplit, minLeft, maxRight, splitterWidth, draggable);
		_offset += sizeof(CReflectedClass*);
		auto& _splitStruct = Rococo::Helpers::GetDefaultProxy(("Rococo.Cute"),("ISplit"), ("ProxyISplit"), _nce.ss);
		CReflectedClass* _sxysplit = _nce.ss.Represent(_splitStruct, split);
		WriteOutput(&_sxysplit->header.pVTables[0], _sf, -_offset);
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

		int32 maxBottom;
		_offset += sizeof(maxBottom);
		ReadInput(maxBottom, _sf, -_offset);

		int32 minTop;
		_offset += sizeof(minTop);
		ReadInput(minTop, _sf, -_offset);

		int32 pixelSplit;
		_offset += sizeof(pixelSplit);
		ReadInput(pixelSplit, _sf, -_offset);

		Rococo::Cute::IMasterWindow* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::Cute::ISplit* split = _pObject->SplitIntoTopAndBottom(pixelSplit, minTop, maxBottom, splitterHeight, draggable);
		_offset += sizeof(CReflectedClass*);
		auto& _splitStruct = Rococo::Helpers::GetDefaultProxy(("Rococo.Cute"),("ISplit"), ("ProxyISplit"), _nce.ss);
		CReflectedClass* _sxysplit = _nce.ss.Represent(_splitStruct, split);
		WriteOutput(&_sxysplit->header.pVTables[0], _sf, -_offset);
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
	void NativeRococoCuteIMasterWindowAddTree(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 createStyleFlags;
		_offset += sizeof(createStyleFlags);
		ReadInput(createStyleFlags, _sf, -_offset);

		Rococo::Cute::IMasterWindow* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::Cute::ITree* tree = _pObject->AddTree(createStyleFlags);
		_offset += sizeof(CReflectedClass*);
		auto& _treeStruct = Rococo::Helpers::GetDefaultProxy(("Rococo.Cute"),("ITree"), ("ProxyITree"), _nce.ss);
		CReflectedClass* _sxytree = _nce.ss.Represent(_treeStruct, tree);
		WriteOutput(&_sxytree->header.pVTables[0], _sf, -_offset);
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
		ss.AddNativeCall(ns, NativeRococoCuteIMasterWindowHandle, nullptr, ("IMasterWindowHandle (Pointer hObject) -> (Pointer ref)"));
		ss.AddNativeCall(ns, NativeRococoCuteIMasterWindowMenu, nullptr, ("IMasterWindowMenu (Pointer hObject) -> (Rococo.Cute.IMenu menu)"));
		ss.AddNativeCall(ns, NativeRococoCuteIMasterWindowSplitIntoLeftAndRight, nullptr, ("IMasterWindowSplitIntoLeftAndRight (Pointer hObject)(Int32 pixelSplit)(Int32 minLeft)(Int32 maxRight)(Int32 splitterWidth)(Bool draggable) -> (Rococo.Cute.ISplit split)"));
		ss.AddNativeCall(ns, NativeRococoCuteIMasterWindowSplitIntoTopAndBottom, nullptr, ("IMasterWindowSplitIntoTopAndBottom (Pointer hObject)(Int32 pixelSplit)(Int32 minTop)(Int32 maxBottom)(Int32 splitterHeight)(Bool draggable) -> (Rococo.Cute.ISplit split)"));
		ss.AddNativeCall(ns, NativeRococoCuteIMasterWindowSetMinimumSize, nullptr, ("IMasterWindowSetMinimumSize (Pointer hObject)(Int32 dx)(Int32 dy) -> "));
		ss.AddNativeCall(ns, NativeRococoCuteIMasterWindowSetMaximumSize, nullptr, ("IMasterWindowSetMaximumSize (Pointer hObject)(Int32 dx)(Int32 dy) -> "));
		ss.AddNativeCall(ns, NativeRococoCuteIMasterWindowAddTree, nullptr, ("IMasterWindowAddTree (Pointer hObject)(Int32 createStyleFlags) -> (Rococo.Cute.ITree tree)"));
	}
}}
