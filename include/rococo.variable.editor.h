#ifndef VARIABLE_EDITOR_H
#define VARIABLE_EDITOR_H

namespace Rococo
{
	ROCOCOAPI ISelection
	{
		virtual size_t Count() const = 0;
		virtual const wchar_t* GetElement(size_t index) const = 0;
		virtual const wchar_t* GetDefaultItem() const = 0;
	};

	ROCOCOAPI IStringValidator
	{
		virtual bool ValidateAndReportErrors(const wchar_t* text) = 0;
	};

	ROCOCOAPI IVariableEditorEventHandler
	{
		virtual void OnButtonClicked(LPCWSTR variableName) = 0;
	};

	ROCOCOAPI IVariableEditor
	{
		virtual void AddIntegerEditor(LPCWSTR variableName, LPCWSTR variableDesc, int minimum, int maximum, int defaultValue) = 0;
		virtual void AddPushButton(LPCWSTR variableName, LPCWSTR variableDesc) = 0;
		virtual void AddSelection(LPCWSTR variableName, LPCWSTR variableDesc, wchar_t* buffer, DWORD capacityIncludingNullCharacter, ISelection& selection, IStringValidator* validator = nullptr) = 0;
		virtual void AddStringEditor(LPCWSTR variableName, LPCWSTR variableDesc, wchar_t* buffer, DWORD capacityIncludingNullCharacter, IStringValidator* validator = nullptr) = 0;
		virtual void AddTab(LPCWSTR tabName, LPCWSTR tabToolTip) = 0;
		virtual void AddFilenameEditor(LPCWSTR variableName, LPCWSTR variableDesc, wchar_t* buffer, DWORD capacityIncludingNullCharacter, const wchar_t* filter, IStringValidator* validator = nullptr) = 0;
		virtual bool IsModalDialogChoiceYes() = 0;
		virtual int GetInteger(LPCWSTR variableName) = 0;
		virtual void Free() = 0;
	};

	IVariableEditor* CreateVariableEditor(HWND hWndOwner, const Vec2i& span, int32 labelWidth, LPCWSTR appQueryName, LPCWSTR defaultTab, LPCWSTR defaultTooltip, IVariableEditorEventHandler* eventHandler = nullptr, const POINT* topLeft = nullptr);
}

#endif