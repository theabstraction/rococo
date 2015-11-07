#include "dystopia.h"
#include "meshes.h"

namespace
{
	using namespace Rococo;
	using namespace Dystopia;

	ID_MESH GenNextHouseId()
	{
		static ID_MESH nextId(0x21000000);
		nextId = ID_MESH(nextId.value + 1);
		return nextId;
	}
}

namespace Dystopia
{
	ID_MESH GenerateRandomHouse(Environment& e)
	{
		//e.meshes.BuildMesh(vertices, nVertices, )
		return ID_MESH(1);
	}
}