#include "rococo.mplat.h"

#include <ctype.h>
#include <rococo.hashtable.h>
#include <rococo.parse.h>

#include <string>
#include <algorithm>
#include <rococo.handles.h>
#include <rococo.maths.h>

#include <rococo.animation.h>

#include <Windows.h>

using namespace Rococo;
using namespace Rococo::Entities;
using namespace Rococo::Strings;

struct ScriptedBone
{
	Metres length{ 0.1f };
	Vec3 scale{ 1.0f, 1.0f, 1.0f };
	Vec3 parentOffset{ 0.0f, 0.0f, 0.0f };
	Quat quat{ {0.0f, 0.0f, 0.0f}, 1.0f };
	BoneAngles angles{ 0 };
	std::string parent;

	void GetTransformRelativeToParent(Matrix4x4& m)
	{
		Matrix4x4 T = Matrix4x4::Translate(parentOffset);
		Matrix4x4 R;
		Matrix4x4::FromQuat(quat, R);
		m = T * R;
	}
};

bool AreAnglesUsed(const BoneAngles& angles)
{
	return angles.facing != 0 || angles.roll != 0 || angles.tilt != 0;
}

bool IsQuatUsed(const Quat& q)
{
	return q.s != 1.0f;
}

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

	struct SkeleBind
	{
		Skeleton* skeleton = nullptr;
		ID_SKELETON id;
	};

	stringmap<SkeleBind> nameToSkele;
	HandleTable<Skeleton*, HSKELE_SALTBITS> handleToSkele;

	Skeletons() : handleToSkele("Skeletons", 128)
	{

	}

	void Clear() override
	{
		for (auto i : nameToSkele)
		{
			delete i.second.skeleton;
		}
		nameToSkele.clear();
		handleToSkele.Clear(nullptr);
	}

	ID_SKELETON GetByNameAndReturnId(cstr name, ISkeleton** ppSkeleton) override
	{
		auto i = nameToSkele.find(name);
		if (i != nameToSkele.end())
		{
			*ppSkeleton = i->second.skeleton;
			return i->second.id;
		}
		else
		{
			return ID_SKELETON::Invalid();
		}
	}

	bool TryGet(ID_SKELETON id, ISkeleton** ppSkeleton) override
	{
		Skeleton* skele;
		if (handleToSkele.TryGet(THandle<HSKELE_SALTBITS>(id), &skele))
		{
			*ppSkeleton = skele;
			return true;
		}
		else
		{
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

	Vec3 offset;
	Rococo::Quat quat{ {0,0,0},1 };

	Matrix4x4 model;
	std::vector<IBone*> children;

	BoneImpl(Skeleton& _skeleton, cr_vec3 _offset, cr_quat _quat, BoneImpl* _parent, cstr _shortName, Metres _length) :
		skeleton(_skeleton), parent(_parent), length(_length), offset(_offset), quat(_quat)
	{
		Matrix4x4 R;
		Matrix4x4::FromQuat(quat, R);

		Matrix4x4 T = Matrix4x4::Translate(_offset);

		model = T * R;

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

	cr_quat Quat() const override
	{
		return quat;
	}

	void SetQuat(cr_quat q) override
	{
		this->quat = q;
		Matrix4x4 R;
		Matrix4x4::FromQuat(q, R);

		Matrix4x4 T = Matrix4x4::Translate(this->offset);
		this->model = T * R;
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

	IBone* AttachBone(cr_vec3 offset, cr_quat quat, Metres length, cstr shortName) override
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
			BoneImpl* b = new BoneImpl(skeleton, offset, quat, this, shortName, length);
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
			daddy.AttachBone(bone.parentOffset, bone.quat, bone.length, c->first);
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

struct Skeletons;

struct RigBuilder : public IRigBuilder
{
	Skeletons& skeletons;
	Skeletons& poses;
	stringmap<ScriptedBone> bones;

	RigBuilder(Skeletons& _skeletons, Skeletons& _poses):
		skeletons(_skeletons), poses(_poses)
	{

	}

	ScriptedBone& CreateBone(cstr name)
	{
		ValidateNameAccordingToBoneStyleRules(name);

		auto i = bones.find((cstr)name);
		if (i != bones.end())
		{
			Throw(0, "%s: Duplicate name, %s already exists in the rig builder", __FUNCTION__, (cstr)name);
		}

		i = bones.insert(name, ScriptedBone()).first;
		return i->second;
	}


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
	
	void ClearSkeletons() override
	{
		skeletons.Clear();
		poses.Clear();
	}

	void AddBone(const fstring& name) override
	{
		CreateBone(name);
	}

	void AddBoneX(const fstring& name, const fstring& parent, Metres length, float dx, float dy, float dz, Degrees rX, Degrees rY, Degrees rZ)
	{
		auto& b = CreateBone(name);
		b.length = length;
		b.parentOffset = { dx, dy, dz };

		BoneAngles angles;
		angles.facing = rZ;
		angles.roll = rY;
		angles.tilt = rX;
		b.angles = angles;

		ComputeBoneQuatFromAngles(b.quat, b.angles);

		if (parent.length > 0)
		{
			SetParentOfChild(parent, name);
		}
	}

	void SetLength(const fstring& name, Metres length)
	{
		GetBone(name).length = length;
	}

	void ClearBuilder() override
	{
		bones.clear();
	}

	void ClearPoses() override
	{
		poses.Clear();
	}

	void SetParentOfChild(const fstring& parent, const fstring& ofChild) override
	{
		if (parent.length >= MAXLEN_BONENAME)
		{
			Throw(0, "%s: parent name was too long. Maximum length is %u characters", MAXLEN_BONENAME - 1);
		}

		if (ofChild.length >= MAXLEN_BONENAME)
		{
			Throw(0, "%s: ofChild name was too long. Maximum length is %u characters", MAXLEN_BONENAME - 1);
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

	void SetVec3OffsetFromParent(const fstring& name, const Vec3& positionOffset) override
	{
		GetBone(name).parentOffset = positionOffset;
	}

	void SetOffsetFromParent(const fstring& name, float dx, float dy, float dz) override
	{
		GetBone(name).parentOffset = { dx, dy, dz };
	}

	void SetRotationFromParent(const fstring& name, Degrees rX, Degrees rY, Degrees rZ) override
	{
		auto& b = GetBone(name);

		if (AreAnglesUsed(b.angles))
		{
			Throw(0, "%s(%s...) Bone euler angles have already been specified", __FUNCTION__, name.buffer);
		}

		if (IsQuatUsed(b.quat))
		{
			Throw(0, "%s(%s...) The bone quaternion has already been specified", __FUNCTION__, name.buffer);
		}

		BoneAngles angles;
		angles.facing = rZ;
		angles.roll = rY;
		angles.tilt = rX;
		b.angles = angles;

		ComputeBoneQuatFromAngles(b.quat, b.angles);
	}

	void SetQuatFromParent(const fstring& name, const Quat& q, boolean32 validateQuat) override
	{
		auto& b = GetBone(name);

		if (AreAnglesUsed(b.angles))
		{
			Throw(0, "%s(%s...) Bone euler angles have already been specified", __FUNCTION__, name.buffer);
		}

		if (IsQuatUsed(b.quat))
		{
			Throw(0, "%s(%s...) The bone quaternion has already been specified", __FUNCTION__, name.buffer);
		}

		float ds2 = Square(q.s) + Square(q.v.x) + Square(q.v.y) + Square(q.v.z);
		const float MIN_DS2 = 0.98f;
		const float MAX_DS2 = 1.02f;
		if (validateQuat && (ds2 < MIN_DS2 || ds2 > MAX_DS2))
		{
			Throw(0, "%s: quat mod square is %f. Range is [%f,%f]", (cstr)name, ds2, MIN_DS2, MAX_DS2);
		}

		b.quat = q;
	}

	ID_SKELETON CommitTo(Skeletons& target, cstr name)
	{
		auto i = target.nameToSkele.insert(name, Skeletons::SkeleBind());
		if (!i.second)
		{
			Throw(0, "%s: Skeleton %s already exists", __FUNCTION__, (cstr)name);
		}

		auto* s = new Skeleton();
		i.first->second.skeleton = s;
		s->name = name;

		s->root = new BoneImpl(*s, Vec3{ 0,0,0 }, Quat{ {0,0,0}, 1.0f }, nullptr, "~", 0_metres);

		for (auto c = bones.begin(); c != bones.end(); )
		{
			auto& bone = c->second;

			if (bone.parent.size() == 0)
			{
				s->root->AttachBone(bone.parentOffset, bone.quat, bone.length, c->first);
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

		try
		{
			if (!bones.empty())
			{
				Throw(0, "%s %s: %llu elements in the skeleton did not link to the root. Fix ancestors of '%s'", __FUNCTION__, (cstr)name, bones.size(), (cstr)bones.begin()->first);
			}

			auto handle = target.handleToSkele.CreateNew();
			target.handleToSkele.Set(handle, s);
			ID_SKELETON id{ handle.Value() };
			i.first->second.id = id;
			return id;
		}
		catch (IException&)
		{
			s->root->Free();
			delete s;
			target.nameToSkele.erase(i.first);
			throw;
		}
	}

	ID_SKELETON CommitToSkeleton(const fstring& name) override
	{
		ID_SKELETON id = CommitTo(skeletons, name);
		return id;
	}

	ID_POSE CommitToPose(const fstring& name) override
	{
		if (name.length < 1 || name.length >= MAX_POSENAME_LEN)
		{
			Throw(0, "%s - invalid pose name '%s'. Length must be between 1 and %u characters", __FUNCTION__, name.buffer, MAX_POSENAME_LEN);
		}

		auto id = CommitTo(poses, name);
		return ID_POSE{ id };
	}
};

struct Rigs: public IRigs
{
	Skeletons skeletons;
	Skeletons poses;
	RigBuilder builder;

	Rigs(): builder(skeletons, poses) {}

	IRigBuilder& Builder() override
	{
		return builder;
	}

	ISkeletons& Skeles() override
	{
		return skeletons;
	}

	ISkeletons& Poses() override
	{
		return poses;
	}

	void Free() override
	{
		delete this;
	}
};

namespace Rococo::Entities
{
	IRigs* CreateRigBuilder()
	{
		return new Rigs();
	}
}