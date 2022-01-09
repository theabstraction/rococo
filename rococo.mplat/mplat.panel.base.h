#pragma once

#include <rococo.mplat.h>
#include <vector>
#include <string>
#include <rococo.fonts.h>
#include <rococo.textures.h>
#include <rococo.strings.h>

// TODO: this breaks the Rococo convention of no STL headers or classes in Rococo headers. I regret 
// writing this code, it really needs  to be replaced and serves as An example of why deriving classes
// from concrete bases is bad.

namespace Rococo
{
	using namespace Rococo::Events;

	namespace MPlatImpl
	{
		class BasePane : virtual public IPaneSupervisor
		{
			GuiRect rect{ 0, 0, 0, 0 };
			bool isVisible{ true };

			IPaneSupervisor* parent = nullptr;
			std::vector<IPaneSupervisor*> children;

			struct UICommand
			{
				std::string command;
				boolean32 defer;
			};

			std::vector<UICommand> commands;

			struct UIPopulator
			{
				std::string name;
			};

			std::vector<UIPopulator> populators;

			ColourScheme scheme
			{
			   RGBAb(160,160,160, 192),
			   RGBAb(192,192,192, 192),
			   RGBAb(255,255,255, 224),
			   RGBAb(224,224,224, 224),
			   RGBAb(255,255,255, 255),
			   RGBAb(160,160,160, 224),
			   RGBAb(192,192,192, 224),
			   RGBAb(255,255,255, 255),
			   RGBAb(224,224,224, 255),
			   RGBAb(255,255,255, 255),
			};

			HString bkImageName;
			Textures::BitmapLocation bkBitmap = { {0,0,0,0},0,{0,0} };
		public:

			void SetParent(IPaneSupervisor* panel);
			IPaneSupervisor* Parent();

			void SetCommand(int32 stateIndex, boolean32 deferAction, const fstring& text) override;
			void Invoke(IPublisher& publisher, int32 stateIndex);
			void SetPopulator(int32 stateIndex, const fstring& populatorName);
			void AppendEventToChildren(IPublisher& publisher, const MouseEvent& me, const Vec2i& absTopLeft, int stateIndex = 0);
			bool AppendEventToChildren(IPublisher& publisher, const KeyboardEvent& ke, const Vec2i& focusPoint, const Vec2i& absTopLeft, int stateIndex = 0);
			void AlignLeftEdges(int32 borderPixels, boolean32 preserveSpan) override;
			void AlignRightEdges(int32 borderPixels, boolean32 preserveSpan) override;
			void AlignTopEdges(int32 borderPixels, boolean32 preserveSpan) override;
			void AlignBottomEdges(int32 borderPixels, boolean32 preserveSpan) override;
			void LayoutVertically(int32 vertBorder, int32 vertSpacing) override;
			void SetRect(const GuiRect& rect) override;
			const GuiRect& ClientRect() const override;
			void GetRect(GuiRect& outRect) override;
			boolean32 IsVisible() override;
			boolean32 IsNormalized() override;
			void SetVisible(boolean32 visible) override;
			IPaneSupervisor* operator[](int index) override;
			IPaneSupervisor* GetChild(int index);
			int Children() const override;
			void AddChild(IPaneSupervisor* child) override;
			void RemoveChild(IPaneSupervisor* child) override;
			void FreeAllChildren() override;
			void SetBkImage(const fstring& pingPath) override;
			void SetScheme(const ColourScheme& scheme) override;
			const ColourScheme& Scheme() const;
			void SetColourBk1(RGBAb normal, RGBAb hilight) override;
			void SetColourBk2(RGBAb normal, RGBAb hilight) override;
			void SetColourEdge1(RGBAb normal, RGBAb hilight) override;
			void SetColourEdge2(RGBAb normal, RGBAb hilight) override;
			void SetColourFont(RGBAb normal, RGBAb hilight) override;
			void Populate(IPublisher& publisher, IGuiRenderContext& grc, int32 stateIndex, const Vec2i& topLeft);
			void RenderBkImage(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality);
			void RenderBackground(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality);
			void RenderChildren(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality);
		}; // class BasePane

		struct PaneDelegate : public IPaneSupervisor
		{
			IPaneBuilderSupervisor* current;

			void Free() override
			{
				delete this;
			}

			bool AppendEvent(const KeyboardEvent& me, const Vec2i& focusPoint, const Vec2i& absTopLeft) override
			{
				return current->Supervisor()->AppendEvent(me, focusPoint, absTopLeft);
			}

			void AppendEvent(const MouseEvent& me, const Vec2i& absTopLeft) override
			{
				current->Supervisor()->AppendEvent(me, absTopLeft);
			}

			const GuiRect& ClientRect() const override
			{
				return current->Supervisor()->ClientRect();
			}

			void SetParent(IPaneSupervisor* parent) override
			{
				return current->Supervisor()->SetParent(parent);
			}

			void SetScheme(const ColourScheme& scheme) override
			{
				current->Supervisor()->SetScheme(scheme);
			}

			const ColourScheme& Scheme() const override
			{
				return current->Supervisor()->Scheme();
			}

			IPaneSupervisor* operator[](int index) override
			{
				return (*current->Supervisor())[index];
			}

			int Children() const override
			{
				return current->Supervisor()->Children();
			}

			void AddChild(IPaneSupervisor* child) override
			{
				return current->Supervisor()->AddChild(child);
			}

			void RemoveChild(IPaneSupervisor* child) override
			{
				current->Supervisor()->RemoveChild(child);
			}

			void FreeAllChildren() override
			{
				current->Supervisor()->FreeAllChildren();
			}

			void SetColourBk1(RGBAb normal, RGBAb hilight) override
			{
				return current->Supervisor()->SetColourBk1(normal, hilight);
			}

			void SetColourBk2(RGBAb normal, RGBAb hilight) override
			{
				return current->Supervisor()->SetColourBk2(normal, hilight);
			}

			void SetColourEdge1(RGBAb normal, RGBAb hilight) override
			{
				return  current->Supervisor()->SetColourEdge1(normal, hilight);
			}

			void SetColourEdge2(RGBAb normal, RGBAb hilight) override
			{
				return  current->Supervisor()->SetColourEdge2(normal, hilight);
			}

			void SetColourFont(RGBAb normal, RGBAb hilight) override
			{
				return  current->Supervisor()->SetColourFont(normal, hilight);
			}

			boolean32/* isVisible */ IsVisible() override
			{
				return  current->Supervisor()->IsVisible();
			}

			boolean32/* isNormalized */ IsNormalized() override
			{
				return  current->Supervisor()->IsNormalized();
			}

			void SetVisible(boolean32 isVisible) override
			{
				return  current->Supervisor()->SetVisible(isVisible);
			}

			void GetRect(GuiRect& rect) override
			{
				return  current->Supervisor()->GetRect(rect);
			}

			void SetRect(const GuiRect& rect) override
			{
				return  current->Supervisor()->SetRect(rect);
			}

			void AlignLeftEdges(int32 border, boolean32 preserveSpan) override
			{
				return  current->Supervisor()->AlignLeftEdges(border, preserveSpan);
			}

			void AlignRightEdges(int32 x, boolean32 preserveSpan) override
			{
				return  current->Supervisor()->AlignRightEdges(x, preserveSpan);
			}

			void AlignTopEdges(int32 border, boolean32 preserveSpan) override
			{
				return  current->Supervisor()->AlignLeftEdges(border, preserveSpan);
			}

			void AlignBottomEdges(int32 border, boolean32 preserveSpan) override
			{
				return  current->Supervisor()->AlignRightEdges(border, preserveSpan);
			}

			void LayoutVertically(int32 vertBorder, int32 vertSpacing) override
			{
				return  current->Supervisor()->LayoutVertically(vertBorder, vertSpacing);
			}

			void SetCommand(int32 stateIndex, boolean32 deferAction, const fstring& text) override
			{
				return  current->Supervisor()->SetCommand(stateIndex, deferAction, text);
			}

			void SetPopulator(int32 stateIndex, const fstring& populatorName) override
			{
				return  current->Supervisor()->SetPopulator(stateIndex, populatorName);
			}

			void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) override
			{
				return current->Render(grc, topLeft, modality);
			}

			IPaneContainer* Root()
			{
				return current->Root();
			}
		};

		IPaneBuilderSupervisor* CreateDebuggingOverlay(Platform& platfore);
		Rococo::IArrayBox* AddArrayBox(Platform& platform, BasePane& panel, int32 fontIndex, const fstring& populatorEventKey, const GuiRect& rect);
		ITextOutputPane* AddTextOutput(IPublisher& publisher, BasePane& panel, int32 fontIndex, const fstring& eventKey, const GuiRect& rect);
		Rococo::IScroller* AddScroller(IPublisher& publisher, BasePane& panel, const fstring& key, const GuiRect& rect, boolean32 isVertical);
		Rococo::IContextMenuPane* AddContextMenuPane(IPublisher& publisher, IKeyboardSupervisor& keyboard, BasePane& panel, const fstring& key, const GuiRect& rect, IContextMenuSupervisor& cm);
		Rococo::ILabelPane* AddLabel(IPublisher& publisher, BasePane& panel, int32 fontIndex, const fstring& text, const GuiRect& rect);
		Rococo::IEnumListPane* AddEnumList(Platform& platform, BasePane& panel, int32 fontIndex, const fstring& populatorId, const GuiRect& rect);
		Rococo::ISlider* AddSlider(IPublisher& publisher, IRenderer& renderer, BasePane& panel, int32 fontIndex, const fstring& text, const GuiRect& rect, float minValue, float maxValue);
		IScrollbar* CreateScrollbar(bool _isVertical);
		void RenderLabel(IGuiRenderContext& grc, cstr text, const GuiRect& absRect, int horzAlign, int vertAlign, Vec2i padding, int fontIndex, const ColourScheme& scheme, bool enableHighlights);
		Rococo::IRadioButton* AddRadioButton(IPublisher& publisher, BasePane& panel, int32 fontIndex, const fstring& text, const fstring& key, const fstring& value, const GuiRect& rect);
		Rococo::ITabContainer* AddTabContainer(IPublisher& publisher, IKeyboardSupervisor& keyboard, BasePane& pane, int32 tabHeight, int32 fontIndex, const GuiRect& rect);
	} // MPlatImpl
} // Rococo
