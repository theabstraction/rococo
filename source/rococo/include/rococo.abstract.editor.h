#pragma once

#include <rococo.types.h>

namespace Rococo::Strings
{
	class HString;
}

// Abstract Editor - namespace for the property+palette+blank-slate GUI
namespace Rococo::Abedit
{
	struct ControlPropertyId
	{
		uint16 value;
	};

	ROCOCO_INTERFACE IPropertySerializer
	{
		virtual void AddHeader(cstr displayName, cstr displayText) = 0;
		virtual void Target(cstr propertyIdentifier, cstr displayName, Rococo::Strings::HString& value, int capacity) = 0;
		virtual void Target(cstr propertyIdentifier, cstr displayName, int32& value, Rococo::Validators::IValueValidator<int32>& validator, Rococo::Validators::IValueFormatter<int32>& formatter) = 0;
		virtual void Target(cstr propertyIdentifier, cstr displayName, int64& value, Rococo::Validators::IValueValidator<int64>& validator, Rococo::Validators::IValueFormatter<int64>& formatter) = 0;
		virtual void Target(cstr propertyIdentifier, cstr displayName, float& value, Rococo::Validators::IValueValidator<float>& validator, Rococo::Validators::IValueFormatter<float>& formatter) = 0;
		virtual void Target(cstr propertyIdentifier, cstr displayName, double& value, Rococo::Validators::IValueValidator<double>& validator, Rococo::Validators::IValueFormatter<double>& formatter) = 0;
		virtual void Target(cstr propertyIdentifier, cstr displayName, bool& value, Rococo::Validators::IValueValidator<bool>& validator, Rococo::Validators::IValueFormatter<bool>& formatter) = 0;
		virtual void Target(cstr propertyIdentifier, cstr displayName, uint32& value, Rococo::Validators::IValueValidator<uint32>& validator, Rococo::Validators::IValueFormatter<uint32>& formatter) = 0;
		virtual void Target(cstr propertyIdentifier, cstr displayName, uint64& value, Rococo::Validators::IValueValidator<uint64>& validator, Rococo::Validators::IValueFormatter<uint64>& formatter) = 0;
	};

	ROCOCO_INTERFACE IPropertyManager
	{
		virtual void SerializeProperties(IPropertySerializer& serializer) = 0;
	};

	ROCOCO_INTERFACE IUIProperties
	{
		// Invoke SerializeProperties on the manager using the internal property builder
		virtual void Build(IPropertyManager& manager) = 0;

		// Tells the UI system to attempt to validate and copy data from the visual editor to the property manager
		virtual void UpdateFromVisuals(ControlPropertyId id, IPropertyManager& manager) = 0;

		// Try to get the latest edited string for the given property
		virtual bool TryGetEditorString(cstr propertyIdentifier, OUT Rococo::Strings::HString& value) = 0;
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
		virtual void OnEditorChanged(ControlPropertyId id) = 0;
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