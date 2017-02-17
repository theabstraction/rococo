#include "dystopia.h"
#include "rococo.maths.h"
#include "skeleton.h"
#include "meshes.h"
#include <array>
#include <unordered_map>

#include <sexy.types.h>
#include <Sexy.S-Parser.h>

#include "rococo.keys.h"

namespace
{
	using namespace Rococo;
	using namespace Dystopia;
	using namespace Sexy;
	using namespace Sexy::Sex;


	struct Keyframe
	{
		ID_KEYFRAME id;
		SkeletonType type;
		std::vector<BoneOrientation> limbs;
	};

	struct Animation
	{
		ID_ANIMATION id;
		std::vector<FrameInfo> keyframeIds;
	};

	class BoneLibrary : public IBoneLibrarySupervisor
	{
	private:
		IInstallation& installation;
		IRenderer& renderer;
		ISourceCache& sources;

		StringKey filename;

		std::unordered_map<StringKey, Keyframe*, HashStringKey> keyframes;
		std::vector<Keyframe*> keyframesByIndex;

		std::unordered_map<StringKey, Animation*, HashStringKey> animations;
		std::vector<Animation*> animationsByIndex;

		void ParseBone(cr_sex sbone, BoneOrientation& bone)
		{
			if (sbone.NumberOfElements() != 3)
			{
				ThrowSex(sbone, L"Expecting 3 elements in bone definition");
			}

			auto& hint = GetAtomicArg(sbone[0]);
			cr_sex squat = sbone[1];
			cr_sex sdisp = sbone[2];

			if (sdisp.NumberOfElements() != 3)
			{
				ThrowSex(sdisp, L"Expecting vector (x y z)");
			}

			bone.parentToChildDisplacement = GetVec3Value(sdisp[0], sdisp[1], sdisp[2]);
			bone.rotation = GetQuat(squat);
		}

		void ParseKeyframe(cr_sex sdirective)
		{
			if (sdirective.NumberOfElements() != 4)
			{
				ThrowSex(sdirective, L"Expecting 4 elements in a keyframe directive. (keyframe <name> <type> (...)) ");
			}

			auto* nkf = new Keyframe();
			auto& keyframeName = GetAtomicArg(sdirective[1]);
			auto i = keyframes.insert(std::make_pair(StringKey(keyframeName, true), nkf));
			if (!i.second)
			{
				delete nkf;
				ThrowSex(sdirective[1], L"Duplicate keyframe name.", keyframeName);
			}

			keyframesByIndex.push_back(nkf);
			nkf->id = ID_KEYFRAME((uint32)keyframesByIndex.size());

			auto& skeletonType = GetAtomicArg(sdirective[2]);

			auto& k = *nkf;

			if (!TryShortParse(skeletonType, k.type))
			{
				ThrowSex(sdirective[2], L"Unknown skeleton type");
			}

			cr_sex sbones = sdirective[3];
			if (sbones.NumberOfElements() != LimbIndex_Count)
			{
				ThrowSex(sbones, L"Expecting %d bone elements in %s keyframe definition", LimbIndex_Count, skeletonType.buffer);
			}

			k.limbs.resize(sbones.NumberOfElements());

			for (int i = 0; i < sbones.NumberOfElements(); ++i)
			{
				auto& bone = k.limbs[i];
				auto& sbone = sbones[i];
				ParseBone(sbone, bone);
			}
		}

		void ParseAnimationFrame(cr_sex sframe, FrameInfo& frame)
		{
			if (sframe.NumberOfElements() != 2)
			{
				ThrowSex(sframe, L"Expecting (<keyframe-name> <expiry-time-seconds>)");
			}

			auto& kname = GetAtomicArg(sframe[0]);
			auto& duration = GetAtomicArg(sframe[1]);

			frame.actualisationTime.value = (float)_wtof(duration);

			if (frame.actualisationTime.value <= 0.01_seconds)
			{
				ThrowSex(sframe, L"Bad expire time for frame %s. Must be more than 0.01 seconds", kname.buffer);
			}

			if (kname.buffer[0] == L'#')
			{
				frame.id = ID_KEYFRAME::Invalid();

				auto j = animations.find(StringKey(kname.buffer + 1, false));
				if (j == animations.end())
				{
					ThrowSex(sframe[0], L"Could not find animation %s", kname.buffer + 1);
				}

				frame.nextAnimationId = j->second->id;
			}
			else
			{
				frame.nextAnimationId = ID_ANIMATION::Invalid();
				auto k = keyframes.find(StringKey(kname, false));
				if (k == keyframes.end())
				{
					ThrowSex(sframe[0], L"Cannot find keyframe");
				}

				frame.id = k->second->id;
			}
		}

		void ParseAnimation(cr_sex sdirective)
		{
			if (sdirective.NumberOfElements() != 4)
			{
				ThrowSex(sdirective, L"Expecting 4 elements in a keyframe directive. (animation <type> <type> (...)) ");
			}

			auto& skeletonType = GetAtomicArg(sdirective[1]);

			SkeletonType type;
			if (!TryShortParse(skeletonType, type))
			{
				ThrowSex(sdirective[1], L"Expecting SkeletonType short name");
			}

			auto& animationName = GetAtomicArg(sdirective[2]);

			auto anim = new Animation();
			auto i = animations.insert(std::make_pair(StringKey(animationName, true), anim));
			if (!i.second)
			{
				delete anim;
				ThrowSex(sdirective, L"Duplicate animation name: %s", animationName.buffer);
			}

			animationsByIndex.push_back(anim);
			anim->id = ID_ANIMATION((uint32)animationsByIndex.size());

			cr_sex sframes = sdirective[3];

			if (!IsCompound(sframes))
			{
				ThrowSex(sframes, L"Expecting compound expression - sequence of animation frames");
			}

			anim->keyframeIds.resize(sframes.NumberOfElements());
			for (int i = 0; i < sframes.NumberOfElements(); ++i)
			{
				ParseAnimationFrame(sframes[i], anim->keyframeIds[i]);
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
				else if (sdirective[0] == L"animation")
				{
					ParseAnimation(sdirective);
				}
				else
				{
					ThrowSex(sdirective, L"Unknown expression");
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
				Vec2i p = pex.Start();
            Vec2i q = pex.End();
				Throw(0, L"Error parsering %s\n%s: %s\n(%d,%d) to (%d,%d)\n%s", resourcePath, pex.Name(), pex.Message(), p.x, p.y, q.x, q.y, pex.Specimen());
			}
		}
	public:
		BoneLibrary(IInstallation& _installation, IRenderer& _renderer, ISourceCache& _sources) :
			installation(_installation),
			renderer(_renderer),
			sources(_sources),
			filename(L"", false)
		{

		}

		~BoneLibrary()
		{
			Clear();
		}

		void Clear()
		{
			animationsByIndex.clear();
			keyframesByIndex.clear();

			for (auto i : animations)
			{
				delete i.second;
			}

			for (auto i : keyframes)
			{
				delete i.second;
			}

			keyframes.clear();
			animations.clear();
		}

		virtual void Free()
		{
			delete this;
		}

		virtual void Reload(const wchar_t* _filename)
		{
			this->filename = StringKey(_filename, true);

			while (true)
			{
				try
				{
					Clear();
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

		virtual AnimationSequence GetAnimationSequenceByName(const wchar_t* name)
		{
			auto i = animations.find(StringKey(name, false));
			if (i == animations.end())
			{
				return{ nullptr, nullptr, 0, ID_ANIMATION::Invalid() };
			}
			else
			{
				auto& a = *i->second;

				return
				{
					&a.keyframeIds[0],
					&a.keyframeIds[0] + a.keyframeIds.size(),
					a.keyframeIds.size(),
					a.id
				};
			}
		}

		virtual AnimationSequence GetAnimationSequenceById(ID_ANIMATION id)
		{
			size_t index = id.value - 1;
			if (index > animationsByIndex.size())
			{
				Throw(0, L"GetAnimationSequenceById(%u): id invalid.", id.value);
			}

			auto& a = *animationsByIndex[index];
			return
			{
				&a.keyframeIds[0],
				&a.keyframeIds[0] + a.keyframeIds.size(),
				a.keyframeIds.size(),
				a.id
			};
		}

		void EnumerateKeyframeBones(const Keyframe& keyframe, IBoneEnumerator& onFrame)
		{
			onFrame.OnType(keyframe.type);

			int32 index = 0;
			for (auto bone : keyframe.limbs)
			{
				onFrame.OnBone(bone, index++);
			}
		}

		virtual int32 EnumerateKeyframeBonesByName(const wchar_t* name, IBoneEnumerator& onFrame)
		{
			StringKey key(name, false);
			auto i = keyframes.find(key);
			if (i != keyframes.end())
			{
				return 0;
			}
			else
			{
				EnumerateKeyframeBones(*i->second, onFrame);
				return (int32)i->second->limbs.size();
			}
		}

		virtual int32 EnumerateKeyframeBonesById(ID_KEYFRAME id, IBoneEnumerator& onFrame)
		{
			uint32 index = id.value - 1;
			if (index >= keyframesByIndex.size())
			{
				Throw(0, L"Bad keyframe id #%u", id.value);
			}

			auto& k = *keyframesByIndex[index];

			EnumerateKeyframeBones(k, onFrame);

			return (int32)k.limbs.size();
		}

		virtual void UpdateLib(const wchar_t* updatedFile)
		{
			if (wcscmp(filename + 1, updatedFile) == 0)
			{
				sources.Release(filename);
				Reload(filename);
			}
		}
	};
}


namespace Dystopia
{
	IBoneLibrarySupervisor* CreateBoneLibrary(IInstallation& installation, IRenderer& renderer, ISourceCache& sources)
	{
		return new BoneLibrary(installation, renderer, sources);
	}
}