#include "rococo.mplat.h"

#include <string>
#include <unordered_map>

using namespace Rococo;
using namespace Rococo::Entities;

struct Bone
{

};

struct RigBuilder : IRigBuilderSupervisor
{
	RigBuilderContext c;
	std::unordered_map<std::string, Bone> bones;

	RigBuilder(RigBuilderContext& rbc): c(rbc)	{}

	void AddBone(const fstring& name) override
	{
		auto i = bones.find((cstr) name);
		if (i != bones.end())
		{
			Throw(0, "%s: Duplicate name, %s already exists in the rig builder", __FUNCTION__, (cstr)name);
		}

		i = bones.insert(std::make_pair(std::string(name), Bone())).first;
	}

	void Clear() override
	{

	}

	void Free() override
	{
		delete this;
	}
};

namespace Rococo::Entities
{
	IRigBuilderSupervisor* CreateRigBuilder(RigBuilderContext& rbc)
	{
		return new RigBuilder(rbc);
	}
}