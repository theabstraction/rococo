#pragma once

#include <rococo.types.h>

namespace Rococo::Reflection
{
	using namespace Rococo;

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

	ROCOCO_INTERFACE IReflectionVisitor
	{
		virtual EReflectionDirection Direction() const = 0;
		virtual void Reflect(cstr name, IReflectionVisitor& subTarget, ReflectionMetaData& metaData) = 0;
		virtual void Reflect(cstr name, int32 &value, ReflectionMetaData& metaData) = 0;
		virtual void Reflect(cstr name, int64& value, ReflectionMetaData& metaData) = 0;
		virtual void Reflect(cstr name, float& value, ReflectionMetaData& metaData) = 0;
		virtual void Reflect(cstr name, double& value, ReflectionMetaData& metaData) = 0;
		virtual void Reflect(cstr name, bool& value, ReflectionMetaData& metaData) = 0;
		virtual void SetSection(cstr sectionName) = 0;
	};

	ROCOCO_INTERFACE IReflectionTarget
	{
		virtual void Visit(IReflectionVisitor& v) = 0;
	};
}

#define ROCOCO_REFLECT(visitor, field) { auto defaultMetaData = Rococo::Reflection::ReflectionMetaData::Default(); visitor.Reflect(#field, field, defaultMetaData); }
#define ROCOCO_REFLECT_EX(visitor, field, metaData) visitor.Reflect(#field, field, metaData);