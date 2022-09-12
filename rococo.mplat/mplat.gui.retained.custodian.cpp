#include <rococo.gui.retained.h>

using namespace Rococo::Gui;

namespace ANON
{
	struct Custodian : IGuiRetainedCustodianSupervisor
	{
		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo::Gui
{
	IGuiRetainedCustodianSupervisor* CreateCustodian()
	{
		return new ANON::Custodian();
	}
}