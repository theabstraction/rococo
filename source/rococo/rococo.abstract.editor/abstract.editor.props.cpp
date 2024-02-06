#include <rococo.abstract.editor.win32.h>
#include <rococo.window.h>

using namespace Rococo;
using namespace Rococo::Windows;

namespace ANON
{
	struct Properties : Rococo::Abedit::IUIPropertiesSupervisor
	{
		IParentWindowSupervisor& panelArea;

		Properties(IParentWindowSupervisor& _panelArea) : panelArea(_panelArea)
		{

		}

		void Free() override
		{
			delete this;
		}

		void Populate()
		{
			Rococo::Windows::AddLabel(panelArea, GuiRect{ 10, 10, 90, 30 }, "Geoff", 1, WS_VISIBLE, 0);
			Rococo::Windows::AddEditor(panelArea, GuiRect{ 100, 10, 190, 30 }, "Capes", 2, WS_VISIBLE, 0);
		}
	};
}

namespace Rococo::Abedit::Internal
{
	IUIPropertiesSupervisor* CreateProperties(Rococo::Windows::IParentWindowSupervisor& panelArea)
	{
		return new ANON::Properties(panelArea);
	}
}