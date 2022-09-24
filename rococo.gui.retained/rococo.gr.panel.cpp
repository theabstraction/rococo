#include <rococo.gui.retained.h>
#include <vector>
#include <rococo.maths.h>

namespace GRANON
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
		GRAnchors anchors = { 0 };
		GRAnchorPadding padding = { 0 };
		bool isMarkedForDeletion = false;
		AutoFree<ISchemeSupervisor> scheme;

		GRPanel(IGRPanelRoot& _root, IGRPanelSupervisor* _parent): root(_root), parent(_parent), uniqueId(nextId++)
		{

		}

		virtual ~GRPanel()
		{
			static_cast<IGuiRetainedSupervisor&>(root.GR()).NotifyPanelDeleted(uniqueId);
			ClearChildren();
		}

		void MarkForDelete() override
		{
			isMarkedForDeletion = true;
			root.QueueGarbageCollect();
		}

		bool IsMarkedForDeletion() const override
		{
			return isMarkedForDeletion;
		}

		GRAnchors Anchors() override
		{
			return anchors;
		}

		IGRPanel& Add(GRAnchors anchors) override
		{
			static_assert(sizeof GRAnchors == sizeof uint32);
			uint32& uThisAnchor = reinterpret_cast<uint32&>(this->anchors);
			uint32& uArgAnchor = reinterpret_cast<uint32&>(anchors);
			uThisAnchor = uThisAnchor | uArgAnchor;
			return *this;
		}

		IGRPanel& Set(GRAnchors anchors) override
		{
			this->anchors = anchors;
			return *this;
		}

		GRAnchorPadding Padding() override
		{
			return padding;
		}

		IGRPanel& Set(GRAnchorPadding padding) override
		{
			this->padding = padding;
			return *this;
		}

		IGRPanel* GetChild(int32 index)
		{
			if (index < 0 || index >= children.size())
			{
				return nullptr;
			}

			return children[index];
		}

		void ClearChildren() override
		{
			for (auto* child : children)
			{
				child->Free();
			}

			children.clear();
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

		bool isLayoutValid = false;

		void ConfirmLayout() override
		{
			isLayoutValid = true;
		}

		void InvalidateLayout(bool invalidateAnscestors) override
		{
			isLayoutValid = false;

			if (invalidateAnscestors && parent)
			{
				parent->InvalidateLayout(invalidateAnscestors);
			}
		}

		bool RequiresLayout() const override
		{
			return !isLayoutValid;
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

		void GarbageCollectRecursive() override
		{
			bool removeMarkedChildren = false;

			if (isMarkedForDeletion)
			{
				for (auto* child : children)
				{
					child->Free();
				}
				children.clear();
				return;
			}

			for (auto* child : children)
			{
				child->GarbageCollectRecursive();
				if (child->IsMarkedForDeletion())
				{
					removeMarkedChildren = true;
				}
			}

			if (removeMarkedChildren)
			{
				std::vector<IGRPanelSupervisor*> newChildren;
				for (auto* child : children)
				{
					if (child->IsMarkedForDeletion())
					{
						child->Free();
					}
					else
					{
						newChildren.push_back(child);
					}
				}

				children.clear();
				children.swap(newChildren);
			}
		}

		IGRPanel& AddChild() override
		{
			auto* child = new GRPanel(root, this);
			children.push_back(child);
			return* child;
		}

		RGBAb GetColour(ESchemeColourSurface surface, RGBAb defaultColour) const override
		{
			RGBAb result;
			if (!TryGetColour(surface, result))
			{
				return defaultColour;
			}
			return result;
		}

		bool TryGetColour(ESchemeColourSurface surface, RGBAb& colour) const override
		{
			if (scheme && scheme->TryGetColour(surface, colour))
			{
				return true;
			}

			if (parent)
			{
				return parent->TryGetColour(surface, colour);
			}
			else
			{
				return Root().Scheme().TryGetColour(surface, colour);
			}
		}

		IGRPanel& Set(ESchemeColourSurface surface, RGBAb colour) override
		{
			if (!scheme)
			{
				scheme = CreateScheme();
			}

			scheme->SetColour(surface, colour);
			return *this;
		}

		IGRPanel* Parent() override
		{
			return parent;
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
			if (this->span != span)
			{
				this->span = span;
				InvalidateLayout(true);
			}
			return *this;
		}

		Vec2i ParentOffset() const override
		{
			return parentOffset;
		}

		void LayoutRecursive(Vec2i absoluteOrigin) override
		{
			if (!isLayoutValid)
			{
				Vec2i parentOrigin = parentOffset + absoluteOrigin;
				absRect = { parentOrigin.x, parentOrigin.y, parentOrigin.x + span.x, parentOrigin.y + span.y };

				if (widget)
				{
					widget->Layout(absRect);

					// Layout may invalidate the parent origin or span, in the case that a control adjusts its own size and location
					parentOrigin = parentOffset + absoluteOrigin;
					absRect = { parentOrigin.x, parentOrigin.y, parentOrigin.x + span.x, parentOrigin.y + span.y };
				}

				for (auto* child : children)
				{
					child->LayoutRecursive(parentOrigin);
				}

				isLayoutValid = true;
			}
		}

		void RenderRecursive(IGRRenderContext& g) override
		{
			if (!widget)
			{
				return;
			}

			if (span.x > 0 && span.y > 0)
			{
				widget->Render(g);

				for (auto* child : children)
				{
					child->RenderRecursive(g);
				}
			}
		}

		IGRPanelRoot& Root() const override
		{
			return root;
		}

		EventRouting RouteCursorClickEvent(CursorEvent& ce, bool filterChildrenByParentRect) override
		{
			if (filterChildrenByParentRect && !IsPointInRect(ce.position, absRect))
			{
				return EventRouting::NextHandler;
			}

			ce.history.RecordWidget(*widget);

			for (auto* child : children)
			{
				EventRouting routing = child->RouteCursorClickEvent(ce, filterChildrenByParentRect);
				if (routing == EventRouting::Terminate)
				{
					return EventRouting::Terminate;
				}
			}

			if (!widget || !IsPointInRect(ce.position, absRect))
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
			if (this->parentOffset != offset)
			{
				this->parentOffset = offset;
				InvalidateLayout(true);
			}
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
		return new GRANON::GRPanel(root, parent);
	}

	ROCOCO_GUI_RETAINED_API void LayoutChildByAnchors(IGRPanel& child, const GuiRect& parentDimensions)
	{
		auto anchors = child.Anchors();
		auto padding = child.Padding();

		Vec2i newPos = child.ParentOffset();
		Vec2i newSpan = child.Span();

		if (anchors.left)
		{
			newPos.x = padding.left;

			if (anchors.right)
			{
				if (anchors.expandsHorizontally)
				{
					newSpan.x = Width(parentDimensions) - padding.left - padding.right;
				}
				else
				{
					newPos.x = Centre(parentDimensions).x - (child.Span().x >> 1) - padding.right;
				}
			}
			else if (anchors.expandsHorizontally)
			{
				newSpan.x += child.ParentOffset().x - newPos.x;
			}
		}

		if (anchors.right)
		{
			if (!anchors.left)
			{
				if (anchors.expandsHorizontally)
				{
					newSpan.x = parentDimensions.right - child.ParentOffset().x - padding.right;
				}
				else
				{
					newPos.x = parentDimensions.right - child.Span().x - padding.right;
				}
			}
		}

		newPos.x = max(parentDimensions.left, newPos.x);
		newPos.x = min(parentDimensions.right, newPos.x);
		newSpan.x = max(0, newSpan.x);

		if (anchors.top)
		{
			newPos.y = padding.top;

			if (anchors.bottom)
			{
				if (anchors.expandsVertically)
				{
					newSpan.y = Height(parentDimensions) - padding.top - padding.bottom;
				}
				else
				{
					newPos.y = Centre(parentDimensions).y - child.Span().y - padding.bottom;
				}
			}
			else if (anchors.expandsVertically)
			{
				newSpan.y += child.ParentOffset().y - newPos.y;
			}
		}

		if (anchors.bottom)
		{
			if (!anchors.top)
			{
				if (anchors.expandsVertically)
				{
					newSpan.y += parentDimensions.top - child.ParentOffset().y;
				}
				else
				{
					newPos.y = parentDimensions.top - child.Span().y - padding.bottom;
				}
			}
		}

		child.SetParentOffset(newPos);
		child.Resize(newSpan);
	}

	ROCOCO_GUI_RETAINED_API void LayoutChildrenByAnchors(IGRPanel& parent, const GuiRect& parentDimensions)
	{
		int index = 0;
		while (auto* child = parent.GetChild(index++))
		{
			LayoutChildByAnchors(*child, parentDimensions);
		}
	}

}