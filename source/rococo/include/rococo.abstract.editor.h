#pragma once

#include <rococo.types.h>

#define IMPLEMENTATION_TYPE_WIN32_HWND "Win32-HWND"

namespace Rococo::Editors
{
	struct IUIPropertiesEditor;
}

namespace Rococo::Reflection
{
	struct IPropertyUIEvents;
	struct IPropertyEditor;
	struct IEstateAgent;
	struct IPropertyVenue;
}

// Abstract Editor - namespace for the property+palette+blank-slate GUI
namespace Rococo::Abedit
{
	ROCOCO_INTERFACE IUIPalette
	{

	};

	ROCOCO_INTERFACE IUIPaletteSupervisor: IUIPalette
	{
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE IAbstractEditor
	{
		virtual [[nodiscard]] bool IsVisible() const = 0;
		virtual [[nodiscard]] IUIPalette& Palette() = 0;
		virtual [[nodiscard]] Editors::IUIPropertiesEditor& Properties() = 0;
	};

	ROCOCO_INTERFACE IAbstractEditorSupervisor : IAbstractEditor
	{
		virtual void Free() = 0;
		virtual void HideWindow() = 0;
		virtual cstr Implementation() const = 0;
	};

	ROCOCO_INTERFACE IAbeditMainWindow
	{
		virtual void Free() = 0;
		virtual void Hide() = 0;
		virtual [[nodiscard]] bool IsVisible() const = 0;
	};

	ROCOCO_INTERFACE IAbstractEditorMainWindowEventHandler
	{
		virtual void GetErrorTitle(char* titleBuffer, size_t capacity) const = 0;
		virtual void OnRequestToClose(IAbeditMainWindow& sender) = 0;
		virtual void OnSelectFileToLoad(IAbeditMainWindow& sender) = 0;
		virtual void OnSelectSave(IAbeditMainWindow& sender) = 0;
		virtual void OnSlateResized() = 0;
	};

	struct EditorSessionConfig
	{
		int defaultWidth; // +ve to specify a default editor width
		int defaultHeight; // +ve to specify a default editor height
		int defaultPosLeft; // -ve to use system default
		int defaultPosTop; // -ve to use system default
		bool slateHasMenu; // true to enable menu on the blank slate window
	};

	ROCOCO_INTERFACE IAbstractEditorFactory
	{
		[[nodiscard]] virtual IAbstractEditorSupervisor* CreateAbstractEditor(const EditorSessionConfig& config, IAbstractEditorMainWindowEventHandler& eventHandler) = 0;
	};
}

#ifdef _WIN32
namespace Rococo::Windows
{
	struct IParentWindowSupervisor;
}

namespace Rococo::Abedit
{
	ROCOCO_INTERFACE IWin32AbstractEditorSupervisor : IAbstractEditorSupervisor
	{
		virtual Windows::IParentWindowSupervisor& Slate() = 0;
		virtual void SetTitleWithPath(const wchar_t* mainTitle, const wchar_t* filePath) = 0;
	};
}
#endif