#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <rococo.strings.h>
#include <rococo.ui.h>
#include <rococo.vkeys.h>

using namespace Rococo;
using namespace Rococo::Gui;

namespace GRANON
{
	struct GRButton : IGRWidgetButton, IGRWidgetSupervisor, IGRWidgetLayout
	{
		IGRPanel& panel;
		EGRClickCriterion clickCriterion = EGRClickCriterion::OnDown;
		EGREventPolicy eventPolicy = EGREventPolicy::PublicEvent;

		bool isRaised = true;
		bool isMenu = false;
		bool forSubmenu = false;
		bool isEventHandlerCPPOnly = false;

		Strings::HString raisedImagePath;
		Strings::HString pressedImagePath;
		IGRImage* raisedImage = nullptr;
		IGRImage* pressedImage = nullptr;

		bool isStretched = false;

		bool triggersOnKeyUp = true;

		GRButton(IGRPanel& owningPanel) : panel(owningPanel)
		{
			alignment.Add(EGRAlignment::HCentre).Add(EGRAlignment::VCentre);
			panel.Add(EGRPanelFlags::AcceptsFocus);
			SyncMinimalSpan();
		}

		virtual ~GRButton()
		{

		}

		void TriggerOnKeyDown() override
		{
			triggersOnKeyUp = false;
		}

		void LayoutBeforeFit() override
		{
			SyncMinimalSpan();
		}

		void LayoutBeforeExpand() override
		{

		}

		void LayoutAfterExpand() override
		{

		}

		void Free() override
		{
			delete this;
		}

		GRButtonFlags ButtonFlags() const override
		{
			GRButtonFlags flags;
			flags.forSubMenu = forSubmenu;
			flags.isEnabled = true;
			flags.isMenu = isMenu;
			flags.isRaised = isRaised;
			return flags;
		}

		void FireEvent(Vec2i clickPosition)
		{
			if (eventPolicy == EGREventPolicy::PublicEvent)
			{		
				// We cannot copy the meta data string, because it may be invalidated by the time the consumer comes to read it
				GRWidgetEvent asyncWidgetEvent{ EGRWidgetEventType::BUTTON_CLICK, panel.Id(), iMetadata, "", clickPosition, isEventHandlerCPPOnly };
				RouteEventToHandler(panel, asyncWidgetEvent);
			}
			else if (eventPolicy == EGREventPolicy::NotifyAncestors)
			{
				GRWidgetEvent widgetEvent{ EGRWidgetEventType::BUTTON_CLICK, panel.Id(), iMetadata, sMetaData.c_str(), clickPosition, isEventHandlerCPPOnly };

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
			panel.Focus();

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
					FireEvent(ce.position);
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
					FireEvent(ce.position);
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
						FireEvent(ce.position);
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
						FireEvent(ce.position);
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

		bool keyboardPrepped = false;

		EGREventRouting OnKeyEvent(GRKeyEvent& key) override
		{
			if (!triggersOnKeyUp)
			{
				keyboardPrepped = true;
			}

			if (triggersOnKeyUp)
			{
				if (!key.osKeyEvent.IsUp())
				{
					switch (key.osKeyEvent.VKey)
					{
					case IO::VirtualKeys::VKCode_ENTER:
						keyboardPrepped = true;
						return EGREventRouting::Terminate;
					}
				}
				else
				{
					switch (key.osKeyEvent.VKey)
					{
					case IO::VirtualKeys::VKCode_ENTER:
						if (keyboardPrepped)
						{
							keyboardPrepped = false;

							SyncMinimalSpan();

							if (panel.Root().CapturedPanelId() == panel.Id())
							{
								panel.Root().ReleaseCursor();
							}

							FireEvent(Centre(panel.AbsRect()));
						}
						return EGREventRouting::Terminate;
					}
				}
			}
			else if (!key.osKeyEvent.IsUp())
			{
				switch (key.osKeyEvent.VKey)
				{
				case IO::VirtualKeys::VKCode_ENTER:
					SyncMinimalSpan();

					if (panel.Root().CapturedPanelId() == panel.Id())
					{
						panel.Root().ReleaseCursor();
					}

					FireEvent(Centre(panel.AbsRect()));
					return EGREventRouting::Terminate;
				}
			}

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
				GuiRect buttonRect = Expand(panel.AbsRect(), -2);
				GRRenderState rs(false, false, false);
				RGBAb colour = panel.GetColour(EGRSchemeColourSurface::MENU_BUTTON, rs);
				g.DrawRect(panel.AbsRect(), colour);
				DrawMenuButton(panel, buttonRect, false, isRaised, g);
			}
			else
			{
				DrawButton(panel, false, isRaised, g, backSurface);
			}

			bool isHovered = g.IsHovered(panel);

			bool imageRendered = false;

			IGRImage* image = isRaised ? raisedImage : pressedImage;

			GRRenderState rs(!isRaised, isHovered, false);

			if (image)
			{
				imageRendered = image->Render(panel, alignment, spacing, isStretched, g);

				GuiRect fogRect = panel.AbsRect();
				fogRect.left += 1;
				fogRect.right -= 1;
				fogRect.top += 1;
				fogRect.bottom -= 1;
				g.DrawRect(fogRect, panel.GetColour(EGRSchemeColourSurface::BUTTON_IMAGE_FOG, rs, RGBAb(0, 0, 0, 128)));
			}

			if (!imageRendered)
			{
				if (substituteBetterFontAccordingly)
				{
					alignment.Add(EGRAlignment::AutoFonts);
				}
				else
				{
					alignment.Remove(EGRAlignment::AutoFonts);
				}

				RGBAb shadowColour = isMenu ? RGBAb(0,0,0,0) : panel.GetColour(EGRSchemeColourSurface::BUTTON_SHADOW, rs);
				RGBAb colour = panel.GetColour(isMenu ? EGRSchemeColourSurface::MENU_BUTTON_TEXT : textSurface, rs);
				DrawButtonText(panel, alignment, spacing, title.to_fstring(), colour, shadowColour, fontId, g);
			}
		}

		GRAlignmentFlags alignment;
		Vec2i spacing { 0,0 };

		IGRWidgetButton& SetAlignment(GRAlignmentFlags alignment, Vec2i spacing) override
		{
			this->alignment = alignment;
			this->spacing = spacing;
			return *this;
		}

		IGRWidgetButton& SetImagePath(cstr imagePath) override
		{
			this->raisedImagePath = imagePath ? imagePath : "";
			this->pressedImagePath = imagePath ? imagePath : "";
			raisedImage = panel.Root().Custodian().CreateImageFromPath("raised button", this->raisedImagePath.c_str());
			pressedImage = panel.Root().Custodian().CreateImageFromPath("pressed button", this->pressedImagePath.c_str());
			SyncMinimalSpan();
			return *this;
		}

		IGRWidgetButton& SetPressedImagePath(cstr imagePath) override
		{
			this->pressedImagePath = imagePath ? imagePath : "";
			pressedImage = panel.Root().Custodian().CreateImageFromPath("pressed button", this->pressedImagePath.c_str());
			SyncMinimalSpan();
			return *this;
		}

		IGRWidgetButton& SetRaisedImagePath(cstr imagePath) override
		{
			this->raisedImagePath = imagePath ? imagePath : "";
			raisedImage = panel.Root().Custodian().CreateImageFromPath("raised button", this->raisedImagePath.c_str());
			SyncMinimalSpan();
			return *this;
		}

		Vec2i ImageSpan() const override
		{
			auto* image = pressedImage ? pressedImage : raisedImage;
			return image ? image->Span() : Vec2i{ 0,0 };
		}

		EGRSchemeColourSurface backSurface = EGRSchemeColourSurface::BUTTON;

		IGRWidgetButton& SetBackSurface(EGRSchemeColourSurface backSurface) override
		{
			this->backSurface = backSurface;
			return *this;
		}

		EGRSchemeColourSurface textSurface = EGRSchemeColourSurface::BUTTON_TEXT;

		IGRWidgetButton& SetTextSurface(EGRSchemeColourSurface textSurface) override
		{
			this->textSurface = textSurface;
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
		Strings::HString sMetaData;

		IGRWidgetButton& SetMetaData(const GRControlMetaData& metaData, bool isCppOnly) override
		{
			iMetadata = metaData.intData;
			sMetaData = metaData.stringData ? metaData.stringData : "";
			isEventHandlerCPPOnly = isCppOnly;
			return *this;
		}

		GRControlMetaData MetaData() override
		{
			return GRControlMetaData { iMetadata, sMetaData.c_str() };
		}

		EGREventRouting OnChildEvent(GRWidgetEvent&, IGRWidget&) override
		{
			return EGREventRouting::NextHandler;
		}

		Strings::HString title;
		GRFontId fontId = GRFontId::MENU_FONT;

		IGRWidgetButton& SetTitle(cstr title) override
		{
			isDirty = true;
			this->title = title == nullptr ? "" : title;
			panel.SetDesc(title);
			SyncMinimalSpan();
			return *this;
		}

		bool substituteBetterFontAccordingly = false;

		IGRWidgetButton& SetPressedNoCallback(bool pressed) override
		{
			isRaised = !pressed;
			return *this;
		}

		IGRWidgetButton& SetFontId(GRFontId id, bool substituteBetterFontAccordingly) override
		{
			isDirty = true;
			fontId = id;
			this->substituteBetterFontAccordingly = substituteBetterFontAccordingly;
			return *this;
		}

		bool expandToFitTextX = false;
		bool expandToFitTextY = false;
		bool isDirty = true;

		void FitTextHorizontally() override
		{
			expandToFitTextX = true;
			isDirty = true;
			SyncMinimalSpan();
		}

		void FitTextVertically() override
		{
			expandToFitTextY = true;
			isDirty = true;
			SyncMinimalSpan();
		}

		size_t GetTitle(char* titleBuffer, size_t nBytes) const override
		{
			if (titleBuffer == nullptr || nBytes == 0)
			{
				return title.length();
			}

			Strings::CopyString(titleBuffer, nBytes, title, title.length());
			return title.length();
		}

		Vec2i EvaluateMinimalSpan() const
		{
			Vec2i extraSpan;
			extraSpan.x = panel.Padding().left + panel.Padding().right;
			extraSpan.y = panel.Padding().top + panel.Padding().bottom;

			const IGRImage* image = isRaised ? raisedImage : pressedImage;
			if (image)
			{
				return image->Span() + extraSpan;
			}

			if (title.length() == 0)
			{
				return Vec2i { 8, 8 } + extraSpan;
			}

			return panel.Root().Custodian().EvaluateMinimalSpan(fontId, fstring{ title.c_str(), (int32) title.length() }) + extraSpan;
		}

		void SyncMinimalSpan()
		{
			Vec2i minimalSpan = EvaluateMinimalSpan();
			panel.SetMinimalSpan(minimalSpan);

			if (expandToFitTextX)
			{
				panel.SetConstantWidth(minimalSpan.x);
			}

			if (expandToFitTextY)
			{
				panel.SetConstantHeight(minimalSpan.y);
			}
		}

		bool isToggler = false;

		void MakeToggleButton() override
		{
			isToggler = true;
		}

		void SetStretchImage(bool isStretched) override
		{
			this->isStretched = isStretched;
			isDirty = true;
		}

		void Toggle() override
		{
			if (!isToggler)
			{
				RaiseError(panel, EGRErrorCode::InvalidArg, __FUNCTION__, "The button is not a toggler");
			}

			isRaised = !isRaised;

			FireEvent(Centre(panel.AbsRect()));
		}

		EGRQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			auto result = QueryForParticularInterface<IGRWidgetButton, GRButton>(this, ppOutputArg, interfaceId);
			if (result == EGRQueryInterfaceResult::SUCCESS)
			{
				return EGRQueryInterfaceResult::SUCCESS;
			}

			return QueryForParticularInterface<IGRWidgetLayout, GRButton>(this, ppOutputArg, interfaceId);
		}

		IGRWidget& Widget()
		{
			return *this;
		}

		cstr GetImplementationTypeName() const override
		{
			return "GRButton";
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

	ROCOCO_GUI_RETAINED_API void DrawButton(IGRPanel& panel, bool focused, bool raised, IGRRenderContext& g, EGRSchemeColourSurface backSurface)
	{
		DrawPanelBackgroundEx(panel, g, backSurface, EGRSchemeColourSurface::BUTTON_EDGE_TOP_LEFT, EGRSchemeColourSurface::BUTTON_EDGE_BOTTOM_RIGHT, 1.0f, raised, focused);
	}

	ROCOCO_GUI_RETAINED_API void DrawMenuButton(IGRPanel& panel, const GuiRect& rect, bool focused, bool raised, IGRRenderContext& g)
	{
		UNUSED(focused);

		bool hovered = g.IsHovered(panel);
		
		GRRenderState rs(!raised, hovered, false);
		RGBAb colour = panel.GetColour(EGRSchemeColourSurface::MENU_BUTTON, rs);
		g.DrawRect(rect, colour);

		RGBAb colour1 = panel.GetColour(EGRSchemeColourSurface::MENU_BUTTON_EDGE_TOP_LEFT, rs);
		RGBAb colour2 = panel.GetColour(EGRSchemeColourSurface::MENU_BUTTON_EDGE_BOTTOM_RIGHT, rs);
		g.DrawRectEdge(rect, colour1, colour2);
	}

	ROCOCO_GUI_RETAINED_API void DrawButtonText(IGRPanel& panel, GRAlignmentFlags alignment, Vec2i spacing, const fstring& text, RGBAb colour, RGBAb shadowColour, GRFontId fontId, IGRRenderContext& g, Vec2i shadowOffset)
	{
		if (text.length == 0) return;

		GuiRect targetRect = panel.AbsRect();

		if (targetRect.left > targetRect.right)
		{
			swap_args(targetRect.left, targetRect.right);
		}

		if (targetRect.bottom < targetRect.top)
		{
			swap_args(targetRect.top, targetRect.bottom);
		}

		if (shadowColour.alpha)
		{
			GuiRect shadowRect = targetRect;
			shadowRect.left += shadowOffset.x;
			shadowRect.right += shadowOffset.x;
			shadowRect.top += shadowOffset.y;
			shadowRect.bottom += shadowOffset.y;
			g.DrawText(fontId, shadowRect, alignment, spacing, text, shadowColour);
		}

		g.DrawText(fontId, targetRect, alignment, spacing, text, colour);
	}
}