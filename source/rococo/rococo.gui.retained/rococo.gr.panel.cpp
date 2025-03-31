#include <rococo.gui.retained.ex.h>
#include <vector>
#include <rococo.maths.i32.h>
#include <rococo.maths.h>

#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>

using namespace Rococo::Strings;

namespace GRANON
{
	using namespace Rococo;
	using namespace Rococo::Gui;

	static int64 nextId = 1;

	struct GRPanel: IGRPanelSupervisor
	{
		GRPanel* parent;
		IGRPanelRoot& root;
		IGRWidgetSupervisor* widget = nullptr; // Should always be set immediately after construction
		Vec2i parentOffset{ 0,0 };
		Vec2i span { 0, 0};
		Vec2i minimalSpan{ 0,0 };
		std::vector<GRPanel*> children;
		int64 uniqueId;
		GuiRect absRect{ 0,0,0,0 };
		GRAnchorPadding padding = { 0 };
		bool isMarkedForDeletion = false;
		AutoFree<IGRSchemeSupervisor> scheme;
		bool preventInvalidationFromChildren = false;
		bool isCollapsed = false;
		int64 refCount = 0;
		int64 flags = 0;
		HString desc;
		const Sex::ISExpression* associatedSExpression = nullptr;

		GRPanel(IGRPanelRoot& _root, IGRPanelSupervisor* _parent): root(_root), parent(static_cast<GRPanel*>(_parent)), uniqueId(nextId++)
		{
			refCount = 1;
		}

		const Sex::ISExpression* GetAssociatedSExpression() const override
		{
			return associatedSExpression;
		}

		void SetAssociatedSExpression(Sex::cr_sex s) override
		{
			associatedSExpression = &s;
		}

		IGRPanel& Add(EGRPanelFlags flag) override
		{
			flags |= (int64) flag;
			return *this;
		}

		void AppendDesc(Strings::StringBuilder& sb) override
		{
			sb.AppendFormat("%s (id %lld)", desc.c_str(), Id());
		}

		void SetDesc(cstr text) override
		{
			desc = text;
		}

		bool HasFlag(EGRPanelFlags flag) const override
		{
			return (flags & (int64)flag) != 0;
		}

		IGRPanel& Remove(EGRPanelFlags flag) override
		{
			flags &= ~(int64)flag;
			return *this;
		}

		virtual ~GRPanel()
		{
			widget->Free();
			static_cast<IGRSystemSupervisor&>(root.GR()).NotifyPanelDeleted(uniqueId);
			ClearChildren();
		}

		bool HasFocus() const override
		{
			return Root().GR().GetFocusId() == Id();
		}

		void Focus() override
		{
			Root().GR().SetFocus(Id());

			for (auto* target = this->parent; target != nullptr; target = target->parent)
			{
				auto* focusNotifier = Cast<IGRFocusNotifier>(target->Widget());
				if (focusNotifier)
				{
					focusNotifier->OnDeepChildFocusSet(Id());
				}
			}
		}

		bool IsCollapsed() const override
		{
			return isCollapsed;
		}

		void MarkForDelete() override
		{
			isMarkedForDeletion = true;
			root.QueueGarbageCollect();
		}

		Vec2i MinimalSpan() const override
		{
			return minimalSpan;
		}

		bool IsMarkedForDeletion() const override
		{
			return isMarkedForDeletion;
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

		void SetMinimalSpan(Vec2i span) override
		{
			this->minimalSpan = span;
		}

		IGRPanel* GetChild(int32 index) override
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
				child->ReleasePanel();
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

		void InvalidateLayout(bool invalidateAncestors) override
		{
			isLayoutValid = false;

			if (!preventInvalidationFromChildren && invalidateAncestors && parent)
			{
				parent->InvalidateLayout(invalidateAncestors);
			}
			else
			{
				// We have an orphaned the invalidation
				root.GR().UpdateNextFrame(*this);
			}
		}

		void PreventInvalidationFromChildren() override
		{
			preventInvalidationFromChildren = true;
		}

		bool RequiresLayout() const override
		{
			return !isLayoutValid;
		}

		EGREventRouting NotifyAncestors(GRWidgetEvent& event, IGRWidget& sourceWidget) override
		{
			if (parent == nullptr)
			{
				return EGREventRouting::NextHandler;
			}

			auto& parentWidget = static_cast<IGRWidgetManager&>(parent->Widget());

			if (parentWidget.OnChildEvent(event, sourceWidget) == EGREventRouting::Terminate)
			{
				return EGREventRouting::Terminate;
			}

			return parent->NotifyAncestors(event, sourceWidget);
		}

		enum class ESizingRule
		{
			Constant,
			ExpandToParent,
			FitChildren
		};
		
		ESizingRule widthSizing = ESizingRule::Constant;
		ESizingRule heightSizing = ESizingRule::Constant;

		ELayoutDirection layoutDirection = ELayoutDirection::LeftToRight;

		int SumChildWidth()
		{
			int sum = 0;

			for (auto* child : children)
			{
				sum += child->Span().x;
			}

			return sum;
		}

		int MaxChildWidth()
		{
			int m = 0;

			for (auto* child : children)
			{
				m = max(child->Span().x, m);
			}

			return m;
		}

		int SumChildHeight()
		{
			int sum = 0;

			for (auto* child : children)
			{
				sum += child->Span().y;
			}

			return sum;
		}

		int MaxChildHeight()
		{
			int m = 0;

			for (auto* child : children)
			{
				m = max(child->Span().y, m);
			}

			return m;
		}

		void Layout() override
		{
			SetPanelLayoutRecursive(ELayoutPhase::BeforeFit);			
			ShrinkToFitRecursive();

			SetPanelLayoutRecursive(ELayoutPhase::BeforeExpand);
			ExpandToParentRecursive();

			SetPanelLayoutRecursive(ELayoutPhase::AfterExpand);
			SetAbsRectRecursive();
		}

		enum class ELayoutPhase
		{
			BeforeFit,
			BeforeExpand,
			AfterExpand
		};

		void SetPanelLayoutRecursive(ELayoutPhase phase)
		{
			auto* layout = Cast<IGRWidgetLayout>(Widget());
			if (layout)
			{
				switch (phase)
				{
				case ELayoutPhase::BeforeFit:
					layout->LayoutBeforeFit();
					break;
				case ELayoutPhase::BeforeExpand:
					layout->LayoutBeforeExpand();
					break;
				default:
					layout->LayoutAfterExpand();
					break;
				}
			}

			for (auto* child : children)
			{
				child->SetPanelLayoutRecursive(phase);
			}
		}

		void SetAbsRectRecursive()
		{
			absRect.left = parent ? parent->absRect.left : 0;
			absRect.left += parentOffset.x + (parent ? parent->padding.left : 0);

			absRect.top = parent ? parent->absRect.top : 0;
			absRect.top += parentOffset.y + (parent ? parent->padding.top : 0);
			absRect.right = absRect.left + span.x;
			absRect.bottom = absRect.top + span.y;

			if (watcher)
			{
				watcher->OnSetAbsRect(*this, absRect);
			}

			int dx = 0;
			int dy = 0;

			switch (layoutDirection)
			{
			case ELayoutDirection::LeftToRight:
				for (auto* child : children)
				{
					child->parentOffset.x = dx;
					child->parentOffset.y = 0;
					child->SetAbsRectRecursive();
					dx += child->span.x;
				}
				break;
			case ELayoutDirection::RightToLeft:
				dx = Width(absRect) - padding.right;
				for (auto i = children.rbegin(); i != children.rend(); ++i)
				{
					auto* child = *i;
					dx -= child->span.x;
					child->parentOffset.x = dx;
					child->parentOffset.y = 0;
					child->SetAbsRectRecursive();
				}
				break;
			case ELayoutDirection::TopToBottom:
				for (auto* child : children)
				{
					child->parentOffset.x = 0;
					child->parentOffset.y = dy;
					child->SetAbsRectRecursive();
					dy += child->span.y;
				}
				break;
			case ELayoutDirection::BottomToTop:
				
				break;
			default:
				for (auto* child : children)
				{
					child->SetAbsRectRecursive();
				}
			}
		}

		void ExpandHorizontalSpansForChildren()
		{
			switch (layoutDirection)
			{
			case ELayoutDirection::LeftToRight:
			case ELayoutDirection::RightToLeft:
			{
				int nExpandingChildren = 0;
				int totalXSpanOfFixedWidthChildren = 0;

				for (auto* child : children)
				{
					if (child->widthSizing == ESizingRule::ExpandToParent)
					{
						nExpandingChildren++;
					}
					else
					{
						totalXSpanOfFixedWidthChildren += child->span.x;
					}
				}

				int freeSpace = span.x - totalXSpanOfFixedWidthChildren;
				if (freeSpace <= 0 || nExpandingChildren == 0)
				{
					for (auto* child : children)
					{
						if (child->widthSizing == ESizingRule::ExpandToParent)
						{
							child->span.x = 0;
						}
					}
				}
				else
				{
					int averageSpan = freeSpace / nExpandingChildren;

					for (auto* child : children)
					{
						if (child->widthSizing == ESizingRule::ExpandToParent)
						{
							child->span.x = averageSpan;
						}
					}
				}
			}
			break;
			default:
			{
				for (auto* child : children)
				{
					if (child->widthSizing == ESizingRule::ExpandToParent)
					{
						child->span.x = span.x;
					}
				}
			}
			break;
			}
		}

		void ExpandVerticalSpansForChildren()
		{
			switch (layoutDirection)
			{
			case ELayoutDirection::BottomToTop:
			case ELayoutDirection::TopToBottom:
			{
				int nExpandingChildren = 0;
				int totalYSpanOfFixedWidthChildren = 0;

				for (auto* child : children)
				{
					if (child->heightSizing == ESizingRule::ExpandToParent)
					{
						nExpandingChildren++;
					}
					else
					{
						totalYSpanOfFixedWidthChildren += child->span.y;
					}
				}

				int freeSpace = span.y - totalYSpanOfFixedWidthChildren;
				if (freeSpace <= 0 || nExpandingChildren == 0)
				{
					for (auto* child : children)
					{
						if (child->heightSizing == ESizingRule::ExpandToParent)
						{
							child->span.y = 0;
						}
					}
				}
				else
				{
					int averageSpan = freeSpace / nExpandingChildren;

					for (auto* child : children)
					{
						if (child->heightSizing == ESizingRule::ExpandToParent)
						{
							child->span.y = averageSpan;
						}
					}
				}
			}
			break;
			default:
			{
				for (auto* child : children)
				{
					if (child->heightSizing == ESizingRule::ExpandToParent)
					{
						child->span.y = span.y - padding.left - padding.right;
					}
				}
			}
			break;
			}
		}
		void ExpandToParentRecursive()
		{
			ExpandHorizontalSpansForChildren();
			ExpandVerticalSpansForChildren();

			for (auto* child : children)
			{
				child->ExpandToParentRecursive();
			}
		}

		void ShrinkToFitRecursive()
		{
			for (auto* child : children)
			{
				child->ShrinkToFitRecursive();
			}

			int width = 0;
			int height = 0;

			switch (layoutDirection)
			{
			case ELayoutDirection::LeftToRight:
			case ELayoutDirection::RightToLeft:
				width = SumChildWidth();
				height = MaxChildHeight();
				break;
			case ELayoutDirection::TopToBottom:
			case ELayoutDirection::BottomToTop:
				width = MaxChildWidth();
				height = SumChildHeight();
				break;
			}

			if (widthSizing == ESizingRule::FitChildren)
			{
				span.x = width + padding.left + padding.right;
			}

			if (heightSizing == ESizingRule::FitChildren)
			{
				span.y = height + padding.top + padding.bottom;
			}
		}

		IGRPanel& SetLayoutDirection(ELayoutDirection direction) override
		{
			this->layoutDirection = direction;
			return *this;
		}

		IGRPanel& SetFitChildrenHorizontally() override
		{
			widthSizing = ESizingRule::FitChildren;
			return *this;
		}

		IGRPanel& SetFitChildrenVertically() override
		{
			heightSizing = ESizingRule::FitChildren;
			return *this;
		}

		IGRPanel& SetExpandToParentHorizontally() override
		{
			widthSizing = ESizingRule::ExpandToParent;
			return *this;
		}

		IGRPanel& SetExpandToParentVertically() override
		{
			heightSizing = ESizingRule::ExpandToParent;
			return *this;
		}

		IGRPanel& SetConstantWidth(int width) override
		{
			if (watcher)
			{
				watcher->OnSetConstantWidth(*this, width);
			}

			widthSizing = ESizingRule::Constant;
			span.x = width;
			return *this;
		}

		IGRPanel& SetConstantHeight(int height) override
		{
			if (watcher)
			{
				watcher->OnSetConstantHeight(*this, height);
			}

			heightSizing = ESizingRule::Constant;
			span.y = height;
			return *this;
		}

		IGRPanel& SetConstantSpan(Vec2i ds) override
		{
			SetConstantWidth(ds.x);
			SetConstantHeight(ds.y);	
			return *this;
		}

		void GarbageCollectRecursive() override
		{
			bool removeMarkedChildren = false;

			if (isMarkedForDeletion)
			{
				for (auto* child : children)
				{
					child->ReleasePanel();
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
				std::vector<GRPanel*> newChildren;
				for (auto* child : children)
				{
					if (child->IsMarkedForDeletion())
					{
						child->ReleasePanel();
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

		RGBAb GetColour(EGRSchemeColourSurface surface, GRRenderState rs, RGBAb defaultColour) const override
		{
			RGBAb result;
			if (!TryGetColour(surface, result, rs))
			{
				return defaultColour;
			}
			return result;
		}

		void SetCollapsed(bool isCollapsed) override
		{
			this->isCollapsed = isCollapsed;
		}

		bool TryGetColour(EGRSchemeColourSurface surface, RGBAb& colour, GRRenderState rs) const override
		{
			if (scheme && scheme->TryGetColour(surface, colour, rs))
			{
				return true;
			}

			if (parent)
			{
				return parent->TryGetColour(surface, colour, rs);
			}
			else
			{
				return Root().Scheme().TryGetColour(surface, colour, rs);
			}
		}

		IGRPanel& Set(EGRSchemeColourSurface surface, RGBAb colour, GRRenderState rs) override
		{
			if (!scheme)
			{
				scheme = CreateGRScheme();
			}

			scheme->SetColour(surface, colour, rs);
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

		void ReleasePanel() override
		{
			refCount--;
			if (refCount == 0)
			{
				delete this;
			}
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

		void RenderRecursive(IGRRenderContext& g, const GuiRect& clipRect) override
		{
			if (!widget || isCollapsed)
			{
				return;
			}

			if (span.x > 0 && span.y > 0)
			{
				GuiRect laxClipRect = clipRect.IsNormalized() ? Expand(clipRect, 1) : clipRect;
				g.EnableScissors(laxClipRect);

				if (extraRenderer)
				{
					extraRenderer->PreRender(*this, clipRect, g);
				}

				if (!extraRenderer || !extraRenderer->IsReplacementForWidgetRendering(*this)) widget->Render(g);

				if (extraRenderer)
				{
					extraRenderer->PostRender(*this, clipRect, g);
				}

				for (auto* child : children)
				{
					GuiRect childClipRect = doesClipChildren ? IntersectNormalizedRects(clipRect, child->AbsRect()) : child->AbsRect();
					child->RenderRecursive(g, childClipRect);
				}
			}
			else
			{
				root.IncBadSpanCountThisFrame(*this);
			}
		}

		IGRPanelRoot& Root() const override
		{
			return root;
		}

		EGREventRouting RouteCursorClickEvent(GRCursorEvent& ce, bool filterChildrenByParentRect) override
		{
			if (isCollapsed)
			{
				return EGREventRouting::NextHandler;
			}

			if (filterChildrenByParentRect && !IsPointInRect(ce.position, absRect))
			{
				return EGREventRouting::NextHandler;
			}

			ce.history.RecordWidget(*widget);

			for (auto* child : children)
			{
				EGREventRouting routing = child->RouteCursorClickEvent(ce, filterChildrenByParentRect);
				if (routing == EGREventRouting::Terminate)
				{
					return EGREventRouting::Terminate;
				}
			}

			if (!widget || !IsPointInRect(ce.position, absRect))
			{
				return EGREventRouting::NextHandler;
			}

			return widget->OnCursorClick(ce);
		}

		void BuildCursorMovementHistoryRecursive(GRCursorEvent& ce, IGRPanelEventBuilder& eb) override
		{
			if (!IsPointInRect(ce.position, absRect))
			{
				return;
			}

			eb += { uniqueId, this, absRect };

			for (auto* child : children)
			{
				child->BuildCursorMovementHistoryRecursive(ce, eb);
			}
		}

		void BuildWidgetCallstackRecursiveUnderPoint(Vec2i targetPoint, IGRPanelEventBuilder& wb) override
		{
			if (!IsPointInRect(targetPoint, absRect))
			{
				return;
			}

			wb += { uniqueId, this, absRect };

			for (auto* child : children)
			{
				child->BuildWidgetCallstackRecursiveUnderPoint(targetPoint, wb);
			}
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

		void SetWidget(IGRWidgetSupervisor& widget) override
		{
			this->widget = &widget;
			this->desc = widget.GetImplementationTypeName();
		}

		IGRWidget& Widget() override
		{
			return *widget;
		}

		bool doesClipChildren = true;

		void SetClipChildren(bool enabled) override
		{
			doesClipChildren = enabled;
		}

		bool DoesClipChildren() const override
		{
			return doesClipChildren;
		}

		// It is recommended to place the implementation in either the widget or an ancestor widget, that way the pointer is valid for the life of the panel
		IGRPanelRenderer* extraRenderer = nullptr;

		// Get extra rendering before and after widget rendering for the panel
		IGRPanelRenderer* GetPanelRenderer() override
		{
			return extraRenderer;
		}

		// Add extra rendering before and after widget rendering for the panel
		IGRPanel& SetPanelRenderer(IGRPanelRenderer* renderer) override
		{
			extraRenderer = renderer;
			return *this;
		}

		IGRPanelWatcher* watcher = nullptr;

		IGRPanelWatcher* SetPanelWatcher(IGRPanelWatcher* newWatcher) override
		{
			auto* old = watcher;
			watcher = newWatcher;
			return old;
		}

		EGREventRouting RouteToParentRecursive(GRWidgetEvent& ev, IGRWidget& sender)
		{
			if (parent)
			{
				auto followup = parent->widget->OnChildEvent(ev, sender);
				if (followup == EGREventRouting::NextHandler)
				{
					return parent->RouteToParentRecursive(ev, sender);
				}
				else
				{
					return EGREventRouting::Terminate;
				}
			}
			else
			{
				return EGREventRouting::NextHandler;
			}
		}

		EGREventRouting RouteToParent(GRWidgetEvent& ev) override
		{
			auto followup = RouteToParentRecursive(ev, *widget);
			if (followup == EGREventRouting::NextHandler)
			{
				return RouteEventToHandler(*this, ev);
			}

			return EGREventRouting::Terminate;
		}
	};
}

namespace Rococo::Gui
{
	IGRPanelSupervisor* CreatePanel(IGRPanelRoot& root, IGRPanelSupervisor* parent)
	{
		return new GRANON::GRPanel(root, parent);
	}

	ROCOCO_GUI_RETAINED_API void CopyColour(IGRPanel& src, IGRPanel& target, EGRSchemeColourSurface srcSurface, EGRSchemeColourSurface trgSurface, GRRenderState rs)
	{
		RGBAb c = src.GetColour(srcSurface, rs);
		target.Set(trgSurface, c, rs);
	}

	ROCOCO_GUI_RETAINED_API void CopyAllColours(IGRPanel& src, IGRPanel& target, EGRSchemeColourSurface srcSurface, EGRSchemeColourSurface trgSurface)
	{
		CopyColour(src, target, srcSurface, trgSurface, GRRenderState(false, false, false));
		CopyColour(src, target, srcSurface, trgSurface, GRRenderState(false, false, true));
		CopyColour(src, target, srcSurface, trgSurface, GRRenderState(false, true, false));
		CopyColour(src, target, srcSurface, trgSurface, GRRenderState(false, true, true));
		CopyColour(src, target, srcSurface, trgSurface, GRRenderState(true, false, false));
		CopyColour(src, target, srcSurface, trgSurface, GRRenderState(true, false, true));
		CopyColour(src, target, srcSurface, trgSurface, GRRenderState(true, true, false));
		CopyColour(src, target, srcSurface, trgSurface, GRRenderState(true, true, true));
	}

	void Throw(IGRPanel& panel, EGRErrorCode code, cstr function, cstr format, ...)
	{
		va_list args;
		va_start(args, format);
		char desc[256];
		Strings::StackStringBuilder sb(desc, sizeof desc);
		panel.AppendDesc(sb);

		char message[1024];
		strcpy_s(message, desc);
		Strings::SafeVFormat(message + strlen(desc), sizeof message - strlen(desc), format, args);

		RaiseError(panel, code, function, message);
		va_end(args);
	}

	ROCOCO_GUI_RETAINED_API void InvalidateLayoutForAllChildren(IGRPanel& panel)
	{
		int32 index = 0;
		while (auto* child = panel.GetChild(index++))
		{
			child->InvalidateLayout(false);
		}
	}

	ROCOCO_GUI_RETAINED_API void InvalidateLayoutForAllDescendants(IGRPanel& panel)
	{
		int32 index = 0;
		while (auto* child = panel.GetChild(index++))
		{
			child->InvalidateLayout(false);
			InvalidateLayoutForAllDescendants(*child);
		}
	}

	ROCOCO_GUI_RETAINED_API cstr IGRPanelWatcher::InterfaceId()
	{
		return "IGRPanelWatcher";
	}

	ROCOCO_GUI_RETAINED_API cstr IGRNavigator::InterfaceId()
	{
		return "IGRNavigator";
	}

	// Enumerates all children and their descendants, and returns the one with the given id. If it is not found the function returns nullptr
	ROCOCO_GUI_RETAINED_API bool IsCandidateDescendantOfParent(IGRPanel& parent, IGRPanel& candidate)
	{
		for (auto* ancestor = candidate.Parent(); ancestor != nullptr; ancestor = ancestor->Parent())
		{
			if (ancestor == &parent)
			{
				return true;
			}
		}

		return false;
	}

	ROCOCO_GUI_RETAINED_API IGRPanel* TrySetDeepFocus(IGRPanel& panel)
	{
		if (panel.HasFlag(EGRPanelFlags::AcceptsFocus))
		{
			panel.Focus();
			return &panel;
		}

		int32 index = 0;
		while (auto* child = panel.GetChild(index++))
		{
			IGRPanel* candidate = TrySetDeepFocus(*child);
			if (candidate)
			{
				return candidate;
			}
		}

		return nullptr;
	}

	ROCOCO_GUI_RETAINED_API cstr IGRFocusNotifier::InterfaceId()
	{
		return "IGRFocusNotifier";
	}

	ROCOCO_GUI_RETAINED_API cstr IGRWidgetLayout::InterfaceId()
	{
		return "IGRWidgetLayout";
	}

	ROCOCO_GUI_RETAINED_API IGRCustodian& GetCustodian(IGRPanel& panel)
	{
		return panel.Root().Custodian();
	}

	ROCOCO_GUI_RETAINED_API IGRCustodian& GetCustodian(IGRWidget& widget)
	{
		return GetCustodian(widget.Panel());
	}

	ROCOCO_GUI_RETAINED_API void RaiseError(IGRPanel& panel, EGRErrorCode errCode, cstr function, const char* format, ...)
	{
		char buf[1024];
		va_list args;
		va_start(args, format);
		vsnprintf_s(buf, _TRUNCATE, format, args);
		va_end(args);
		panel.Root().Custodian().RaiseError(panel.GetAssociatedSExpression(), errCode, function, "%s", buf);
	}
}