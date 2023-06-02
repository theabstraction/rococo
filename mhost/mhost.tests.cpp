#include <rococo.reflector.h>
#include <rococo.strings.h>
#include <stdlib.h>
#include <vector>

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

struct TestKennel : IReflectionTarget
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
};

struct TestElement : IReflectionTarget
{
	HString name;
	int id;

	TestElement()
	{
		char buf[128];
		SafeFormat(buf, "Geoff #%d", rand());

		name = buf;

		id = rand();
	}

	void Visit(IReflectionVisitor& v) override
	{
		v.SetSection("Geoff");
		ROCOCO_REFLECT(v, name);
		ROCOCO_REFLECT(v, id);
	}
};

template<class T>
void Visit(IReflectionVisitor& v, T& t)
{
	t.Visit(v);
}

template<class T>
void Reflect(IReflectionVisitor& v, T& elements, const char* name)
{
	v.EnterContainer();
	int i = 0;
	for (auto& element : elements)
	{
		char index[16];
		SafeFormat(index, "%d", i++);
		v.EnterElement(index);
		Visit(v, element);
		v.LeaveElement();
	}
	v.LeaveContainer();
}

struct TestHouse : IReflectionTarget
{
	HString houseName = "BigBen";
	int houseNumber = 631;
	HString streetAddress = "Cheshire Street";
	HString town = "Nuneaton";
	HString postcode = "N01 0NN";

	TestKennel kennel;

	std::vector<TestElement> geoffs;

	TestHouse()
	{
		geoffs.resize(20);
	}

	void Visit(IReflectionVisitor& v) override
	{
		v.SetSection("House");
		ROCOCO_REFLECT(v, houseName);
		ROCOCO_REFLECT(v, houseNumber);
		ROCOCO_REFLECT(v, streetAddress);
		ROCOCO_REFLECT(v, town);
		ROCOCO_REFLECT(v, postcode);
		ROCOCO_REFLECT(v, kennel)
		Reflect(v, geoffs, "geoffs");
	}
} s_TestStruct;

namespace MHost
{
	IReflectionTarget& GetTestTarget()
	{
		return s_TestStruct;
	}
}