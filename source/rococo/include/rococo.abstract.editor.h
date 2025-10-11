// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.

#pragma once

#include <rococo.types.h>
#include <rococo.visitors.h>
#include <rococo.eventargs.h>

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

namespace Rococo::Visitors
{
	struct IUITree;
}

// Abstract Editor - namespace for the property+palette+blank-slate GUI
namespace Rococo::Abedit
{
	struct WindowResizedArgs : Rococo::Events::EventArgs
	{
		GuiRect screenPosition;

		enum class SourceId
		{
			MainWindow
		};

		SourceId source;
	};

	ROCOCO_INTERFACE IUIPalette
	{

	};

	ROCOCO_INTERFACE IUIPaletteSupervisor: IUIPalette
	{
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE IAbstractEditor
	{
		[[nodiscard]] virtual Windows::IWindow& Window() = 0;
		[[nodiscard]] virtual bool IsVisible() const = 0;
		[[nodiscard]] virtual IUIPalette& Palette() = 0;
		[[nodiscard]] virtual Editors::IUIPropertiesEditor& Properties() = 0;
		[[nodiscard]] virtual Visitors::IUITree& NavigationTree() = 0;
		[[nodiscard]] virtual Windows::IWindow& ContainerWindow() = 0;
		[[nodiscard]] virtual Windows::IMenuBuilder& Menu() = 0;
		virtual void RefreshSlate() = 0;
		virtual void SetNavigationHandler(Visitors::ITreeControlHandler* handler) = 0;
	};

	ROCOCO_INTERFACE IAbstractEditorSupervisor : IAbstractEditor
	{
		virtual void BringToFront() = 0;
		virtual void Free() = 0;
		virtual void HideWindow() = 0;
		virtual cstr Implementation() const = 0;
	};

	ROCOCO_INTERFACE IAbeditMainWindow
	{
		virtual void Free() = 0;
		virtual void Hide() = 0;
		[[nodiscard]] virtual bool IsVisible() const = 0;
		[[nodiscard]] virtual Visitors::IUITree& NavigationTree() = 0;
		[[nodiscard]] virtual Windows::IMenuBuilder& MenuBuilder() = 0;
		virtual void SetNavigationEventHandler(Visitors::ITreeControlHandler* handler) = 0;
	};

	struct AbeditMenuEvent : Events::EventArgs
	{
		uint16 menuId;
		IAbeditMainWindow* sender;
	};

	ROCOCO_INTERFACE IAbstractEditorMainWindowEventHandler
	{
		virtual void GetErrorTitle(char* titleBuffer, size_t capacity) const = 0;
		virtual void OnRequestToClose(IAbeditMainWindow& sender) = 0;
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
		[[nodiscard]] virtual IAbstractEditorSupervisor* CreateAbstractEditor(const EditorSessionConfig& config, IAbstractEditorMainWindowEventHandler& eventHandler, Rococo::Events::IPublisher& publisher) = 0;
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
		virtual void SetTitleWithPath(cstr mainTitle, cstr filePath) = 0;
	};
}
#endif