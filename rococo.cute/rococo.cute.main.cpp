#include <rococo.cute.h>
#include <rococo.io.h>
#include "rococo.cute.h"

using namespace Rococo;
using namespace Rococo::Cute;

int RunMessageLoop(IMasterWindowFactory& factory);

int Main(IInstallation& installation, IMasterWindowFactory& factory)
{
	using namespace Rococo::Cute;

	ExecuteScriptSpec spec;
	ExecuteWindowScript("!scripts/create.main.window.sxy", installation, spec, factory);

	return RunMessageLoop(factory);
}
