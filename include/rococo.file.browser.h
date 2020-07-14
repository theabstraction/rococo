#pragma once

#include <rococo.types.h>

namespace Rococo
{
	struct IInstallation;

	namespace Browser
	{
		void DuplicateSubString(const U32FilePath& src, size_t start, size_t end, U32FilePath& dest);

		enum class BrowserComponent
		{
			FILE_ENTRY,
			FILE_SECTION,
			TREE_SECTION,
			TREE_FOLDER_ENTRY,
			FOLDER_ICON,
			FILE_SCROLLER_SLIDER,
			FILE_SCROLLER_SLIDER_BACK,
			STATUS_ERROR,
			LINE_EDITOR
		};

		struct VScrollerRects
		{
			GuiRect up;
			GuiRect down;
			GuiRect slider;
		};

		ROCOCOAPI IFileBrowserRenderContext
		{
			virtual void DrawArrowButton(Vec2 direction, const GuiRect & rect) = 0;
			virtual void DrawAsciiText(const GuiRect& rect, BrowserComponent component, cstr buffer) = 0;
			virtual void DrawAsciiTextWithCaret(int pos, const GuiRect& rect, cstr buffer) = 0;
			virtual void DrawU16Text(const GuiRect& rect, BrowserComponent component, const wchar_t* buffer) = 0;
			virtual GuiRect GetContainerRect() const = 0;
			virtual void DrawIcon(const GuiRect& rect, BrowserComponent component) = 0;
			virtual void DrawBackground(const GuiRect& rect, BrowserComponent component) = 0;
			virtual void SetClipRect(const GuiRect& rect) = 0;
			virtual void DrawBorder(const GuiRect& rect, BrowserComponent component) = 0;
		};

		ROCOCOAPI IFileBrowserStyle
		{
			virtual int32 RowHeight(BrowserComponent component) const = 0;
			virtual GuiRect BorderDeltas(BrowserComponent component) const = 0;
			virtual int32 HorizontalSpan(BrowserComponent component) const = 0;
		};

		ROCOCOAPI IBrowserFileChangeNotification
		{
			virtual void OnFileSelect(const U32FilePath & path, bool doubleClick) = 0;
		};

		ROCOCOAPI IFileBrowser
		{
			virtual void ClickAt(Vec2i pos, bool clickedDown) = 0;
			virtual void RaiseContextAt(Vec2i pos) = 0;
			virtual void Render(IFileBrowserRenderContext& rc) = 0;
			virtual void GetSelectedFile(U32FilePath& path) const = 0;
			virtual void WheelAt(Vec2i cursorPos, int dWheel) = 0;
			virtual void Free() = 0;
		};

		ROCOCOAPI IFileCallback
		{
			virtual void OnFile(const U32FilePath & root, const U32FilePath & subpath, cstr timestamp, uint64 length) = 0;
		};

		struct FileTimestamp
		{
			char text[64];
			uint64 osFileTime;
		};

		ROCOCOAPI IDirectoryPopulator
		{
			virtual size_t FileCount() const = 0;
			virtual const U32FilePath& GetFile(size_t index, uint64& fileLength, FileTimestamp& timestamp) const = 0;
			virtual size_t DirectoryCount() const = 0;
			virtual const U32FilePath& GetDirectory(size_t index) const = 0;
			virtual void SetCurrentDirectory(const U32FilePath& path) = 0;
			virtual void ForEachSubPathFromCurrent(IEventCallback<U32FilePath>& cb) = 0;
			virtual void GetFullPath(U32FilePath& fullPath, const U32FilePath& subdir) const = 0;
			virtual void GetFullPathToFile(const U32FilePath& shortFileName, U32FilePath& fullPath) const = 0;
			// Specify a prefix that we are allowed to navigate through, but not out of.
			virtual void LimitRoot(const U32FilePath& prefix) = 0;
			virtual void Free() = 0;
		};
		struct FileBrowsingAPI
		{
			IDirectoryPopulator& directoryPopulator;
			IFileBrowserStyle& style;
		};

		IFileBrowser* CreateFileBrowser(FileBrowsingAPI& api, IBrowserFileChangeNotification& onSelChange);

		IDirectoryPopulator* CreatePingPopulator(IInstallation& installation);
	} // Browser
} // Rococo
