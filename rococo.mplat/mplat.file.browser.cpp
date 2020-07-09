#include "rococo.mplat.h"
#include <rococo.file.browser.h>
#include <rococo.strings.h>
#include <rococo.maths.h>
#include <rococo.textures.h>

using namespace Rococo;
using namespace Rococo::Events;

struct PingPopulator : public IDirectoryPopulator
{
	U32FilePath initialDirectory;
	IInstallation& installation;

	PingPopulator(IInstallation& _installation) :
		installation(_installation)
	{
		Rococo::PathFromAscii("!", '/', initialDirectory);
	}

	const U32FilePath& InitialDirectory() const override
	{
		return initialDirectory;
	}

	void EnumerateSubfolders(const U32FilePath& root, IFileCallback& cb, bool recurse) override
	{
		U8FilePath pingPath;
		ToU8(root, pingPath);

		WideFilePath sysPath;
		installation.ConvertPingPathToSysPath(pingPath, sysPath.buf, sysPath.CAPACITY);

		struct : public IEventCallback<IO::FileItemData>
		{
			IFileCallback* cb;
			const wchar_t* sysPath;
			U32FilePath root;

			void OnEvent(IO::FileItemData& item) override
			{
				if (item.isDirectory)
				{
					if (*item.containerRelRoot == 0) // Only interested in the first layer of folders
					{
						U8FilePath buf; // ascii should be fine, since we are in a subdir of a ping path
						SafeFormat(buf.buf, buf.CAPACITY, "%S%S", item.containerRelRoot, item.itemRelContainer);
						U32FilePath itemPath;
						Rococo::PathFromAscii(buf, '/', itemPath);
						cb->OnFile(root, itemPath, nullptr, 0);
					}
				}
			}
		} invokeCallback;

		invokeCallback.cb = &cb;
		invokeCallback.sysPath = sysPath;
		invokeCallback.root = root;

		Rococo::IO::ForEachFileInDirectory(sysPath, invokeCallback, recurse);
	}

	void EnumerateFiles(const U32FilePath& root, IFileCallback& cb, bool recurse) override
	{
		U8FilePath pingPath;
		ToU8(root, pingPath);

		WideFilePath sysPath;
		installation.ConvertPingPathToSysPath(pingPath, sysPath.buf, sysPath.CAPACITY);

		struct : public IEventCallback<IO::FileItemData>
		{
			IFileCallback* cb;
			const wchar_t* sysPath;
			U32FilePath root;
			void OnEvent(IO::FileItemData& item) override
			{
				if (!item.isDirectory)
				{
					if (*item.containerRelRoot == 0) // Only interested in the first layer of folders
					{
						char buf[260];
						SafeFormat(buf, 260, "%S%S", item.containerRelRoot, item.itemRelContainer);
						U32FilePath itemPath;
						Rococo::PathFromAscii(buf, '/', itemPath);

						IO::FileAttributes attr;
						if (IO::TryGetFileAttributes(item.fullPath, attr))
						{
							cb->OnFile(root, itemPath, attr.timestamp, attr.fileLength);
						}
						else
						{
							cb->OnFile(root, itemPath, "", 0);
						}
					}
				}
			}
		} invokeCallback;

		invokeCallback.cb = &cb;
		invokeCallback.sysPath = sysPath;
		invokeCallback.root = root;

		Rococo::IO::ForEachFileInDirectory(sysPath, invokeCallback, recurse);
	}

	void Free() override
	{
		delete this;
	}
};

struct FileBrowserStyle: public IFileBrowserStyle
{
	int32 RowHeight(BrowserComponent component) const override
	{
		return 34;
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

struct FileBrowserRC : public IFileBrowserRendererContext
{
	IGuiRenderContext& gc;
	GuiRect bounds;

	FileBrowserRC(IGuiRenderContext& _gc, const GuiRect& _bounds) : 
		gc(_gc), bounds(_bounds)
	{
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

		GuiMetrics metrics;
		gc.Renderer().GetGuiMetrics(metrics);

		if (IsPointInRect(metrics.cursorPosition, rect))
		{
			switch (component)
			{
			case BrowserComponent::TREE_FOLDER_ENTRY:
				fillCol1 = fillCol2 = RGBAb(212, 212, 0, 255);
				edgeCol1 = edgeCol2 = RGBAb(255, 255, 255, 255);
				break;
			default:
				fillCol1 = fillCol2 = RGBAb(0, 0, 48, 255);
				edgeCol1 = edgeCol2 = RGBAb(255, 255, 255, 255);
				break;
			}
		}
		else
		{
			switch (component)
			{
			case BrowserComponent::TREE_FOLDER_ENTRY:
				fillCol1 = fillCol2 = RGBAb(192, 192, 0, 255);
				edgeCol1 = edgeCol2 = RGBAb(224, 224, 224, 255);
				break;
			default:
				fillCol1 = fillCol2 = RGBAb(0, 0, 32, 255);
				edgeCol1 = edgeCol2 = RGBAb(224, 224, 224, 255);
				break;
			}
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

	void RenderSubFolder(cstr subpath, int index, int depth, const GuiRect& rect, GuiRect& outputPathTarget) override
	{
		enum { HEIGHT = 32, LEFTBORDER = 10, HBORDER = 4, VBORDER = 4, ROWSPACE = 4, ICONSPACE = 32 };

		GuiRect textRect;
		textRect.left = ICONSPACE + rect.left + depth * 20;
		textRect.right = textRect.left + 120;
		textRect.top = rect.top + index * (HEIGHT + ROWSPACE);
		textRect.bottom = textRect.top + HEIGHT;

		GuiRect fontRect = textRect;
		fontRect.left += LEFTBORDER;
		fontRect.right -= HBORDER;
		fontRect.top += VBORDER;
		fontRect.bottom -= VBORDER;

		outputPathTarget = textRect;

		GuiRect iconRect{ textRect.left - ICONSPACE, textRect.top, textRect.left,  textRect.bottom };
		DrawIcon(iconRect, BrowserComponent::FOLDER_ICON);

		DrawAsciiText(fontRect, BrowserComponent::TREE_FOLDER_ENTRY, subpath);

		GuiMetrics metrics;
		gc.Renderer().GetGuiMetrics(metrics);
		if (IsPointInRect(metrics.cursorPosition, textRect))
		{
			Graphics::DrawBorderAround(gc, textRect, Vec2i{ 1,1 }, RGBAb(128, 128, 128, 255), RGBAb(96,96,96,255));
		}
	}

	void SetClipRect(const GuiRect& rect) override
	{
		gc.FlushLayer();
		gc.SetScissorRect(ToF(rect));
	}
};

struct MPlatFileBrowser: public IMPlatFileBrowser, public IObserver, public IUIElement
{
	PingPopulator pingPopulator;
	FileBrowserStyle style;
	FileBrowsingAPI api;

	AutoFree<IFileBrowser> browser;
	IPublisher& publisher;

	MPlatFileBrowser(IPublisher& _publisher, IInstallation& _installation):
		pingPopulator(_installation),
		api { pingPopulator, style }, 
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

	}

	void OnMouseLClick(Vec2i cursorPos, bool clickedDown)  override
	{
		if (!clickedDown) browser->ClickAt(cursorPos);
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