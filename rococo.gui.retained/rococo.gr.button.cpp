#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <string>

using namespace Rococo;
using namespace Rococo::Gui;

namespace GRANON
{
	struct GRButton : IGRWidgetButton, IGRWidget
	{
		IGRPanel& panel;
		GRClickCriterion clickCriterion = GRClickCriterion::OnDown;
		GREventPolicy eventPolicy = GREventPolicy::PublicEvent;

		bool isRaised = true;
		bool isMenu = false;
		bool forSubmenu = false;

		std::string raisedImagePath;
		std::string pressedImagePath;
		AutoFree<IGRImageMemento> raisedImage;
		AutoFree<IGRImageMemento> pressedImage;

		GRButton(IGRPanel& owningPanel) : panel(owningPanel)
		{
			alignment.Add(GRAlignment::HCentre).Add(GRAlignment::VCentre);
			SyncMinimalSpan();
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
				RouteEventToHandler(panel, widgetEvent);
			}
			else if (eventPolicy == GREventPolicy::NotifyAncestors)
			{
				EventRouting routing = panel.NotifyAncestors(widgetEvent, *this);
				if (routing != EventRouting::Terminate)
				{
					// Nothing handled it
					RouteEventToHandler(panel, widgetEvent);
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

					SyncMinimalSpan();
					FireEvent(ce);
				}
				else if (clickCriterion == GRClickCriterion::OnDownThenUp)
				{
					if (!isToggler)
					{
						isRaised = false;
					}
					SyncMinimalSpan();
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
					SyncMinimalSpan();
					FireEvent(ce);
				}
				else if (clickCriterion == GRClickCriterion::OnDownThenUp)
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
				else if (clickCriterion == GRClickCriterion::OnDown)
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

				return EventRouting::Terminate;
			}

			return EventRouting::NextHandler;
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
				g.DrawRect(fogRect, panel.GetColour(ESchemeColourSurface::BUTTON_IMAGE_FOG, rs, RGBAb(0, 0, 0, 128)));
			}

			if (!imageRendered)
			{
				RGBAb colour = panel.GetColour(ESchemeColourSurface::BUTTON_TEXT, rs);
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

		EQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			if (!interfaceId || *interfaceId == 0) return EQueryInterfaceResult::INVALID_ID;
			if (DoInterfaceNamesMatch(interfaceId, "IGRWidgetButton"))
			{
				if (ppOutputArg) *ppOutputArg = static_cast<IGRWidgetButton*>(this);
				return EQueryInterfaceResult::SUCCESS;
			}

			return EQueryInterfaceResult::NOT_IMPLEMENTED;
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
		bool hovered = g.IsHovered(panel);

		GRRenderState rs(!raised, hovered, false);

		RGBAb colour = panel.GetColour(ESchemeColourSurface::BUTTON, rs);
		g.DrawRect(panel.AbsRect(), colour);

		RGBAb colour1 = panel.GetColour(ESchemeColourSurface::BUTTON_EDGE_TOP_LEFT, rs);
		RGBAb colour2 = panel.GetColour(ESchemeColourSurface::BUTTON_EDGE_BOTTOM_RIGHT, rs);

		g.DrawRectEdge(panel.AbsRect(), colour1, colour2);
	}

	ROCOCO_GUI_RETAINED_API void DrawMenuButton(IGRPanel& panel, bool focused, bool raised, IGRRenderContext& g)
	{
		bool hovered = g.IsHovered(panel);

		GRRenderState rs(!raised, hovered, false);
		RGBAb colour = panel.GetColour(ESchemeColourSurface::MENU_BUTTON, rs);
		g.DrawRect(panel.AbsRect(), colour);

		RGBAb colour1 = panel.GetColour(ESchemeColourSurface::MENU_BUTTON_EDGE_TOP_LEFT, rs);
		RGBAb colour2 = panel.GetColour(ESchemeColourSurface::MENU_BUTTON_EDGE_BOTTOM_RIGHT, rs);
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