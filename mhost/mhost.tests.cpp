#include <rococo.reflector.h>
#include <rococo.strings.h>

using namespace Rococo;
using namespace Rococo::Strings;
using namespace Rococo::Reflection;

struct TestDog : IReflectionTarget
{
	HString name;
	HString breed;
	bool isHappy;

	void Visit(IReflectionVisitor& v) override
	{
		v.SetSection("Dog");
		ROCOCO_REFLECT(v, name);
		ROCOCO_REFLECT(v, breed);
		ROCOCO_REFLECT(v, isHappy);
	}
};

struct TestKennel: IReflectionTarget
{
	int32 taxCode = 0x1891A;
	float32 cost = 2000.0f;

	TestDog dog;
	
	TestKennel()
	{
		dog.breed = "Blood Hound";
		dog.name = "Toby";
		dog.isHappy = true;
	};

	void Visit(IReflectionVisitor& v) override
	{
		v.SetSection("Kennel");
		ROCOCO_REFLECT(v, taxCode);
		ROCOCO_REFLECT(v, cost);
		ROCOCO_REFLECT(v, dog)
	}
} s_TestStruct;

namespace MHost
{
	IReflectionTarget& GetTestTarget()
	{
		return s_TestStruct;
	}
}