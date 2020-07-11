#include "rococo.mplat.h"
#include <rococo.file.browser.h>
#include <rococo.strings.h>
#include <rococo.maths.h>
#include <rococo.textures.h>

using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::Browser;

struct FileBrowserStyle: public IFileBrowserStyle
{
	int32 RowHeight(BrowserComponent component) const override
	{
		return 34;
	}

	int32 HorizontalSpan(BrowserComponent component) const override
	{
		return 23;
	}

	GuiRect BorderDeltas(BrowserComponent component) const
	{
		return GuiRect{ 1, 1, 1, 1 };
	}
};

GuiRectf ToF(const GuiRect& g)
{
	return GuiRectf{ (float) g.left, (float)g.top, (float)g.right, (float)g.bottom };
}

struct FileBrowserRC : public IFileBrowserRenderContext
{
	IGuiRenderContext& gc;
	GuiRect bounds;

	FileBrowserRC(IGuiRenderContext& _gc, const GuiRect& _bounds) : 
		gc(_gc), bounds(_bounds)
	{
	}

	void DrawArrowButton(Vec2 direction, const GuiRect& rect)
	{
		RGBAb edgeBL(192, 192, 192, 224);
		RGBAb edgeTR(224, 224, 224, 224);
		RGBAb arrow(255, 255, 255, 224);
		RGBAb button(165, 165, 165, 224);

		GuiMetrics metrics;
		gc.Renderer().GetGuiMetrics(metrics);

		if (IsPointInRect(metrics.cursorPosition, rect))
		{
			edgeBL.alpha = 255;
			edgeTR.alpha = 255;
			arrow.alpha = 255;
			button.alpha = 255;
		}

		Graphics::DrawRectangle(gc, rect, button, button);

		if (direction.y > 0)
		{
			Graphics::DrawTriangleFacingUp(gc, rect, arrow);
		}
		else if (direction.y < 0)
		{
			Graphics::DrawTriangleFacingDown(gc, rect, arrow);
		}
		else if (direction.x > 0)
		{
			Graphics::DrawTriangleFacingRight(gc, rect, arrow);
		}
		else if (direction.x < 0)
		{
			Graphics::DrawTriangleFacingLeft(gc, rect, arrow);
		}
		else
		{
			Throw(0, "DrawArrowButton: Direction vector must have at least one non-zero component");
		}

		Graphics::DrawBorderAround(gc, rect, { 1,1 }, edgeBL, edgeTR);
	}

	void DrawAsciiText(const GuiRect& rect, BrowserComponent component, cstr buffer) override
	{
		GuiMetrics metrics;
		gc.Renderer().GetGuiMetrics(metrics);

		RGBAb textColour = IsPointInRect(metrics.cursorPosition, rect) ? RGBAb(255, 255, 255, 255) : RGBAb(200, 200, 200, 255);
		Graphics::DrawText(gc, ToF(rect), Graphics::Alignment_Left, to_fstring(buffer), 1, textColour);
	}

	void DrawU16Text(const GuiRect& rect, BrowserComponent component, const wchar_t* buffer)
	{
		char asciitext[1024];
		SafeFormat(asciitext, 1024, "%S", buffer);
		DrawAsciiText(rect, component, asciitext);
	}

	GuiRect GetContainerRect() const
	{
		return bounds;
	}

	void DrawBackground(const GuiRect& rect, BrowserComponent component)
	{
		RGBAb fillCol1, fillCol2;
		RGBAb edgeCol1, edgeCol2;

		switch (component)
		{
		case BrowserComponent::TREE_FOLDER_ENTRY:
			fillCol1 = fillCol2 = RGBAb(192, 192, 0, 224);
			edgeCol1 = edgeCol2 = RGBAb(224, 224, 224, 224);
			break;
		case BrowserComponent::FILE_SCROLLER_SLIDER_BACK:
			fillCol1 = fillCol2 = RGBAb(0, 0, 0, 224);
			edgeCol1 = edgeCol2 = RGBAb(224, 224, 224, 224);
			break;
		case BrowserComponent::FILE_SCROLLER_SLIDER:
			fillCol1 = fillCol2 = RGBAb(128, 128, 128, 224);
			edgeCol1 = edgeCol2 = RGBAb(224, 224, 224, 224);
			break;
		default:
			fillCol1 = fillCol2 = RGBAb(0, 0, 32, 224);
			edgeCol1 = edgeCol2 = RGBAb(224, 224, 224, 224);
			break;
		}

		GuiMetrics metrics;
		gc.Renderer().GetGuiMetrics(metrics);

		if (IsPointInRect(metrics.cursorPosition, rect))
		{
			fillCol1.alpha = fillCol2.alpha = edgeCol1.alpha = edgeCol2.alpha = 255;
		}

		Graphics::DrawRectangle(gc, rect, fillCol1, fillCol2);
		Graphics::DrawBorderAround(gc, rect, Vec2i{ 1,1 }, edgeCol1, edgeCol2);
	}

	void DrawIcon(const GuiRect& rect, BrowserComponent component)
	{
		cstr file;
		switch (component)
		{
		case BrowserComponent::FOLDER_ICON:
			file = "!textures/toolbars/folder.tiff";
			break;
		default:
			file = nullptr;
		}

		if (!file) return;

		Textures::BitmapLocation loc;
		if (gc.Renderer().SpriteBuilder().TryGetBitmapLocation(file, loc))
		{
			Graphics::DrawSprite(TopLeft(rect), loc, gc);
		}
	}

	void DrawBorder(const GuiRect& rect, BrowserComponent component)
	{
		RGBAb br(192, 192, 192, 0);
		RGBAb tl(224, 224, 224, 0);

		GuiMetrics metrics;
		gc.Renderer().GetGuiMetrics(metrics);

		if (IsPointInRect(metrics.cursorPosition, rect))
		{
			br.alpha = tl.alpha = 255;
		}

		Graphics::DrawBorderAround(gc, rect, { 1,1 }, br, tl);
	}

	void SetClipRect(const GuiRect& rect) override
	{
		gc.FlushLayer();
		gc.SetScissorRect(ToF(rect));
	}
};

struct MPlatFileBrowser: public IMPlatFileBrowser, public IObserver, public IUIElement
{
	AutoFree<IDirectoryPopulator> pingPopulator;
	FileBrowserStyle style;
	FileBrowsingAPI api;

	AutoFree<IFileBrowser> browser;
	IPublisher& publisher;

	MPlatFileBrowser(IPublisher& _publisher, IInstallation& _installation):
		pingPopulator(CreatePingPopulator(_installation)),
		api { *pingPopulator, style }, 
		publisher(_publisher)
	{
		browser = CreateFileBrowser(api);
	}

	~MPlatFileBrowser()
	{
		publisher.Unsubscribe(this);
	}

	void OnEvent(Event& ev) override
	{
		if (ev == evUIPopulate)
		{
			UIPopulate& pop = As<UIPopulate>(ev);
			if (Eq(pop.name, "File.Browser"))
			{
				pop.renderElement = this;
			}
		}
	}

	void Engage() override
	{
		publisher.Subscribe(this, evUIPopulate);
	}

	void Free() override
	{
		delete this;
	}

	bool OnKeyboardEvent(const KeyboardEvent& key) override
	{
		return false;
	}

	void OnRawMouseEvent(const MouseEvent& ev)  override
	{
	}

	void OnMouseMove(Vec2i cursorPos, Vec2i delta, int dWheel)  override
	{
		if (dWheel != 0)
		{
			browser->WheelAt(cursorPos, dWheel);
		}
	}

	void OnMouseLClick(Vec2i cursorPos, bool clickedDown)  override
	{
		browser->ClickAt(cursorPos, clickedDown);
	}

	void OnMouseRClick(Vec2i cursorPos, bool clickedDown)  override
	{
		if (!clickedDown) browser->RaiseContextAt(cursorPos);
	}

	void Render(IGuiRenderContext& gc, const GuiRect& absRect)  override
	{
		FileBrowserRC rc(gc, absRect);
		browser->Render(rc);
	}
};


namespace Rococo
{
	IMPlatFileBrowser* CreateMPlatFileBrowser(Events::IPublisher& publisher, IInstallation& installation)
	{
		return new MPlatFileBrowser(publisher, installation);
	}
}