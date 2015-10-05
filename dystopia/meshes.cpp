#include "meshes.h"

namespace
{
	using namespace Rococo;

	class MeshLoader : public IMeshLoader
	{
		IRenderer& renderer;
		IInstallation& installation;
	public:
		MeshLoader(IRenderer& _renderer, IInstallation& _installation): renderer(_renderer), installation(_installation)
		{

		}

		virtual ID_MESH LoadMesh(const wchar_t* resourcePath)
		{
			AutoFree<IExpandingBuffer> meshFileImage(CreateExpandingBuffer(64 + 1024));
			installation.LoadResource(resourcePath, *meshFileImage, 64 * 1024);
			return (size_t)-1;
		}

		virtual void Free()
		{
			delete this;
		}
	};
}

namespace Rococo
{
	IMeshLoader* CreateMeshLoader(IRenderer& renderer, IInstallation& installation)
	{
		return new MeshLoader(renderer, installation);
	}
}