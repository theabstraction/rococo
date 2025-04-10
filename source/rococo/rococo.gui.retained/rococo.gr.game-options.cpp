#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <rococo.game.options.h>
#include <rococo.hashtable.h>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::Game::Options;

namespace GRANON
{
	struct GRGameOptionsList : IGRWidgetGameOptions, IGRWidgetSupervisor, IGameOptionsBuilder
	{
		IGRPanel& panel;
		IGameOptions& options;
		stringmap<IGRWidgetGameOptionsChoice*> mapNameToChoiceControl;
		stringmap<IGRWidgetGameOptionsBool*> mapNameToBoolControl;
		stringmap<IGRWidgetGameOptionsScalar*> mapNameToScalarControl;
		GRFontId titleFont = GRFontId::NONE;

		GRGameOptionsList(IGRPanel& _panel, IGameOptions& _options) : panel(_panel), options(_options)
		{
			panel.SetMinimalSpan({ 100, 24 });
			panel.SetLayoutDirection(ELayoutDirection::TopToBottom);
			panel.SetExpandToParentHorizontally();
			panel.SetExpandToParentVertically();
			if (panel.Parent() == nullptr)
			{
				// We require a parent so that we can anchor to its dimensions
				RaiseError(panel, EGRErrorCode::InvalidArg, __FUNCTION__, "Panel parent was null");
				return;
			}

			panel.Set(GRAnchorPadding{ 4,4,4,4 });
			panel.SetChildPadding(16);

			FontSpec font;
			font.FontName = "Tahoma";
			font.CharHeight = 48;
			titleFont = GetCustodian(panel).Fonts().BindFontId(font);
		}

		void PostConstruct()
		{
			options.AddOptions(*this);
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

		IGRPanel& Panel() override
		{
			return panel;
		}

		void Render(IGRRenderContext& rc) override
		{
			CopyAllColours(panel, panel, EGRSchemeColourSurface::GAME_OPTION_TEXT, EGRSchemeColourSurface::TEXT);
			SetUniformColourForAllRenderStates(panel, EGRSchemeColourSurface::LABEL_BACKGROUND, RGBAb(32, 32, 32, 255));
			CopyAllColours(panel, panel, EGRSchemeColourSurface::GAME_OPTION_BACKGROUND, EGRSchemeColourSurface::BACKGROUND);

			DrawPanelBackground(panel, rc);

			int nChildren = panel.EnumerateChildren(nullptr);

			if (nChildren > 0)
			{
				auto* lastChild = panel.GetChild(nChildren - 1);
				int domainHeight = lastChild->ParentOffset().y + lastChild->Span().y + panel.ChildPadding();

				GRWidgetEvent updatedDomainHeight;
				updatedDomainHeight.eventType = EGRWidgetEventType::UPDATED_HEIGHT;
				updatedDomainHeight.clickPosition = Centre(panel.AbsRect());
				updatedDomainHeight.iMetaData = domainHeight;
				updatedDomainHeight.isCppOnly = true;
				updatedDomainHeight.panelId = panel.Id();
				updatedDomainHeight.sMetaData = GetImplementationTypeName();
				panel.NotifyAncestors(updatedDomainHeight, *this);
			}
		}

		EGREventRouting OnDropDownCollapsed(IGRWidget& sourceWidget)
		{
			auto* carousel = Cast<IGRWidgetCarousel>(sourceWidget);
			if (carousel)
			{
				auto& dropDown = carousel->DropDown();
				dropDown.Panel().Root().GR().SetFocus(-1);
				dropDown.Panel().Root().ReleaseCursor();
				dropDown.Panel().SetRenderLast(false);

				int nChildren = panel.EnumerateChildren(nullptr);
				for (int i = 0; i < nChildren; i++)
				{
					auto* child = panel.GetChild(i);
					child->Remove(EGRPanelFlags::HintObscure);
				}

				return EGREventRouting::Terminate;
			}
			
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnDropDownExpanded(IGRWidget& sourceWidget)
		{
			auto* carousel = Cast<IGRWidgetCarousel>(sourceWidget);
			if (carousel)
			{
				auto& dropDown = carousel->DropDown();
				dropDown.Panel().Focus();
				dropDown.Panel().CaptureCursor();
				dropDown.Panel().SetRenderLast(true);

				int nChildren = panel.EnumerateChildren(nullptr);

				for (int i = 0; i < nChildren; i++)
				{
					auto* child = panel.GetChild(i);
					if (child->Widget() != sourceWidget.Panel().Parent()->Widget())
					{
						// Something other than the carousel
						child->Add(EGRPanelFlags::HintObscure);
					}
				}

				return EGREventRouting::Terminate;
			}

			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnChoiceMade(const GRWidgetEvent& choice, IGRWidget& sourceWidget)
		{
			auto* choiceSelector = Cast<IGRWidgetGameOptionsChoice>(sourceWidget);
			if (!choiceSelector)
			{
				return EGREventRouting::NextHandler;
			}

			for (auto& i : mapNameToChoiceControl)
			{
				if (i.second == choiceSelector)
				{
					cstr optionName = i.first;
					cstr optionValue = choice.sMetaData;

					options.DB().Invoke(optionName, optionValue);
				}
			}
			return EGREventRouting::Terminate;
		}

		EGREventRouting OnScrollerReleased(GRWidgetEvent&, IGRWidget& sourceWidget)
		{
			// If the scroller is part of the scrollable drop down menu, return return focus and cursor capture to the menu

			for (IGRPanel* ancestor = sourceWidget.Panel().Parent(); ancestor != nullptr; ancestor = ancestor->Parent())
			{
				auto* dropDown = Cast<IGRWidgetScrollableMenu>(ancestor->Widget());
				if (dropDown)
				{
					dropDown->Panel().Focus().CaptureCursor();
					break;
				}
			}

			return EGREventRouting::Terminate;
		}

		EGREventRouting OnChildEvent(GRWidgetEvent& widgetEvent, IGRWidget& sourceWidget)
		{
			switch (widgetEvent.eventType)
			{
			case EGRWidgetEventType::DROP_DOWN_COLLAPSED:
				return OnDropDownCollapsed(sourceWidget);
			case EGRWidgetEventType::DROP_DOWN_EXPANDED:
				return OnDropDownExpanded(sourceWidget);
			case EGRWidgetEventType::CHOICE_MADE:
				return OnChoiceMade(widgetEvent, sourceWidget);
			case EGRWidgetEventType::SCROLLER_RELEASED:
				return OnScrollerReleased(widgetEvent, sourceWidget);
			}

			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnKeyEvent(GRKeyEvent&) override
		{
			return EGREventRouting::NextHandler;
		}

		EGRQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			return Gui::QueryForParticularInterface<IGRWidgetGameOptions>(this, ppOutputArg, interfaceId);
		}

		IGRWidget& Widget()
		{
			return *this;
		}

		cstr GetImplementationTypeName() const override
		{
			return "GRGameOptionsList";
		}

		template<class T>
		void GuaranteeUnique(T& t, cstr name)
		{
			auto i = t.find(name);
			if (i != t.end())
			{
				RaiseError(panel, EGRErrorCode::InvalidArg, __FUNCTION__, "GameOption<%s> already defined. GRGameOptionsList does not support duplicate names", name);
			}
		}

		IChoiceInquiry& AddChoice(cstr name) override
		{
			GuaranteeUnique(mapNameToChoiceControl, name);
			IGRWidgetGameOptionsChoice& choiceWidget = CreateGameOptionsChoice(*this, titleFont);
			mapNameToChoiceControl.insert(name, &choiceWidget);
			return choiceWidget.Inquiry();
		}

		IBoolInquiry& AddBool(cstr name) override
		{
			GuaranteeUnique(mapNameToBoolControl, name);
			IGRWidgetGameOptionsBool& boolWidget = CreateGameOptionsBool(*this, titleFont);
			mapNameToBoolControl.insert(name, &boolWidget);
			return boolWidget.Inquiry();
		}

		IScalarInquiry& AddScalar(cstr name) override
		{
			GuaranteeUnique(mapNameToScalarControl, name);
			IGRWidgetGameOptionsScalar& scalarWidget = CreateGameOptionsScalar(*this, titleFont);
			mapNameToScalarControl.insert(name, &scalarWidget);
			return scalarWidget.Inquiry();
		}
	};

	struct GRGameOptionsFactory : IGRWidgetFactory
	{
		IGameOptions& options;

		GRGameOptionsFactory(IGameOptions& _options): options(_options)
		{

		}

		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			return *new GRGameOptionsList(panel, options);
		}
	};
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetGameOptions::InterfaceId()
	{
		return "IGRWidgetGameOptions";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetGameOptions& CreateGameOptionsList(IGRWidget& parent, IGameOptions& options)
	{
		auto& gr = parent.Panel().Root().GR();

		GRANON::GRGameOptionsFactory factory(options);
		auto& l = static_cast<GRANON::GRGameOptionsList&>(gr.AddWidget(parent.Panel(), factory));
		l.PostConstruct();
		return l;
	}
}