#pragma once

#include <rococo.types.h>

#ifndef INCLUDED_ROCOCO_REFLECTOR
# define INCLUDED_ROCOCO_REFLECTOR
#endif

namespace Rococo::Reflection
{
	struct ReflectionMetaData
	{
		bool isReadOnly = false;

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
	};

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
		Section(cstr sectionName, IReflectionVisitor& _visitor) : visitor(_visitor)
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
		Container(cstr name, IReflectionVisitor& _visitor) : visitor(_visitor)
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
		Element(cstr name, IReflectionVisitor& _visitor) : visitor(_visitor)
		{
			visitor.EnterElement(name);
		}

		~Element()
		{
			visitor.LeaveElement();
		}
	};

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

#define ROCOCO_REFLECT(visitor, field) { auto defaultMetaData = Rococo::Reflection::ReflectionMetaData::Default(); auto value = Reflect(field); visitor.Reflect(#field, value, defaultMetaData); }
#define ROCOCO_REFLECT_EX(visitor, field, metaData) visitor.Reflect(#field, field, metaData);
#define ROCOCO_REFLECT_READ_ONLY(visitor, field) { auto readOnlyMetaData = Rococo::Reflection::ReflectionMetaData::ReadOnly(); auto value = Reflect(field); visitor.Reflect(#field, value, readOnlyMetaData); }

#ifdef INCLUDED_ROCOCO_STRINGS
# include <rococo.strings.reflection.h>
#endif
