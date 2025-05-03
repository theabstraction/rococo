#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <rococo.game.options.h>
#include <rococo.strings.h>
#include <rococo.ui.h>
#include <rococo.vkeys.h>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::Game::Options;
using namespace Rococo::Strings;

namespace GRANON
{
	struct GRGameOptionStringWidget : IGRWidgetGameOptionsString, IGRWidgetSupervisor, IStringInquiry, IGREditFilter, IGRWidgetLayout
	{
		IGRPanel& panel;
		IGRWidgetText* title = nullptr;
		IGRWidgetEditBox* editor = nullptr;

		GameOptionConfig config;

		GRGameOptionStringWidget(IGRPanel& _panel) : panel(_panel)
		{
			_panel.SetMinimalSpan({ 100, 24 });
			_panel.SetLayoutDirection(ELayoutDirection::TopToBottom);
			_panel.Add(EGRPanelFlags::AcceptsFocus);
			if (_panel.Parent() == nullptr)
			{
				// We require a parent so that we can anchor to its dimensions
				RaiseError(_panel, EGRErrorCode::InvalidArg, __FUNCTION__, "Panel parent was null");
				return;
			}

			panel.SetExpandToParentHorizontally();
			panel.Set(GRAnchorPadding{ 1, 1, 1, 1 });
		}

		void PostConstruct(const GameOptionConfig& config, int maxCharacters)
		{
			this->config = config;

			if (config.TitlesOnLeft)
			{
				panel.SetLayoutDirection(ELayoutDirection::None);
			}

			title = &AddGameOptionTitleWidget(*this, config);

			MakeTransparent(title->Panel(), EGRSchemeColourSurface::CONTAINER_TOP_LEFT);
			MakeTransparent(title->Panel(), EGRSchemeColourSurface::CONTAINER_BOTTOM_RIGHT);

			int height = (int)(config.FontHeightToOptionHeightMultiplier * GetCustodian(panel).Fonts().GetFontHeight(config.TitleFontId));
			panel.SetConstantHeight(height);

			editor = &CreateEditBox(*this, this, maxCharacters, config.CarouselButtonFontId);

			editor->Panel().SetExpandToParentHorizontally();
			editor->Panel().SetExpandToParentVertically();

			GRAlignmentFlags alignment;
			alignment.Add(EGRAlignment::HCentre);
			editor->SetAlignment(alignment, {0,0});

			CopyAllColours(panel, editor->Panel(), EGRSchemeColourSurface::CAROUSEL_TOP_LEFT, EGRSchemeColourSurface::CONTAINER_TOP_LEFT);
			CopyAllColours(panel, editor->Panel(), EGRSchemeColourSurface::CAROUSEL_BOTTOM_RIGHT, EGRSchemeColourSurface::CONTAINER_BOTTOM_RIGHT);
			CopyAllColours(panel, editor->Panel(), EGRSchemeColourSurface::CAROUSEL_TEXT, EGRSchemeColourSurface::TEXT);
			CopyAllColours(panel, editor->Panel(), EGRSchemeColourSurface::CAROUSEL_TEXT, EGRSchemeColourSurface::EDIT_TEXT);
		}

		void LayoutBeforeFit() override
		{

		}

		void LayoutBeforeExpand() override
		{

		}

		void LayoutAfterExpand() override
		{
			if (config.TitlesOnLeft)
			{
				int height = panel.Span().y;
				int width = panel.Span().x;
				title->Panel().SetConstantHeight(height);
				title->Panel().SetConstantWidth(width / 2);
				title->Panel().SetParentOffset({ 0,0 });

				editor->Panel().SetConstantWidth((width / 2) - config.StringSlotPadding.left - config.StringSlotPadding.right);
				editor->Panel().SetConstantHeight(height - config.StringSlotPadding.bottom - config.StringSlotPadding.top);
				editor->Panel().SetParentOffset({ (width / 2) + config.StringSlotPadding.left, config.StringSlotPadding.top });
			}
		}

		void OnUpdate(IGRWidgetEditBox& editor, IGREditorMicromanager& manager) override
		{
			if (filter)
			{
				filter->OnUpdate(editor, manager);
			}
		}

		void Free() override
		{
			delete this;
		}

		EGREventRouting OnCursorClick(GRCursorEvent& we) override
		{
			UNUSED(we);
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

		IGRPanel& Panel() override
		{
			return panel;
		}

		void Render(IGRRenderContext& rc) override
		{
			DrawGameOptionBackground(*title, panel, rc);
		}

		EGREventRouting OnChildEvent(GRWidgetEvent& widgetEvent, IGRWidget& sourceWidget)
		{
			UNUSED(widgetEvent);
			UNUSED(sourceWidget);
			return EGREventRouting::NextHandler;
		}

		int speed = 0;

		EGREventRouting OnKeyEvent(GRKeyEvent& ke) override
		{
			UNUSED(ke);
			return EGREventRouting::NextHandler;
		}

		EGRQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			auto result = QueryForParticularInterface<IGRWidgetLayout>(this, ppOutputArg, interfaceId);
			if (result == EGRQueryInterfaceResult::SUCCESS)
			{
				return result;
			}

			return QueryForParticularInterface<IGRWidgetGameOptionsString>(this, ppOutputArg, interfaceId);
		}

		IGRWidget& Widget()
		{
			return *this;
		}

		cstr GetImplementationTypeName() const override
		{
			return "GRGameOptionStringWidget";
		}

		void SetTitle(cstr text) override
		{
			title->SetText(text);
		}

		IStringInquiry& Inquiry() override
		{
			return *this;
		}

		void SetActiveValue(cstr textValue) override
		{
			editor->SetText(textValue);
		}

		void SetHint(cstr text) override
		{
			panel.SetHint(text);
		}

		IGREditFilter* filter = nullptr;

		void SetFilter(IGREditFilter* filter) override
		{
			this->filter = filter;
		}
	};

	struct GRGameOptionsStringFactory : IGRWidgetFactory
	{
		GRGameOptionsStringFactory()
		{

		}

		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			return *new GRGameOptionStringWidget(panel);
		}
	};
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetGameOptionsString::InterfaceId()
	{
		return "IGRWidgetGameOptionsString";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetGameOptionsString& CreateGameOptionsString(IGRWidget& parent, const GameOptionConfig& config, int maxCharacters)
	{
		auto& gr = parent.Panel().Root().GR();

		GRANON::GRGameOptionsStringFactory factory;
		auto& l = static_cast<GRANON::GRGameOptionStringWidget&>(gr.AddWidget(parent.Panel(), factory));
		l.PostConstruct(config, maxCharacters);
		return l;
	}
}