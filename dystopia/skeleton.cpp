#include "dystopia.h"
#include "rococo.maths.h"
#include "skeleton.h"
#include "meshes.h"
#include <array>
#include <unordered_map>
#include "rococo.keys.h"

namespace
{
	using namespace Rococo;
	using namespace Dystopia;

	const ID_MESH BODY_HEAD_MESH_ID(0x41000000);
	const ID_MESH BODY_L_UPPER_ARM_MESH_ID(0x41000001);
	const ID_MESH BODY_TORSO_MESH_ID(0x41000002);
	const ID_MESH BODY_UPPER_LEG_MESH_ID(0x41000003);
	const ID_MESH BODY_FOOT_MESH_ID(0x41000004);
	const ID_MESH BODY_L_LOWER_ARM_MESH_ID(0x41000005);
	const ID_MESH BODY_LOWER_LEG_MESH_ID(0x41000006);
	const ID_MESH BODY_L_HAND_MESH_ID(0x41000007);
	const ID_MESH BODY_R_UPPER_ARM_MESH_ID(0x41000008);
	const ID_MESH BODY_R_LOWER_ARM_MESH_ID(0x41000009);
	const ID_MESH BODY_R_HAND_MESH_ID(0x4100000A);

	struct Limb
	{
		Matrix4x4 bodyToLimb;
		ID_SYS_MESH bodyMeshId;
		LimbIndex parentLimbIndex;
	};

	ID_SYS_MESH BindMesh(const wchar_t* resourceName, ID_MESH id, IMeshLoader& loader)
	{
		if (loader.GetRendererId(id) == ID_SYS_MESH::Invalid())
		{
			loader.Load(to_fstring(resourceName), id);
		}

		return loader.GetRendererId(id);
	}

	ID_SYS_MESH ReflectMesh(ID_MESH srcId, ID_MESH id, IMeshLoader& loader)
	{
		if (loader.GetRendererId(id) == ID_SYS_MESH::Invalid())
		{
			loader.CreateReflection(srcId, id);
		}

		return loader.GetRendererId(id);
	}

	typedef std::array<Limb, LimbIndex_Count> Bones;

	struct LimbMeshes
	{
		ID_SYS_MESH id_head;
		ID_SYS_MESH id_l_upper_arm;
		ID_SYS_MESH id_r_upper_arm;
		ID_SYS_MESH id_torso;
		ID_SYS_MESH id_upper_leg;
		ID_SYS_MESH id_foot;
		ID_SYS_MESH id_l_lower_arm;
		ID_SYS_MESH id_r_lower_arm;
		ID_SYS_MESH id_lower_leg;
		ID_SYS_MESH id_l_hand;
		ID_SYS_MESH id_r_hand;
	};

	void SetLimbMeshes(Bones& bones, const LimbMeshes& meshes)
	{
		bones[LimbIndex_Head].bodyMeshId = meshes.id_head;
		bones[LimbIndex_Head].parentLimbIndex = LimbIndex_Torso;

		bones[LimbIndex_LeftUpperArm].bodyMeshId = meshes.id_l_upper_arm;
		bones[LimbIndex_LeftUpperArm].parentLimbIndex = LimbIndex_Torso;

		bones[LimbIndex_RightUpperArm].bodyMeshId = meshes.id_r_upper_arm;
		bones[LimbIndex_RightUpperArm].parentLimbIndex = LimbIndex_Torso;

		bones[LimbIndex_Torso].bodyMeshId = meshes.id_torso;
		bones[LimbIndex_Torso].parentLimbIndex = LimbIndex_Torso;

		bones[LimbIndex_LeftUpperLeg].bodyMeshId = meshes.id_upper_leg;
		bones[LimbIndex_LeftUpperLeg].parentLimbIndex = LimbIndex_Torso;

		bones[LimbIndex_RightUpperLeg].bodyMeshId = meshes.id_upper_leg;
		bones[LimbIndex_RightUpperLeg].parentLimbIndex = LimbIndex_Torso;

		bones[LimbIndex_LeftFoot].bodyMeshId = meshes.id_foot;
		bones[LimbIndex_LeftFoot].parentLimbIndex = LimbIndex_LeftLowerLeg;

		bones[LimbIndex_RightFoot].bodyMeshId = meshes.id_foot;
		bones[LimbIndex_RightFoot].parentLimbIndex = LimbIndex_RightLowerLeg;

		bones[LimbIndex_LeftLowerLeg].bodyMeshId = meshes.id_lower_leg;
		bones[LimbIndex_LeftLowerLeg].parentLimbIndex = LimbIndex_LeftUpperLeg;

		bones[LimbIndex_RightLowerLeg].bodyMeshId = meshes.id_lower_leg;
		bones[LimbIndex_RightLowerLeg].parentLimbIndex = LimbIndex_RightUpperLeg;

		bones[LimbIndex_LeftHand].bodyMeshId = meshes.id_l_hand;
		bones[LimbIndex_LeftHand].parentLimbIndex = LimbIndex_LeftLowerArm;

		bones[LimbIndex_RightHand].bodyMeshId = meshes.id_r_hand;
		bones[LimbIndex_RightHand].parentLimbIndex = LimbIndex_RightLowerArm;

		bones[LimbIndex_LeftLowerArm].bodyMeshId = meshes.id_l_lower_arm;
		bones[LimbIndex_LeftLowerArm].parentLimbIndex = LimbIndex_LeftUpperArm;

		bones[LimbIndex_RightLowerArm].bodyMeshId = meshes.id_r_lower_arm;
		bones[LimbIndex_RightLowerArm].parentLimbIndex = LimbIndex_RightUpperArm;
	}

	BoneOrientation Morph(const BoneOrientation& boneA, const BoneOrientation& boneB, float t)
	{
		BoneOrientation result;
		result.parentToChildDisplacement = Lerp(boneA.parentToChildDisplacement, boneB.parentToChildDisplacement, t);
		result.rotation = InterpolateRotations(boneA.rotation, boneB.rotation, t);
		return result;
	}

	void Morph(IBoneLibrary& lib, ID_KEYFRAME a, ID_KEYFRAME b, float t, Bones& bones)
	{
		struct : IBoneEnumerator
		{
			std::array<BoneOrientation, LimbIndex_Count> bones;

			virtual void OnBone(const BoneOrientation& bone, int32 index)
			{
				if (index > bones.size())
				{
					Throw(0, L"Bad bone index in morph call.");
				}
				bones[index] = bone;
			}

			// Called prior to OnBone to tell the enumerator what type of skeleton we have
			virtual void OnType(SkeletonType type)
			{
				if (type != SkeletonType_HumanMale)
				{
					Throw(0, L"Unknown skeleton type");
				}
			}
		} frameA, frameB;

		lib.EnumerateKeyframeBonesById(a, frameA);
		lib.EnumerateKeyframeBonesById(b, frameB);

		std::array<BoneOrientation, LimbIndex_Count> morphedBones;

		for (int i = 0; i < LimbIndex_Count; ++i)
		{
			morphedBones[i] = Morph(frameA.bones[i], frameB.bones[i], t);
		}

		for (int i = 0; i < LimbIndex_Count; ++i)
		{
			Matrix4x4 T = Matrix4x4::Translate(morphedBones[i].parentToChildDisplacement);

			Matrix4x4 R;
			Matrix4x4::FromQuat(morphedBones[i].rotation, R);

			bones[i].bodyToLimb = T * R;
		}
	}

	class Skeleton : public ISkeletonSupervisor
	{
		Environment& e;
		Bones currentFrame;

		LimbMeshes limbMeshes;

		Seconds animationStart;

		ID_ANIMATION animationId;
		ID_KEYFRAME currentKeyframeId;
		ID_KEYFRAME morphTargetKeyframeId;

		float currentActualisationTime;

		void UpdateCurrentFrame(Seconds gameTime)
		{
			if (animationStart == 0.0f)
			{
				animationStart = gameTime;
				currentActualisationTime = gameTime;
			}

			morphTargetKeyframeId = ID_KEYFRAME::Invalid();

			float nextActualisationTime = 0.0_seconds;
			
			while (true)
			{
				const auto& seq = e.boneLibrary.GetAnimationSequenceById(animationId);
				for (auto& i : seq)
				{
					if (gameTime - animationStart < i.actualisationTime)
					{
						morphTargetKeyframeId = i.id;
						nextActualisationTime = animationStart + i.actualisationTime;
						break;
					}
					else
					{
						if (i.id != ID_KEYFRAME::Invalid())
						{
							currentKeyframeId = i.id;
						}
						
						currentActualisationTime = animationStart + i.actualisationTime;
					}
				}

				if (morphTargetKeyframeId == ID_KEYFRAME::Invalid())
				{
					auto* lastFrame = seq.lastFrame_plus_one - 1;
					ID_ANIMATION nextAnimationId = lastFrame->nextAnimationId;

					if (nextAnimationId == ID_ANIMATION::Invalid())
					{
						Morph(e.boneLibrary, currentKeyframeId, currentKeyframeId, 0.0f, currentFrame);
						return;
					}

					animationId = nextAnimationId;

					if (lastFrame->actualisationTime <= 0)
					{
						Throw(0, L"Bad expire time for animation %u", animationId.value);
					}

					animationStart.value += lastFrame->actualisationTime;
				}
				else
				{
					break;
				}
			}

			float duration = nextActualisationTime - currentActualisationTime;
			float interpolationValue = (gameTime - animationStart) / duration;

			Morph(e.boneLibrary, currentKeyframeId, morphTargetKeyframeId, interpolationValue, currentFrame);
		}
	public:
		Skeleton(Environment& _e) : e(_e)
		{
			limbMeshes.id_head = BindMesh(L"!mesh/body.head.sxy", BODY_HEAD_MESH_ID, e.meshes);
			limbMeshes.id_l_lower_arm = BindMesh(L"!mesh/body.lower.arm.sxy", BODY_L_LOWER_ARM_MESH_ID, e.meshes);
			limbMeshes.id_r_lower_arm = ReflectMesh(BODY_L_LOWER_ARM_MESH_ID, BODY_R_LOWER_ARM_MESH_ID, e.meshes);
			limbMeshes.id_torso = BindMesh(L"!mesh/body.torso.sxy", BODY_TORSO_MESH_ID, e.meshes);
			limbMeshes.id_lower_leg = BindMesh(L"!mesh/body.lower.leg.sxy", BODY_LOWER_LEG_MESH_ID, e.meshes);
			limbMeshes.id_foot = BindMesh(L"!mesh/body.foot.sxy", BODY_FOOT_MESH_ID, e.meshes);
			limbMeshes.id_l_upper_arm = BindMesh(L"!mesh/body.upper.arm.sxy", BODY_L_UPPER_ARM_MESH_ID, e.meshes);
			limbMeshes.id_r_upper_arm = ReflectMesh(BODY_L_UPPER_ARM_MESH_ID, BODY_R_UPPER_ARM_MESH_ID, e.meshes);
			limbMeshes.id_upper_leg = BindMesh(L"!mesh/body.upper.leg.sxy", BODY_UPPER_LEG_MESH_ID, e.meshes);
			limbMeshes.id_l_hand = BindMesh(L"!mesh/body.hand.sxy", BODY_L_HAND_MESH_ID, e.meshes);
			limbMeshes.id_r_hand = ReflectMesh(BODY_L_HAND_MESH_ID, BODY_R_HAND_MESH_ID, e.meshes);

			SetLimbMeshes(currentFrame, limbMeshes);
			SetCurrentAnimation(AnimationType_Standstill);

			morphTargetKeyframeId = currentKeyframeId = e.boneLibrary.GetAnimationSequenceById(animationId).firstFrame->id;
		}

		virtual void Free()
		{
			delete this;
		}

		virtual void Render(IRenderContext& rc, const ObjectInstance& instance, Seconds gameTime)
		{
			UpdateCurrentFrame(gameTime);

			for (int i = 0; i < currentFrame.size(); ++i)
			{
				Matrix4x4 transform = currentFrame[i].bodyToLimb;
					
				int child_of_k = i;
				for (int k = currentFrame[i].parentLimbIndex; k != child_of_k; child_of_k = k, k = currentFrame[k].parentLimbIndex)
				{
					Multiply(/* OUTPUT  */ transform, currentFrame[k].bodyToLimb, transform);
				}

				ObjectInstance limbInstance{ instance.orientation * transform, instance.highlightColour };
				rc.Draw(currentFrame[i].bodyMeshId, &limbInstance, 1);
			}
		}

		virtual void SetCurrentAnimation(AnimationType type)
		{
			const wchar_t* name = nullptr;

			switch (type)
			{
			case AnimationType_Standstill:
				name = L"standstill";
				break;
			case AnimationType_Running:
				name = L"running";
				break;
			default:
				Throw(0, L"Unknown animation type");
			}

			const auto& seq = e.boneLibrary.GetAnimationSequenceByName(name);
			animationId = seq.id;

			if (animationId == animationId.Invalid())
			{
				Throw(0, L"Could not find animation '%s' in the bone library", name);
			}

			this->animationStart = 0.00_seconds;
		}
	};
}

namespace Dystopia
{
	ISkeletonSupervisor* CreateSkeleton(Environment& e)
	{
		return new Skeleton(e);
	}
}