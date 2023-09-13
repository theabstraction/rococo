#pragma once

#include <rococo.types.h>

#ifndef INCLUDED_ROCOCO_REFLECTOR
# define INCLUDED_ROCOCO_REFLECTOR
#endif

namespace Rococo::Reflection
{
	using namespace Rococo;

	template<class T>
	T& Reflect(T& a)
	{
		return a;
	}

	struct ReflectionMetaData
	{
		static ReflectionMetaData Default()
		{
			return ReflectionMetaData();
		}
	};

	enum class EReflectionDirection
	{
		READ_ONLY,
		WRITE,
		SIZE_CHECK,
	};

	struct IReflectionTarget;

	ROCOCO_INTERFACE IReflectedString
	{
		virtual int32 Length() const = 0;
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
		virtual void Reflect(cstr name, float& value, ReflectionMetaData& metaData) = 0;
		virtual void Reflect(cstr name, double& value, ReflectionMetaData& metaData) = 0;
		virtual void Reflect(cstr name, bool& value, ReflectionMetaData& metaData) = 0;
		virtual void Reflect(cstr name, IReflectedString& stringValue, ReflectionMetaData& metaData) = 0;
		virtual void SetSection(cstr instanceName) = 0;
	};

	ROCOCO_INTERFACE IReflectionTarget
	{
		virtual void Visit(IReflectionVisitor& v) = 0;
	};
}

#define ROCOCO_REFLECT(visitor, field) { auto defaultMetaData = Rococo::Reflection::ReflectionMetaData::Default(); auto value = Reflect(field); visitor.Reflect(#field, value, defaultMetaData); }
#define ROCOCO_REFLECT_EX(visitor, field, metaData) visitor.Reflect(#field, field, metaData);

#ifdef INCLUDED_ROCOCO_STRINGS
# include <rococo.strings.reflection.h>
#endif