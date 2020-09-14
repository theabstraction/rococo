#include "rococo.mplat.h"

#include <ctype.h>
#include <unordered_map>

#include <rococo.parse.h>

using namespace Rococo;
using namespace Rococo::Entities;

struct Bone
{
	Metres length{ 0.1f };
	Vec3 scale{ 1.0f, 1.0f, 1.0f };
	Vec3 parentOffset{ 0.0f, 0.0f, 0.0f };
	Quat quat;
	std::string parent;
};

void ValidateDefinitive(cstr name)
{
	if (name == nullptr || *name == 0)
	{
		Throw(0, "Blank string");
	}
}

void ValidateNameAccordingToBoneSytleRules(cstr name)
{
	ValidateDefinitive(name);

	if (!isalnum(*name))
	{
		Throw(0, "%s: Expecting alphanumeric character in the first position", name);
	}

	bool inDot = false;

	for (cstr p = name+1; *p != 0; p++)
	{
		char c = *p;
		if (c == '.')
		{
			if (inDot)
			{
				Throw(0, "%s: double dots not allowed", name);
			}
			else
			{
				inDot = true;
			}
		}
		else
		{
			inDot = false;

			if (!isalnum(c) && c != '_')
			{
				Throw(0, "%s: illegal character at position %llu", name, p - name);
			}
		}
	}

	if (inDot)
	{
		Throw(0, "%s: trailing dot not allowed", name);
	}
}

struct RigBuilder : IRigBuilderSupervisor
{
	RigBuilderContext c;
	std::unordered_map<std::string, Bone> bones;

	Bone& GetBone(cstr name)
	{
		ValidateDefinitive(name);
		auto i = bones.find(name);
		if (i == bones.end())
		{
			Throw(0, "Cannot find bone '%s'", name);
		}
		return i->second;
	}

	RigBuilder(RigBuilderContext& rbc): c(rbc)	{}

	void AddBone(const fstring& name) override
	{
		ValidateNameAccordingToBoneSytleRules(name);

		auto i = bones.find((cstr) name);
		if (i != bones.end())
		{
			Throw(0, "%s: Duplicate name, %s already exists in the rig builder", __FUNCTION__, (cstr)name);
		}

		i = bones.insert(std::make_pair(std::string(name), Bone())).first;
	}

	void SetLength(const fstring& name, Metres length)
	{
		GetBone(name).length = length;
	}

	void Clear() override
	{
		bones.clear();
	}

	void SetParentOfChild(const fstring& parent, const fstring& ofChild) override
	{
		std::string previousParent = GetBone(ofChild).parent;
		GetBone(ofChild).parent = parent;

		std::string currentParent = GetBone(ofChild).parent;

		enum { MAX_DEPTH = 1000000 }; // A kilometre long snake, with millimetre bones?
		for (int i = 0; i < MAX_DEPTH; ++i)
		{
			if (currentParent.empty()) return;
			std::string grandad = GetBone(currentParent.c_str()).parent;
			currentParent = grandad;
		}

		GetBone(ofChild).parent = previousParent;

		Throw(0, "Cannot set %s to be the parent of %s. Recursive depth reached (looped?).", (cstr)parent, (cstr)ofChild);
	}

	void SetScale(const fstring& name, float sx, float sy, float sz) override
	{
		GetBone(name).scale = { sx, sy, sz };
	}

	void SetOffsetFromParent(const fstring& name, const Vec3& positionOffset) override
	{
		GetBone(name).parentOffset = positionOffset;
	}

	void SetOrientFromParent(const fstring& name, const Quat& q, boolean32 validateQuat)
	{
		ValidateDefinitive(name);

		float ds2 = Square(q.s) + Square(q.v.x) + Square(q.v.y) + Square(q.v.z);
		const float MIN_DS2 = 0.98f;
		const float MAX_DS2 = 1.02f;
		if (validateQuat && (ds2 < MIN_DS2 || ds2 > MAX_DS2))
		{
			Throw(0, "%s: quat mod square is %f. Range is [%f,%f]", (cstr)name, ds2, MIN_DS2, MAX_DS2);
		}

		GetBone(name).quat = q;
	}

	void CommitToSkeleton(const fstring& name) override
	{

	}

	void CommitToPose(const fstring& name) override
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