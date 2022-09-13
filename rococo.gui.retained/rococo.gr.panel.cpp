#include <rococo.gui.retained.h>
#include <vector>

namespace ANON
{
	using namespace Rococo;
	using namespace Rococo::Gui;

	struct GRPanel : IGRPanelSupervisor
	{
		IGRPanelRoot& root;
		AutoFree<IGRLayoutSupervisor> layout;
		IGRWidget* widget = nullptr; // Should always be set immediately after construction
		Vec2i parentOffset{ 0,0 };
		Vec2i span { 0, 0};
		std::vector<IGRPanelSupervisor*> children;

		GRPanel(IGRPanelRoot& _root, IGRLayoutSupervisor* _layout): root(_root), layout(_layout)
		{

		}

		virtual ~GRPanel()
		{
			for (auto* child : children)
			{
				child->Free();
			}
		}

		IGRPanel& AddChild()
		{
			auto* child = new GRPanel(root, layout);
			children.push_back(child);
			return* child;
		}

		IGRLayout& LayoutSystem() override
		{
			return *layout;
		}

		void Free() override
		{
			delete this;
		}

		void Resize(Vec2i span) override
		{
			this->span = span;
		}

		Vec2i ParentOffset() const override
		{
			return parentOffset;
		}

		IGRPanelRoot& Root() override
		{
			return root;
		}

		void SetParentOffset(Vec2i offset) override
		{
			this->parentOffset = offset;
		}

		Vec2i Span() const override
		{
			return span;
		}

		void SetWidget(IGRWidget& widget) override
		{
			this->widget = &widget;
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