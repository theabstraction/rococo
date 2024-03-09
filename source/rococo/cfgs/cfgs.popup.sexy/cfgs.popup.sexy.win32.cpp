#include <rococo.os.win32.h>
#include <rococo.window.h>
#include <rococo.cfgs.h>

using namespace Rococo;
using namespace Rococo::CFGS;
using namespace Rococo::Windows;

namespace ANON
{
	struct Popup : ICFGSContextPopupSupervisor, StandardWindowHandler
	{
		HWND hHostWindow;
		ICFGSDatabase& db;

		AutoFree<IParentWindowSupervisor> window;

		Popup(HWND _hHostWindow, ICFGSDatabase& _db) : hHostWindow(_hHostWindow), db(_db)
		{

		}

		void Free() override
		{
			delete this;
		}

		void MakeVisibleAt(Vec2i screenPosition)
		{
			MoveWindow(*window, screenPosition.x, screenPosition.y, 640, 480, FALSE);
			ShowWindow(*window, SW_SHOW);
		}

		void Create()
		{
			Windows::WindowConfig config;
			DWORD style = 0;
			SetPopupWindowConfig(config, GuiRect{ 0, 0, 640, 480 }, hHostWindow, "ContextPopup", style, 0);
			window = Windows::CreateChildWindow(config, this);
		}
	};
}

extern "C" CFGS_CONTEXT_POPUP_API ICFGSContextPopupSupervisor* CFGSCreateWin32ContextPopup(HWND hHostWindow, ICFGSDatabase& db)
{
	auto *popup = new ANON::Popup(hHostWindow, db);
	popup->Create();
	return popup;
}
