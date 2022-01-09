#include "rococo.mplat.h"

#include "rococo.hashtable.h"

using namespace Rococo;
using namespace Rococo::Graphics;

namespace
{
	class Sprites: public ISpritesSupervisor
	{
		int64 nextId = 1;
	public:
		IRenderer& renderer;
		stringmap<ID_SPRITE> nameToId;
		std::unordered_map<ID_SPRITE, cstr, ID_SPRITE> idToName;

		Sprites(IRenderer& refRenderer) : renderer(refRenderer) {}

		ID_SPRITE TryGetId(const fstring& pingPath) override
		{
			if (pingPath.length < 2) Throw(0, "Expecting at least two characters in the [pingPath]");
			cstr name;
			U8FilePath expandedPath;
			name = renderer.Installation().TryExpandMacro(pingPath, expandedPath) ? expandedPath.buf : pingPath.buffer;

			auto i = nameToId.find(pingPath);
			if (i != nameToId.end())
			{
				return i->second;
			}

			auto id = ID_SPRITE(nextId++);
			i = nameToId.insert(name, id).first;
			idToName[id] = i->first;
			return id;
		}

		void AppendPingPath(ID_SPRITE id, IStringPopulator& sb) override
		{
			auto i = idToName.find(id);
			if (i == idToName.end())
			{
				Throw(0, "Unknown sprite id: %llu", id.value);
			}

			sb.Populate(i->second);
		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo::Graphics
{
	ISpritesSupervisor* CreateSpriteTable(IRenderer& renderer)
	{
		return new Sprites(renderer);
	}
}