#include <rococo.gui.retained.h>

using namespace Rococo;
using namespace Rococo::Gui;

namespace ANON
{
	struct Button : IGRWidgetButton
	{
		IGRPanel& panel;

		Button(IGRPanel& owningPanel) : panel(owningPanel)
		{

		}

		void Free() override
		{
			delete this;
		}

		void Layout(const GuiRect& parentDimensions) override
		{

		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		void Render(IGRRenderContext& g) override
		{
			DrawButton(panel, false, false, false, g);
		}
	};

	struct ButtonFactory : IGRWidgetFactory
	{
		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			return *new Button(panel);
		}
	} s_ButtonFactory;
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API IGRWidgetButton* CreateWidgetButton(IGRPanel& panel)
	{
		return new ANON::Button(panel);
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetFactory& GetWidgetButtonFactory()
	{
		return ANON::s_ButtonFactory;
	}
}