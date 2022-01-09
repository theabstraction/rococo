#include <rococo.api.h>
#include <rococo.file.browser.h>
#include <string.h>
#include <rococo.strings.h>
#include <rococo.maths.h>
#include <rococo.io.h>

using namespace Rococo;
using namespace Rococo::Browser;

#include <vector>

void ToU8FilePathWithSubstitutions(const U32FilePath& src, U8FilePath& dest, char substitute = '?')
{
	char* q = dest.buf;
	const char32_t* p = src;

	while (*p != 0)
	{
		char32_t c = *p;
		*q = (c < 32 || c > 127) ? substitute : (char)*p;
		p++, q++;
	}

	*q = 0;
}


bool Eq(const char32_t* p, const char32_t* q)
{
	while (*p != 0)
	{
		if (*p++ != *q++)
		{
			return false;
		}
	}

	return *q == 0;
}

bool TryCalcVScrollerRects(const GuiRect& rect, int64 top, int64 pageSize, int64 domain, VScrollerRects& rects)
{
	if (domain > 0 && pageSize < domain)
	{
		int32 buttonHeight = rect.right - rect.left - 2;

		rects.up = rect;
		rects.up.left += 3;
		rects.up.right -= 3;
		rects.up.top += 3;
		rects.up.bottom = rects.up.top + buttonHeight;

		rects.down = rect;
		rects.down.left += 3;
		rects.down.right -= 3;
		rects.down.bottom -= 3;
		rects.down.top = rects.down.bottom - buttonHeight;

		double sliderRatio = (double)pageSize / (double)domain;

		double sliderSpan = (double)(rects.down.top - rects.up.bottom - 2);

		int32 sliderHeight = (int32)(sliderRatio * sliderSpan);

		rects.slider = rect;
		rects.slider.left += 3;
		rects.slider.right -= 3;

		int32 slideAreaHeight = rects.down.top - rects.up.bottom;

		int32 sliderPos = (int32)((double)slideAreaHeight * (double)top / (double)domain);

		rects.slider.top = rects.up.bottom + 1 + sliderPos;
		rects.slider.bottom = rects.slider.top + sliderHeight;
		return true;
	}

	return false;
}


void DrawVScroller(IFileBrowserRenderContext& rc, const GuiRect& rect, int64 top, int64 pageSize, int64 domain, VScrollerRects& rects)
{
	rects.slider = { 0,0,0,0 };
	if (TryCalcVScrollerRects(rect, top, pageSize, domain, rects))
	{
		rc.DrawBackground(rect, BrowserComponent::FILE_SCROLLER_SLIDER_BACK);
		rc.DrawBackground(rects.slider, BrowserComponent::FILE_SCROLLER_SLIDER);

		rc.DrawArrowButton({ 0,1 }, rects.up);
		rc.DrawArrowButton({ 0,-1 }, rects.down);
	}
}


void RenderSubFolder(IFileBrowserRenderContext& rc, cstr subpath, int index, int depth, const GuiRect& rect, GuiRect& outputPathTarget)
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
	rc.DrawIcon(iconRect, BrowserComponent::FOLDER_ICON);
	rc.DrawAsciiText(fontRect, BrowserComponent::TREE_FOLDER_ENTRY, subpath);
	rc.DrawBorder(textRect, BrowserComponent::TREE_FOLDER_ENTRY);
}


struct FileBrowser : public IFileBrowser
{
	FileBrowsingAPI& api;
	IBrowserFileChangeNotification& selectionChangeNotifierSink;

	int64 cursorPos = 0;

	GuiRect containerRect{ -1, -1, -1, -1 };
	GuiRect fileRect { -1, -1, -1, -1 };
	GuiRect fileScrollRect{ -1, -1, -1, -1 };

	FileBrowser(FileBrowsingAPI& _api, IBrowserFileChangeNotification& _selectionChangeNotifierSink) :
		api(_api),
		selectionChangeNotifierSink(_selectionChangeNotifierSink)
	{
		SyncDomain();
	}

	~FileBrowser()
	{
	}

	void SyncDomain()
	{
		fileScrollDomain = api.style.RowHeight(BrowserComponent::FILE_ENTRY) * (api.directoryPopulator.FileCount() + api.directoryPopulator.DirectoryCount());
	}

	void WheelAt(Vec2i pos, int dWheel) override
	{
		if (IsPointInRect(pos, fileRect))
		{
			int64 pageSize = fileRect.bottom - fileRect.top;
			int64 delta = -dWheel * api.style.RowHeight(BrowserComponent::FILE_ENTRY);
			cursorPos = clamp(cursorPos + delta, 0LL,  fileScrollDomain - pageSize);
		}
	}

	void ClickAt(Vec2i pos, bool isDown) override
	{
		if (isDown)
		{
			return;
		}

		for (auto& fd : folders)
		{
			if (IsPointInRect(pos, fd.rect))
			{
				OnFolderClicked(fd);
				return;
			}
		}

		if (IsPointInRect(pos, fileRect))
		{
			for (auto& f : files)
			{
				if (IsPointInRect(pos, f.rect))
				{
					OnFileClicked(f);
					return;
				}
			}
		}
		else if (IsPointInRect(pos, lastRenderedFileVScrollRects.up))
		{
			cursorPos = max(0LL, cursorPos - api.style.RowHeight(BrowserComponent::FILE_ENTRY));
		}
		else if (IsPointInRect(pos, lastRenderedFileVScrollRects.down))
		{
			int64 pageSize = fileRect.bottom - fileRect.top;
			cursorPos = min(fileScrollDomain - pageSize, cursorPos + api.style.RowHeight(BrowserComponent::FILE_ENTRY));
		}
		else if (IsPointInRect(pos, fileScrollRect))
		{
			if (pos.y > lastRenderedFileVScrollRects.slider.bottom)
			{
				int64 pageSize = fileRect.bottom - fileRect.top;
				cursorPos = min(fileScrollDomain - pageSize, cursorPos + pageSize);
			}
			else if (pos.y < lastRenderedFileVScrollRects.slider.top)
			{
				int64 pageSize = fileRect.bottom - fileRect.top;
				cursorPos = max(0LL, cursorPos - pageSize);
			}
		}
	}

	void RaiseContextAt(Vec2i pos) override
	{

	}

	int64 fileScrollDomain = 0;

	void Layout(IFileBrowserRenderContext& rc)
	{
		GuiRect containerRect = rc.GetContainerRect();
		if (memcmp(&containerRect, &this->containerRect, sizeof(GuiRect)) != 0)
		{
			this->containerRect = containerRect;
			fileRect = containerRect;
			fileRect.left = (4 * containerRect.left + containerRect.right) / 5;
			fileScrollRect = containerRect;
			fileScrollRect.left = fileScrollRect.right - api.style.HorizontalSpan(BrowserComponent::FILE_SCROLLER_SLIDER_BACK) - 1;
			fileScrollRect.right -= 1;
			fileScrollRect.top += 1;
			fileScrollRect.bottom -= 1;
		}
	}

	void DrawString(IFileBrowserRenderContext& rc, BrowserComponent component, const U32FilePath& path, const GuiRect& rect)
	{
		U8FilePath asciiPath;
		ToU8FilePathWithSubstitutions(path, asciiPath, '?');
		rc.DrawAsciiText(rect, component, asciiPath.buf);
	}

	void RenderFileArea(IFileBrowserRenderContext& rc)
	{
		GuiRect targetRect = fileRect;
		rc.DrawBackground(targetRect, BrowserComponent::FILE_SECTION);
		rc.SetClipRect(fileRect);

		GuiRect borderDeltas = api.style.BorderDeltas(BrowserComponent::FILE_ENTRY);

		int32 rowHeight = api.style.RowHeight(BrowserComponent::FILE_ENTRY);

		targetRect.left += borderDeltas.left;
		targetRect.right -= borderDeltas.right;
		targetRect.top += borderDeltas.top - (int32) cursorPos;
		targetRect.bottom = targetRect.top + rowHeight - borderDeltas.bottom;

		files.clear();

		for (size_t i = 0; i < api.directoryPopulator.DirectoryCount(); i++)
		{
			if (targetRect.bottom <= fileRect.top)
			{
				targetRect.top += rowHeight;
				targetRect.bottom = targetRect.top + rowHeight - borderDeltas.bottom;
				continue;
			}

			const auto& dirpath = api.directoryPopulator.GetDirectory(i);

			GuiRect iconRect = targetRect;
			iconRect.right = iconRect.left + iconRect.bottom - iconRect.top; // make square
			rc.DrawIcon(iconRect, BrowserComponent::FOLDER_ICON);

			GuiRect subtargetRect = targetRect;
			subtargetRect.left = iconRect.right + borderDeltas.left;
			subtargetRect.top += 4;
			subtargetRect.bottom -= 4;
			DrawString(rc, BrowserComponent::FILE_ENTRY, dirpath, subtargetRect);

			targetRect.top += rowHeight;
			targetRect.bottom = targetRect.top + rowHeight - borderDeltas.bottom;

			if (targetRect.top >= fileRect.bottom)
			{
				break;
			}
		}

		for (size_t j = 0; j < api.directoryPopulator.FileCount(); j++)
		{
			if (targetRect.bottom <= fileRect.top)
			{
				targetRect.top += rowHeight;
				targetRect.bottom = targetRect.top + rowHeight - borderDeltas.bottom;
				continue;
			}

			uint64 fileLength;
			FileTimestamp lastModifiedAt;
			const auto& f = api.directoryPopulator.GetFile(j, fileLength, lastModifiedAt);
			
			GuiRect textRect = targetRect;
			textRect.top += 4;
			textRect.bottom -= 4;
			DrawString(rc, BrowserComponent::FILE_ENTRY, f, textRect);

			GuiRect infoRect = textRect;
			infoRect.left = textRect.right - 480;

			char desc[64];
			if (fileLength < 10000)
			{
				char fullLength[64];
				SafeFormat(fullLength, sizeof(fullLength), "%llu bytes", fileLength);
				SafeFormat(desc, sizeof(desc), "%s %12s", lastModifiedAt.text, fullLength);
			}
			else if (fileLength < 1048576)
			{
				char fullLength[64];
				SafeFormat(fullLength, sizeof(fullLength), "%llu kb", fileLength / 1024);
				SafeFormat(desc, sizeof(desc), "%s %12s", lastModifiedAt.text, fullLength);
			}
			else
			{
				char fullLength[64];
				SafeFormat(fullLength, sizeof(fullLength), "%llu Mb", fileLength / 1048576);
				SafeFormat(desc, sizeof(desc), "%s %12s", lastModifiedAt.text, fullLength);
			}

			GuiRect fullrect = textRect;
			textRect.right = infoRect.right;
			files.push_back({ f, fullrect });

			rc.DrawAsciiText(infoRect, BrowserComponent::FILE_ENTRY, desc);

			targetRect.top += rowHeight;
			targetRect.bottom = targetRect.top + rowHeight - borderDeltas.bottom;

			if (targetRect.top >= fileRect.bottom)
			{
				break;
			}
		}

		int64 pageSize = fileRect.bottom - fileRect.top;
		DrawVScroller(rc, fileScrollRect, cursorPos, pageSize, fileScrollDomain, lastRenderedFileVScrollRects);
	}

	VScrollerRects lastRenderedFileVScrollRects;

	struct FolderDesc
	{
		U32FilePath subdir;
		int depth;
		int index;
		GuiRect rect;
	};

	struct FileDesc
	{
		U32FilePath filename;
		GuiRect rect;
	};

	std::vector<FileDesc> files;

	U32FilePath selectedFile = { 0 };

	OS::ticks lastClick;

	void OnFileClicked(const FileDesc& fd)
	{
		OS::ticks now = OS::CpuTicks();

		bool doubleClick = false;

		if (Eq(fd.filename, selectedFile))
		{
			OS::ticks oneQuarterSecond = OS::CpuHz() >> 2;
			if (now - lastClick < oneQuarterSecond)
			{
				doubleClick = true;
			}
		}

		lastClick = now;

		selectedFile = fd.filename;
		selectionChangeNotifierSink.OnFileSelect(selectedFile, doubleClick);
	}

	void GetSelectedFile(U32FilePath& path) const override
	{
		path = selectedFile;
	}

	void OnFolderClicked(const FolderDesc& fd)
	{
		selectedFile = { 0 };
		api.directoryPopulator.SetCurrentDirectory(fd.subdir);
		folders.clear(); // sanity - prevent invalidated folder tree from being used until next render
		SyncDomain();
		selectionChangeNotifierSink.OnFileSelect(fd.subdir, false);
	}

	std::vector<FolderDesc> folders; // This gets populated on each render cycle, giving rectangles of each folder

	void AddFolderRect(const U32FilePath& subdir, int index, int depth, const GuiRect& rect)
	{
		FolderDesc desc;
		desc.subdir = subdir;
		desc.depth = depth;
		desc.rect = rect;
		desc.index = index;

		folders.push_back(desc);
	}

	void RenderFolderTree(IFileBrowserRenderContext& rc)
	{
		GuiRect treeRect = containerRect;
		treeRect.right = fileRect.left;

		rc.SetClipRect(treeRect);

		rc.DrawBackground(treeRect, BrowserComponent::TREE_SECTION);

		folders.clear();

		struct : IEventCallback<U32FilePath>
		{
			int depth = 0;
			int index = 0;
			FileBrowser* This;
			IFileBrowserRenderContext* rc;
			void OnEvent(U32FilePath& subpath) override
			{
				U8FilePath asciiRep;
				ToU8FilePathWithSubstitutions(subpath, asciiRep, '?');
				GuiRect outputSubfolderRect;

				bool foundSubDir = false;

				size_t len = strlen(asciiRep);
				if (asciiRep.buf[len - 1] == Rococo::IO::GetFileSeparator())
				{
					asciiRep.buf[len - 1] = 0;
				}

				for (const char* p = asciiRep + len; p > asciiRep; p--)
				{
					if (*p == '/' && p[1] != 0)
					{
						RenderSubFolder(*rc, p + 1, index++, depth++, This->containerRect, outputSubfolderRect);
						foundSubDir = true;
						break;
					}
				}

				if (!foundSubDir)
				{
					RenderSubFolder(*rc, asciiRep, index++, depth++, This->containerRect, outputSubfolderRect);
				}
				
				This->AddFolderRect(subpath, index, depth, outputSubfolderRect);
			}
		} renderSubfolder;
		renderSubfolder.This = this;
		renderSubfolder.rc = &rc;

		api.directoryPopulator.ForEachSubPathFromCurrent(renderSubfolder);

		for (int i = 0; i < api.directoryPopulator.DirectoryCount(); ++i)
		{
			auto& dir = api.directoryPopulator.GetDirectory(i);
			GuiRect outputSubfolderRect;
			
			U8FilePath asciiRep;
			ToU8FilePathWithSubstitutions(dir, asciiRep, '?');

			size_t len = strlen(asciiRep);
			if (asciiRep.buf[len - 1] == Rococo::IO::GetFileSeparator())
			{
				asciiRep.buf[len - 1] = 0;
			}

			RenderSubFolder(rc, asciiRep, renderSubfolder.index, renderSubfolder.depth, containerRect, outputSubfolderRect);

			U32FilePath fullDir;
			api.directoryPopulator.GetFullPath(fullDir, dir);

			AddFolderRect(fullDir, renderSubfolder.index++, renderSubfolder.depth, outputSubfolderRect);
		}
	}

	void Render(IFileBrowserRenderContext& rc) override
	{
		Layout(rc);
		RenderFileArea(rc);
		RenderFolderTree(rc);

		GuiRect noClip{ -1, -1, 10000, 10000 };
		rc.SetClipRect(noClip);
	}

	void Free() override
	{
		delete this;
	}
};

namespace Rococo
{
	namespace Browser
	{
		IFileBrowser* CreateFileBrowser(FileBrowsingAPI& api, IBrowserFileChangeNotification& selectionChangeNotifierSink)
		{
			return new FileBrowser(api, selectionChangeNotifierSink);
		}

		void DuplicateSubString(const U32FilePath& src, size_t start, size_t end, U32FilePath& dest)
		{
			const char32_t* s = src.buf + start;
			const char32_t* e = src.buf + end;

			if (e - s >= dest.CAPACITY)
			{
				Throw(0, "DuplicateSubString: Dest had insufficient capacity");
			}

			char32_t* p = dest.buf;

			while (s < e)
			{
				*p++ = *s++;
			}

			*p++ = 0;
		}
	} // Browser
} // Rococo