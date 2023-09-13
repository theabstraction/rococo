#include <rococo.animation.h>
#include <rococo.maths.h>
#include <rococo.strings.h>
#include <vector>

#include <components/rococo.components.animation.h>

using namespace Rococo;
using namespace Rococo::Entities;
using namespace Rococo::Strings;

namespace Rococo::Animation
{
	struct KeyFrame
	{
		// The duration from the start of this keyframe to the start of the next keyframe
		Seconds duration;
		boolean32 loop;
		char name[MAX_POSENAME_LEN];
		mutable ID_POSE poseId;
	};

	ID_SKELETON ToSkeletonId(ID_POSE id)
	{
		return ID_SKELETON{ id.value };
	}

	ID_POSE ToPoseId(ID_SKELETON id)
	{
		return ID_POSE{ id.value };
	}

	void LerpPoseBonesToPuppet(IBone& puppetBone, const IBone* a, const IBone* b, const float t)
	{
		if (a && Eq(puppetBone.ShortName(), a->ShortName()))
		{
			if (b && Eq(puppetBone.ShortName(), b->ShortName()))
			{
				auto lerpAB = InterpolateRotations(a->Quat(), b->Quat(), t);
				puppetBone.SetQuat(lerpAB);

				auto len = std::distance(puppetBone.begin(), puppetBone.end());

				if (len != std::distance(a->begin(), a->end()))
					return;

				if (len != std::distance(b->begin(), b->end()))
					return;

				auto i = a->begin();
				auto j = b->begin();

				for (auto* child : puppetBone)
				{
					LerpPoseBonesToPuppet(*child, *i++, *j++, t);
				}
			}
		}
	}

	class AnimationImpl : public IAnimationComponent
	{
		std::vector<KeyFrame> frames;
		float t = 0;
		Seconds totalAnimationDuration = 0.0_seconds;
	public:
		AnimationImpl(InstanceInfo&)
		{

		}

		ComponentTypeInfo TypeInfo() const override
		{
			return ComponentTypeInfo{ "AnimationImpl" };
		};

		void Reflect(ComponentReflectionInfo& info) override
		{
			UNUSED(info);
		}

		void Advance(AnimationAdvanceArgs& args)
		{
			switch (frames.size())
			{
			case 0: // No frames set
				t = 0;
				return;
			case 1: // Only 1 frame, so set a static pose
				t = 0;
				MatchKeyFrame(args, frames[0]);
				return;
			default: // > 1 frame, so interpolate between them
				break;
			}

			t += args.dt;
		pickOutFrame:
			float sumOfDurations = 0;

			for (int i = 0; i < frames.size(); ++i)
			{
				auto& k = frames[i];

				sumOfDurations += k.duration;

				const float timeLeft = sumOfDurations - t;
				if (timeLeft > 0)
				{
					// i is the index of the first key frame that has not expired
					const KeyFrame& start = frames[i];
					const KeyFrame& end = frames[(i + 1) % frames.size()];

					float interpolationFactor = 1.0f - timeLeft / start.duration;
					Lerp(args, start, end, interpolationFactor);
					return;
				}
			}

			// We have rolled past the end
			const auto& lastFrame = frames.back();
			if (lastFrame.loop)
			{
				while (t > totalAnimationDuration)
				{
					t -= totalAnimationDuration;
				}
				// ...and we are looping so return to the top and try again
				goto pickOutFrame;
			}
			else
			{
				MatchKeyFrame(args, lastFrame);
			}
		}

		void Lerp(AnimationAdvanceArgs& args, const KeyFrame& start, const KeyFrame& end, float q)
		{
			ISkeleton* startPose = nullptr;
			if (!args.poses.TryGet(ToSkeletonId(start.poseId), &startPose))
			{
				start.poseId = ToPoseId(args.poses.GetByNameAndReturnId(start.name, &startPose));
			}

			ISkeleton* endPose = nullptr;
			if (!args.poses.TryGet(ToSkeletonId(end.poseId), &endPose))
			{
				end.poseId = ToPoseId(args.poses.GetByNameAndReturnId(end.name, &endPose));
			}

			if (startPose == nullptr) startPose = endPose;
			if (endPose == nullptr) endPose = startPose;

			if (!startPose)
			{
				return;
			}

			auto* puppetRoot = args.puppet.Root();
			if (puppetRoot)
			{
				LerpPoseBonesToPuppet(*puppetRoot, startPose->Root(), endPose->Root(), q);
			}
		}

		void MatchKeyFrame(AnimationAdvanceArgs& args, const KeyFrame& key)
		{
			ISkeleton* pose = nullptr;
			if (!args.poses.TryGet(ToSkeletonId(key.poseId), &pose))
			{
				key.poseId = ToPoseId(args.poses.GetByNameAndReturnId(key.name, &pose));
			}

			if (pose)
			{
				auto* puppetRoot = args.puppet.Root();
				if (puppetRoot)
				{
					LerpPoseBonesToPuppet(*puppetRoot, pose->Root(), pose->Root(), 0.0f);
				}
			}
		}

		void AddKeyFrame(const fstring& frameName, Seconds duration, boolean32 loopAnimation) override
		{
			constexpr float minDuration = 0.01f;

			if (duration < minDuration)
			{
				Throw(0, "%s: minimum duration is %f seconds", __FUNCTION__, minDuration);
			}

			if (frameName.length == 0)
			{
				frames.clear();
				return;
			}

			KeyFrame key;
			key.duration = duration;
			key.loop = loopAnimation;
			SafeFormat(key.name, "%s", (cstr)frameName);

			frames.push_back(key);

			totalAnimationDuration.value += duration;
		}
	};
}

#include <components/rococo.ecs.builder.inl>

DEFINE_DEFAULT_FACTORY(IAnimationComponent, Rococo::Animation::AnimationImpl);
