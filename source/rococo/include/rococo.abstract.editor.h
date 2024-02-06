#pragma once

#include <rococo.types.h>

// Abstract Editor - namespace for the property+palette+blank-slate GUI
namespace Rococo::Abedit
{
	ROCOCO_INTERFACE IUIProperties
	{
		virtual void Populate() = 0;
	};

	ROCOCO_INTERFACE IUIPalette
	{

	};

	ROCOCO_INTERFACE IUIBlankSlate
	{

	};

	ROCOCO_INTERFACE IUIPropertiesSupervisor: IUIProperties
	{
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE IUIPaletteSupervisor: IUIPalette
	{
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE IUIBlankSlateSupervisor : IUIBlankSlate
	{
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE IAbstractEditor
	{
		virtual bool IsVisible() const = 0;
		virtual IUIBlankSlate& Slate() = 0;
		virtual IUIPalette& Palette() = 0;
		virtual IUIProperties& Properties() = 0;
	};

	ROCOCO_INTERFACE IAbstractEditorSupervisor : IAbstractEditor
	{
		virtual void Free() = 0;
		virtual void HideWindow() = 0;
	};

	ROCOCO_INTERFACE IAbeditMainWindow
	{
		virtual void Free() = 0;
		virtual void Hide() = 0;
		virtual bool IsVisible() const = 0;
	};

	ROCOCO_INTERFACE IAbstractEditorMainWindowEventHandler
	{
		virtual void OnRequestToClose(IAbeditMainWindow& sender) = 0;
	};

	struct EditorSessionConfig
	{
		int defaultWidth; // +ve to specify a default editor width
		int defaultHeight; // +ve to specify a default editor height
		int defaultPosLeft; // -ve to use system default
		int defaultPosTop; // -ve to use system default
	};

	ROCOCO_INTERFACE IAbstractEditorFactory
	{
		virtual IAbstractEditorSupervisor* CreateAbstractEditor(const EditorSessionConfig& config, IAbstractEditorMainWindowEventHandler& eventHandler) = 0;
	};
}