#include <rococo.reflector.h>

using namespace Rococo;
using namespace Rococo::Reflection;

struct TestStruct: IReflectionTarget
{
	int32 iField = 7;
	float32 f32Field = 19.0f;
	float64 f64Field = -38.0;
	char bufferField[256];
	bool bField = false;
	int64 lField = 192;

	void Visit(IReflectionVisitor& v) override
	{
		v.SetSection("TestStruct");
		ROCOCO_REFLECT(v, iField);
		ROCOCO_REFLECT(v, f32Field);
		ROCOCO_REFLECT(v, f64Field);
		ROCOCO_REFLECT(v, bField);
		ROCOCO_REFLECT(v, lField);
	}
} s_TestStruct;

namespace MHost
{
	IReflectionTarget& GetTestTarget()
	{
		return s_TestStruct;
	}
}