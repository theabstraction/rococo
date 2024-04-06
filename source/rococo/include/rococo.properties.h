#pragma once

#include <rococo.types.h>

namespace Rococo::Reflection
{
	struct IEstateAgent;

	// Generally an editor associated with a property of some object
	ROCOCO_INTERFACE IPropertyEditor
	{
		// The unique identifier URL for the property
		virtual [[nodiscard]] cstr Id() const = 0;

		// Test that the widget system id for the property editor matches the internally assigned id
		virtual [[nodiscard]] bool IsForControl(UI::SysWidgetId id) const = 0;

		// The widget system id for the control/editor associated with the property
		virtual [[nodiscard]] UI::SysWidgetId ControlId() const = 0;

		// Attempts to get the editor string for the live editor associated with the property
		virtual [[nodiscard]] bool TryGetEditorString(REF Rococo::Strings::HString& value) = 0;

		// Indicates that the property has potentially been changed in the editor since the last commit
		virtual [[nodiscard]] bool IsDirty() const = 0;

		// Refresh the live editor associated with the property data
		virtual void UpdateWidget(const void* data, size_t sizeOfData) = 0;
	};

	ROCOCO_INTERFACE IEnumVector
	{
		// Returns the number of elements in the enumeration
		[[nodiscard]] virtual size_t Count() const = 0;

		// Populates the ith enum name. Returns true if i is within bounds
		/* discardable */ virtual bool GetEnumName(size_t i, Strings::IStringPopulator& populator) const = 0;

		// Populates the ith enum description or not if i is out of bounds. Returns true if i is within bounds
		/* discardable */ virtual bool GetEnumDescription(size_t i, Strings::IStringPopulator& populator) const = 0;
	};

	ROCOCO_INTERFACE IEnumVectorSupervisor : IEnumVector
	{
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE IEnumDescriptor
	{
		// get or create an enum list. IEnumListSupervisor::Free() is called to free the list when the caller no longer needs it
		virtual IEnumVectorSupervisor * CreateEnumList() = 0;
	};

	struct OptionRef
	{
		Rococo::Strings::HString& value;
	};

	//  The value-type dependent arguments of property visitation
	template<typename VALUE_TYPE>
	struct PrimitiveMarshaller
	{
		VALUE_TYPE& value;
		Rococo::Validators::IValueValidator<VALUE_TYPE>& validator;
		Rococo::Validators::IValueFormatter<VALUE_TYPE>& formatter;
	};

	// Generates the value-type dependent arguments of property visitation without having to explicitly set the type, it is inferred from the leading argument
	template<typename VALUE_TYPE>
	PrimitiveMarshaller<VALUE_TYPE> Marshal(REF VALUE_TYPE& value, Rococo::Validators::IValueValidator<VALUE_TYPE>& validator, Rococo::Validators::IValueFormatter<VALUE_TYPE>& formatter)
	{
		return PrimitiveMarshaller<VALUE_TYPE> { value, validator, formatter };
	}

	ROCOCO_INTERFACE IArrayProperty
	{
		virtual void Append() = 0;
	};

	// When a property is visited, an editor associated with the property may raise events. These are responded to by the implementors of this interface
	ROCOCO_INTERFACE IPropertyUIEvents
	{
		virtual void OnArrayEvent(cstr arrayId, Function<void(IArrayProperty&)> callback) = 0;
		virtual void OnBooleanButtonChanged(IPropertyEditor & property) = 0;
		virtual void OnPropertyEditorLostFocus(IPropertyEditor& property) = 0;
		virtual void OnDependentVariableChanged(cstr propertyId, IEstateAgent& agent) = 0;
	};

	// The value-type invariant arguments of property visitation
	struct PropertyMarshallingStub
	{
		// A unique id string that can be used to look-up and identify the exact property specified in the stub
		cstr propertyIdentifier;

		// Friendly name for a property, appears as a label for the property in a user-interface systems 
		cstr displayName;

		// In the case visitation is performed by a user-interface system, events associated with the property editors are handled by the interface here
		IPropertyUIEvents& eventHandler;
	};

	struct ArrayHeaderControl
	{
		uint64 minElements;
		uint64 maxElements;
		bool isExpandable: 1;
		bool isElementDeleteSupported: 1;
		bool isOrderImmutable : 1;
	};

	// A visitor to an agent is informed of the properties of the agent via these methods
	// They can be used to serialize data and to sync widgets
	ROCOCO_INTERFACE IPropertyVisitor
	{
		virtual void VisitHeader(cstr propertyId, cstr displayName, cstr displayText) = 0;

		// return true if the visitor is writing data from the visual editors to the target variables
		[[nodiscard]] virtual bool IsWritingToReferences() const = 0;

		// Target a variable to visit
		virtual void VisitProperty(PropertyMarshallingStub& stub, Rococo::Strings::HString& value, int capacity) = 0;

		// Target a variable to visit
		virtual void VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<int32>& marshaller) = 0;

		// Target a variable to visit
		virtual void VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<uint32>& marshaller) = 0;

		// Target a variable to visit
		virtual void VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<int64>& marshaller) = 0;

		// Target a variable to visit
		virtual void VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<uint64>& marshaller) = 0;

		// Target a variable to visit
		virtual void VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<float>& marshaller) = 0;

		// Target a variable to visit
		virtual void VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<double>& marshaller) = 0;

		// Target a variable to visit
		virtual void VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<bool>& marshaller) = 0;

		// Target an option to visit, also provides an option list
		virtual void VisitOption(PropertyMarshallingStub& stub, REF OptionRef& value, int stringCapacity, IEnumDescriptor& enumDesc) = 0;

		virtual void BeginArray(PropertyMarshallingStub& arrayStub, const ArrayHeaderControl& control) = 0;

		// Exits the current array section
		virtual void EndArray() = 0;

		// Adds a new section based on an integer index
		virtual void BeginIndex(int index) = 0;

		// Exits the current index section
		virtual void EndIndex() = 0;
	};

	// An object that handles property vistors. Can be thought of as 'the object to be serialized'. Event handler events may be triggered by the visitation
	ROCOCO_INTERFACE IEstateAgent
	{
		virtual void AcceptVisit(IPropertyVisitor& visitor, IPropertyUIEvents& eventHandler) = 0;
	};

	// Provides a method for visitation
	ROCOCO_INTERFACE IPropertyVenue
	{
		virtual void VisitVenue(IPropertyVisitor& visitor) = 0;
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
	PropertyMarshallingStub pm_stub { id, displayName, eventHandler }; \
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
	PropertyMarshallingStub pm_stub { id, displayName, eventHandler }; \
	visitor.VisitProperty(pm_stub, REF variableRef, capacity); \
}	

namespace Rococo::Reflection
{
	template<typename ARRAY>
	inline size_t GetArrayLength(const ARRAY& theArray)
	{
		return theArray.size();
	}

	template<typename ARRAY>
	inline auto& GetArrayElementRef(ARRAY& theArray, size_t index)
	{
		return theArray[index];
	}

	template<typename ARRAY>
	inline auto* GetArrayElementPointer(ARRAY& theArray, size_t index)
	{
		return theArray[index];
	}

	// Invokes element.AcceptVisit(visitor, index, eventHandler)
	template<typename ELEMENT>
	struct ELEMENT_VISITOR_BY_METHOD
	{
		void AcceptVisitor(IPropertyVisitor& visitor, ELEMENT& element, size_t index, IPropertyUIEvents& eventHandler)
		{
			element.AcceptVisitor(visitor, index, eventHandler);
		}
	};

	template<typename ARRAY, typename ELEMENT_VISITOR>
	inline void VisitObjectArray(IPropertyVisitor& visitor, IPropertyUIEvents& eventHandler, ARRAY& theArray, ELEMENT_VISITOR& elementVisitor)
	{
		size_t arrayLength = GetArrayLength(theArray);
		for (size_t i = 0; i < arrayLength; i++)
		{
			auto& elementRef = GetArrayElementRef(theArray, i);
			elementVisitor.AcceptVisit(visitor, elementRef, i, eventHandler);
		}
	}

	template<typename ARRAY>
	inline void VisitPointerArray(IPropertyVisitor& visitor, IPropertyUIEvents& eventHandler, ARRAY& theArray)
	{
		size_t arrayLength = GetArrayLength(theArray);
		for (size_t i = 0; i < arrayLength; i++)
		{
			auto* elementPtr = GetArrayElementPointer(theArray, i);
			if (elementPtr)
			{
				elementPtr->AcceptVisitor(visitor, i, eventHandler);
			}
		}
	}
}
