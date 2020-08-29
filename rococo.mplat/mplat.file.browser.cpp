#include "rococo.mplat.h"
#include <rococo.file.browser.h>
#include <rococo.strings.h>
#include <rococo.maths.h>
#include <rococo.textures.h>
#include <rococo.ui.h>

using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::Browser;
using namespace Rococo::IO;

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

		RGBAb textColour;
		
		switch (component)
		{
		case BrowserComponent::STATUS_ERROR:
			textColour = IsPointInRect(metrics.cursorPosition, rect) ? RGBAb(255, 0, 0, 255) : RGBAb(200, 0, 0, 255);
			break;
		case BrowserComponent::LINE_EDITOR:
			textColour = IsPointInRect(metrics.cursorPosition, rect) ? RGBAb(255, 255, 255, 255) : RGBAb(240, 240, 240, 255);
			break;
		default:
			textColour = IsPointInRect(metrics.cursorPosition, rect) ? RGBAb(255, 255, 255, 255) : RGBAb(200, 200, 200, 255);
			break;
		}
		Graphics::DrawText(gc, ToF(rect), Graphics::Alignment_Left, to_fstring(buffer), 1, textColour);
	}

	void DrawAsciiTextWithCaret(int pos, const GuiRect& rect, cstr buffer) override
	{
		GuiMetrics metrics;
		gc.Renderer().GetGuiMetrics(metrics);

		auto& clip = ToF(rect);

		RGBAb textColour = IsPointInRect(metrics.cursorPosition, rect) ? RGBAb(255, 255, 255, 255) : RGBAb(240, 240, 240, 255);
		Graphics::DrawTextWithCaret(gc, ToF(rect), Graphics::Alignment_Left, to_fstring(buffer), 1, textColour, clip, pos);
	}

	void DrawU16Text(const GuiRect& rect, BrowserComponent component, const wchar_t* buffer)
	{
		char asciitext[1024];
		SafeFormat(asciitext, 1024, "%ls", buffer);
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
			fillCol1 = fillCol2 = RGBAb(192, 192, 0, 200);
			edgeCol1 = edgeCol2 = RGBAb(224, 224, 224, 200);
			break;
		case BrowserComponent::LINE_EDITOR:
			fillCol1 = fillCol2 = RGBAb(0, 0, 0, 200);
			edgeCol1 = edgeCol2 = RGBAb(224, 224, 224, 200);
			break;
		case BrowserComponent::FILE_SCROLLER_SLIDER_BACK:
			fillCol1 = fillCol2 = RGBAb(0, 0, 0, 200);
			edgeCol1 = edgeCol2 = RGBAb(224, 224, 224, 200);
			break;
		case BrowserComponent::FILE_SCROLLER_SLIDER:
			fillCol1 = fillCol2 = RGBAb(128, 128, 128, 200);
			edgeCol1 = edgeCol2 = RGBAb(224, 224, 224, 200);
			break;
		case BrowserComponent::STATUS_ERROR:
			fillCol1 = fillCol2 = RGBAb(255, 255, 255, 200);
			edgeCol1 = edgeCol2 = RGBAb(255, 255, 255, 200);
			break;
		default:
			fillCol1 = fillCol2 = RGBAb(0, 0, 32, 200);
			edgeCol1 = edgeCol2 = RGBAb(224, 224, 224, 200);
			break;
		}

		GuiMetrics metrics;
		gc.Renderer().GetGuiMetrics(metrics);

		if (IsPointInRect(metrics.cursorPosition, rect))
		{
			fillCol1.alpha = fillCol2.alpha = 224;
			edgeCol1.alpha = edgeCol2.alpha = 255;
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

struct StatusBar : IUIElement
{
	StatusBar()
	{

	}

	HString status;

	bool OnKeyboardEvent(const KeyboardEvent& key) override
	{
		return false;
	}

	void OnRawMouseEvent(const MouseEvent& ev) override
	{

	}

	void OnMouseMove(Vec2i cursorPos, Vec2i delta, int dWheel) override
	{

	}

	void OnMouseLClick(Vec2i cursorPos, bool clickedDown) override
	{

	}

	void OnMouseRClick(Vec2i cursorPos, bool clickedDown) override
	{

	}

	void Render(IGuiRenderContext& gc, const GuiRect& absRect) override
	{
		FileBrowserRC rc(gc, absRect);

		//	gc.SetScissorRect(ToF(absRect));

		GuiRect textRect = absRect;
		textRect.left += 4;
		textRect.right -= 4;
		textRect.top += 1;
		textRect.bottom -= 1;

		if (status.length() > 0)
		{
			rc.DrawBackground(absRect, BrowserComponent::STATUS_ERROR);
		}
		rc.DrawAsciiText(textRect, BrowserComponent::STATUS_ERROR, status);

		GuiRectf noScissor = { -1, -1, 1000000, 1000000 };
		//	gc.SetScissorRect(noScissor);
	}
};

struct FilenameEditor : IUIElement, public IKeyboardSink
{
	IFileBrowser& browser;
	IDirectoryPopulator& populator;
	IGUIStack& gui;
	IKeyboardSupervisor& keyboard;

	U32FilePath fullPath = { U"!", U'/' };
	U8FilePath asciiRep = { "!", '/' };

	bool editing = false;
	int caretPos = 0;

	FilenameEditor(IFileBrowser& _browser, IDirectoryPopulator& _populator, IGUIStack& _gui, IKeyboardSupervisor& _keyboard) :
		browser(_browser), populator(_populator), gui(_gui), keyboard(_keyboard)
	{

	}

	~FilenameEditor()
	{
		if (editing) gui.DetachKeyboardSink(this);
	}

	bool OnKeyboardEvent(const KeyboardEvent& key) override
	{
		if (editing)
		{
			if (key.VKey == IO::VKCode_ENTER && key.IsUp())
			{
				editing = false;
				gui.DetachKeyboardSink(this);
				return true;
			}
			else
			{
				keyboard.AppendKeyboardInputToEditBuffer(caretPos, asciiRep.buf, asciiRep.CAPACITY, key);
				return true;
			}
		}
		return false;
	}

	void OnRawMouseEvent(const MouseEvent& ev) override
	{

	}

	void OnMouseMove(Vec2i cursorPos, Vec2i delta, int dWheel) override
	{

	}

	void OnMouseLClick(Vec2i cursorPos, bool clickedDown) override
	{
		if (!clickedDown)
		{
			editing = !editing;

			if (editing)
			{
				gui.AttachKeyboardSink(this);
			}
			else
			{
				gui.DetachKeyboardSink(this);
			}
		}
	}

	void OnMouseRClick(Vec2i cursorPos, bool clickedDown) override
	{

	}

	void Render(IGuiRenderContext& gc, const GuiRect& absRect) override
	{
		FileBrowserRC rc(gc, absRect);

		GuiRect textRect = absRect;
		textRect.left += 4;
		textRect.right -= 4;
		textRect.top += 4;
		textRect.bottom -= 4;

		if (editing)
		{
			rc.DrawBackground(absRect, BrowserComponent::LINE_EDITOR);
			rc.DrawAsciiTextWithCaret(caretPos, textRect, asciiRep);
		}
		else
		{
			rc.DrawAsciiText(textRect, BrowserComponent::FILE_ENTRY, asciiRep);
		}
	}

	void Sync()
	{
		U32FilePath selectedFile;
		browser.GetSelectedFile(selectedFile);
		populator.GetFullPathToFile(selectedFile, fullPath);
		ToU8(fullPath, asciiRep);

		caretPos = StringLength(asciiRep);

		if (editing)
		{
			gui.DetachKeyboardSink(this);
			editing = false;
		}
	}
};

auto evGetCaption = "Panel.Browser.GetCaption"_event;

struct MPlatFileBrowser: public IMPlatFileBrowser, public IObserver, public IUIElement, public IBrowserFileChangeNotification
{
	AutoFree<IDirectoryPopulator> pingPopulator;
	FileBrowserStyle style;
	FileBrowsingAPI api;

	AutoFree<IBrowserRules> rules;
	AutoFree<IFileBrowser> browser;
	IPublisher& publisher;

	FilenameEditor filenameEditor;
	StatusBar statusBar;

	IBrowserFileChangeNotification& onChange;

	MPlatFileBrowser(IPublisher& _publisher, IInstallation& _installation, IGUIStack& gui, IKeyboardSupervisor& keyboard, IBrowserFileChangeNotification& _onChange):
		pingPopulator(CreatePingPopulator(_installation)),
		api { *pingPopulator, style }, 
		publisher(_publisher),
		browser(CreateFileBrowser(api, *this)),
		filenameEditor(*browser, *pingPopulator, gui, keyboard),
		onChange(_onChange)
	{
	}

	~MPlatFileBrowser()
	{
		publisher.Unsubscribe(this);
	}

	void OnFileSelect(const U32FilePath& path, bool doubleClick)
	{
		filenameEditor.Sync();

		if (doubleClick)
		{
			onChange.OnFileSelect(path, doubleClick);
		}
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
			else if (Eq(pop.name, "File.Browser.Filename"))
			{
				pop.renderElement = &filenameEditor;
			}
			else if (Eq(pop.name, "File.Browser.Status"))
			{
				pop.renderElement = &statusBar;
			}
		}
		else if (ev == evGetCaption)
		{
			auto& args = As<UIInvoke>(ev);
			rules->GetCaption(args.command, sizeof(args.command));
		}
	}

	bool Select() override
	{
		U32FilePath fullPath;
		PathFromAscii(filenameEditor.asciiRep, '/', fullPath);

		if (rules->Select(fullPath))
		{
			return true;
		}

		statusBar.status = rules->GetLastError();

		return false;
	}

	void Engage(IBrowserRulesFactory& factory) override
	{
		publisher.Unsubscribe(this);
		publisher.Subscribe(this, evUIPopulate);
		publisher.Subscribe(this, evGetCaption);

		rules = factory.CreateRules();

		U32FilePath rootPrefix;
		rules->GetRoot(rootPrefix);

		ToU8(rootPrefix, filenameEditor.asciiRep);
		filenameEditor.caretPos = StringLength(filenameEditor.asciiRep);

		pingPopulator->LimitRoot(rootPrefix);
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
	IMPlatFileBrowser* CreateMPlatFileBrowser(Events::IPublisher& publisher, IInstallation& installation, IGUIStack& gui, IKeyboardSupervisor& keyboard, IBrowserFileChangeNotification& onChange)
	{
		return new MPlatFileBrowser(publisher, installation, gui, keyboard, onChange);
	}
}