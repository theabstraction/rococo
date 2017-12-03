#include <rococo.mplat.h>

namespace ANON
{
	using namespace Rococo;
	using namespace Rococo::Graphics;

	struct Messaging : public IMessagingSupervisor
	{
		Platform* platform = nullptr;

		void Log(const fstring& message) override
		{
			platform->gui.LogMessage("%s", (cstr)message);
		}

		void PostCreate(Platform& platform)  override
		{
			this->platform = &platform;
		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo
{
	namespace Graphics
	{
		IMessagingSupervisor* CreateMessaging()
		{
			return new ANON::Messaging();
		}
	}
}