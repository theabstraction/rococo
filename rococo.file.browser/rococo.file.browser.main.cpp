#include <rococo.file.browser.h>
#include <string.h>
#include <rococo.strings.h>
#include <rococo.maths.h>

using namespace Rococo;

#include <vector>

template<typename T> void ForEachSubPath(const FilePath<T>& dir, IEventCallback<FilePath<T>>& cb, size_t skipStartChars)
{
	FilePath<T> tempCharBuffer = { 0 };
	tempCharBuffer.pathSeparator = dir.pathSeparator;

	T* p = tempCharBuffer.buf;

	T sep = dir.pathSeparator;

	for (const T* s = dir.buf + skipStartChars; *s != 0; ++s)
	{
		T c = *s;
		if (c != sep)
		{
			*p++ = c;
		}
		else
		{
			*p++ = sep;
			*p = 0;
			cb.OnEvent(tempCharBuffer);
			if (s[1] == 0) return;
		}
	}

	if (p != tempCharBuffer.buf)
	{
		*p++ = sep;
		*p++ = 0;
		cb.OnEvent(tempCharBuffer);
	}
}

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

	dest.pathSeparator = (char)src.pathSeparator;
}

template<class T> size_t Length(const T* s)
{
	const T* p = s;
	for (; *p != 0; p++)
	{

	}

	return p - s;
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

	return *q != 0;
}

void AddPathSeparatror(U32FilePath& target)
{
	size_t len = Length(target.buf);
	if (len + 1 >= target.CAPACITY)
	{
		Throw(0, "AddPathSeparatror(target): length > CAPACITY");
	}

	target.buf[len] = target.pathSeparator;
	target.buf[len + 1] = 0;
}

void Merge(U32FilePath& target, const char32_t* prefix, const U32FilePath& suffix)
{
	size_t lenPrefix = Length(prefix);
	size_t lenSuffix = Length(suffix.buf);

	if (lenPrefix + lenSuffix + 1 >= target.CAPACITY)
	{
		Throw(0, "Merge(target, prefix, suffix): Cannot merge paths. Combined length > CAPACITY");
	}

	target.pathSeparator = suffix.pathSeparator;
	memcpy(target.buf, prefix, sizeof(char32_t) * lenPrefix);
	
	for (size_t i = 0; i < lenSuffix; ++i)
	{
		target.buf[i + lenPrefix] = suffix[i];
	}

	target.buf[lenPrefix + lenSuffix] = 0;
}

struct FileBrowser : public IFileBrowser
{
	FileBrowsingAPI& api;

	U32FilePath currentDirectory;

	std::vector<U32FilePath> subDirectories;

	struct FileInfo
	{
		U32FilePath path;
		char timestamp[32];
		size_t length;
	};
	std::vector<FileInfo> subFiles;

	int64 cursorPos = 0;

	GuiRect containerRect{ -1, -1, -1, -1 };
	GuiRect fileRect { -1, -1, -1, -1 };
	GuiRect fileScrollRect{ -1, -1, -1, -1 };

	FileBrowser(FileBrowsingAPI& _api) : api(_api)
	{
		auto& initialDirectory = api.directoryPopulator.InitialDirectory();
		currentDirectory = initialDirectory;
		Repopulate();
	}

	~FileBrowser()
	{
		ClearSubdirs();
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
		if (!isDown)
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

		if (IsPointInRect(pos, lastRenderedFileVScrollRects.up))
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

	void ClearSubdirs()
	{
		subDirectories.clear();
		subFiles.clear();
	}

	void Repopulate()
	{
		ClearSubdirs();

		struct : IFileCallback
		{
			std::vector<U32FilePath>* subDirectories;

			void OnFile(const U32FilePath& root, const U32FilePath& subpath, cstr timestamp, uint64 length) override
			{
				subDirectories->push_back(subpath);
				AddPathSeparatror(subDirectories->back());
			}
		} addToSubList;

		addToSubList.subDirectories = &subDirectories;

		api.directoryPopulator.EnumerateSubfolders(currentDirectory, addToSubList, false);

		struct : IFileCallback
		{
			std::vector<FileInfo>* subFiles;
			FileBrowsingAPI* api;

			void OnFile(const U32FilePath& root, const U32FilePath& subpath, cstr timestamp, uint64 length) override
			{
				FileInfo info;
				info.path = subpath;
				SafeFormat(info.timestamp, sizeof(info.timestamp), "%s", timestamp);
				info.length = length;
				subFiles->push_back(info);
			}
		} addToFileList;
		addToFileList.subFiles = &subFiles;
		addToFileList.api = &api;

		api.directoryPopulator.EnumerateFiles(currentDirectory, addToFileList, false);

		fileScrollDomain = api.style.RowHeight(BrowserComponent::FILE_ENTRY) * (subFiles.size() + subDirectories.size());
	}

	int64 fileScrollDomain = 0;

	void Layout(IFileBrowserRendererContext& rc)
	{
		GuiRect containerRect = rc.GetContainerRect();
		if (memcmp(&containerRect, &this->containerRect, sizeof(GuiRect)) != 0)
		{
			this->containerRect = containerRect;
			fileRect = containerRect;
			fileRect.left = (4 * containerRect.left + containerRect.right) / 5;
			fileScrollRect = containerRect;
			fileScrollRect.left = fileScrollRect.right - api.style.HorizontalSpan(BrowserComponent::FILE_SCROLLER) - 1;
			fileScrollRect.right -= 1;
			fileScrollRect.top += 1;
			fileScrollRect.bottom -= 1;
		}
	}

	void DrawString(IFileBrowserRendererContext& rc, BrowserComponent component, const U32FilePath& path, const GuiRect& rect)
	{
		U8FilePath asciiPath;
		ToU8FilePathWithSubstitutions(path, asciiPath, '?');
		rc.DrawAsciiText(rect, component, asciiPath.buf);
	}

	void RenderFileArea(IFileBrowserRendererContext& rc)
	{
		GuiRect targetRect = fileRect;
		rc.DrawBackground(targetRect, BrowserComponent::FILE_SECTION);
		rc.SetClipRect(fileRect);

		GuiRect borderDeltas = api.style.BorderDeltas(BrowserComponent::FILE_ENTRY);

		int32 rowHeight = api.style.RowHeight(BrowserComponent::FILE_ENTRY);

		if (subDirectories.empty())
		{
			Repopulate();
		}

		targetRect.left += borderDeltas.left;
		targetRect.right -= borderDeltas.right;
		targetRect.top += borderDeltas.top - (int32) cursorPos;
		targetRect.bottom = targetRect.top + rowHeight - borderDeltas.bottom;

		for (auto info : subDirectories)
		{
			if (targetRect.bottom <= fileRect.top)
			{
				targetRect.top += rowHeight;
				targetRect.bottom = targetRect.top + rowHeight - borderDeltas.bottom;
				continue;
			}

			GuiRect iconRect = targetRect;
			iconRect.right = iconRect.left + iconRect.bottom - iconRect.top; // make square
			rc.DrawIcon(iconRect, BrowserComponent::FOLDER_ICON);

			GuiRect subtargetRect = targetRect;
			subtargetRect.left = iconRect.right + borderDeltas.left;
			subtargetRect.top += 4;
			subtargetRect.bottom -= 4;
			DrawString(rc, BrowserComponent::FILE_ENTRY, info, subtargetRect);

			targetRect.top += rowHeight;
			targetRect.bottom = targetRect.top + rowHeight - borderDeltas.bottom;

			if (targetRect.top >= fileRect.bottom)
			{
				break;
			}
		}

		for (auto info : subFiles)
		{
			if (targetRect.bottom <= fileRect.top)
			{
				targetRect.top += rowHeight;
				targetRect.bottom = targetRect.top + rowHeight - borderDeltas.bottom;
				continue;
			}

			auto& f = info.path;
			
			GuiRect textRect = targetRect;
			textRect.top += 4;
			textRect.bottom -= 4;
			DrawString(rc, BrowserComponent::FILE_ENTRY, f, textRect);

			GuiRect infoRect = textRect;
			infoRect.left = textRect.right - 480;

			char desc[64];
			if (info.length < 10000)
			{
				char fullLength[64];
				SafeFormat(fullLength, sizeof(fullLength), "%llu bytes", info.length);
				SafeFormat(desc, sizeof(desc), "%s %12s", info.timestamp, fullLength);
			}
			else if (info.length < 1048576)
			{
				char fullLength[64];
				SafeFormat(fullLength, sizeof(fullLength), "%llu kb", info.length / 1024);
				SafeFormat(desc, sizeof(desc), "%s %12s", info.timestamp, fullLength);
			}
			else
			{
				char fullLength[64];
				SafeFormat(fullLength, sizeof(fullLength), "%llu Mb", info.length / 1048576);
				SafeFormat(desc, sizeof(desc), "%s %12s", info.timestamp, fullLength);
			}

			rc.DrawAsciiText(infoRect, BrowserComponent::FILE_ENTRY, desc);

			targetRect.top += rowHeight;
			targetRect.bottom = targetRect.top + rowHeight - borderDeltas.bottom;

			if (targetRect.top >= fileRect.bottom)
			{
				break;
			}
		}

		rc.DrawVScroller(fileScrollRect, cursorPos, fileRect.bottom - fileRect.top, fileScrollDomain, lastRenderedFileVScrollRects);
	}

	VScrollerRects lastRenderedFileVScrollRects;

	struct FolderDesc
	{
		U32FilePath subdir;
		int depth;
		int index;
		GuiRect rect;
	};

	void OnFolderClicked(const FolderDesc& fd)
	{
		auto& dir = currentDirectory;
		int depth = 0;
		char32_t sep = dir.pathSeparator;

		if (Eq(fd.subdir, U"!"))
		{
			PathFromAscii("!", '/', currentDirectory);
			folders.clear(); // sanity - prevent invalidated folder tree from being used until next render
			Repopulate();
			return;
		}

		if (fd.subdir[0] == U'!')
		{
			currentDirectory = fd.subdir;
		}
		else
		{
			Merge(currentDirectory, U"!", fd.subdir);
		}

		folders.clear(); // sanity - prevent invalidated folder tree from being used until next render
		Repopulate();
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

	void RenderFolderTree(IFileBrowserRendererContext& rc)
	{
		GuiRect treeRect = containerRect;
		treeRect.right = fileRect.left;

		rc.SetClipRect(containerRect);

		rc.DrawBackground(treeRect, BrowserComponent::TREE_SECTION);

		folders.clear();

		struct : IEventCallback<U32FilePath>
		{
			int depth = 0;
			int index = 0;
			FileBrowser* This;
			IFileBrowserRendererContext* rc;
			void OnEvent(U32FilePath& subpath) override
			{
				U8FilePath asciiRep;
				ToU8FilePathWithSubstitutions(subpath, asciiRep, '?');
				GuiRect outputSubfolderRect;

				bool foundSubDir = false;

				size_t len = Length(asciiRep.buf);
				for (const char* p = asciiRep + len; p > asciiRep; p--)
				{
					if (*p == asciiRep.pathSeparator && p[1] != 0)
					{
						rc->RenderSubFolder(p + 1, index++, depth++, This->containerRect, outputSubfolderRect);
						foundSubDir = true;
					}
				}

				if (!foundSubDir)
				{
					rc->RenderSubFolder(asciiRep, index++, depth++, This->containerRect, outputSubfolderRect);
				}
				
				This->AddFolderRect(subpath, index, depth, outputSubfolderRect);
			}
		} renderSubfolder;
		renderSubfolder.This = this;
		renderSubfolder.rc = &rc;

		U32FilePath root;
		PathFromAscii("!", '/', root);
		renderSubfolder.OnEvent(root);
		ForEachSubPath<char32_t>(currentDirectory, renderSubfolder, 1);

		for (auto& dir : subDirectories)
		{
			GuiRect outputSubfolderRect;
			
			U8FilePath asciiRep;
			ToU8FilePathWithSubstitutions(dir, asciiRep, '?');

			rc.RenderSubFolder(asciiRep, renderSubfolder.index, renderSubfolder.depth, containerRect, outputSubfolderRect);

			U32FilePath fullDir;
			Merge(fullDir, currentDirectory, dir);

			AddFolderRect(fullDir, renderSubfolder.index++, renderSubfolder.depth, outputSubfolderRect);
		}
	}

	void Render(IFileBrowserRendererContext& rc) override
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
	IFileBrowser* CreateFileBrowser(FileBrowsingAPI& api)
	{
		return new FileBrowser(api);
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

		dest.pathSeparator = src.pathSeparator;
	}

	void ToU8(const U32FilePath& src, U8FilePath& dest)
	{
		char* q = dest.buf;
		const char32_t* p = src;

		while (*p != 0)
		{
			char32_t c = *p;

			if (c < 32 || c > 127)
			{
				Throw(0, "Cannot convert Unicode to ascii. Character value out of range at position #llu", p - src.buf);
			}
			else
			{
				*q = (char)*p;
			}

			p++, q++;
		}

		*q = 0;

		dest.pathSeparator = (char)src.pathSeparator;
	}

	void ToWide(const U32FilePath& src, WideFilePath& dest)
	{
		wchar_t* q = dest.buf;
		const char32_t* p = src;

		while (*p != 0)
		{
			char32_t c = *p;

			if (c < 32 || c > 32767)
			{
				Throw(0, "Cannot convert Unicode to wide char. Character value out of range at position #llu", p - src.buf);
			}
			else
			{
				*q = (char)*p;
			}

			p++, q++;
		}

		*q = 0;

		dest.pathSeparator = (char)src.pathSeparator;
	}

	void PathFromAscii(cstr ascii_string, char separator, U32FilePath& path)
	{
		path.pathSeparator = separator;

		char32_t* q = path.buf;
		const char* p = ascii_string;

		while (*p != 0)
		{
			if (*p < 32 || *p > 127)
			{
				Throw(0, "Cannot convert 8-bit to char32_t. Character value out of range at position #llu", p - ascii_string);
			}
			*q++ = *p++;
		}

		*q = 0;
	}

	void PathFromWide(const wchar_t* wide_string, wchar_t separator, U32FilePath& path)
	{
		path.pathSeparator = separator;

		char32_t* q = path.buf;
		const wchar_t* p = wide_string;

		while (*p != 0)
		{
			if (*p < 32 || *p >= 32767)
			{
				Throw(0, "Cannot convert wide to char32_t. Character value out of range at position #llu", p - wide_string);
			}
			*q++ = *p++;
		}

		*q = 0;
	}
}