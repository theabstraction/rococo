#include "rococo.mplat.h"

#include <ctype.h>
#include <rococo.hashtable.h>
#include <rococo.parse.h>

#include <algorithm>

#include <rococo.handles.h>

using namespace Rococo;
using namespace Rococo::Entities;

struct ScriptedBone
{
	Metres length{ 0.1f };
	Vec3 scale{ 1.0f, 1.0f, 1.0f };
	Vec3 parentOffset{ 0.0f, 0.0f, 0.0f };
	Quat quat;
	std::string parent;
};

enum { MAXLEN_BONENAME = 16 };

void ValidateDefinitive(cstr name)
{
	if (name == nullptr || *name == 0)
	{
		Throw(0, "Blank string");
	}
}

void ValidateNameAccordingToBoneStyleRules(cstr name)
{
	ValidateDefinitive(name);

	if (strlen(name) >= MAXLEN_BONENAME)
	{
		Throw(0, "%s: name was too long. Maximum length is %u characters", __FUNCTION__, MAXLEN_BONENAME - 1);
	}

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

struct Skeleton : public ISkeleton
{
	struct BoneImpl* root;
	cstr name;

	cstr Name() const override
	{
		return name;
	}

	IBone* Root() override;
};

struct Skeletons : public ISkeletons
{
	enum { HSKELE_SALTBITS = 16 };

	stringmap<Skeleton*> nameToSkele;
	HandleTable<Skeleton*, HSKELE_SALTBITS> handleToSkele;

	Skeletons() : handleToSkele("Skeletons", 128)
	{

	}

	void Clear() override
	{
		for (auto i : nameToSkele)
		{
			delete i.second;
		}
		nameToSkele.clear();
		handleToSkele.Clear(nullptr);
	}

	bool TryGet(cstr name, ISkeleton** ppSkeleton) override
	{
		auto i = nameToSkele.find(name);
		if (i != nameToSkele.end())
		{
			*ppSkeleton = i->second;
			return true;
		}
		else
		{
			return false;
		}
	}

	bool TryGet(ID_SKELETON id, ISkeleton** ppSkeleton) override
	{
		Skeleton* skele = nullptr;
		if (handleToSkele.TryGet(THandle<HSKELE_SALTBITS>(id), &skele))
		{
			*ppSkeleton = skele;
			return true;
		}
		else
		{
			*ppSkeleton = nullptr;
			return false;
		}
	}
};

struct BoneImpl : IBone
{
	Skeleton& skeleton;
	BoneImpl* parent;
	Metres length;

	char shortName[MAXLEN_BONENAME];

	Matrix4x4 model;
	std::vector<IBone*> children;

	BoneImpl(Skeleton& _skeleton, cr_m4x4 m, BoneImpl* _parent, cstr _shortName, Metres _length) :
		skeleton(_skeleton), parent(_parent), length(_length), model(m)
	{
		SecureFormat(shortName, MAXLEN_BONENAME, "%s", _shortName);
	}

	virtual ~BoneImpl()
	{
		// Recursively delete all children
		Detach();
		for (auto c : children)
		{
			auto* b = static_cast<BoneImpl*>(c);
			b->parent = nullptr;
			delete b;
		}
	}

	void Free() override
	{
		delete this;
	}

	const Matrix4x4& GetMatrix() const override
	{
		return model;
	}

	void SetMatrix(const Matrix4x4& m) override
	{
		model = m;
	}

	cstr ShortName() const override
	{
		return shortName;
	}

	void GetFullName(BonePath& path) override
	{
		enum { MAX_ANCESTORS = 16 };
		BoneImpl* ancestors[MAX_ANCESTORS];
		int32 index = 0;

		for (auto* ancestor = this; ancestor != nullptr; ancestor = ancestor->parent)
		{
			if (index == MAX_ANCESTORS)
			{
				Throw(0, "%s. Cannot enumerate ancestors. maximum depth reached", __FUNCTION__);
			}
			ancestors[index++] = ancestor;
		}

		size_t writeAt = 0;

		for (int32 id = index - 1; id >= 0; id--)
		{
			auto* s = ancestors[id]->shortName;
			size_t len = strlen(s);

			if (len + writeAt + 2 >= sizeof(BonePath))
			{
				Throw(0, "%s. Cannot enumerate ancestors. The full name was too long", __FUNCTION__);
			}

			memcpy(path.text + writeAt, s, len);
			writeAt += len;

			if (id > 0)
			{
				path.text[writeAt++] = '.';
			}
		}

		path.text[writeAt++] = 0;
	}

	IBone* Parent() const override
	{
		return parent;
	}

	IBone** begin() override
	{
		return children.empty() ? nullptr : children.data();
	}

	IBone** end() override
	{
		return children.empty() ? nullptr : children.data() + children.size();
	}

	const IBone** begin() const override
	{
		return (const IBone**) (children.empty() ? nullptr : children.data());
	}

	const IBone** end() const override
	{
		return (const IBone**) (children.empty() ? nullptr : children.data() + children.size());
	}

	Metres Length() const override
	{
		return length;
	}

	void SetLength(Metres length) override
	{
		this->length = length;
	}

	IBone* AttachBone(const Matrix4x4& m, Metres length, cstr shortName) override
	{
		if (shortName == nullptr || *shortName == 0)
		{
			Throw(0, "%s: blank shortname", __FUNCTION__);
		}

		for (auto c : children)
		{
			if (Eq(c->ShortName(), shortName))
			{
				Throw(0, "%s: a bone already exists in %s with the same name", __FUNCTION__, skeleton.Name());
			}
		}

		try
		{
			BoneImpl* b = new BoneImpl(skeleton, m, this, shortName, length);
			b->model = m;
			children.push_back(b);
			return b;
		}
		catch (IException& ex)
		{
			Throw(0, "%s: error adding bone in %s. %s", __FUNCTION__, skeleton.Name(), ex.Message());
		}
	}

	void Detach() override
	{
		if (parent)
		{
			auto i = std::remove(parent->children.begin(), parent->children.end(), this);
			parent->children.erase(i, parent->children.end());
			parent = nullptr;
		}
	}
};

IBone* Skeleton::Root()
{
	return root;
}

void AttachBonesAndRemoveFromMap_Recursive(stringmap<ScriptedBone>& bones, IBone& daddy)
{
	for (auto c = bones.begin(); c != bones.end(); )
	{
		auto& bone = c->second;
		if (Eq(bone.parent.c_str(), daddy.ShortName()))
		{
			auto* child = daddy.AttachBone(Matrix4x4::Identity(), bone.length, c->first);
			c = bones.erase(c);
		}
		else
		{
			c++;
		}
	}

	for (auto child : daddy)
	{
		AttachBonesAndRemoveFromMap_Recursive(bones, *child);
	}
}

struct RigBuilder : IRigBuilderSupervisor
{
	RigBuilderContext c;
	stringmap<ScriptedBone> bones;
	Skeletons skeletons;

	ScriptedBone& GetBone(cstr name)
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
		ValidateNameAccordingToBoneStyleRules(name);

		auto i = bones.find((cstr) name);
		if (i != bones.end())
		{
			Throw(0, "%s: Duplicate name, %s already exists in the rig builder", __FUNCTION__, (cstr)name);
		}

		i = bones.insert(name, ScriptedBone()).first;
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
		if (parent.length >= MAXLEN_BONENAME)
		{
			Throw(0, "%s: parent name was too long. Maximum length is %u characters", MAXLEN_BONENAME-1);
		}

		if (ofChild.length >= MAXLEN_BONENAME)
		{
			Throw(0, "%s: ofChild name was too long. Maximum length is %u characters", MAXLEN_BONENAME-1);
		}

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

	ISkeletons& Skeles()
	{
		return skeletons;
	}

	void CommitToSkeleton(const fstring& name) override
	{
		auto i = skeletons.nameToSkele.insert(name, nullptr);
		if (!i.second)
		{
			Throw(0, "%s: Skeleton %s already exists", __FUNCTION__, (cstr)name);
		}

		auto* s = new Skeleton();
		i.first->second = s;
		s->name = name;

		s->root = new BoneImpl(*s, Matrix4x4::Identity(), nullptr, "~", 0_metres);
		
		for (auto c = bones.begin(); c != bones.end(); )
		{
			auto& bone = c->second;

			if (bone.parent.size() == 0)
			{
				auto* child =  s->root->AttachBone(Matrix4x4::Identity(), bone.length, c->first);
				c = bones.erase(c);
			}
			else
			{
				c++;
			}
		}

		for (auto* child : *s->root)
		{
			AttachBonesAndRemoveFromMap_Recursive(bones, *child);
		}

		if (!bones.empty())
		{
			s->root->Free();
			delete s;
			skeletons.nameToSkele.erase(i.first);
			Throw(0, "%s %s: %llu elements in the skeleton did not link to the root. Fix ancestors of '%s'", __FUNCTION__, (cstr)name, bones.size(), (cstr) bones.begin()->first);
		}
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