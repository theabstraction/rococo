#include <rococo.mvc.h>

using namespace Rococo;
using namespace Rococo::MVC;

namespace ANON
{
	struct Controller: IMVC_ControllerSupervisor
	{
		Controller(IMVC_Host& _host, cstr _commandLine)
		{
			UNUSED(_commandLine);
			UNUSED(_host);
		}

		void Free() override
		{
			delete this;
		}

		bool IsRunning() const override
		{
			return false;
		}
	};
}

// Control-Flow Graph System
namespace Rococo::CFGS
{
	IMVC_ControllerSupervisor* CreateMVCControllerInternal(IMVC_Host& host, cstr commandLine)
	{
		return new ANON::Controller(host, commandLine);
	}
}