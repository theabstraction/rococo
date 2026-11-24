// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#pragma once

// The GUI-free part of the SexyStudio API: the completion interfaces an editor drives, and the database set they work on.
// No window, widget or IDE type may appear here. rococo.sexystudio.api.h includes this and adds the IDE (ISexyStudioGUI, ISexyStudioFactory1)

#include <rococo.types.h>

namespace Rococo::AutoComplete
{
	struct ISexyEditor;
}

namespace Rococo::SexyStudio
{
	struct ISexyDatabase;

	ROCOCO_INTERFACE ICalltip
	{
		virtual void SetCalltipForReplacement(cstr tip) = 0;
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

	ROCOCO_INTERFACE ISexyDatabaseSet
	{
		virtual Rococo::SexyStudio::ISexyDatabase& GetDatabase() = 0;
		virtual const Rococo::SexyStudio::ISexyDatabase& GetDatabase() const = 0;

		// Finds sexystudio.config.sexml
		virtual void SetDirectoryContainingConfigFile(cstr sysPathToConfigDirectory) = 0;
	};
}
