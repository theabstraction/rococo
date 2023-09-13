#include <rococo.types.h>
#define ROCOCO_ASSETS_API ROCOCO_API_EXPORT

#include <rococo.assets.h>

namespace ANON
{
	using namespace Rococo::Assets;

	struct AssetManager : IAssetManagerSupervisor
	{
		AssetManager()
		{

		}

		virtual ~AssetManager()
		{

		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo::Assets
{
	ROCOCO_ASSETS_API IAssetManagerSupervisor* CreateAssetManager()
	{
		return new ANON::AssetManager();
	}
}