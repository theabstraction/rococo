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
	};

	struct Reflected_HString;

	struct ReflectionMetaData
	{
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

		MetaVariantData min;
		MetaVariantData max;

		ReflectionMetaData& Range(int32 minValue, int32 maxValue)
		{
			min.i32Value = minValue;
			max.i32Value = maxValue;
			return *this;
		}

		ReflectionMetaData& Range(int64 minValue, int64 maxValue)
		{
			min.i64Value = minValue;
			max.i64Value = maxValue;
			return *this;
		}

		ReflectionMetaData& Range(float minValue, float maxValue)
		{
			min.f32Value = minValue;
			max.f32Value = maxValue;
			return *this;
		}

		ReflectionMetaData& Range(double minValue, double maxValue)
		{
			min.f64Value = minValue;
			max.f64Value = maxValue;
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

	ROCOCO_INTERFACE IReflectedString
	{
		virtual uint32 Capacity() const = 0;
		virtual cstr ReadString() const = 0;
		virtual void WriteString(cstr s) = 0;
	};

	ROCOCO_INTERFACE IReflectionVisitor
	{
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
		virtual void Reflect(cstr name, IReflectedString& stringValue, ReflectionMetaData& metaData) = 0;
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

	// Adds method virtual void Visit(IReflectionVisitor& v)
	ROCOCO_INTERFACE IReflectionTarget
	{
		virtual void Visit(IReflectionVisitor& v) = 0;
	};

	template<class T>
	T& Reflect(T& a)
	{
		return a;
	}
}

#define ROCOCO_REFLECT(visitor, field) { auto value = Rococo::Reflection::Reflect(field); auto defaultMetaData = Rococo::Reflection::ReflectionMetaData::Default().FullRange(value); visitor.Reflect(#field, value, defaultMetaData); }
#define ROCOCO_REFLECT_EX(visitor, field, metaData) visitor.Reflect(#field, field, metaData);
#define ROCOCO_REFLECT_READ_ONLY(visitor, field) { auto value = Rococo::Reflection::Reflect(field); auto& readOnlyMetaData = Rococo::Reflection::ReflectionMetaData::ReadOnly().FullRange(value); visitor.Reflect(#field, value, readOnlyMetaData); }

#ifdef INCLUDED_ROCOCO_STRINGS
# include <rococo.strings.reflection.h>
#endif
