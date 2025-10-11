// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#pragma once

#include <rococo.api.h>

namespace Rococo::AutoComplete
{
	struct ISexyEditor;
}

namespace Rococo::SexyStudio
{
	struct ISexyDatabase;
}

namespace Rococo::SexyStudio
{
	enum class EMetaDataType: int
	{
		BuildDate = 0,
		Copyright = 1,
		Author = 2,
		Email = 3
	};

	ROCOCO_INTERFACE ICalltip
	{
		virtual void SetCalltipForReplacement(cstr tip) = 0;
	};

	ROCOCO_INTERFACE ISexyStudioBase
	{
		virtual cstr GetInterfaceURL(int index) = 0;
		virtual cstr GetMetaDataString(EMetaDataType index) = 0;
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE ISexyFieldEnumerator
	{
		virtual void OnFieldType(Strings::cr_substring fieldType, Strings::cr_substring searchRoot) = 0;
		virtual void OnField(cstr fieldName, Strings::cr_substring memberSearchToken) = 0;
		virtual void OnHintFound(Strings::cr_substring hint) = 0;
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

	ROCOCO_INTERFACE ISexyStudioCompletionGaffer
	{
		virtual void ReplaceCurrentSelectionWithCallTip(AutoComplete::ISexyEditor& editor) = 0;

		/*
		cr_substring candidate - some substring in .sxy source text
		char args[1024] - output buffer
		*/
		virtual void GetHintForCandidate(Strings::cr_substring candidate, char args[1024]) = 0;

		// Tells the Sexy IDE to compute the definition of the selected token and invoke ISexyEditor::GotoDefinition with the result
		virtual void GotoDefinitionOfSelectedToken(AutoComplete::ISexyEditor& editor) = 0;

		virtual void ReplaceSelectedText(AutoComplete::ISexyEditor& editor, cstr item) = 0;

		virtual void UpdateAutoComplete(AutoComplete::ISexyEditor& editor, crwstr filepath = nullptr) = 0;
	};

	ROCOCO_INTERFACE ISexyStudioInstance1
	{
		virtual ISexyStudioCompletionGaffer& Gaffer() = 0;
		virtual ISexyStudioGUI& Gui() = 0;

		virtual Rococo::SexyStudio::ISexyDatabase& GetDatabase() = 0;
		virtual const Rococo::SexyStudio::ISexyDatabase& GetDatabase() const = 0;

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

