#include "meshes.h"

namespace
{
	using namespace Rococo;

	class MeshLoader : public IMeshLoader
	{
		IRenderer& renderer;
		IOS& os;
	public:
		MeshLoader(IRenderer& _renderer, IOS& _os): renderer(_renderer), os(_os)
		{

		}

		virtual ID_MESH LoadMesh(const wchar_t* resourcePath)
		{
			AutoFree<IExpandingBuffer> meshFileImage(CreateExpandingBuffer(64 + 1024));
			os.LoadResource(resourcePath, *meshFileImage, 64 * 1024);
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
	IMeshLoader* CreateMeshLoader(IRenderer& renderer, IOS& os)
	{
		return new MeshLoader(renderer, os);
	}
}