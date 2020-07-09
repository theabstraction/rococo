#include <rococo.file.browser.h>
#include <string.h>
#include <rococo.strings.h>
#include <rococo.maths.h>

using namespace Rococo;

#include <vector>

template<typename T> void ForEachSubPath(const FilePath<T>& dir, IEventCallback<FilePath<T>>& cb)
{
	FilePath<T> tempCharBuffer = { 0 };
	tempCharBuffer.pathSeparator = dir.pathSeparator;

	T* p = tempCharBuffer.buf;

	T sep = dir.pathSeparator;

	for (const T* s = dir.buf; *s != 0; ++s)
	{
		T c = *s;
		if (c != sep)
		{
			*p++ = c;
		}
		else
		{
			*p++ = 0;
			cb.OnEvent(tempCharBuffer);
			p = tempCharBuffer.buf;
		}
	}

	if (p != tempCharBuffer.buf)
	{
		*p++ = 0;
		cb.OnEvent(tempCharBuffer);
	}
}

void ToU8FilePath(const U32FilePath& src, U8FilePath& dest, char substitute = '?')
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

	GuiRect containerRect{ -1, -1, -1, -1 };
	GuiRect fileRect { -1, -1, -1, -1 };

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

	void ClickAt(Vec2i pos) override
	{
		for (auto& fd : folders)
		{
			if (IsPointInRect(pos, fd.rect))
			{
				OnFolderClicked(fd);
				return;
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

	}

	void Layout(IFileBrowserRendererContext& rc)
	{
		GuiRect containerRect = rc.GetContainerRect();
		if (memcmp(&containerRect, &this->containerRect, sizeof(GuiRect)) != 0)
		{
			this->containerRect = containerRect;
			fileRect = containerRect;
			fileRect.left = (4 * containerRect.left + containerRect.right) / 5;
		}
	}


	void DrawString(IFileBrowserRendererContext& rc, BrowserComponent component, const U32FilePath& path, const GuiRect& rect)
	{
		U8FilePath asciiPath;
		ToU8FilePath(path, asciiPath, '?');
		rc.DrawAsciiText(rect, component, asciiPath.buf);
	}

	void RenderFileArea(IFileBrowserRendererContext& rc)
	{
		GuiRect targetRect = fileRect;
		rc.DrawBackground(targetRect, BrowserComponent::FILE_SECTION);
		rc.SetClipRect(fileRect);

		GuiRect borderDeltas = api.style.BorderDeltas(BrowserComponent::FILE_ENTRY);

		int32 rowHeight = api.style.RowHeight(BrowserComponent::FILE_ENTRY);

		targetRect.left += borderDeltas.left;
		targetRect.right -= borderDeltas.right;
		targetRect.top += borderDeltas.top;
		targetRect.bottom = targetRect.top + rowHeight - borderDeltas.bottom;

		if (subDirectories.empty())
		{
			Repopulate();
		}

		for (auto info : subFiles)
		{
			auto& f = info.path;
			size_t len = Length(f.buf);
			if (f.buf[len - 1] == f.pathSeparator)
			{
				GuiRect iconRect = targetRect;
				iconRect.right = iconRect.left + iconRect.bottom - iconRect.top; // make square
				rc.DrawIcon(iconRect, BrowserComponent::FOLDER_ICON);

				GuiRect subtargetRect = targetRect;
				subtargetRect.left = iconRect.right + borderDeltas.left;
				subtargetRect.top += 4;
				subtargetRect.bottom -= 4;
				DrawString(rc, BrowserComponent::FILE_ENTRY, f, subtargetRect);
			}
			else
			{
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
			}

			targetRect.top += rowHeight;
			targetRect.bottom = targetRect.top + rowHeight - borderDeltas.bottom;

			if (targetRect.top >= fileRect.bottom)
			{
				break;
			}
		}
	}

	struct FolderDesc
	{
		char text[64];
		int depth;
		GuiRect rect;
	};

	void OnFolderClicked(const FolderDesc& fd)
	{
		auto& dir = currentDirectory;
		int depth = 0;
		char32_t sep = dir.pathSeparator;

		if (Eq(fd.text, "!"))
		{
			PathFromAscii("!", '/', currentDirectory);
			folders.clear(); // sanity - prevent invalidated folder tree from being used until next render
			Repopulate();
			return;
		}

		size_t len = Length(dir.buf);
		for (size_t i = 0; i != len; ++i)
		{
			char32_t c = dir[i];
			if (c == sep)
			{
				depth++;
				if (depth == fd.depth)
				{
					U32FilePath newDir;
					DuplicateSubString(dir, 0, i, newDir);
					currentDirectory = newDir;
					folders.clear(); // sanity - prevent invalidated folder tree from being used until next render
					Repopulate();
					return;
				}
			}
		}
	}

	std::vector<FolderDesc> folders; // This gets populated on each render cycle, giving rectangles of each folder

	void AddFolderRect(cstr text, int index, int depth, const GuiRect& rect)
	{
		FolderDesc desc;
		SafeFormat(desc.text, sizeof(desc.text), "%s", text);
		desc.depth = depth;
		desc.rect = rect;

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
				ToU8FilePath(subpath, asciiRep, '?');
				GuiRect outputSubfolderRect;
				rc->RenderSubFolder(asciiRep, index++, depth++, This->containerRect, outputSubfolderRect);
				This->AddFolderRect(asciiRep, index, depth, outputSubfolderRect);
			}
		} renderSubfolder;
		renderSubfolder.This = this;
		renderSubfolder.rc = &rc;
		ForEachSubPath<char32_t>(currentDirectory, renderSubfolder);

		for (auto& dir : subDirectories)
		{
			GuiRect outputSubfolderRect;
			
			U8FilePath asciiRep;
			ToU8FilePath(dir, asciiRep, '?');

			rc.RenderSubFolder(asciiRep, renderSubfolder.index, renderSubfolder.depth, containerRect, outputSubfolderRect);
			AddFolderRect(asciiRep, renderSubfolder.index++, renderSubfolder.depth, outputSubfolderRect);
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