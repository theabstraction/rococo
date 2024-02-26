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
	// Interface for controlling an ensemble of property editors. These are generally invoked outside the containing window class to populate and read visual controls from associated properties
	ROCOCO_INTERFACE IUIPropertiesEditor
	{
		// Invoke VisitVenue on the venue using the internal property builder
		virtual void BuildEditorsForProperties(Reflection::IPropertyVenue& venue) = 0;

		// Tells the UI system to attempt to validate and copy data from the visual editor for the specified property to the venue
		virtual void UpdateFromVisuals(Reflection::IPropertyEditor& sourceEditor, Reflection::IPropertyVenue& targetVenue) = 0;

		// Try to get the latest edited string for the given property
		[[nodiscard]] virtual bool TryGetEditorString(cstr propertyIdentifier, OUT Rococo::Strings::HString& value) = 0;

		// Tell the editor that an agent's property has changed and it should update the associated editor/view to reflect the change
		virtual void Refresh(cstr onlyThisPropertyId, Reflection::IEstateAgent& sourceAgent) = 0;
	};

	// Supervisor interface for controlling an ensemble of property editors. Generally these methods are called by the host window (see CreatePropertiesEditor)
	ROCOCO_INTERFACE IUIPropertiesEditorSupervisor : IUIPropertiesEditor
	{
		virtual void AdvanceSelection(UI::SysWidgetId id) = 0;
		virtual void Free() = 0;
		virtual void LayouVertically() = 0;
		virtual void NavigateByTabFrom(UI::SysWidgetId id, int delta) = 0;
		virtual void OnButtonClicked(UI::SysWidgetId id) = 0;
		virtual void OnEditorChanged(UI::SysWidgetId id) = 0;
		virtual void OnEditorLostKeyboardFocus(UI::SysWidgetId id) = 0;
	};

	enum GRID_EVENT_WHEEL_FLAGS
	{
		GRID_EVENT_WHEEL_FLAGS_NONE = 0,
		GRID_EVENT_WHEEL_FLAGS_HELD_CTRL = 0x0008,
		GRID_EVENT_WHEEL_FLAGS_HELD_LBUTTON = 0x0001,
		GRID_EVENT_WHEEL_FLAGS_HELD_R_RBUTTON = 0x0002,
		GRID_EVENT_WHEEL_FLAGS_HELD_M_BUTTON = 0x0010,
		GRID_EVENT_WHEEL_FLAGS_HELD_SHIFT = 0x0004
	};

	ROCOCO_INTERFACE IUI2DGridEvents
	{
		// Triggered when the control wheel (such as the mouse wheel) rotates a definite number of clicks. 
		// The gridEventWheelFlags is a combination of GRID_EVENT_WHEEL_FLAGS bits
		virtual void GridEvent_OnControlWheelRotated(int32 clicks, uint32 gridEventWheelFlags, Vec2i cursorPosition) = 0;
	};

	ROCOCO_INTERFACE IUI2DGridSlate
	{
		virtual double ScaleFactor() const = 0;
		virtual void SetScaleFactor(double newValue) = 0;
		virtual void SetHorizontalDomain(double left, double right) = 0;
		virtual void SetVerticalDomain(double top, double bottom) = 0;
		virtual void SetCentrePosition(double x, double y) = 0;
		virtual void SetSmallestGradation(double gradationDelta) = 0;
	};

	ROCOCO_INTERFACE IUI2DGridSlateSupervisor : IUI2DGridSlate
	{
		virtual void Free() = 0;
		virtual void ResizeToParent() = 0;
	};
}

namespace Rococo::Windows
{
	struct IParentWindowSupervisor;

	// Create a properties editor window, hosted by the propertiesPanelArea. The host needs to respond to window events and invoke IUIPropertiesEditorSupervisor method appropriately
	// An example is given in rococo.abstract.editor\abstract.editor.window.cpp
	ROCOCO_WINDOWS_API Editors::IUIPropertiesEditorSupervisor* CreatePropertiesEditor(IParentWindowSupervisor& propertiesPanelArea);
	ROCOCO_WINDOWS_API Editors::IUI2DGridSlateSupervisor* Create2DGrid(IParentWindowSupervisor& gridArea, uint32 style, Editors::IUI2DGridEvents& eventHandler);
}