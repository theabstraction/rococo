#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <string>

using namespace Rococo;
using namespace Rococo::Gui;

namespace GRANON
{
	struct GRButton : IGRWidgetButton, IGRWidgetSupervisor
	{
		IGRPanel& panel;
		EGRClickCriterion clickCriterion = EGRClickCriterion::OnDown;
		EGREventPolicy eventPolicy = EGREventPolicy::PublicEvent;

		bool isRaised = true;
		bool isMenu = false;
		bool forSubmenu = false;
		bool isEventHandlerCPPOnly = false;

		std::string raisedImagePath;
		std::string pressedImagePath;
		AutoFree<IGRImageMemento> raisedImage;
		AutoFree<IGRImageMemento> pressedImage;

		GRButton(IGRPanel& owningPanel) : panel(owningPanel)
		{
			alignment.Add(EGRAlignment::HCentre).Add(EGRAlignment::VCentre);
			SyncMinimalSpan();
		}

		void Free() override
		{
			delete this;
		}

		void Layout(const GuiRect& panelDimensions) override
		{
			UNUSED(panelDimensions);
		}

		GRButtonFlags GetButtonFlags() const override
		{
			GRButtonFlags flags;
			flags.forSubMenu = forSubmenu;
			flags.isEnabled = true;
			flags.isMenu = isMenu;
			flags.isRaised = isRaised;
			return flags;
		}

		void FireEvent(GRCursorEvent& ce)
		{
			GRWidgetEvent widgetEvent{ EGRWidgetEventType::BUTTON_CLICK, panel.Id(), iMetadata, sMetaData.c_str(), ce.position, isEventHandlerCPPOnly };

			if (eventPolicy == EGREventPolicy::PublicEvent)
			{		
				RouteEventToHandler(panel, widgetEvent);
			}
			else if (eventPolicy == EGREventPolicy::NotifyAncestors)
			{
				EGREventRouting routing = panel.NotifyAncestors(widgetEvent, *this);
				if (routing != EGREventRouting::Terminate)
				{
					// Nothing handled it
					RouteEventToHandler(panel, widgetEvent);
				}
			}
		}

		EGREventRouting OnCursorClick(GRCursorEvent& ce) override
		{
			if (ce.click.LeftButtonDown)
			{
				if (clickCriterion == EGRClickCriterion::OnDown)
				{
					if (isToggler)
					{
						isRaised = !isRaised;
					}
					else
					{
						isRaised = false;
					}

					SyncMinimalSpan();
					FireEvent(ce);
				}
				else if (clickCriterion == EGRClickCriterion::OnDownThenUp)
				{
					if (!isToggler)
					{
						isRaised = false;
					}
					SyncMinimalSpan();
					panel.CaptureCursor();
				}
				return EGREventRouting::Terminate;
			}
			else if (ce.click.LeftButtonUp)
			{
				if (clickCriterion == EGRClickCriterion::OnUp)
				{
					if (isToggler)
					{
						isRaised = !isRaised;
					}
					else
					{
						isRaised = true;
					}
					SyncMinimalSpan();
					FireEvent(ce);
				}
				else if (clickCriterion == EGRClickCriterion::OnDownThenUp)
				{
					bool flipped = true;

					if (isToggler)
					{
						isRaised = !isRaised;
					}
					else if (!isRaised)
					{
						isRaised = true;
					}
					else
					{
						flipped = false;
					}

					if (flipped)
					{
						SyncMinimalSpan();
						FireEvent(ce);
					}

					if (panel.Root().CapturedPanelId() == panel.Id())
					{
						panel.Root().ReleaseCursor();
					}
				}
				else if (clickCriterion == EGRClickCriterion::OnDown)
				{
					if (!isToggler)
					{
						isRaised = true;
						SyncMinimalSpan();
					}
					else
					{
						FireEvent(ce);
					}
				}

				return EGREventRouting::Terminate;
			}

			return EGREventRouting::NextHandler;
		}

		void OnCursorEnter() override
		{

		}

		void OnCursorLeave() override
		{
			if (!isToggler && !isRaised)
			{
				isRaised = true;
			}
		}

		EGREventRouting OnCursorMove(GRCursorEvent& ce) override
		{
			if (!IsPointInRect(ce.position, panel.AbsRect()) && panel.Root().CapturedPanelId() == panel.Id())
			{
				// The cursor has been moved outside the button, so capture should be lost
				panel.Root().ReleaseCursor();
			}
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnKeyEvent(GRKeyEvent&) override
		{
			return EGREventRouting::NextHandler;
		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		void Render(IGRRenderContext& g) override
		{
			if (isMenu)
			{
				DrawMenuButton(panel, false, isRaised, g);
			}
			else
			{
				DrawButton(panel, false, isRaised, g);
			}

			bool isHovered = g.IsHovered(panel);

			bool imageRendered = false;

			IGRImageMemento* image = isRaised ? raisedImage : pressedImage;

			GRRenderState rs(!isRaised, isHovered, false);

			if (image)
			{
				imageRendered = image->Render(panel, alignment, spacing, g);

				GuiRect fogRect = panel.AbsRect();
				fogRect.left += 1;
				fogRect.right -= 1;
				fogRect.top += 1;
				fogRect.bottom -= 1;
				g.DrawRect(fogRect, panel.GetColour(EGRSchemeColourSurface::BUTTON_IMAGE_FOG, rs, RGBAb(0, 0, 0, 128)));
			}

			if (!imageRendered)
			{
				RGBAb colour = panel.GetColour(EGRSchemeColourSurface::BUTTON_TEXT, rs);
				colour.alpha = isHovered ? colour.alpha : 3 * (colour.alpha / 4);
				DrawButtonText(panel, alignment, spacing, { title.c_str(), (int32)title.size() }, colour, g);
			}
		}

		GRAlignmentFlags alignment { 0 };
		Vec2i spacing { 0,0 };

		IGRWidgetButton& SetAlignment(GRAlignmentFlags alignment, Vec2i spacing) override
		{
			this->alignment = alignment;
			this->spacing = spacing;
			return *this;
		}

		IGRWidgetButton& SetImagePath(cstr imagePath) override
		{
			this->raisedImagePath = imagePath ? imagePath : std::string();
			this->pressedImagePath = imagePath ? imagePath : std::string();
			raisedImage = panel.Root().Custodian().CreateImageMemento("raised button", this->raisedImagePath.c_str());
			pressedImage = panel.Root().Custodian().CreateImageMemento("pressed button", this->pressedImagePath.c_str());
			SyncMinimalSpan();
			return *this;
		}

		IGRWidgetButton& SetPressedImagePath(cstr imagePath) override
		{
			this->pressedImagePath = imagePath ? imagePath : std::string();
			pressedImage = panel.Root().Custodian().CreateImageMemento("pressed button", this->pressedImagePath.c_str());
			SyncMinimalSpan();
			return *this;
		}

		IGRWidgetButton& SetRaisedImagePath(cstr imagePath) override
		{
			this->raisedImagePath = imagePath ? imagePath : std::string();
			raisedImage = panel.Root().Custodian().CreateImageMemento("raised button", this->raisedImagePath.c_str());
			SyncMinimalSpan();
			return *this;
		}

		IGRWidgetButton& SetClickCriterion(EGRClickCriterion criterion) override
		{
			this->clickCriterion = criterion;
			return *this;
		}

		IGRWidgetButton& SetEventPolicy(EGREventPolicy policy) override
		{
			this->eventPolicy = policy;
			return *this;
		}

		int64 iMetadata = 0;
		std::string sMetaData;

		IGRWidgetButton& SetMetaData(const GRControlMetaData& metaData, bool isCppOnly) override
		{
			iMetadata = metaData.intData;
			sMetaData = metaData.stringData ? metaData.stringData : std::string();
			isEventHandlerCPPOnly = isCppOnly;
			return *this;
		}

		GRControlMetaData GetMetaData() override
		{
			return GRControlMetaData { iMetadata, sMetaData.c_str() };
		}

		EGREventRouting OnChildEvent(GRWidgetEvent&, IGRWidget&)
		{
			return EGREventRouting::NextHandler;
		}

		std::string title;

		IGRWidgetButton& SetTitle(cstr title) override
		{
			this->title = title == nullptr ? std::string() : title;
			SyncMinimalSpan();
			return *this;
		}

		size_t GetTitle(char* titleBuffer, size_t nBytes) const override
		{
			if (titleBuffer == nullptr || nBytes == 0)
			{
				return title.size();
			}

			strncpy_s(titleBuffer, nBytes, title.c_str(), _TRUNCATE);
			return title.size();
		}

		Vec2i EvaluateMinimalSpan() const
		{
			const IGRImageMemento* image = isRaised ? raisedImage : pressedImage;
			if (image)
			{
				return image->Span() + Vec2i{2,2};
			}

			if (title.empty())
			{
				return Vec2i { 8, 8 } ;
			}

			return panel.Root().Custodian().EvaluateMinimalSpan(GRFontId::MENU_FONT, fstring{ title.c_str(), (int32) title.length() }) + Vec2i { 2, 2 };
		}

		void SyncMinimalSpan()
		{
			Vec2i minimalSpan = EvaluateMinimalSpan();
			panel.SetMinimalSpan(minimalSpan);
		}

		bool isToggler = false;

		void MakeToggleButton() override
		{
			isToggler = true;
		}

		EGRQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			if (!interfaceId || *interfaceId == 0) return EGRQueryInterfaceResult::INVALID_ID;
			if (DoInterfaceNamesMatch(interfaceId, "IGRWidgetButton"))
			{
				if (ppOutputArg) *ppOutputArg = static_cast<IGRWidgetButton*>(this);
				return EGRQueryInterfaceResult::SUCCESS;
			}

			return EGRQueryInterfaceResult::NOT_IMPLEMENTED;
		}

		IGRWidget& Widget()
		{
			return *this;
		}
	};

	struct GRButtonFactory : IGRWidgetFactory
	{
		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			return *new GRButton(panel);
		}
	} s_ButtonFactory;
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetButton::InterfaceId()
	{
		return "IGRWidgetButton";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetButton& CreateButton(IGRWidget& parent)
	{
		auto& gr = parent.Panel().Root().GR();
		auto& widget = gr.AddWidget(parent.Panel(), GRANON::s_ButtonFactory);
		IGRWidgetButton* button = Cast<IGRWidgetButton>(widget);
		return *button;
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetButton& CreateMenuButton(IGRWidget& parent, bool forSubmenu)
	{
		auto& gr = parent.Panel().Root().GR();
		auto& button = static_cast<GRANON::GRButton&>(gr.AddWidget(parent.Panel(), GRANON::s_ButtonFactory));
		button.isMenu = true;
		button.forSubmenu = forSubmenu;
		return button;
	}

	ROCOCO_GUI_RETAINED_API void DrawButton(IGRPanel& panel, bool focused, bool raised, IGRRenderContext& g)
	{
		UNUSED(focused);

		bool hovered = g.IsHovered(panel);

		GRRenderState rs(!raised, hovered, false);

		RGBAb colour = panel.GetColour(EGRSchemeColourSurface::BUTTON, rs);
		g.DrawRect(panel.AbsRect(), colour);

		RGBAb colour1 = panel.GetColour(EGRSchemeColourSurface::BUTTON_EDGE_TOP_LEFT, rs);
		RGBAb colour2 = panel.GetColour(EGRSchemeColourSurface::BUTTON_EDGE_BOTTOM_RIGHT, rs);

		g.DrawRectEdge(panel.AbsRect(), colour1, colour2);
	}

	ROCOCO_GUI_RETAINED_API void DrawMenuButton(IGRPanel& panel, bool focused, bool raised, IGRRenderContext& g)
	{
		UNUSED(focused);

		bool hovered = g.IsHovered(panel);

		GRRenderState rs(!raised, hovered, false);
		RGBAb colour = panel.GetColour(EGRSchemeColourSurface::MENU_BUTTON, rs);
		g.DrawRect(panel.AbsRect(), colour);

		RGBAb colour1 = panel.GetColour(EGRSchemeColourSurface::MENU_BUTTON_EDGE_TOP_LEFT, rs);
		RGBAb colour2 = panel.GetColour(EGRSchemeColourSurface::MENU_BUTTON_EDGE_BOTTOM_RIGHT, rs);
		g.DrawRectEdge(panel.AbsRect(), colour1, colour2);
	}

	ROCOCO_GUI_RETAINED_API void DrawButtonText(IGRPanel& panel, GRAlignmentFlags alignment, Vec2i spacing, const fstring& text, RGBAb colour, IGRRenderContext& g)
	{
		if (text.length == 0) return;

		GuiRect targetRect = panel.AbsRect();
		targetRect.left += spacing.x;
		targetRect.right -= spacing.x;
		targetRect.top += spacing.y;
		targetRect.bottom -= spacing.y;

		if (targetRect.left > targetRect.right)
		{
			std::swap(targetRect.left, targetRect.right);
		}

		if (targetRect.bottom < targetRect.top)
		{
			std::swap(targetRect.top, targetRect.bottom);
		}

		g.DrawText(GRFontId::MENU_FONT, targetRect, panel.AbsRect(), alignment, spacing, text, colour);
	}
}