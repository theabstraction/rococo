#include <rococo.types.h>
#include <stdio.h>
#include <components/rococo.components.animation.h>
#include <rococo.ecs.h>
#include <rococo.os.h>

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

int main(int argc, char* argv[])
{
	UNUSED(argc);
	UNUSED(argv);

	struct ECSErrorHandler : IECSErrorHandler
	{
		void OnError(cstr functionName, int lineNumber, cstr message, bool expectedToThrow, ECS_ErrorCause cause) override
		{
			UNUSED(cause);
			UNUSED(expectedToThrow);
			printf("Bad karma: %s #%d: %s\n", functionName, lineNumber, message);
			Throw(0, "%s line %d: %s", functionName, lineNumber, message);
		}
	} ecsErrorHandler;


	//Rococo::OS::SetBreakPoints(Rococo::OS::BreakFlag_All);
	//	Rococo::OS::SetBreakPoints(Rococo::OS::BreakFlag_None);

	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);

	try
	{
		AutoFree<IECSSupervisor> ecs = CreateECS(ecsErrorHandler, 32_megabytes);
		API::ForIAnimationComponent::LinkToECS(*ecs);
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

		try
		{
			b = API::ForIAnimationComponent::Add(id);
			VALIDATE(false);
		}
		catch (IException& )
		{

		}

		auto id2 = ecs->NewROID();
		VALIDATE(id2);
		VALIDATE(id != id2);
		b = API::ForIAnimationComponent::Add(id2);
		VALIDATE(b);
		VALIDATE(a != b);

		auto lambda = [](ROID roid, IAnimationComponent& anim)->EFlowLogic
		{
			UNUSED(anim);
			VALIDATE(roid);
			return EFlowLogic::CONTINUE;
		};

		API::ForIAnimationComponent::ForEach(lambda);
		
		printf("All is well\n");
		WaitASecond();
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