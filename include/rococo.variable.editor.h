#ifndef VARIABLE_EDITOR_H
#define VARIABLE_EDITOR_H

namespace Rococo
{
	ROCOCOAPI ISelection
	{
		virtual size_t Count() const = 0;
		virtual cstr GetElement(size_t index) const = 0;
		virtual cstr GetDefaultItem() const = 0;
	};

	ROCOCOAPI IStringValidator
	{
		virtual bool ValidateAndReportErrors(cstr text) = 0;
	};

	ROCOCOAPI IVariableEditorEventHandler
	{
		virtual void OnButtonClicked(cstr variableName) = 0;
	};

	ROCOCOAPI IVariableEditor
	{
		virtual void AddIntegerEditor(cstr variableName, cstr variableDesc, int minimum, int maximum, int defaultValue) = 0;
		virtual void AddPushButton(cstr variableName, cstr variableDesc) = 0;
		virtual void AddSelection(cstr variableName, cstr variableDesc, rchar* buffer, DWORD capacityIncludingNullCharacter, ISelection& selection, IStringValidator* validator = nullptr) = 0;
		virtual void AddStringEditor(cstr variableName, cstr variableDesc, rchar* buffer, DWORD capacityIncludingNullCharacter, IStringValidator* validator = nullptr) = 0;
		virtual void AddTab(cstr tabName, cstr tabToolTip) = 0;
		virtual void AddFilenameEditor(cstr variableName, cstr variableDesc, rchar* buffer, DWORD capacityIncludingNullCharacter, cstr filter, IStringValidator* validator = nullptr) = 0;
		virtual bool IsModalDialogChoiceYes() = 0;
		virtual int GetInteger(cstr variableName) = 0;
		virtual void Free() = 0;
	};

	IVariableEditor* CreateVariableEditor(HWND hWndOwner, const Vec2i& span, int32 labelWidth, cstr appQueryName, cstr defaultTab, cstr defaultTooltip, IVariableEditorEventHandler* eventHandler = nullptr, const POINT* topLeft = nullptr);
}

#endif