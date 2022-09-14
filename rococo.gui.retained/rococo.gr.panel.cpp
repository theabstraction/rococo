#include <rococo.gui.retained.h>
#include <vector>
#include <rococo.maths.h>

namespace ANON
{
	using namespace Rococo;
	using namespace Rococo::Gui;

	static int64 nextId = 1;

	struct GRPanel : IGRPanelSupervisor
	{
		IGRPanelRoot& root;
		AutoFree<IGRLayoutSupervisor> layout;
		IGRWidget* widget = nullptr; // Should always be set immediately after construction
		Vec2i parentOffset{ 0,0 };
		Vec2i span { 0, 0};
		std::vector<IGRPanelSupervisor*> children;
		int64 uniqueId;
		GuiRect absRect{ 0,0,0,0 };

		GRPanel(IGRPanelRoot& _root, IGRLayoutSupervisor* _layout): root(_root), layout(_layout), uniqueId(nextId++)
		{

		}

		virtual ~GRPanel()
		{
			for (auto* child : children)
			{
				child->Free();
			}
		}

		int64 Id() const override
		{
			return uniqueId;
		}

		IGRPanel& AddChild()
		{
			auto* child = new GRPanel(root, layout);
			children.push_back(child);
			return* child;
		}

		GuiRect AbsRect() const override
		{
			return absRect;
		}


		IGRLayout& LayoutSystem() override
		{
			return *layout;
		}

		void Free() override
		{
			delete this;
		}

		IGRPanel& Resize(Vec2i span) override
		{
			this->span = span;
			return *this;
		}

		Vec2i ParentOffset() const override
		{
			return parentOffset;
		}

		void LayoutRecursive(Vec2i absoluteOrigin) override
		{
			Vec2i parentOrigin = parentOffset + absoluteOrigin;
			absRect = { parentOrigin.x, parentOrigin.y, parentOrigin.x + span.x, parentOrigin.y + span.y };

			for (auto* child : children)
			{
				child->LayoutRecursive(parentOrigin);
			}
		}

		void RenderRecursive(IGRRenderContext& g) override
		{
			if (!widget)
			{
				return;
			}

			widget->Render(g);

			for (auto* child : children)
			{
				child->RenderRecursive(g);
			}
		}

		IGRPanelRoot& Root() override
		{
			return root;
		}

		IGRPanel& SetParentOffset(Vec2i offset) override
		{
			this->parentOffset = offset;
			return *this;
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