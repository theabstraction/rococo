#include <Domme/Animals/ICat.hpp>
#include <rococo.sexy.api.h>
#include <rococo.ide.h>
#include <rococo.io.h>
#include <rococo.os.win32.h>
#include <rococo.allocators.h>
#include <sexy.script.h>
#include <Sexy.S-Parser.h>
#include <rococo.os.h>
#include <rococo.task.queue.h>
#include <sexy.script.exports.h>

using namespace Rococo;
using namespace Rococo::Domme;
using namespace Rococo::Script;
using namespace Rococo::Animals;
using namespace Rococo::Windows::IDE;

int MainProtected(int argc, char* argv[])
{
	UNUSED(argc);
	UNUSED(argv);

	Rococo::OS::SetBreakPoints(Rococo::OS::Flags::BreakFlag_All);

	AutoFree<IO::IOSSupervisor> io = IO::GetIOS();
	AutoFree<IO::IInstallationSupervisor> installation = IO::CreateInstallation(L"content.indicator.txt", *io);
	AutoFree<IAllocatorSupervisor> allocator = Rococo::Memory::CreateTrackingAllocator(16, 1024, "test.cat.allocator");

	AutoFree<ISourceCache> sourceCache = CreateSourceCache(*installation, *allocator);

	struct AppControl: OS::IAppControl, Tasks::ITaskQueue
	{
		void AdvanceSysMonitors() override
		{

		}

		// Returns the task queue for the main thread. Include <rococo.task.queue.h> for the definition of the full interface
		Tasks::ITaskQueue& MainThreadQueue() override
		{
			return *this;
		}

		bool isRunning = true;

		bool IsRunning() const override
		{
			return isRunning;
		}

		void ShutdownApp() override
		{
			isRunning = false;
		}

		void AddTask(Rococo::Function<void()> lambda) override
		{

		}

		bool ExecuteNext() override
		{
			return false;
		}
	} appControl;

	IDebuggerWindow* debuggerWindow = CreateDebuggerWindowForStdout(Windows::NoParent(), appControl);

	AutoFree<IScriptSystemFactory> ssFactory = CreateScriptSystemFactory_1_5_0_0();

	Rococo::Domme::ScriptingResources scripting{ *sourceCache, *debuggerWindow, *ssFactory };

	try
	{
		AutoFree<ICatSupervisor> cat = CreateCat(scripting, "!scripts/domme/cat.sxy");
		cat->MakeBiscuits(127000, 25.0f);
		cat->SV_Terminate();
	}
	catch (Sex::ParseException& ex)
	{
		printf("Error loading %s at line %d pos %d:\n\t%s", ex.Source()->Tree().Source().Name(), ex.Start().y, ex.Start().x, ex.Message());
		if (ex.ErrorCode() == 0)
		{
			return E_FAIL;
		}
		return ex.ErrorCode();
	}
	return 0;
}

int main(int argc, char* argv[])
{
	try
	{
		return MainProtected(argc, argv);
	}
	catch (IException& ex)
	{
		printf("%s\n", ex.Message());

		int err = ex.ErrorCode();
		if (err != 0)
		{
			char msg[256];
			Rococo::OS::FormatErrorMessage(msg, sizeof msg, err);
			printf("Code %d (0x%8.8X): %s", err, err, msg);
		}

		return err;
	}
}