#include "dystopia.h"
#include "skeleton.h"
#include "meshes.h"
#include <array>
#include <unordered_map>

#include <sexy.types.h>
#include <Sexy.S-Parser.h>

namespace
{
	using namespace Rococo;
	using namespace Dystopia;
	using namespace Sexy;
	using namespace Sexy::Sex;

	const ID_MESH BODY_HEAD_MESH_ID(0x41000000);
	const ID_MESH BODY_ARM_MESH_ID(0x41000001);
	const ID_MESH BODY_TORSO_MESH_ID(0x41000002);
	const ID_MESH BODY_LEG_MESH_ID(0x41000003);
	const ID_MESH BODY_FOOT_MESH_ID(0x41000004);

	enum LimbIndex
	{
		LimbIndex_Head,
		LimbIndex_LeftArm,
		LimbIndex_RightArm,
		LimbIndex_Torso,
		LimbIndex_LeftLeg,
		LimbIndex_RightLeg,
		LimbIndex_LeftFoot,
		LimbIndex_RightFoot,
		LimbIndex_Count
	};

	struct Limb
	{
		Matrix4x4 bodyToLimb;
		ID_SYS_MESH bodyMeshId;
	};

	ID_SYS_MESH BindMesh(const wchar_t* resourceName, ID_MESH id, IMeshLoader& loader)
	{
		if (loader.GetRendererId(id) == ID_SYS_MESH::Invalid())
		{
			loader.Load(to_fstring(resourceName), id);
		}

		return loader.GetRendererId(id);
	}

	typedef std::array<Limb, LimbIndex_Count> Bones;

	struct LimbMeshes
	{
		ID_SYS_MESH id_head;
		ID_SYS_MESH id_arm;
		ID_SYS_MESH id_torso;
		ID_SYS_MESH id_leg;
		ID_SYS_MESH id_foot;
	};

	void SetStanding(Bones& bones, const LimbMeshes& meshes)
	{
		bones[LimbIndex_Head] = Limb{ Matrix4x4::Translate(Vec3{ 0,0,1.75f }), meshes.id_head };
		bones[LimbIndex_LeftArm] = Limb{ Matrix4x4::Translate(Vec3{ -0.2f,0,1.10f }), meshes.id_arm };
		bones[LimbIndex_RightArm] = Limb{ Matrix4x4::Translate(Vec3{ 0.2f,0,1.10f }), meshes.id_arm };
		bones[LimbIndex_Torso] = Limb{ Matrix4x4::Translate(Vec3{ 0,0,1.20f }), meshes.id_torso };
		bones[LimbIndex_LeftLeg] = Limb{ Matrix4x4::Translate(Vec3{ -0.1f,0,0.65f }), meshes.id_leg };
		bones[LimbIndex_RightLeg] = Limb{ Matrix4x4::Translate(Vec3{ 0.1f,0,0.65f }), meshes.id_leg };
		bones[LimbIndex_LeftFoot] = Limb{ Matrix4x4::Translate(Vec3{ -0.15f,0.05f,0.01f }), meshes.id_foot };
		bones[LimbIndex_RightFoot] = Limb{ Matrix4x4::Translate(Vec3{ 0.15f,0.05f,0.01f }), meshes.id_foot };
	}

	void SetRunning(Bones& bones, const LimbMeshes& meshes, Radians phi)
	{
		bones[LimbIndex_Head] = Limb{ Matrix4x4::Translate(Vec3{ 0,0,1.75f }), meshes.id_head };
		bones[LimbIndex_LeftArm] = Limb{ Matrix4x4::Translate(Vec3{ -0.2f,0,1.10f }), meshes.id_arm };
		bones[LimbIndex_RightArm] = Limb{ Matrix4x4::Translate(Vec3{ 0.2f,0,1.10f }), meshes.id_arm };
		bones[LimbIndex_Torso] = Limb{ Matrix4x4::Translate(Vec3{ 0,0,1.20f }), meshes.id_torso };

		Matrix4x4 rotXForward = Matrix4x4::RotateRHAnticlockwiseX(phi);
		Matrix4x4 rotXBackward = Matrix4x4::RotateRHAnticlockwiseX(-phi);
		bones[LimbIndex_LeftLeg] = Limb{ Matrix4x4::Translate(Vec3{ -0.1f,   0,    0.65f }) * rotXForward, meshes.id_leg };
		bones[LimbIndex_RightLeg] = Limb{ Matrix4x4::Translate(Vec3{ 0.1f,    0,    0.65f }) * rotXBackward, meshes.id_leg };
		bones[LimbIndex_LeftFoot] = Limb{ Matrix4x4::Translate(Vec3{ -0.15f, 0.35f, 0.35f }) * rotXForward, meshes.id_foot };
		bones[LimbIndex_RightFoot] = Limb{ Matrix4x4::Translate(Vec3{ 0.15f, -0.35f, 0.35f }) * rotXBackward, meshes.id_foot };
	}

	class Skeleton : public ISkeletonSupervisor
	{
		Environment& e;
		Bones currentFrame;

		AnimationType currentAnimation;

		LimbMeshes limbMeshes;

		float lastAnimationTime;

		void UpdateCurrentFrame(float gameTime, float animationDT)
		{

		}
	public:
		Skeleton(Environment& _e) : e(_e), lastAnimationTime(0)
		{
			limbMeshes.id_head = BindMesh(L"!mesh/body.head.sxy", BODY_HEAD_MESH_ID, e.meshes);
			limbMeshes.id_arm = BindMesh(L"!mesh/body.arm.sxy", BODY_ARM_MESH_ID, e.meshes);
			limbMeshes.id_torso = BindMesh(L"!mesh/body.torso.sxy", BODY_TORSO_MESH_ID, e.meshes);
			limbMeshes.id_leg = BindMesh(L"!mesh/body.leg.sxy", BODY_LEG_MESH_ID, e.meshes);
			limbMeshes.id_foot = BindMesh(L"!mesh/body.foot.sxy", BODY_FOOT_MESH_ID, e.meshes);

			SetRunning(currentFrame, limbMeshes, 0.0_degrees);
		}

		virtual void Free()
		{
			delete this;
		}

		virtual void Render(IRenderContext& rc, const ObjectInstance& instance, float gameTime, float frameDt)
		{
			if (gameTime > lastAnimationTime)
			{
				float dt = lastAnimationTime - gameTime;
				UpdateCurrentFrame(gameTime, dt);
				lastAnimationTime = gameTime;
			}

			Vec3 pos = instance.orientation.GetPosition();
			float f = fmodf(pos.x + pos.y + pos.z, 1.0f) * PI() / 3.0f;
			SetRunning(currentFrame, limbMeshes, Radians{ f });

			for (auto& limb : currentFrame)
			{
				ObjectInstance limbInstance{ instance.orientation * limb.bodyToLimb, instance.highlightColour };
				rc.Draw(limb.bodyMeshId, &limbInstance, 1);
			}
		}

		virtual void SetCurrentAnimation(AnimationType type)
		{
			currentAnimation = type;
			switch (type)
			{
			case AnimationType_Standstill:
				SetStanding(currentFrame, limbMeshes);
				break;
			case AnimationType_Running:
				SetRunning(currentFrame, limbMeshes, 0.0_degrees);
				break;
			}
		}
	};

	struct BoneOrientation
	{
		Quat rotation;
		Vec3 parentToChildDisplacement;
		float unused;
	};

	struct Keyframe
	{
		SkeletonType type;
		std::vector<BoneOrientation> limbs;
	};

	class BoneLibrary : public IBoneLibrarySupervisor
	{
	private:
		IInstallation& installation;
		IRenderer& renderer;
		ISourceCache& sources;

		std::unordered_map<std::wstring, Keyframe> keyframes;
	public:
		BoneLibrary(IInstallation& _installation, IRenderer& _renderer, ISourceCache& _sources) :
			installation(_installation),
			renderer(_renderer),
			sources(_sources)
		{

		}

		void ParseKeyframe(cr_sex sdirective)
		{
			if (sdirective.NumberOfElements() < 3)
			{
				ThrowSex(sdirective, L"Expecting at least 3 elements in a keyframe directive");

				auto& keyframeName = GetAtomicArg(sdirective[1]);
				auto result = keyframes.insert(std::make_pair(std::wstring(keyframeName), Keyframe()));
				if (!result.second)
				{
					ThrowSex(sdirective[1], L"Duplicate keyframe name.", keyframeName);
				}

				auto& skeletonType = GetAtomicArg(sdirective[2]);
				
			}
		}

		void ParseBoneScript(cr_sex root)
		{
			if (root.NumberOfElements() < 2)
			{
				ThrowSex(root, L"There must be at least two elements in the root expression of a bone library file");
			}

			auto& sversion = root[0];
			if (sversion.NumberOfElements() != 3)
			{
				ThrowSex(root, L"Expecting 3 elements in a bone library version expression");
			}

			if (sversion[0] != L"'" || sversion[1] != L"file.type" || sversion[2] != L"bone.library.1.0.0.0")
			{
				ThrowSex(sversion, L"Expecting version expression: (' file.type bone.library.1.0.0.0)");
			}

			for (int i = 1; i < root.NumberOfElements(); ++i)
			{
				auto& sdirective = root[i];
				if (!IsCompound(sdirective))
				{
					ThrowSex(sdirective, L"Expecting compound expression");
				}

				if (sdirective[0] == L"keyframe")
				{
					ParseKeyframe(sdirective);
				}
				else
				{
					ThrowSex(sdirective, L"Unknown expression");
				}
			}
		}

		virtual void Free()
		{
			delete this;
		}

		virtual void Reload(const wchar_t* filename)
		{
			while (true)
			{
				try
				{
					keyframes.clear();
					ProtectedLoad(filename);
					return;
				}
				catch (IException& ex)
				{
					installation.OS().FireUnstable();
					CMD_ID id = ShowContinueBox(renderer.Window(), ex.Message());
					switch (id)
					{
					case CMD_ID_EXIT:
						Throw(ex.ErrorCode(), L"%s", ex.Message());
						break;
					case CMD_ID_RETRY:
						break;
					case CMD_ID_IGNORE:
						return;
					}
				}
			}
		}

		void ProtectedLoad(const wchar_t* resourcePath)
		{
			try
			{
				auto tree = sources.GetSource(resourcePath);
				ParseBoneScript(tree->Root());
			}
			catch (Sexy::Sex::ParseException& pex)
			{
				SourcePos p = pex.Start();
				SourcePos q = pex.End();
				Throw(0, L"Error parsering %s\n%s: %s\n(%d,%d) to (%d,%d)\n%s", resourcePath, pex.Name(), pex.Message(), p.X, p.Y, q.X, q.Y, pex.Specimen());
			}
		}

	};
}

namespace Dystopia
{
	ISkeletonSupervisor* CreateSkeleton(Environment& e)
	{
		return new Skeleton(e);
	}

	IBoneLibrarySupervisor* CreateBoneLibrary(IInstallation& installation, IRenderer& renderer, ISourceCache& sources)
	{
		return new BoneLibrary(installation, renderer, sources);
	}
}