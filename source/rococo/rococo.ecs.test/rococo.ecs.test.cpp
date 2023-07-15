#include <rococo.types.h>
#include <stdio.h>
#include <components/rococo.components.animation.h>
#include <rococo.ecs.h>
#include <rococo.os.h>

using namespace Rococo;
using namespace Rococo::Components;

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
			Throw(0, "%s line %d: %s", functionName, lineNumber, message);
		}
	} ecsErrorHandler;


	Rococo::OS::SetBreakPoints(Rococo::OS::BreakFlag_All);
	//	Rococo::OS::SetBreakPoints(Rococo::OS::BreakFlag_None);

	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);

	try
	{
		AutoFree<IECSSupervisor> ecs = CreateECS(ecsErrorHandler, 32_megabytes);
		API::ForIAnimationComponent::LinkToECS(*ecs);
		auto id = ecs->NewROID();
		auto animation = API::ForIAnimationComponent::Add(id);
		auto secondAnimation = API::ForIAnimationComponent::Get(id);
		printf("All is well\n");
	}
	catch (IException& ex)
	{
		printf("%s\n", ex.Message());
	}

	printf("Press return to quit\n");
	getc(stdin);
	return 0;
}