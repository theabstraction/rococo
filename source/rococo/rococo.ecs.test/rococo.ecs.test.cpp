#include <rococo.types.h>
#include <components/rococo.components.animation.h>
#include <components/rococo.ecs.h>
#include <rococo.os.h>
#include <rococo.time.h>

#include <stdio.h>
#include <vector>


#ifdef _WIN32
#include <rococo.os.win32.h>

void WaitASecond()
{
	Sleep(1000);
}
#else
void WaitASecond()
{
	sleep(1);
}
#endif

using namespace Rococo;
using namespace Rococo::Components;

void Validate(bool value, cstr msg, int lineNumber)
{
	if (!value)
	{
		OS::TripDebugger();
		Throw(0, "Validation failed: %s at line %d", msg, lineNumber);
	}
}

#define VALIDATE(x) Validate(x, #x, __LINE__);

void TestAllocSpeed()
{
	auto start = Time::TickCount();

	std::vector<void*> ptrs;
	for (int i = 0; i < 250'000; i++)
	{
		if ((i % 50000) == 0)
		{
			printf("Allocating %d+\n", i);
		}
		void* buffer = malloc(192);
		ptrs.push_back(buffer);
	}

	auto mid = Time::TickCount();

	for (int i = 0; i < 250'000; i++)
	{
		if ((i % 50000) == 0)
		{
			printf("Freeing %d+\n", i);
		}
		free(ptrs[i]);
	}

	auto end = Time::TickCount();

	printf("Allocation Time: %.0fns per 192 byte block\n", 1000'000'000.0f * (mid - start) / (250'000.0 * Time::TickHz()));
	printf("      Free Time: %.0fns per 192 byte block\n", 1000'000'000.0f * (end - mid) / (250'000.0 * Time::TickHz()));
	printf("Total Time: %.4fs\n", (end - start) / (double)Time::TickHz());
}

int main(int argc, char* argv[])
{
	UNUSED(argc);
	UNUSED(argv);

	//Rococo::OS::SetBreakPoints(Rococo::OS::Flags::BreakFlag_All);
	//Rococo::OS::SetBreakPoints(Rococo::OS::Flags::BreakFlag_None);

	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);

	try
	{
		struct AutoComponentRelease
		{
			~AutoComponentRelease()
			{
				ECS::ReleaseTablesForIAnimationComponent();
			}
		} autoReleaser;

		AutoFree<IECSSupervisor> ecs = CreateECS(32_megabytes);
		size_t nSlots = ecs->AvailableRoidCount();
		printf("Initialized ECS with %llu slots\n", nSlots);

		ECS::LinkToECS_IAnimationComponentTable(*ecs);
		auto id = ecs->NewROID();
		VALIDATE(id);

		auto a = API::ForIAnimationComponent::Add(id);
		VALIDATE(a);

		VALIDATE(a.Life().GetRefCount() == 1);

		auto b = API::ForIAnimationComponent::Get(id);
		VALIDATE(b);

		VALIDATE(a.Life().GetRefCount() == 2);

		b = Ref<IAnimationComponent>();

		VALIDATE(a.Life().GetRefCount() == 1);

		VALIDATE(a.Roid() == id);
		VALIDATE(!b);
		VALIDATE(!b.Roid());

		/*
		try
		{
			b = API::ForIAnimationComponent::Add(id);
			VALIDATE(false);
		}
		catch (IException& )
		{

		}
		*/

		auto id2 = ecs->NewROID();
		VALIDATE(id2);
		VALIDATE(id != id2);
		b = API::ForIAnimationComponent::Add(id2);
		VALIDATE(b);
		VALIDATE(a != b);

		int count = 0;
		auto incCount = [&count](ROID roid, IAnimationComponent& anim)->EFlowLogic
		{
			UNUSED(anim);
			VALIDATE(roid);
			count++;
			return EFlowLogic::CONTINUE;
		};

		API::ForIAnimationComponent::ForEach(incCount);

		VALIDATE(count == 2);
		VALIDATE(ecs->ActiveRoidCount() == 2);

		VALIDATE(nSlots - ecs->AvailableRoidCount() == 2);

		ecs->Deprecate(id2);
		VALIDATE(b);
		ecs->CollectGarbage();

		// We have two outstanding references a and b, thus the ROID should not be garbage collected;
		VALIDATE(ecs->ActiveRoidCount() == 1);
		VALIDATE(ecs->IsActive(id));
		VALIDATE(!ecs->IsActive(id2));
		VALIDATE(a);
		VALIDATE(b);
		VALIDATE(!a.Life().IsDeprecated());
		VALIDATE(b.Life().IsDeprecated());
		auto c = API::ForIAnimationComponent::Add(id2);
		VALIDATE(!c);
		a.Release();
		VALIDATE(!a);
		a = API::ForIAnimationComponent::Get(id);
		VALIDATE(a);
		a.Deprecate();
		VALIDATE(a.Life().IsDeprecated());
		VALIDATE(!ecs->IsActive(id));
		a.Release();
		VALIDATE(!a);
		b.Release();
		VALIDATE(!b);
		VALIDATE(!c);

		ecs->CollectGarbage();
		VALIDATE(ecs->ActiveRoidCount() == 0);

		size_t nSlotsNow = ecs->AvailableRoidCount();

		VALIDATE(nSlotsNow == nSlots);

		printf("All is well\n");
		//WaitASecond();

		return 0;
	}
	catch (IException& ex)
	{
		char err[2048];
		Rococo::OS::BuildExceptionString(err, sizeof err, ex, true);
		printf("%s\n", err);
		WaitASecond();
		return -1;
	}
}