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
	using namespace Rococo::Windows;
	using namespace Rococo::AutoComplete;

	enum class EMetaDataType: int
	{
		BuildDate = 0,
		Copyright = 1,
		Author = 2,
		Email = 3
	};

	ROCOCOAPI ISexyStudioBase
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

	ROCOCOAPI ISexyStudioEventHandler
	{
		virtual EIDECloseResponse OnIDEClose(IWindow & topLevelParent) = 0;
	};

	ROCOCOAPI ISexyStudioInstance1
	{
		virtual IWindow& GetIDEFrame() = 0;
		virtual void ReplaceCurrentSelectionWithCallTip(ISexyEditor& editor) = 0;
		virtual Rococo::SexyStudio::ISexyDatabase& GetDatabase() = 0;

		/*
			cr_substring candidate - some substring in .sxy source text
			char args[1024] - output buffer
		*/
		virtual void GetHintForCandidate(cr_substring candidate, char args[1024]) = 0;
		virtual void ReplaceSelectedText(ISexyEditor& editor, cstr item) = 0;
		virtual void SetTitle(cstr title) = 0;
		virtual void Activate() = 0;
		virtual void UpdateAutoComplete(ISexyEditor& editor) = 0;
		virtual bool IsRunning() const = 0;
		virtual void Free() = 0;
	};

	ROCOCOAPI ISexyStudioFactory1: ISexyStudioBase
	{
		virtual ISexyStudioInstance1* CreateSexyIDE(IWindow& topLevelParent, ISexyStudioEventHandler& eventHandler) = 0;
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

