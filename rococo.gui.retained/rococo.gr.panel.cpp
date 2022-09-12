#include <rococo.gui.retained.h>

namespace ANON
{
	using namespace Rococo;
	using namespace Rococo::Gui;

	struct GRPanel : IGRPanelSupervisor
	{
		IGRPanelRoot& root;
		AutoFree<IGRLayoutSupervisor> layout;
		IGRWidget* widget = nullptr; // Should always be set immediately after construction
		Vec2i span { 0, 0};

		GRPanel(IGRPanelRoot& _root, IGRLayoutSupervisor* _layout): root(_root), layout(_layout)
		{

		}

		IGRLayout& LayoutSystem() override
		{
			return *layout;
		}

		void Free() override
		{
			delete this;
		}

		void SetWidget(IGRWidget& widget) override
		{
			this->widget = &widget;
		}

		void Resize(Vec2i span) override
		{
			this->span = span;
		}

		IGRWidget& Widget() override
		{
			return *widget;
		}
	};
}

namespace Rococo::Gui
{
	IGRPanelSupervisor* CreatePanel(IGRPanelRoot& root, IGRLayoutSupervisor* layout)
	{
		return new ANON::GRPanel(root, layout);
	}
}