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
		IGRPanelSupervisor* parent;
		IGRPanelRoot& root;
		IGRWidget* widget = nullptr; // Should always be set immediately after construction
		Vec2i parentOffset{ 0,0 };
		Vec2i span { 0, 0};
		std::vector<IGRPanelSupervisor*> children;
		int64 uniqueId;
		GuiRect absRect{ 0,0,0,0 };

		GRPanel(IGRPanelRoot& _root, IGRPanelSupervisor* _parent): root(_root), parent(_parent), uniqueId(nextId++)
		{

		}

		virtual ~GRPanel()
		{
			for (auto* child : children)
			{
				child->Free();
			}
		}

		int32 EnumerateChildren(IEventCallback<IGRPanel>* callback) override
		{
			if (callback)
			{
				for (auto* child : children)
				{
					callback->OnEvent(*child);
				}
			}

			return (int32) children.size();
		}

		int64 Id() const override
		{
			return uniqueId;
		}

		EventRouting NotifyAncestors(WidgetEvent& event, IGRWidget& sourceWidget) override
		{
			if (parent == nullptr)
			{
				return EventRouting::NextHandler;
			}

			auto& parentWidget = parent->Widget();

			if (parentWidget.OnChildEvent(event, sourceWidget) == EventRouting::Terminate)
			{
				return EventRouting::Terminate;
			}

			return parent->NotifyAncestors(event, sourceWidget);
		}

		IGRPanel& AddChild()
		{
			auto* child = new GRPanel(root, this);
			children.push_back(child);
			return* child;
		}

		GuiRect AbsRect() const override
		{
			return absRect;
		}

		void CaptureCursor() override
		{
			root.CaptureCursor(*this);
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

			if (widget)
			{
				widget->Layout(absRect);
			}

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

		EventRouting RouteCursorClickEvent(CursorEvent& ce) override
		{
			if (!IsPointInRect(ce.position, absRect))
			{
				return EventRouting::NextHandler;
			}

			for (auto* child : children)
			{
				EventRouting routing = child->RouteCursorClickEvent(ce);
				if (routing == EventRouting::Terminate)
				{
					return EventRouting::Terminate;
				}
			}

			if (!widget)
			{
				return EventRouting::NextHandler;
			}

			return widget->OnCursorClick(ce);
		}

		EventRouting RouteCursorMoveEvent(CursorEvent& ce) override
		{
			if (!IsPointInRect(ce.position, absRect))
			{
				return EventRouting::NextHandler;
			}

			for (auto* child : children)
			{
				EventRouting routing = child->RouteCursorMoveEvent(ce);
				if (routing == EventRouting::Terminate)
				{
					return EventRouting::Terminate;
				}
			}

			if (!widget)
			{
				return EventRouting::NextHandler;
			}

			return widget->OnCursorMove(ce);
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
	IGRPanelSupervisor* CreatePanel(IGRPanelRoot& root, IGRPanelSupervisor* parent)
	{
		return new ANON::GRPanel(root, parent);
	}
}