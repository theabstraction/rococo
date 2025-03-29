#define ROCOCO_API __declspec(dllexport)
#include <rococo.reflector.h>
#include <limits>

namespace Rococo::Reflection
{
	ROCOCO_API ReflectionMetaData& ReflectionMetaData::FullRange(int32 unused)
	{
		UNUSED(unused);
		min.i32Value = std::numeric_limits<int32>().min();
		max.i32Value = std::numeric_limits<int32>().max();
		return *this;
	}

	ROCOCO_API ReflectionMetaData& ReflectionMetaData::FullRange(int64 unused)
	{
		UNUSED(unused);
		min.i64Value = std::numeric_limits<int64>().min();
		max.i64Value = std::numeric_limits<int64>().max();
		return *this;
	}

	ROCOCO_API ReflectionMetaData& ReflectionMetaData::FullRange(float unused)
	{
		UNUSED(unused);
		min.f32Value = std::numeric_limits<float>().min();
		max.f32Value = std::numeric_limits<float>().min();
		return *this;
	}

	ROCOCO_API ReflectionMetaData& ReflectionMetaData::FullRange(double unused)
	{
		UNUSED(unused);
		min.f64Value = std::numeric_limits<double>().min();
		max.f64Value = std::numeric_limits<double>().min();
		return *this;
	}
}