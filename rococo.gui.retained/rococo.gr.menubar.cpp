#include <rococo.gui.retained.h>
#include <rococo.maths.h>
#include <string>
#include <vector>
#include <unordered_map>

using namespace Rococo;
using namespace Rococo::Gui;

namespace ANON
{
	struct MenuBranch;

	struct MenuButton
	{
		std::string text;
		int64 iMetaData;
		std::string sMetaData;
		int isEnabled : 1;
	};

	struct MenuItem
	{
		MenuBranch* branch = nullptr;
		MenuButton* button = nullptr;
	};

	struct GRMenuTree
	{
		MenuBranch* root = nullptr;

		std::unordered_map<int64, MenuBranch*> mapIdToBranch;

		~GRMenuTree();

		MenuBranch* FindBranch(GRMenuItemId idContainer)
		{
			auto branchId = mapIdToBranch.find(idContainer.id);
			return (branchId != mapIdToBranch.end()) ? branchId->second : nullptr;
		}

		void Deactivate();

		bool IsActive() const;
	};

	struct MenuBranch
	{
		MenuBranch* parent = nullptr;
		GRMenuTree& tree;
		GRMenuItemId id;
		std::string text;
		std::vector<MenuItem> children;
		uint32 isEnabled : 1 = 1;
		uint32 isActive : 1 = 0;

		MenuBranch(GRMenuTree& _tree, int64 _id, MenuBranch* _parent) : tree(_tree), id({ _id }), parent(_parent)
		{
			_tree.mapIdToBranch[_id] = this;
		}

		int32 GetFirstActiveBranchIndex() const
		{
			int32 index = 0;
			for (auto& child : children)
			{
				if (child.branch && child.branch->isActive)
				{
					return index;
				}
				index++;
			}
			return -1;
		}

		~MenuBranch()
		{
			tree.mapIdToBranch.erase(id.id);

			for (auto& subMenu : children)
			{
				delete subMenu.branch;
				delete subMenu.button;
			}
		}

		void ToggleActive()
		{
			isActive = isActive ? 0 : 1;

			// Making a branch active makes all of its siblings inactive

			if (parent)
			{
				for (auto& sibling : parent->children)
				{
					if (sibling.branch && sibling.branch != this)
					{
						sibling.branch->isActive = false;
					}
				}
			}
		}

		void Deactivate()
		{
			isActive = false;
			for (auto& child : children)
			{
				if (child.branch)
				{
					child.branch->Deactivate();
				}
			}
		}
	};

	GRMenuTree::~GRMenuTree()
	{
		delete root;
	}

	void GRMenuTree::Deactivate()
	{
		root->Deactivate();
	}

	bool GRMenuTree::IsActive() const
	{
		for (auto& child : root->children)
		{
			if (child.branch && child.branch->isActive)
			{
				return true;
			}
		}

		return false;
	}

	struct GRMenuBar : IGRWidgetMenuBar
	{
		GRMenuTree tree;
		int64 nextId = 1;
		IGRPanel& panel;

		GRMenuBar(IGRPanel& owningPanel) : panel(owningPanel)
		{
			auto rootId = 0;
			tree.root = new MenuBranch(tree, rootId, nullptr);
		}

		bool AddButton(GRMenuItemId parentMenu, const GRMenuButtonItem& item) override
		{
			auto* branch = tree.FindBranch(parentMenu);
			if (!branch)
			{
				panel.Root().Custodian().RaiseError(GRErrorCode::InvalidArg, __FUNCTION__, "No sub menu found with matching id");
				return false;
			}

			ANON::MenuItem newMenuItem;
			auto* b = newMenuItem.button = new MenuButton();
			b->iMetaData = item.metaData.intData;
			b->sMetaData = item.metaData.stringData ? item.metaData.stringData : std::string();
			b->isEnabled = item.isEnabled;
			b->text = item.text;

			branch->children.push_back(newMenuItem);
			return true;
		}

		GRMenuItemId AddSubMenu(GRMenuItemId parentMenu, const GRMenuSubMenu& subMenu) override
		{
			auto* parent = tree.FindBranch(parentMenu);
			if (!parent)
			{
				panel.Root().Custodian().RaiseError(GRErrorCode::InvalidArg, __FUNCTION__, "No sub menu found with matching id");
				return GRMenuItemId{ -1 };
			}

			ANON::MenuItem newMenuItem;
			int64 branchId = nextId++;
			auto* b = newMenuItem.branch = new MenuBranch(tree, branchId, parent);
			b->isEnabled = subMenu.isEnabled;
			b->text = subMenu.text;

			parent->children.push_back(newMenuItem);
			return GRMenuItemId{ branchId };
		}

		enum { BUTTON_X_PADDING = 10 };

		Vec2i ShrinkPanelToFitText(IGRWidgetButton& button, Vec2i& lastPos)
		{
			Vec2i minimalSpan = button.EvaluateMinimalSpan();
			Vec2i newSpan = { minimalSpan.x + 2 * BUTTON_X_PADDING,  panel.Span().y };
			button.Panel().Resize(newSpan);
			button.Panel().SetParentOffset(lastPos);
			lastPos.x += newSpan.x + 1;
			return minimalSpan;
		}

		Vec2i ConstructButtonFromMenuItem(MenuItem& item, Vec2i& lastPos)
		{
			GRAlignmentFlags alignment;
			alignment.Add(GRAlignment::Left).Add(GRAlignment::VCentre);

			if (item.button != nullptr)
			{
				auto& button = CreateMenuButton(*this);
				button.SetTitle(item.button->text.c_str());
				button.SetMetaData({ item.button->iMetaData, item.button->text.c_str() });
				button.SetEventPolicy(GREventPolicy::NotifyAncestors);
				button.SetClickCriterion(GRClickCriterion::OnDownThenUp);
				button.SetAlignment(alignment, { BUTTON_X_PADDING,0});
				return ShrinkPanelToFitText(button, lastPos);
			}
			else if (item.branch != nullptr)
			{
				auto& button = CreateMenuButton(*this, true);
				button.SetTitle(item.branch->text.c_str());
				button.SetMetaData({ item.branch->id.id, "<sub-menu-id>" });
				button.SetEventPolicy(GREventPolicy::NotifyAncestors);
				button.SetClickCriterion(GRClickCriterion::OnDownThenUp);
				button.SetAlignment(alignment, { BUTTON_X_PADDING,0 });
				return ShrinkPanelToFitText(button, lastPos);
			}
			else
			{
				return { 0,0 };
			}
		}

		int32 maxDepth = 0;

		void ConstructWidgetsFromBranchRecursive(IGRPanel& origin, MenuBranch& branch, int depth, Vec2i startPos)
		{
			Vec2i lastPos = startPos;

			Vec2i largestMinimalSpan {0, 0};

			int32 currentChildCount = panel.EnumerateChildren(nullptr);

			for (auto& child : branch.children)
			{
				Vec2i minimalSpan = ConstructButtonFromMenuItem(child, lastPos);
				largestMinimalSpan.x = max(minimalSpan.x, largestMinimalSpan.x);
				largestMinimalSpan.y = max(minimalSpan.y, largestMinimalSpan.y);
			}			

			if (depth > 0)
			{
				if (depth > 1)
				{
					maxDepth = depth;
				}
				// Move the new children in a vertical row aligned to the longest menu item amongst them
				Vec2i vertPos = startPos;
				int32 newChildCount = panel.EnumerateChildren(nullptr);

				for (int32 i = currentChildCount; i < newChildCount; ++i)
				{
					IGRPanel* newChildPanel = panel.GetChild(i);
					if (newChildPanel)
					{
						newChildPanel->Resize({ largestMinimalSpan.x + 2 * BUTTON_X_PADDING, panel.Span().y });
						newChildPanel->SetParentOffset(vertPos);
						vertPos.y += panel.Span().y;
					}
				}
			}

			int32 activeBranchIndex = branch.GetFirstActiveBranchIndex();
			if (activeBranchIndex >= 0)
			{
				IGRPanel* activeBranchChild = nullptr;
				if (activeBranchChild = panel.GetChild(currentChildCount + activeBranchIndex))
				{
					Vec2i branchPos;
					if (depth == 0)
					{
						branchPos = activeBranchChild->ParentOffset() + Vec2i{ 0, panel.Span().y };
					}
					else
					{
						branchPos = activeBranchChild->ParentOffset() + Vec2i{ activeBranchChild->Span().x, 0 };
					}
					ConstructWidgetsFromBranchRecursive(*activeBranchChild, *branch.children[activeBranchIndex].branch, depth + 1, branchPos);
				}
			}
		}

		void ConstructWidgetsFromMenuTree()
		{
			ConstructWidgetsFromBranchRecursive(panel, *tree.root, 0, {0,0});
		}

		void Free() override
		{
			delete this;
		}

		void Layout(const GuiRect& panelDimensions) override
		{
			// It should be safe to clear children here, because they are not yet within our callstack
			static_cast<IGRPanelSupervisor&>(panel).ClearChildren();
			ConstructWidgetsFromMenuTree();
		}

		EventRouting OnCursorClick(CursorEvent& ce) override
		{
			return EventRouting::NextHandler;
		}

		EventRouting OnCursorMove(CursorEvent& ce) override
		{
			if (tree.IsActive())
			{
				int index = 0;
				IGRPanel* buttonPanel;
				while (buttonPanel = panel.GetChild(index++))
				{
					if (IsPointInRect(ce.position, buttonPanel->AbsRect()))
					{
						IGRWidgetButton& button = static_cast<IGRWidgetButton&>(buttonPanel->Widget());
						if (button.GetButtonFlags().forSubMenu)
						{
							int64 branchId = button.GetMetaData().intData;
							auto* branch = tree.FindBranch(GRMenuItemId{ branchId });
							if (branch)
							{
								if (!branch->isActive)
								{
									branch->ToggleActive();
									panel.InvalidateLayout();
								}
							}
						}
						break;
					}
				}
			}
			return EventRouting::NextHandler;
		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		void Render(IGRRenderContext& g) override
		{
			
		}

		EventRouting OnChildEvent(WidgetEvent& widgetEvent, IGRWidget& sourceWidget)
		{
			if (widgetEvent.eventType == WidgetEventType::BUTTON_CLICK)
			{
				auto& button = static_cast<IGRWidgetButton&>(sourceWidget);
				auto flags = button.GetButtonFlags();
				if (flags.forSubMenu)
				{
					auto meta = button.GetMetaData();
					auto* branch = tree.FindBranch(GRMenuItemId{ meta.intData });
					if (branch)
					{
						branch->ToggleActive();
						panel.InvalidateLayout();
						if (tree.IsActive()) panel.CaptureCursor();
					}
					return EventRouting::Terminate;
				}
				else
				{
					tree.Deactivate();
					panel.InvalidateLayout();
				}

				panel.Root().ReleaseCursor();
			}

			panel.Root().Custodian().OnGREvent(widgetEvent);
			return EventRouting::Terminate;
		}

		Vec2i EvaluateMinimalSpan() const override
		{
			return Vec2i{ 100, 24 };
		}
	};

	struct GRMenuBarFactory : IGRWidgetFactory
	{
		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			return *new GRMenuBar(panel);
		}
	} s_MenuBarFactory;
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API IGRWidgetMenuBar& CreateMenuBar(IGRWidget& parent)
	{
		auto& gr = parent.Panel().Root().GR();
		auto& bar = static_cast<ANON::GRMenuBar&>(gr.AddWidget(parent.Panel(), ANON::s_MenuBarFactory));
		return bar;
	}
}