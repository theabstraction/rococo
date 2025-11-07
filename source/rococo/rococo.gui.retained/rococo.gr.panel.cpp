// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#include <rococo.gui.retained.ex.h>
#include <vector>
#include <rococo.maths.i32.h>
#include <rococo.maths.h>


#include <rococo.strings.h>

using namespace Rococo::Strings;

namespace GRANON
{
	using namespace Rococo;
	using namespace Rococo::Gui;

	static int64 nextId = 1;

	struct PaddingBools
	{
		int isLeft : 1;
		int isRight: 1;
		int isTop  : 1;
		int isBottom : 1;
	};

	struct GRPanel: IGRPanelSupervisor
	{
		GRPanel* parent;
		IGRPanelRootSupervisor& root;
		IGRWidgetSupervisor* widget = nullptr; // Should always be set immediately after construction
		Vec2i parentOffset{ 0,0 };
		Vec2i span { 0, 0};
		std::vector<GRPanel*> children;
		int64 uniqueId;
		GuiRect absRect{ 0,0,0,0 };
		GRAnchorPadding padding = { 0 };
		bool isMarkedForDeletion = false;
		AutoFree<IGRSchemeSupervisor> scheme;
		bool preventInvalidationFromChildren = false;
		bool isCollapsed = false;
		bool isRenderingLast = false;
		int64 refCount = 0;
		int64 flags = 0;
		int32 childPadding = 0;
		HString desc;
		const Sex::ISExpression* associatedSExpression = nullptr;
		IGRPanelSupervisor* clippingPanel;
		int32 cornerRadius = 4;

		// Currently only used by GradientFill widgets, but we have plans to change fill style more generally across widgets, so is defined here.
		EGRFillStyle fillStyle = EGRFillStyle::SOLID;

		EGRRectStyle rectStyle = EGRRectStyle::SHARP;

		GRPanel(IGRPanelRootSupervisor& _root, IGRPanelSupervisor* _parent): parent(static_cast<GRPanel*>(_parent)), root(_root), uniqueId(nextId++), clippingPanel(this)
		{
			refCount = 1;
		}

		void OnTick(float dt) override
		{
			widget->OnTick(dt);

			for (auto* child : children)
			{
				child->OnTick(dt);
			}
		}

		std::vector<HString> navigationTargets;

		void AddNavigationTarget(cstr target) override
		{
			if (navigationTargets.empty())
			{
				enum { INIT_NAVTARGET_SIZE = 8 };
				navigationTargets.reserve(INIT_NAVTARGET_SIZE);
			}
			navigationTargets.push_back(target);
		}

		int GetNavigationIndex(cstr panelDesc) const
		{
			int index = -1;

			if (panelDesc != nullptr)
			{
				for (int i = 0; i < (int)navigationTargets.size(); i++)
				{
					if (Eq(navigationTargets[i], panelDesc))
					{
						index = i;
					}
				}
			}

			return index;
		}

		DescAndIndex GetNextNavigationTarget(cstr panelDesc) override
		{
			if (navigationTargets.empty())
			{
				return { nullptr , -1 };
			}

			int index;

			int foundIndex = GetNavigationIndex(panelDesc);
			if (foundIndex == -1)
			{
				index = 0;
			}
			else
			{
				index = (foundIndex + 1) % (int)navigationTargets.size();
			}

			return { navigationTargets[index], foundIndex };
		}

		DescAndIndex GetPreviousNavigationTarget(cstr panelDesc) override
		{
			int index;

			int foundIndex = GetNavigationIndex(panelDesc);
			if (foundIndex == -1)
			{
				return { HString(), -1 };
			}
			else if (foundIndex == 0)
			{
				index = (int)navigationTargets.size() - 1;
			}
			else
			{
				index = (foundIndex - 1) % (int)navigationTargets.size();
			}

			return { navigationTargets[index], foundIndex };
		}

		HString directions[static_cast<int>(EGRNavigationDirection::Count)-1];

		IGRPanel& Set(EGRNavigationDirection direction, cstr targetDescription) override
		{
			int index = static_cast<int>(direction);

			if (index <= 0 || index >= static_cast<int>(EGRNavigationDirection::Count))
			{
				RaiseError(*this, EGRErrorCode::InvalidArg, __ROCOCO_FUNCTION__, "Bad [direction] %d", index);
			}
			else
			{
				directions[index - 1] = targetDescription == nullptr ? "" : targetDescription;
			}
			return *this;
		}

		IGRPanel* Navigate(EGRNavigationDirection direction) override
		{
			int index = static_cast<int>(direction);

			if (index <= 0 || index >= static_cast<int>(EGRNavigationDirection::Count))
			{
				RaiseError(*this, EGRErrorCode::InvalidArg, __ROCOCO_FUNCTION__, "Bad [direction] %d", index);
				return nullptr;
			}

			cstr description = directions[index - 1].c_str();
			if (*description == 0)
			{
				return nullptr;
			}

			auto* owner = FindOwner(*widget);
			if (!owner)
			{
				return nullptr;
			}

			return owner->Panel().FindDescendantByDesc(description);
		}

		IGRPanel* FindDescendantByDesc(cstr desc)
		{
			if (desc == nullptr)
			{
				return nullptr;
			}

			for (auto child : children)
			{
				if (Eq(child->desc, desc))
				{
					return child;
				}
			}

			for (auto child : children)
			{
				IGRPanel* candidate = child->FindDescendantByDesc(desc);
				if (candidate)
				{
					return candidate;
				}
			}

			return nullptr;
		}

		void SetClippingPanel(IGRPanel* panel) override
		{
			this->clippingPanel = static_cast<IGRPanelSupervisor*>(panel);
		}

		IGRPanel& SetCornerRadius(int radius) override
		{
			cornerRadius = radius;
			return *this;
		}

		int CornerRadius() const override
		{
			return cornerRadius;
		}

		IGRPanel& SetFillStyle(EGRFillStyle style) override
		{
			fillStyle = style;
			return *this;
		}

		EGRFillStyle FillStyle() const override
		{
			return fillStyle;
		}

		IGRPanel& SetRectStyle(EGRRectStyle style) override
		{
			rectStyle = style;
			return *this;
		}

		EGRRectStyle RectStyle() const override
		{
			return rectStyle;
		}

		void ClearAssociatedExpressions() override
		{
			associatedSExpression = nullptr;
			for (auto* child : children)
			{
				child->ClearAssociatedExpressions();
			}
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

		void PrepPanelAndDescendantsRecursive(IGRPanel& owningPanel)
		{
			for (auto& target : navigationTargets)
			{
				auto* targetPanel = owningPanel.FindDescendantByDesc(target);
				if (!targetPanel)
				{
					RaiseError(*this, EGRErrorCode::Generic, __ROCOCO_FUNCTION__, "Could not find navigation target \"%s\"", target.c_str());
					return;
				}
			}

			for (auto& direction : directions)
			{
				if (direction.length() == 0)
				{
					continue;
				}

				auto* targetPanel = owningPanel.FindDescendantByDesc(direction);
				if (!targetPanel)
				{
					RaiseError(*this, EGRErrorCode::Generic, __ROCOCO_FUNCTION__, "Could not find direction target \"%s\"", direction.c_str());
					return;
				}
			}

			auto* initializer = Cast<IGRWidgetInitializer>(*widget);
			if (initializer)
			{
				initializer->Prep();
			}

			for (auto* child : children)
			{
				child->PrepPanelAndDescendantsRecursive(owningPanel);
			}
		}

		void PrepPanelAndDescendants() override
		{
			auto* owner = FindOwner(*widget);
			if (!owner)
			{
				RaiseError(*this, EGRErrorCode::Generic, __ROCOCO_FUNCTION__, "No owner!");
				return;
			}

			PrepPanelAndDescendantsRecursive(owner->Panel());
		}

		cstr Desc() const override
		{
			return desc;
		}

		void SetDesc(cstr text) override
		{
			desc = text;
		}

		HString hint;

		void SetHint(cstr text) override
		{
			hint = text;
		}

		cstr Hint() const override
		{
			return hint;
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
			if (widget) widget->Free();
			static_cast<IGRSystemSupervisor&>(root.GR()).NotifyPanelDeleted(uniqueId);
			ClearChildren();
		}

		bool HasFocus() const override
		{
			return Root().GR().GetFocusId() == Id();
		}

		IGRPanel& FocusAndNotifyAncestors() override
		{
			Root().GR().SetFocus(Id());

			for (auto* target = this->parent; target != nullptr; target = target->parent)
			{
				auto* focusNotifier = Cast<IGRFocusNotifier>(target->Widget());
				if (focusNotifier)
				{
					EFlowLogic flow = focusNotifier->OnDeepChildFocusSet(Id());
					if (flow == EFlowLogic::BREAK)
					{
						break;
					}
				}
			}

			return *this;
		}

		bool IsCollapsed() const override
		{
			return isCollapsed;
		}

		bool IsCollapsedOrAncestorCollasped() const override
		{
			if (isCollapsed)
			{
				return true;
			}

			if (parent)
			{
				return parent->IsCollapsedOrAncestorCollasped();
			}
			else
			{
				return false;
			}
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

		GRAnchorPadding Padding() override
		{
			GRAnchorPadding modulatedPadding;
			modulatedPadding.left = paddingsArePercentages.isLeft ? padding.left * Span().x / 100  : padding.left;
			modulatedPadding.right = paddingsArePercentages.isRight ? padding.right * Span().x / 100 : padding.right;
			modulatedPadding.top = paddingsArePercentages.isTop ? padding.top * Span().y / 100 : padding.top;
			modulatedPadding.bottom = paddingsArePercentages.isBottom ? padding.bottom * Span().y / 100 : padding.bottom;			
			return modulatedPadding;
		}

		IGRPanel& Set(GRAnchorPadding padding) override
		{
			this->padding = padding;
			return *this;
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
			// We want to preserve the integrity of the children field during destruction, in case a widget destructor enumerates children
			// So we destruct it one member at a time, removing the member first
			while (!children.empty())
			{
				auto* child = children.back();
				children.pop_back();
				child->ReleasePanel();
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

		EGREventRouting NotifyAncestors(GRWidgetEvent& event, IGRWidget& sourceWidget) override
		{
			if (parent == nullptr)
			{
				return EGREventRouting::NextHandler;
			}

			if (parent->Widget().Manager().OnChildEvent(event, sourceWidget) == EGREventRouting::Terminate)
			{
				return EGREventRouting::Terminate;
			}

			return parent->NotifyAncestors(event, sourceWidget);
		}

		enum class ESizingRule
		{
			Constant,
			ConstantPercentage,
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
				if (child->isCollapsed)
				{
					continue;
				}

				sum += child->Span().x;
			}

			return sum;
		}

		int MaxChildWidth()
		{
			int m = 0;

			for (auto* child : children)
			{
				if (child->isCollapsed)
				{
					continue;
				}

				m = max(child->Span().x, m);
			}

			return m;
		}

		int SumChildHeight()
		{
			int sum = 0;

			for (auto* child : children)
			{
				if (child->isCollapsed)
				{
					continue;
				}

				sum += child->Span().y;
			}

			return sum;
		}

		int MaxChildHeight()
		{
			int m = 0;

			for (auto* child : children)
			{
				if (child->isCollapsed)
				{
					continue;
				}

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
				if (child->isCollapsed)
				{
					continue;
				}

				child->SetPanelLayoutRecursive(phase);
			}
		}

		void SetAbsRectRecursive()
		{
			absRect.left = parent ? parent->absRect.left : 0;
			absRect.left += parentOffset.x + (parent ? parent->Padding().left : 0);

			absRect.top = parent ? parent->absRect.top : 0;
			absRect.top += parentOffset.y + (parent ? parent->Padding().top : 0);
			absRect.right = absRect.left + Span().x;
			absRect.bottom = absRect.top + Span().y;

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
					if (child->isCollapsed)
					{
						continue;
					}

					child->parentOffset.x = dx;
					child->parentOffset.y = 0;
					child->SetAbsRectRecursive();
					dx += child->Span().x;
					dx += childPadding;
				}
				break;
			case ELayoutDirection::RightToLeft:
				dx = Width(absRect) - Padding().right;
				for (auto i = children.rbegin(); i != children.rend(); ++i)
				{
					auto* child = *i;

					if (child->isCollapsed)
					{
						continue;
					}

					dx -= child->Span().x;
					dx -= childPadding;
					child->parentOffset.x = dx;
					child->parentOffset.y = 0;
					child->SetAbsRectRecursive();
				}
				break;
			case ELayoutDirection::TopToBottom:
				for (auto* child : children)
				{
					if (child->isCollapsed)
					{
						continue;
					}

					child->parentOffset.x = 0;
					child->parentOffset.y = dy;
					child->SetAbsRectRecursive();
					dy += child->Span().y;
					dy += childPadding;
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

				int totalChildPadding = -childPadding;

				for (auto* child : children)
				{
					if (child->isCollapsed)
					{
						continue;
					}

					totalChildPadding += childPadding;

					if (child->widthSizing == ESizingRule::ExpandToParent)
					{
						nExpandingChildren++;
					}
					else if (child->widthSizing == ESizingRule::ConstantPercentage)
					{
						totalXSpanOfFixedWidthChildren += ((child->span.x * Span().x) / 100);
					}
					else
					{
						totalXSpanOfFixedWidthChildren += child->span.x;
					}
				}

				int freeSpace = Span().x - totalXSpanOfFixedWidthChildren - Padding().left - Padding().right - totalChildPadding;
				if (freeSpace <= 0 || nExpandingChildren == 0)
				{
					for (auto* child : children)
					{
						if (child->isCollapsed)
						{
							continue;
						}

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
						if (child->isCollapsed)
						{
							continue;
						}

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
					if (child->isCollapsed)
					{
						continue;
					}

					if (child->widthSizing == ESizingRule::ExpandToParent)
					{
						child->span.x = Span().x - Padding().left - Padding().right;
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

				int totalChildPadding = -childPadding;

				for (auto* child : children)
				{
					if (child->isCollapsed)
					{
						continue;
					}

					totalChildPadding += childPadding;

					if (child->heightSizing == ESizingRule::ExpandToParent)
					{
						nExpandingChildren++;
					}
					else
					{
						totalYSpanOfFixedWidthChildren += child->Span().y;
					}
				}

				int freeSpace = Span().y - totalYSpanOfFixedWidthChildren - Padding().top - Padding().bottom - totalChildPadding;
				if (freeSpace <= 0 || nExpandingChildren == 0)
				{
					for (auto* child : children)
					{
						if (child->isCollapsed)
						{
							continue;
						}

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
						if (child->isCollapsed)
						{
							continue;
						}

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
					if (child->isCollapsed)
					{
						continue;
					}

					if (child->heightSizing == ESizingRule::ExpandToParent)
					{
						child->span.y = Span().y - Padding().top - Padding().bottom;
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
				if (child->isCollapsed)
				{
					continue;
				}

				child->ExpandToParentRecursive();
			}
		}

		void ShrinkToFitRecursive()
		{
			for (auto* child : children)
			{
				if (child->isCollapsed)
				{
					continue;
				}

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
				span.x = width + Padding().left + Padding().right;
			}

			if (heightSizing == ESizingRule::FitChildren)
			{
				span.y = height + Padding().top + Padding().bottom;
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

		IGRPanel& SetConstantWidth(int width, bool isPercentage) override
		{
			if (watcher)
			{
				watcher->OnSetConstantWidth(*this, width);
			}

			widthSizing = isPercentage ? ESizingRule::ConstantPercentage : ESizingRule::Constant;
			span.x = width;
			return *this;
		}

		IGRPanel& SetConstantHeight(int height, bool isPercentage) override
		{
			if (watcher)
			{
				watcher->OnSetConstantHeight(*this, height);
			}

			heightSizing = isPercentage ? ESizingRule::ConstantPercentage : ESizingRule::Constant;
			span.y = height;
			return *this;
		}

		IGRPanel& SetConstantSpan(Vec2i ds) override
		{
			SetConstantWidth(ds.x, false);
			SetConstantHeight(ds.y, false);	
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

		RGBAb GetColour(EGRSchemeColourSurface surface, GRWidgetRenderState rs, RGBAb defaultColour) const override
		{
			if (surface == EGRSchemeColourSurface::NONE)
			{
				return RGBAb(0, 0, 0, 0);
			}

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

		void SetRenderLast(bool isRenderingLast) override
		{
			this->isRenderingLast = isRenderingLast;
		}

		bool TryGetColour(EGRSchemeColourSurface surface, RGBAb& colour, GRWidgetRenderState rs) const override
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

		IGRPanel& Set(EGRSchemeColourSurface surface, RGBAb colour, GRWidgetRenderState rs) override
		{
			if (!scheme)
			{
				scheme = CreateGRScheme();
			}

			scheme->SetColour(surface, colour, rs);
			return *this;
		}

		IGRPanel& Set(EGRSchemeColourSurface surface, RGBAb colour, EGRColourSpec spec) override
		{
			if (!scheme)
			{
				scheme = CreateGRScheme();
			}

			switch (spec)
			{
			case EGRColourSpec::None:
				break;
			case EGRColourSpec::ForAllRenderStates:
				SetUniformColourForAllRenderStates(*scheme, surface, colour);
				break;
			}

			return *this;
		}

		int32 ChildPadding() const override
		{
			return childPadding;
		}

		PaddingBools paddingsArePercentages{ 0,0,0,0 };

		IGRPanel& SetPaddingAsPercentage(bool left, bool right, bool top, bool bottom)
		{
			paddingsArePercentages = { left, right, top, bottom };
			return *this;
		}

		IGRPanel& SetChildPadding(int32 delta) override
		{
			childPadding = delta;
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

		IGRPanel& CaptureCursor() override
		{
			root.CaptureCursor(*this);
			return *this;
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
			}
			return *this;
		}

		Vec2i ParentOffset() const override
		{
			return parentOffset;
		}

		void RenderRecursive(IGRRenderContext& g, const GuiRect& clipRect, bool isRenderingFirstLayer, int64 focusId) override
		{
			if (!widget || isCollapsed)
			{
				return;
			}

			if (isRenderingFirstLayer && isRenderingLast)
			{
				root.DeferRendering(*this);
				return;
			}

			if (Span().x > 0 && Span().y > 0)
			{
				g.EnableScissors(clipRect);

				if (extraRenderer)
				{
					extraRenderer->PreRender(*this, clipRect, g);
				}

				if (!extraRenderer || !extraRenderer->IsReplacementForWidgetRendering(*this)) widget->Render(g);

				if (extraRenderer)
				{
					extraRenderer->PostRender(*this, clipRect, g);
				}

				if (focusId == this->uniqueId)
				{
					root.GR().RenderFocus(*this, g, clipRect);
				}
			}

			for (auto* child : children)
			{
				GuiRect childClipRect = doesClipChildren ? IntersectNormalizedRects(clipRect, child->AbsRect()) : child->AbsRect();
				if (childClipRect.IsNormalized() || child->isRenderingLast)
				{
					child->RenderRecursive(g, childClipRect, isRenderingFirstLayer, focusId);
				}
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

			if (parent)
			{
				auto* parentClippingPanel = parent->clippingPanel;
				if (parentClippingPanel && parentClippingPanel->DoesClipChildren())
				{
					if (!IsPointInRect(ce.position, parentClippingPanel->AbsRect()))
					{
						return EGREventRouting::NextHandler;
					}
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
			}
			return *this;
		}

		Vec2i Span() const override
		{
			if (isCollapsed)
			{
				return { 0,0 };
			}

			int dx = span.x;
			int dy = span.y;

			if (widthSizing == ESizingRule::ConstantPercentage && parent)
			{
				dx = span.x * parent->Span().x / 100;
			}

			if (heightSizing == ESizingRule::ConstantPercentage && parent)
			{
				dy = span.y * parent->Span().y / 100;
			}

			return { dx, dy };
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
	IGRPanelSupervisor* CreatePanel(IGRPanelRootSupervisor& root, IGRPanelSupervisor* parent)
	{
		return new GRANON::GRPanel(root, parent);
	}

	ROCOCO_GUI_RETAINED_API void CopyColour(IGRPanel& src, IGRPanel& target, EGRSchemeColourSurface srcSurface, EGRSchemeColourSurface trgSurface, GRWidgetRenderState rs)
	{
		RGBAb c = src.GetColour(srcSurface, rs);
		target.Set(trgSurface, c, rs);
	}

	ROCOCO_GUI_RETAINED_API void CopyAllColours(IGRPanel& src, IGRPanel& target, EGRSchemeColourSurface srcSurface, EGRSchemeColourSurface trgSurface)
	{
		GRWidgetRenderState::ForEachPermutation(
			[&src, &target, srcSurface, trgSurface](GRWidgetRenderState rs)
			{
				CopyColour(src, target, srcSurface, trgSurface, rs);
			}
		);
	}

	void Throw(IGRPanel& panel, EGRErrorCode code, cstr function, cstr format, ...)
	{
		va_list args;
		va_start(args, format);

		char message[1024];
		Strings::SafeVFormat(message, sizeof(message), format, args);

		char completeMessage[1280];
		Strings::SafeFormat(completeMessage, "Panel(%s): %s", panel.Desc(), message);

		RaiseError(panel, code, function, completeMessage);
		va_end(args);
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
		if (panel.IsCollapsed())
		{
			return nullptr;
		}

		if (panel.HasFlag(EGRPanelFlags::AcceptsFocus))
		{
			panel.FocusAndNotifyAncestors();
			return &panel;
		}

		DescAndIndex nextTarget = panel.GetNextNavigationTarget(nullptr);
		auto* targetPanel = panel.FindDescendantByDesc(nextTarget.desc);
		if (targetPanel != nullptr)
		{
			targetPanel->FocusAndNotifyAncestors();
			return targetPanel;
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
		SafeVFormat(buf, sizeof(buf),format, args);
		va_end(args);
		panel.Root().Custodian().RaiseError(panel.GetAssociatedSExpression(), errCode, function, "%s", buf);
	}

	ROCOCO_GUI_RETAINED_API void DrawEdge(EGRSchemeColourSurface topLeft, EGRSchemeColourSurface bottomRight, IGRPanel& panel, IGRRenderContext& rc)
	{
		GRWidgetRenderState edgeState(false, rc.IsHovered(panel), false);
		RGBAb topLeftColour = panel.GetColour(topLeft, edgeState);
		RGBAb bottomRightColour = panel.GetColour(bottomRight, edgeState);
		rc.DrawRectEdge(panel.AbsRect(), topLeftColour, bottomRightColour);
	}

	ROCOCO_GUI_RETAINED_API cstr IGRWidget::InterfaceId()
	{
		return "IGRWidget";
	}
}