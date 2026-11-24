// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#pragma once

#include <rococo.api.h>

// ICalltip, ISexyStudioCompletionGaffer and ISexyDatabaseSet are GUI-free and live there. This header adds the IDE
#include <rococo.sexystudio.engine.h>

namespace Rococo::SexyStudio
{
	enum class EMetaDataType: int
	{
		BuildDate = 0,
		Copyright = 1,
		Author = 2,
		Email = 3
	};

	ROCOCO_INTERFACE ISexyStudioBase
	{
		virtual cstr GetInterfaceURL(int index) = 0;
		virtual cstr GetMetaDataString(EMetaDataType index) = 0;
		virtual void Free() = 0;
	};

	enum class EIDECloseResponse
	{
		Continue,
		Shutdown
	};

	ROCOCO_INTERFACE ISexyStudioEventHandler
	{
		// Use the host to open a file with given name and line number. Returns false to allow the default implementation to handle it.
		virtual bool TryOpenEditor(cstr filePath, int lineNumber) = 0;
		virtual EIDECloseResponse OnIDEClose(Windows::IWindow & topLevelParent) = 0;
	};

	ROCOCO_INTERFACE IPreviewEventHandler
	{
		// The editor is asked to jump to a code section. If the handler returns true the previewer reveals the back button
		virtual bool OnJumpToCode(const char* path, int lineNumber) = 0;

		// The previewer back button has been clicked
		virtual void OnBackButtonClicked() = 0;
	};

	ROCOCO_INTERFACE ISexyStudioGUI
	{
		virtual Windows::IWindow& GetIDEFrame() = 0;
		virtual void PopupPreview(Windows::IWindow& hParent, cstr token, const char* path, int lineNumber, IPreviewEventHandler& eventHandler) = 0;
		virtual void SetTitle(cstr title) = 0;
	};

	ROCOCO_INTERFACE ISexyStudioInstance1: ISexyDatabaseSet
	{
		virtual ISexyStudioCompletionGaffer& Gaffer() = 0;
		virtual ISexyStudioGUI& Gui() = 0;		
		virtual void Activate() = 0;
		virtual bool IsRunning() const = 0;
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE ISexyStudioFactory1: ISexyStudioBase
	{
		virtual ISexyStudioInstance1* CreateSexyIDE(Windows::IWindow& topLevelParent, ISexyStudioEventHandler& eventHandler) = 0;
	};
	
	// The name of the interface is passed in the interface parameter. If any parameter is invalid or the URL recognized the function returns a non-zero error code.
	// If the interface is recognized  and all other parameters are correct the function returns zero.
	// The caller is required to call the Free method of the interface to free up memory.
	// Currently the only recognized interface URLs are "Rococo.SexyStudio.ISexyStudioFactory1" and "Rococo.SexyStudio.ISexyStudioBase"
	// int CreateSexyStudioFactory)(void** ppInterface, const char* interfaceURL) is defined in the SexyStudio DLL
	// and retrieved by LoadLibrary and GetProcAddress. See the Win32 documentation for more info on these two functions.
	// N.B CreateSexyStudioFactory is not thread safe and all calls to the function should be done from the same thread
	// Returns an error code on failure, 0 on success.
	typedef int (*FN_CreateSexyStudioFactory)(void** ppInterface, const char* interfaceURL);
}

