#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <rococo.strings.h>
#include <vector>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::Strings;

namespace GRANON
{
	struct GRRadioButtons : IGRWidgetRadioButtons, IGRWidgetSupervisor
	{
		IGRPanel& panel;

		float transparency = 1.0f;

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

		EGREventRouting OnChildEvent(GRWidgetEvent&, IGRWidget&)
		{
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnKeyEvent(GRKeyEvent&) override
		{
			return EGREventRouting::NextHandler;
		}

		EGRQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			return QueryForParticularInterface<IGRWidgetRadioButtons>(this, ppOutputArg, interfaceId);
		}

		cstr GetImplementationTypeName() const override
		{
			return "GRRadioButtons";
		}

		HString defaultButton;
		std::vector<HString> group;

		void AddButtonToGroup(cstr description) override
		{
			if (description == nullptr || *description == 0)
			{
				RaiseError(panel, EGRErrorCode::InvalidArg, __FUNCTION__, "Blank [description]");
			}

			group.push_back(description);
		}

		void SetDefaultButton(cstr description) override
		{
			if (description == nullptr || *description == 0)
			{
				RaiseError(panel, EGRErrorCode::InvalidArg, __FUNCTION__, "Blank [description]");
			}

			defaultButton = description;
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