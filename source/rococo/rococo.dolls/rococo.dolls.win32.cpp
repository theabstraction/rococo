#include <rococo.dolls.h>

namespace Rococo::Dolls
{
	namespace Impl
	{
		struct DollLoader : IDollLoaderSupervisor
		{
			DollLoader()
			{

			}

			void Free() override
			{
				delete this;
			}
		};
	}

	ROCOCO_DOLLS_API IDollLoaderSupervisor* CreateDollLoader()
	{
		return new Impl::DollLoader();
	}
}