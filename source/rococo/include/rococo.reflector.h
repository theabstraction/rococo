#pragma once

#include <rococo.types.h>

#ifndef INCLUDED_ROCOCO_REFLECTOR
# define INCLUDED_ROCOCO_REFLECTOR
#endif

namespace Rococo::Reflection
{
	union MetaVariantData
	{
		int32 i32Value;
		int64 i64Value;
		float f32Value;
		double f64Value;

		MetaVariantData(): i64Value(0)
		{

		}
	};

	struct Reflected_HString;

	struct ReflectionMetaData
	{
		MetaVariantData min;
		MetaVariantData max;
		bool hasMinmax = false;
		bool isReadOnly = false;

		ReflectionMetaData()
		{
			
		}

		static ReflectionMetaData Default()
		{
			return ReflectionMetaData();
		}

		static ReflectionMetaData ReadOnly()
		{
			ReflectionMetaData meta;
			meta.isReadOnly = true;
			return meta;
		}

		bool addThousandMarks = false;

		ReflectionMetaData& AddThousandMarks()
		{
			addThousandMarks = true;
			return *this;
		}

		ReflectionMetaData& Range(int32 dummy, int32 minValue, int32 maxValue)
		{
			UNUSED(dummy);
			min.i32Value = minValue;
			max.i32Value = maxValue;
			hasMinmax = true;
			return *this;
		}

		ReflectionMetaData& Range(int64 dummy, int64 minValue, int64 maxValue)
		{
			UNUSED(dummy);
			min.i64Value = minValue;
			max.i64Value = maxValue;
			hasMinmax = true;
			return *this;
		}

		ReflectionMetaData& Range(float dummy, float minValue, float maxValue)
		{
			UNUSED(dummy);
			min.f32Value = minValue;
			max.f32Value = maxValue;
			hasMinmax = true;
			return *this;
		}

		ReflectionMetaData& Range(double dummy, double minValue, double maxValue)
		{
			UNUSED(dummy);
			min.f64Value = minValue;
			max.f64Value = maxValue;
			hasMinmax = true;
			return *this;
		}

		ROCOCO_API ReflectionMetaData& FullRange(int32 unused);
		ROCOCO_API ReflectionMetaData& FullRange(int64 unused);
		ROCOCO_API ReflectionMetaData& FullRange(float unused);
		ROCOCO_API ReflectionMetaData& FullRange(double unused);

		template<class T>
		ReflectionMetaData& FullRange(T& unused)
		{
			UNUSED(unused);
			return *this;
		}

		int precision = 3;

		// Precision is typically clamped between 0 to 9 decimal places
		ReflectionMetaData& Precision(int value)
		{
			precision = value;
			return *this;
		}
	};

	inline ReflectionMetaData Mutable()
	{
		return ReflectionMetaData::Default();
	}

	inline ReflectionMetaData ReadOnly()
	{
		return ReflectionMetaData::ReadOnly();
	}

	enum class EReflectionDirection
	{
		// The reflection target's data is being read
		READ_ONLY,

		// The reflection target is expected to update its data
		WRITE,

		// The reflection target is expected to guess the size of its data
		SIZE_CHECK
	};

	ROCOCO_INTERFACE IReflectionVisitor
	{
		// Tell the visitor that the reflection target for the visit is no longer a valid pointer and cannot be used for further reads or writes
		// Usually invoked by the destructor of the reflection target.
		virtual void CancelVisit(IReflectionVisitation& visitation) = 0;

		virtual EReflectionDirection Direction() const = 0;
		virtual void EnterContainer(cstr name) = 0;
		virtual void LeaveContainer() = 0;
		virtual void EnterElement(cstr elementKey) = 0;
		virtual void LeaveElement() = 0;
		virtual void Reflect(cstr name, IReflectionTarget& subTarget, ReflectionMetaData& metaData) = 0;
		virtual void Reflect(cstr name, int32 &value, ReflectionMetaData& metaData) = 0;
		virtual void Reflect(cstr name, int64& value, ReflectionMetaData& metaData) = 0;
		virtual void Reflect(cstr name, uint64& value, ReflectionMetaData& metaData) = 0;
		virtual void Reflect(cstr name, float& value, ReflectionMetaData& metaData) = 0;
		virtual void Reflect(cstr name, double& value, ReflectionMetaData& metaData) = 0;
		virtual void Reflect(cstr name, bool& value, ReflectionMetaData& metaData) = 0;
		virtual void Reflect(cstr name, Strings::HString& stringRef, ReflectionMetaData& metaData) = 0;
		virtual void Reflect(cstr name, char* buffer, size_t capacity, ReflectionMetaData& metaData) = 0;

		template<size_t CAPACITY>
		inline void Reflect(cstr name, char(&buffer)[CAPACITY], ReflectionMetaData& metaData)
		{
			return Reflect(name, buffer, CAPACITY, metaData);
		}

		virtual void EnterSection(cstr sectionName) = 0;
		virtual void LeaveSection() = 0;
	};

	class Section
	{
		IReflectionVisitor& visitor;
	public:
		Section(IReflectionVisitor& _visitor, cstr sectionName) : visitor(_visitor)
		{
			visitor.EnterSection(sectionName);
		}

		~Section()
		{
			visitor.LeaveSection();
		}
	};

	class Container
	{
		IReflectionVisitor& visitor;
	public:
		Container(IReflectionVisitor& _visitor, cstr name) : visitor(_visitor)
		{
			visitor.EnterContainer(name);
		}

		~Container()
		{
			visitor.LeaveContainer();
		}
	};

	class Element
	{
		IReflectionVisitor& visitor;
	public:
		Element(IReflectionVisitor& _visitor, cstr name) : visitor(_visitor)
		{
			visitor.EnterElement(name);
		}

		~Element()
		{
			visitor.LeaveElement();
		}
	};

	// A Reflection Visitation occurs when an IReflectionVisitor intends to hold on to a reference to the 
	// reflection target passed in method Visit(...). An example for such a requirement is when a property editor
	// allows the user to make changes to a target after it has been read.
	// We could alternatively have used reference counting or a callback mechanism to achieve the same end, but 
	// I believed the implementation would not be as efficient, requiring substantially more work for the consumer
	// of the API.
	ROCOCO_INTERFACE IReflectionVisitation
	{
		virtual bool AcceptVisitor(IReflectionVisitor& visitor) = 0;
		virtual void OnVisitorGone(IReflectionVisitor& visitor) = 0;
		virtual IReflectionTarget& Target() = 0;
	};

	// Provides a method for telling a reflection visitor for both its internal state and capacity to handle ongoing visitation
	ROCOCO_INTERFACE IReflectionTarget
	{
		// Informs the visitor of the target's internal state by calling visitor methods for each of its fields
		virtual void Visit(IReflectionVisitor& visitor) = 0;

		// Returns the visitation object should the target support perisistent/ongoing modification by the visitor
		virtual IReflectionVisitation* Visitation() = 0;
	};

	template<class T>
	T& Reflect(T& a)
	{
		return a;
	}

	struct VisitationImpl;

	// A visitation implementation token, meant for classes to implement IReflectionVisitation with little work
	// Note that when the visitation object is destructed all visitors are notified via IReflectionVisitor::CancelVisit(IReflectionTarget* target)
	class Visitation: public IReflectionVisitation
	{
		VisitationImpl* impl;
	public:
		ROCOCO_API Visitation(IReflectionTarget& _target);
		ROCOCO_API virtual ~Visitation();

		ROCOCO_API bool AcceptVisitor(IReflectionVisitor& visitor) override;
		ROCOCO_API void OnVisitorGone(IReflectionVisitor& visitor) override;
		ROCOCO_API IReflectionTarget& Target();
	};

	// A visitation base class, adds the visitation token, but leaves Visit(... visitor) to be implemented by derived classes
	class VisitationTarget : public IReflectionTarget
	{
	private:
		Reflection::Visitation visitation;

	public:
		VisitationTarget() : visitation(*this)
		{

		}

		virtual ~VisitationTarget()
		{

		}

		IReflectionVisitation* Visitation() override
		{
			return &visitation;
		}
	};
}

#define ROCOCO_REFLECT(visitor, field) { auto& value = Rococo::Reflection::Reflect(field); auto defaultMetaData = Rococo::Reflection::ReflectionMetaData::Default().FullRange(value); visitor.Reflect(#field, value, defaultMetaData); }
#define ROCOCO_REFLECT_EX(visitor, field, metaData) visitor.Reflect(#field, field, metaData);
#define ROCOCO_REFLECT_READ_ONLY(visitor, field) { auto& value = Rococo::Reflection::Reflect(field); auto& readOnlyMetaData = Rococo::Reflection::ReflectionMetaData::ReadOnly().FullRange(value); visitor.Reflect(#field, value, readOnlyMetaData); }

#ifdef INCLUDED_ROCOCO_STRINGS
# include <rococo.strings.reflection.h>
#endif
