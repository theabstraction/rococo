#pragma once

#include <rococo.types.h>

namespace Rococo::Reflection
{
	struct IPropertyVenue;
	struct IPropertyEditor;
	struct IEstateAgent;
}

namespace Rococo::Editors
{
	ROCOCO_INTERFACE IUIPropertiesEditor
	{
		// Invoke VisitVenue on the venue using the internal property builder
		virtual void BuildEditorsForProperties(Reflection::IPropertyVenue & venue) = 0;

		// Tells the UI system to attempt to validate and copy data from the visual editor for the specified property to the venue
		virtual void UpdateFromVisuals(Reflection::IPropertyEditor& editor, Reflection::IPropertyVenue& venue) = 0;

		// Try to get the latest edited string for the given property
		[[nodiscard]] virtual bool TryGetEditorString(cstr propertyIdentifier, OUT Rococo::Strings::HString& value) = 0;

		// Tell the editor that an agent's property has changed and it should update the associated editor/view to reflect the change
		virtual void Refresh(cstr onlyThisPropertyId, Reflection::IEstateAgent& agent) = 0;
	};

	ROCOCO_INTERFACE IUIPropertiesEditorSupervisor : IUIPropertiesEditor
	{
		virtual void AdvanceSelection(UI::SysWidgetId id) = 0;
		virtual void Free() = 0;
		virtual void Layout() = 0;
		virtual void NavigateByTabFrom(UI::SysWidgetId id, int delta) = 0;
		virtual void OnButtonClicked(UI::SysWidgetId id) = 0;
		virtual void OnEditorChanged(UI::SysWidgetId id) = 0;
		virtual void OnEditorLostKeyboardFocus(UI::SysWidgetId id) = 0;
	};
}

namespace Rococo::Windows
{
	struct IParentWindowSupervisor;
	ROCOCO_WINDOWS_API Editors::IUIPropertiesEditorSupervisor* CreatePropertiesEditor(IParentWindowSupervisor& propertiesPanelArea);
}