#pragma once

#include <rococo.types.h>

namespace Rococo::Strings
{
	class HString;
}

// Abstract Editor - namespace for the property+palette+blank-slate GUI
namespace Rococo::Abedit
{
	// The system dependent widget id for a given editor control associated with a property.
	struct ControlPropertyId
	{
		uint16 value;
	};

	struct IEstateAgent;

	// Generally a variable field of some object
	ROCOCO_INTERFACE IProperty
	{
		// The unique identifier URL for the property
		virtual [[nodiscard]] cstr Id() const = 0;

		// Test that the widget system id for the property editor matches the internally assigned id
		virtual [[nodiscard]] bool IsForControl(ControlPropertyId id) const = 0;

		// The widget system id for the control.
		virtual [[nodiscard]] ControlPropertyId ControlId() const = 0;

		// Attempts to get the editor string
		virtual [[nodiscard]] bool TryGetEditorString(REF Rococo::Strings::HString& value) = 0;

		// Indicates that the property has potentially been changed in the editor since the last commit
		virtual [[nodiscard]] bool IsDirty() const = 0;

		// Refresh the view of the property data
		virtual void UpdateWidget(const void* data, size_t sizeOfData) = 0;
	};

	// When a property is visited, the editor associated with the property can raise events. These are responded to by the implementors of this interface
	ROCOCO_INTERFACE IPropertyUIEvents
	{
		virtual void OnBooleanButtonChanged(IProperty& property) = 0;
		virtual void OnPropertyEditorLostFocus(IProperty& property) = 0;
		virtual void OnDependentVariableChanged(cstr propertyId, IEstateAgent& agent) = 0;
	};

	//  The value-type dependent arguments of property visitation
	template<typename VALUE_TYPE>
	struct UIPrimitiveMarshaller
	{
		VALUE_TYPE& value;
		Rococo::Validators::IValueValidator<VALUE_TYPE>& validator;
		Rococo::Validators::IValueFormatter<VALUE_TYPE>& formatter;
	};

	// Generates the value-type dependent arguments of property visitation without having to explicitly set the type, it is inferred from the leading argument
	template<typename VALUE_TYPE>
	UIPrimitiveMarshaller<VALUE_TYPE> Marshal(REF VALUE_TYPE& value, Rococo::Validators::IValueValidator<VALUE_TYPE>& validator, Rococo::Validators::IValueFormatter<VALUE_TYPE>& formatter)
	{
		return UIPrimitiveMarshaller<VALUE_TYPE> { value, validator, formatter };
	}

	// The value-type invariant arguments of property visitation
	struct UIPropertyMarshallingStub
	{
		cstr propertyIdentifier;
		cstr displayName;
		IPropertyUIEvents& eventHandler;
	};

	// A visitor to an agent is informed of the properties of the agent via these methods
	// They can be used to serialize data and to sync widgets
	ROCOCO_INTERFACE IPropertyVisitor
	{
		virtual void VisitHeader(cstr propertyId, cstr displayName, cstr displayText) = 0;

		// return true if the visitor is writing data from the visual editors to the target variables
		virtual bool IsWritingToReferences() const = 0;

		// Target a variable to visit
		virtual void VisitProperty(UIPropertyMarshallingStub& stub, Rococo::Strings::HString& value, int capacity) = 0;

		// Target a variable to visit
		virtual void VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<int32>& marshaller) = 0;

		// Target a variable to visit
		virtual void VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<uint32>& marshaller) = 0;

		// Target a variable to visit
		virtual void VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<int64>& marshaller) = 0;

		// Target a variable to visit
		virtual void VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<uint64>& marshaller) = 0;

		// Target a variable to visit
		virtual void VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<float>& marshaller) = 0;

		// Target a variable to visit
		virtual void VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<double>& marshaller) = 0;

		// Target a variable to visit
		virtual void VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<bool>& marshaller) = 0;
	};

	// Handles property vistors
	ROCOCO_INTERFACE IEstateAgent
	{
		virtual void AcceptVisit(IPropertyVisitor& visitor, IPropertyUIEvents & eventHandler) = 0;
	};

	ROCOCO_INTERFACE IPropertyVenue
	{
		virtual void VisitVenue(IPropertyVisitor& visitor) = 0;
	};

	ROCOCO_INTERFACE IUIProperties
	{
		// Invoke VisitVenue on the venue using the internal property builder
		virtual void BuildEditorsForProperties(IPropertyVenue& venue) = 0;

		// Tells the UI system to attempt to validate and copy data from the visual editor for the specified property to the venue
		virtual void UpdateFromVisuals(IProperty& p, IPropertyVenue& venue) = 0;

		// Try to get the latest edited string for the given property
		virtual bool TryGetEditorString(cstr propertyIdentifier, OUT Rococo::Strings::HString& value) = 0;

		// Tell the editor that an agent's property has changed and it should update the associated editor/view to reflect the change
		virtual void Refresh(cstr onlyThisPropertyId, IEstateAgent& agent) = 0;
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
		virtual void OnButtonClicked(ControlPropertyId id) = 0;
		virtual void OnEditorChanged(ControlPropertyId id) = 0;
		virtual void OnEditorLostKeyboardFocus(ControlPropertyId id) = 0;
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
		virtual [[nodiscard]] bool IsVisible() const = 0;
		virtual [[nodiscard]] IUIBlankSlate& Slate() = 0;
		virtual [[nodiscard]] IUIPalette& Palette() = 0;
		virtual [[nodiscard]] IUIProperties& Properties() = 0;
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

// MARSHAL_PRIMITIVE tells the visitor about the specified variable. 
// [id] is the unique url that is used in event callback to identify what string should be updated
// [displayName] gives the English text describing the field 
// [eventHandler] receives events from the editors associated with visitation
// [capacity] gives the maximum length to which the string can be expanded
// [validator] prevents the editor allowing incorrect data to be entered
// [formatter] converts the variable to the correct string representation in the editor
#define MARSHAL_PRIMITIVE(visitor, id, displayName, eventHandler, variableRef, validator, formatter) \
{ \
	UIPropertyMarshallingStub pm_stub { id, displayName, eventHandler }; \
	auto pm_marshal = Marshal(REF variableRef, validator, formatter); \
	visitor.VisitProperty(pm_stub, pm_marshal); \
}

// MARSHAL_STRING tells the visitor about the specified variable. 
// [id] is the unique url that is used in event callback to identify what string should be updated
// [displayName] gives the English text describing the field 
// [eventHandler] receives events from the editor
// [variableRef] is updated if the marshaller is writing data from the vistor or read if writing data to the visitor
// [capacity] gives the maximum length to which the string can be expanded
#define MARSHAL_STRING(visitor, id, displayName, eventHandler, variableRef, capacity) \
{ \
	UIPropertyMarshallingStub pm_stub { id, displayName, eventHandler }; \
	visitor.VisitProperty(pm_stub, REF variableRef, capacity); \
}	