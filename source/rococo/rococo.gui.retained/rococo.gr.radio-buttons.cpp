#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <rococo.strings.h>
#include <rococo.hashtable.h>
#include <rococo.ui.h>
#include <rococo.vkeys.h>
#include <vector>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::Strings;

namespace GRANON
{
	struct GRRadioButtons : IGRWidgetRadioButtons, IGRWidgetSupervisor, IGRWidgetLayout, IGRWidgetInitializer
	{
		IGRPanel& panel;

		float transparency = 1.0f;

		HString defaultButtonString;
		std::vector<HString> group;
		stringmap<HString> mapMetaToToggler;

		GRRadioButtons(IGRPanel& owningPanel) : panel(owningPanel)
		{
			group.reserve(8);
		}

		virtual ~GRRadioButtons()
		{

		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		IGRWidget& Widget() override
		{
			return *this;
		}

		void Free() override
		{
			delete this;
		}

		void Prep() override
		{
			for (auto& i : mapMetaToToggler)
			{
				cstr meta = i.first;
				cstr toggler = i.second.c_str();

				IGRWidgetButton* button = FindButtonWithMeta(panel, meta);
				if (!button)
				{
					RaiseError(panel, EGRErrorCode::Generic, __FUNCTION__, "Cannot find child with meta string: %s", meta);
					return;
				}

				auto* togglePanel = FindPanelWithDescription(toggler);
				if (!togglePanel)
				{
					RaiseError(panel, EGRErrorCode::Generic, __FUNCTION__, "Cannot find Panel.Description '%s' for meta: '%s'", toggler, meta);
					return;
				}
			}
		}

		static IGRPanel* FindPanelWithDescription(IGRPanel& panel, cstr description)
		{
			if (Eq(panel.Desc(), description))
			{
				return &panel;
			}

			return panel.FindDescendantByDesc(description);
		}

		IGRPanel* FindPanelWithDescription(cstr description)
		{
			auto* owner = Gui::FindOwner(Widget());
			if (!owner)
			{
				return nullptr;
			}

			auto& framePanel = owner->Panel();
			return FindPanelWithDescription(framePanel, description);
		}

		static IGRWidgetButton* FindButtonWithMeta(IGRPanel& panel, cstr metaString)
		{
			auto* button = Cast<IGRWidgetButton>(panel.Widget());
			if (button)
			{
				if (Eq(button->MetaData().stringData, metaString))
				{
					return button;
				}
				else
				{
					return nullptr;
				}
			}

			int nChildren = panel.EnumerateChildren(nullptr);
			for (int i = 0; i < nChildren; i++)
			{
				auto* child = panel.GetChild(i);
				button = FindButtonWithMeta(*child, metaString);
				if (button)
				{
					return button;
				}
			}

			return nullptr;
		}

		void MakeGroupExclusive(cstr defaultButton)
		{
			defaultButtonString = defaultButton;

			for (auto& member : group)
			{
				cstr meta = member.c_str();

				IGRWidgetButton* button = FindButtonWithMeta(panel, meta);
				if (!button)
				{
					RaiseError(panel, EGRErrorCode::Generic, __FUNCTION__, "Cannot find child with meta string: %s", meta);
					return;
				}

				button->SetEventPolicy(EGREventPolicy::NotifyAncestors);
				button->MakeToggleButton();

				bool isPressed = Eq(defaultButton, meta);
				button->SetPressedNoCallback(isPressed);

				auto i = mapMetaToToggler.find(meta);
				if (i != mapMetaToToggler.end())
				{
					cstr toggleDescription = i->second.c_str();
					auto* panel = FindPanelWithDescription(toggleDescription);
					if (panel)
					{
						panel->SetCollapsed(!isPressed);
					}
				}
			}
		}

		void LayoutBeforeFit() override
		{
			MakeGroupExclusive(defaultButtonString);
		}

		void LayoutBeforeExpand() override
		{

		}

		void LayoutAfterExpand() override
		{

		}

		EGREventRouting OnCursorClick(GRCursorEvent& ce) override
		{
			UNUSED(ce);
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnCursorMove(GRCursorEvent& ce) override
		{
			UNUSED(ce);
			return EGREventRouting::NextHandler;
		}

		void OnCursorEnter() override
		{

		}

		void OnCursorLeave() override
		{

		}

		void Render(IGRRenderContext& g) override
		{
			DrawPanelBackgroundEx(
				panel,
				g, 
				EGRSchemeColourSurface::CONTAINER_BACKGROUND,
				EGRSchemeColourSurface::CONTAINER_TOP_LEFT,
				EGRSchemeColourSurface::CONTAINER_BOTTOM_RIGHT,
				transparency
			);
		}

		EGREventRouting OnChildEvent(GRWidgetEvent& we, IGRWidget& src)
		{
			if (we.eventType == EGRWidgetEventType::BUTTON_CLICK)
			{
				auto* button = Cast<IGRWidgetButton>(src);
				if (button)
				{
					cstr meta = button->MetaData().stringData;
					for (auto& member : group)
					{
						if (Eq(meta, member))
						{
							MakeGroupExclusive(meta);
							return EGREventRouting::Terminate;
						}
					}
				}
			}

			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnKeyEvent(GRKeyEvent& ke) override
		{
			if (ke.osKeyEvent.IsUp())
			{
				return EGREventRouting::NextHandler;
			}

			auto* focusWidget = panel.Root().GR().FindFocusWidget();
			if (!focusWidget)
			{
				return EGREventRouting::NextHandler;
			}

			auto* buttonFocus = Cast<IGRWidgetButton>(*focusWidget);
			if (!buttonFocus)
			{
				return EGREventRouting::NextHandler;
			}

			switch (ke.osKeyEvent.VKey)
			{
			case Rococo::IO::VirtualKeys::VKCode_UP:
				if (navigation == EGRRadioNavigation::Vertical)
				{
					Gui::RotateFocusToNextSibling(*focusWidget, false);
					return EGREventRouting::Terminate;
				}				
				break;
			case Rococo::IO::VirtualKeys::VKCode_LEFT:
				if (navigation == EGRRadioNavigation::Horizontal)
				{
					Gui::RotateFocusToNextSibling(*focusWidget, false);
					return EGREventRouting::Terminate;
				}
				break;
			case Rococo::IO::VirtualKeys::VKCode_DOWN:
				if (navigation == EGRRadioNavigation::Vertical)
				{
					Gui::RotateFocusToNextSibling(*focusWidget, true);
					return EGREventRouting::Terminate;
				}
				break;
			case Rococo::IO::VirtualKeys::VKCode_RIGHT:
				if (navigation == EGRRadioNavigation::Horizontal)
				{
					Gui::RotateFocusToNextSibling(*focusWidget, true);
					return EGREventRouting::Terminate;
				}
				break;
			}

			return EGREventRouting::NextHandler;
		}

		EGRQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			auto result = QueryForParticularInterface<IGRWidgetInitializer>(this, ppOutputArg, interfaceId);
			if (result == EGRQueryInterfaceResult::SUCCESS)
			{
				return result;
			}
			result = QueryForParticularInterface<IGRWidgetLayout>(this, ppOutputArg, interfaceId);
			if (result == EGRQueryInterfaceResult::SUCCESS)
			{
				return result;
			}
			return QueryForParticularInterface<IGRWidgetRadioButtons>(this, ppOutputArg, interfaceId);
		}

		cstr GetImplementationTypeName() const override
		{
			return "GRRadioButtons";
		}

		void AddButtonToGroup(cstr description) override
		{
			if (description == nullptr || *description == 0)
			{
				RaiseError(panel, EGRErrorCode::InvalidArg, __FUNCTION__, "Blank [description]");
			}

			group.push_back(description);
		}

		EGRRadioNavigation navigation = EGRRadioNavigation::None;

		void SetNavigation(EGRRadioNavigation navigation) override
		{
			this->navigation = navigation;
		}

		void AddTab(cstr meta, cstr toggleTarget) override
		{
			if (meta == nullptr || toggleTarget == nullptr)
			{
				RaiseError(panel, EGRErrorCode::Generic, __FUNCTION__, "Null argument");
			}

			for (auto& member : group)
			{
				if (Eq(member, meta))
				{
					mapMetaToToggler[meta] = toggleTarget;
					return;
				}
			}

			RaiseError(panel, EGRErrorCode::Generic, __FUNCTION__, "Could not find tab for %s toggling %s in the radio button group list", meta, toggleTarget);
		}

		void SetDefaultButton(cstr defaultButton) override
		{
			if (defaultButton == nullptr || *defaultButton == 0)
			{
				RaiseError(panel, EGRErrorCode::InvalidArg, __FUNCTION__, "Blank [defaultButton]");
			}

			defaultButtonString = defaultButton;
		}
	};

	struct GRRadioButtonsFactory : IGRWidgetFactory
	{
		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			return *new GRRadioButtons(panel);
		}
	};
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetInitializer::InterfaceId()
	{
		return "IGRWidgetInitializer";
	}

	ROCOCO_GUI_RETAINED_API cstr IGRWidgetRadioButtons::InterfaceId()
	{
		return "IGRWidgetRadioButtons";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetRadioButtons& CreateRadioButtonsManager(IGRWidget& parent)
	{
		auto& gr = parent.Panel().Root().GR();

		GRANON::GRRadioButtonsFactory factory;
		auto& rb = static_cast<GRANON::GRRadioButtons&>(gr.AddWidget(parent.Panel(), factory));
		return rb;
	}
}