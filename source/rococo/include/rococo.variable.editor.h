#ifndef VARIABLE_EDITOR_H
#define VARIABLE_EDITOR_H

namespace Rococo
{
	ROCOCO_INTERFACE ISelection
	{
		virtual size_t Count() const = 0;
		virtual cstr GetElement(size_t index) const = 0;
		virtual cstr GetDefaultItem() const = 0;
	};

	ROCOCO_INTERFACE IStringValidator
	{
		virtual bool ValidateAndReportErrors(cstr textEditorContent, cstr variableName) = 0;
	};

	ROCOCO_INTERFACE IVariableEditorEventHandler
	{
		virtual void OnButtonClicked(cstr variableName) = 0;
	};

	ROCOCO_INTERFACE IVariableEditor
	{
		virtual void AddIntegerEditor(cstr variableName, cstr variableDesc, int minimum, int maximum, int defaultValue) = 0;
        virtual void AddBooleanEditor(cstr variableName, bool state) = 0;
		virtual void AddPushButton(cstr variableName, cstr variableDesc) = 0;
		virtual void AddSelection(cstr variableName, cstr variableDesc, char* buffer, uint32 capacityIncludingNullCharacter, ISelection& selection, IStringValidator* validator = nullptr) = 0;
		virtual void AddStringEditor(cstr variableName, cstr variableDesc, char* buffer, uint32 capacityIncludingNullCharacter, IStringValidator* validator = nullptr) = 0;
		virtual void AddTab(cstr tabName, cstr tabToolTip) = 0;
		virtual void AddFilenameEditor(cstr variableName, cstr variableDesc, char* buffer, uint32 capacityIncludingNullCharacter, cstr filter, IStringValidator* validator = nullptr) = 0;
		virtual bool IsModalDialogChoiceYes() = 0;
        virtual bool GetBoolean(cstr variableName) = 0;
		virtual int GetInteger(cstr variableName) = 0;
		virtual void SetHintError(cstr variableName, cstr message) = 0;
		virtual void Free() = 0;
	};

	ROCOCO_WINDOWS_API IVariableEditor* CreateVariableEditor(Windows::IWindow& parent, const Vec2i& span, int32 labelWidth, cstr appQueryName, cstr defaultTab, cstr defaultTooltip, IVariableEditorEventHandler* eventHandler = nullptr, const Vec2i* topLeft = nullptr);
}

#endif