#include <rococo.gui.retained.h>
#include <rococo.maths.h>
#include <string>

using namespace Rococo;
using namespace Rococo::Gui;

namespace GRANON
{
	struct GRButton : IGRWidgetButton
	{
		IGRPanel& panel;
		GRClickCriterion clickCriterion = GRClickCriterion::OnDown;
		GREventPolicy eventPolicy = GREventPolicy::PublicEvent;

		bool isRaised = true;
		bool isMenu = false;
		bool forSubmenu = false;

		std::string raisedImagePath;
		std::string pressedImagePath;
		AutoFree<IImageMemento> raisedImage;
		AutoFree<IImageMemento> pressedImage;

		GRButton(IGRPanel& owningPanel) : panel(owningPanel)
		{
			alignment.Add(GRAlignment::HCentre).Add(GRAlignment::VCentre);
		}

		void Free() override
		{
			delete this;
		}

		void Layout(const GuiRect& panelDimensions) override
		{

		}

		ButtonFlags GetButtonFlags() const override
		{
			ButtonFlags flags;
			flags.forSubMenu = forSubmenu;
			flags.isEnabled = true;
			flags.isMenu = isMenu;
			flags.isRaised = isRaised;
			return flags;
		}

		void FireEvent(CursorEvent& ce)
		{
			WidgetEvent widgetEvent{ WidgetEventType::BUTTON_CLICK, panel.Id(), iMetadata, sMetaData.c_str(), ce.position };

			if (eventPolicy == GREventPolicy::PublicEvent)
			{				
				panel.Root().Custodian().OnGREvent(widgetEvent);
			}
			else if (eventPolicy == GREventPolicy::NotifyAncestors)
			{
				EventRouting routing = panel.NotifyAncestors(widgetEvent, *this);
				if (routing != EventRouting::Terminate)
				{
					// Nothing handled it
					panel.Root().Custodian().OnGREvent(widgetEvent);
				}
			}
		}

		EventRouting OnCursorClick(CursorEvent& ce) override
		{
			if (ce.click.LeftButtonDown)
			{
				if (clickCriterion == GRClickCriterion::OnDown)
				{
					if (isToggler)
					{
						isRaised = !isRaised;
					}
					else
					{
						isRaised = false;
					}

					FireEvent(ce);
				}
				else if (clickCriterion == GRClickCriterion::OnDownThenUp)
				{
					if (!isToggler)
					{
						isRaised = false;
					}

					panel.CaptureCursor();
				}
				return EventRouting::Terminate;
			}
			else if (ce.click.LeftButtonUp)
			{
				if (clickCriterion == GRClickCriterion::OnUp)
				{
					if (isToggler)
					{
						isRaised = !isRaised;
					}
					else
					{
						isRaised = true;
					}

					FireEvent(ce);
				}
				else if (clickCriterion == GRClickCriterion::OnDownThenUp)
				{
					if (isToggler)
					{
						isRaised = !isRaised;
					}
					else
					{
						isRaised = true;
					}

					FireEvent(ce);

					if (panel.Root().CapturedPanelId() == panel.Id())
					{
						panel.Root().ReleaseCursor();
					}
				}
				else if (clickCriterion == GRClickCriterion::OnDown)
				{
					if (!isToggler)
					{
						isRaised = true;
					}
					else
					{
						FireEvent(ce);
					}
				}

				return EventRouting::Terminate;
			}

			return EventRouting::NextHandler;
		}

		EventRouting OnCursorMove(CursorEvent& ce) override
		{
			if (!IsPointInRect(ce.position, panel.AbsRect()) && panel.Root().CapturedPanelId() == panel.Id())
			{
				// The cursor has been moved outside the button, so capture should be lost
				panel.Root().ReleaseCursor();
			}
			return EventRouting::NextHandler;
		}

		EventRouting OnKeyEvent(KeyEvent& keyEvent) override
		{
			return EventRouting::NextHandler;
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

			IImageMemento* image = isRaised ? raisedImage : pressedImage;

			if (image)
			{
				imageRendered = image->Render(panel, alignment, spacing, g);
				if (isRaised)
				{
					GuiRect fogRect = panel.AbsRect();
					fogRect.left += 1;
					fogRect.right -= 1;
					fogRect.top += 1;
					fogRect.bottom -= 1;
					g.DrawRect(fogRect, panel.GetColour(isHovered ? ESchemeColourSurface::IMAGE_FOG_HOVERED : ESchemeColourSurface::IMAGE_FOG, RGBAb(0, 0, 0, 128)));
				}
			}

			if (!imageRendered)
			{
				RGBAb colour = panel.GetColour(ESchemeColourSurface::BUTTON_TEXT);
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
			raisedImage = panel.Root().Custodian().CreateImageMemento(this->raisedImagePath.c_str());
			pressedImage = panel.Root().Custodian().CreateImageMemento(this->pressedImagePath.c_str());
			return *this;
		}

		IGRWidgetButton& SetPressedImagePath(cstr imagePath) override
		{
			this->pressedImagePath = imagePath ? imagePath : std::string();
			pressedImage = panel.Root().Custodian().CreateImageMemento(this->pressedImagePath.c_str());
			return *this;
		}

		IGRWidgetButton& SetRaisedImagePath(cstr imagePath) override
		{
			this->raisedImagePath = imagePath ? imagePath : std::string();
			raisedImage = panel.Root().Custodian().CreateImageMemento(this->raisedImagePath.c_str());
			return *this;
		}

		IGRWidgetButton& SetClickCriterion(GRClickCriterion criterion) override
		{
			this->clickCriterion = criterion;
			return *this;
		}

		IGRWidgetButton& SetEventPolicy(GREventPolicy policy) override
		{
			this->eventPolicy = policy;
			return *this;
		}

		int64 iMetadata = 0;
		std::string sMetaData;

		IGRWidgetButton& SetMetaData(const ControlMetaData& metaData) override
		{
			iMetadata = metaData.intData;
			sMetaData = metaData.stringData ? metaData.stringData : std::string();
			return *this;
		}

		ControlMetaData GetMetaData() override
		{
			return ControlMetaData { iMetadata, sMetaData.c_str() };
		}

		EventRouting OnChildEvent(WidgetEvent& widgetEvent, IGRWidget& sourceWidget)
		{
			return EventRouting::NextHandler;
		}

		std::string title;

		IGRWidgetButton& SetTitle(cstr title) override
		{
			this->title = title == nullptr ? std::string() : title;
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

		Vec2i EvaluateMinimalSpan() const override
		{
			const IImageMemento* image = isRaised ? raisedImage : pressedImage;
			if (image)
			{
				return image->Span() + Vec2i{2,2};
			}
			if (title.empty())
			{
				return { 8, 8 };
			}

			return panel.Root().Custodian().EvaluateMinimalSpan(GRFontId::MENU_FONT, fstring{ title.c_str(), (int32) title.length() }) + Vec2i { 2, 2 };
		}

		bool isToggler = false;

		void MakeToggleButton() override
		{
			isToggler = true;
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
	ROCOCO_GUI_RETAINED_API IGRWidgetButton& CreateButton(IGRWidget& parent)
	{
		auto& gr = parent.Panel().Root().GR();
		return static_cast<IGRWidgetButton&>(gr.AddWidget(parent.Panel(), GRANON::s_ButtonFactory));
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
		bool hovered = g.IsHovered(panel);

		ESchemeColourSurface surface;
		if (hovered)
		{
			surface = raised ? ESchemeColourSurface::BUTTON_RAISED_AND_HOVERED : ESchemeColourSurface::BUTTON_PRESSED_AND_HOVERED;
		}
		else
		{
			surface = raised ? ESchemeColourSurface::BUTTON_RAISED : ESchemeColourSurface::BUTTON_PRESSED;
		}

		RGBAb colour = panel.GetColour(surface);
		g.DrawRect(panel.AbsRect(), colour);

		ESchemeColourSurface topLeftEdge;
		ESchemeColourSurface bottomRightEdge;

		topLeftEdge = raised ? ESchemeColourSurface::BUTTON_EDGE_TOP_LEFT : ESchemeColourSurface::BUTTON_EDGE_TOP_LEFT_PRESSED;
		bottomRightEdge = raised ? ESchemeColourSurface::BUTTON_EDGE_BOTTOM_RIGHT : ESchemeColourSurface::BUTTON_EDGE_BOTTOM_RIGHT_PRESSED;

		RGBAb colour1 = panel.GetColour(topLeftEdge);
		RGBAb colour2 = panel.GetColour(bottomRightEdge);

		g.DrawRectEdge(panel.AbsRect(), colour1, colour2);
	}

	ROCOCO_GUI_RETAINED_API void DrawMenuButton(IGRPanel& panel, bool focused, bool raised, IGRRenderContext& g)
	{
		bool hovered = g.IsHovered(panel);

		ESchemeColourSurface surface;
		if (hovered)
		{
			surface = raised ? ESchemeColourSurface::MENU_BUTTON_RAISED_AND_HOVERED : ESchemeColourSurface::MENU_BUTTON_PRESSED_AND_HOVERED;
		}
		else
		{
			surface = raised ? ESchemeColourSurface::MENU_BUTTON_RAISED : ESchemeColourSurface::MENU_BUTTON_PRESSED;
		}

		RGBAb colour = panel.GetColour(surface);
		g.DrawRect(panel.AbsRect(), colour);

		ESchemeColourSurface topLeftEdge;
		ESchemeColourSurface bottomRightEdge;

		topLeftEdge = raised ? ESchemeColourSurface::MENU_BUTTON_EDGE_TOP_LEFT : ESchemeColourSurface::MENU_BUTTON_EDGE_TOP_LEFT_PRESSED;
		bottomRightEdge = raised ? ESchemeColourSurface::MENU_BUTTON_EDGE_BOTTOM_RIGHT : ESchemeColourSurface::MENU_BUTTON_EDGE_BOTTOM_RIGHT_PRESSED;

		RGBAb colour1 = panel.GetColour(topLeftEdge);
		RGBAb colour2 = panel.GetColour(bottomRightEdge);

		g.DrawRectEdge(panel.AbsRect(), colour1, colour2);
	}

	ROCOCO_GUI_RETAINED_API void DrawButtonText(IGRPanel& panel, GRAlignmentFlags alignment, Vec2i spacing, const fstring& text, RGBAb colour, IGRRenderContext& g)
	{
		if (text.length == 0) return;

		GuiRect clipRect = panel.AbsRect();
		clipRect.left += spacing.x;
		clipRect.right -= spacing.x;
		clipRect.top += spacing.y;
		clipRect.bottom += spacing.y;

		if (clipRect.left > clipRect.right)
		{
			std::swap(clipRect.left, clipRect.right);
		}

		if (clipRect.bottom < clipRect.top)
		{
			std::swap(clipRect.top, clipRect.bottom);
		}

		g.DrawText(GRFontId::MENU_FONT, clipRect, alignment, spacing, text, colour);
	}
}