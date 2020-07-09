#pragma once

#include <rococo.types.h>

namespace Rococo
{
	union CharTypePointers
	{
		const char* charPtr;
		const wchar_t* char16Ptr;
		const int32* char32Ptr;
	};

	void ToU8(const U32FilePath& src, U8FilePath& dest);
	void ToWide(const U32FilePath& src, WideFilePath& dest);

	void PathFromAscii(cstr c_string, char separator, U32FilePath& path);
	void PathFromWide(const wchar_t* u16Code, wchar_t separator, U32FilePath& path);
	void DuplicateSubString(const U32FilePath& src, size_t start, size_t end, U32FilePath& dest);

	enum class BrowserComponent
	{
		FILE_ENTRY,
		FILE_SECTION,
		TREE_SECTION,
		TREE_FOLDER_ENTRY,
		FOLDER_ICON
	};

	ROCOCOAPI IFileBrowserRendererContext
	{
		virtual void DrawAsciiText(const GuiRect& rect, BrowserComponent component, cstr buffer) = 0;
		virtual void DrawU16Text(const GuiRect& rect, BrowserComponent component, const wchar_t* buffer) = 0;
		virtual GuiRect GetContainerRect() const = 0;
		virtual void DrawIcon(const GuiRect& rect, BrowserComponent component) = 0;
		virtual void DrawBackground(const GuiRect& rect, BrowserComponent component) = 0;
		virtual void RenderSubFolder(cstr subpath, int index, int depth, const GuiRect& rect, GuiRect& outputPathTarget) = 0;
		virtual void SetClipRect(const GuiRect& rect) = 0;
	};

	ROCOCOAPI IFileBrowserStyle
	{
		virtual int32 RowHeight(BrowserComponent component) const = 0;
		virtual GuiRect BorderDeltas(BrowserComponent component) const = 0;
	};

	ROCOCOAPI IFileBrowser
	{
		virtual void ClickAt(Vec2i pos) = 0;
		virtual void RaiseContextAt(Vec2i pos) = 0;
		virtual void Render(IFileBrowserRendererContext & rc) = 0;
		virtual void Free() = 0;
	};

	ROCOCOAPI IFileCallback
	{
		virtual void OnFile(const U32FilePath& root, const U32FilePath&subpath, cstr timestamp, uint64 length) = 0;
	};

	ROCOCOAPI IDirectoryPopulator
	{
		virtual const U32FilePath& InitialDirectory() const = 0;
		virtual void EnumerateFiles(const U32FilePath& root, IFileCallback& cb, bool recurse) = 0;
		virtual void EnumerateSubfolders(const U32FilePath& root, IFileCallback& cb, bool recurse) = 0;
		virtual void Free() = 0;
	};

	struct FileBrowsingAPI
	{
		IDirectoryPopulator& directoryPopulator;
		IFileBrowserStyle& style;
	};

	IFileBrowser* CreateFileBrowser(FileBrowsingAPI& api);
}
