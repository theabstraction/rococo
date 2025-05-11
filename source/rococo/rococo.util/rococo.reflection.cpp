#define ROCOCO_API __declspec(dllexport)
#include <rococo.reflector.h>
#include <limits>
#include <unordered_set>

namespace Rococo::Reflection
{
	ROCOCO_API ReflectionMetaData& ReflectionMetaData::FullRange(int32 unused)
	{
		UNUSED(unused);
		min.i32Value = std::numeric_limits<int32>().min();
		max.i32Value = std::numeric_limits<int32>().max();
		hasMinmax = true;
		return *this;
	}

	ROCOCO_API ReflectionMetaData& ReflectionMetaData::FullRange(int64 unused)
	{
		UNUSED(unused);
		min.i64Value = std::numeric_limits<int64>().min();
		max.i64Value = std::numeric_limits<int64>().max();
		hasMinmax = true;
		return *this;
	}

	ROCOCO_API ReflectionMetaData& ReflectionMetaData::FullRange(float unused)
	{
		UNUSED(unused);
		min.f32Value = std::numeric_limits<float>().min();
		max.f32Value = std::numeric_limits<float>().max();
		hasMinmax = true;
		return *this;
	}

	ROCOCO_API ReflectionMetaData& ReflectionMetaData::FullRange(double unused)
	{
		UNUSED(unused);
		min.f64Value = std::numeric_limits<double>().min();
		max.f64Value = std::numeric_limits<double>().max();
		hasMinmax = true;
		return *this;
	}

	struct VisitationImpl
	{
		IReflectionTarget& target;
		std::unordered_set<IReflectionVisitor*> visitors;

		VisitationImpl(IReflectionTarget& _target) : target(_target)
		{
		}

		~VisitationImpl()
		{
		}
	};

	ROCOCO_API Visitation::Visitation(IReflectionTarget& target) : impl(new VisitationImpl(target))
	{

	}

	ROCOCO_API Visitation::~Visitation()
	{
		for (auto* v : impl->visitors)
		{
			v->CancelVisit(*this);
		}
		delete impl;
	}

	ROCOCO_API bool Visitation::AcceptVisitor(IReflectionVisitor& visitor)
	{
		auto i = impl->visitors.insert(&visitor);
		return i.second;
	}

	ROCOCO_API void Visitation::OnVisitorGone(IReflectionVisitor& visitor)
	{
		impl->visitors.erase(&visitor);
	}

	ROCOCO_API IReflectionTarget& Visitation::Target()
	{
		return impl->target;
	}
}