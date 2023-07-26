#include <rococo.types.h>
#define ROCOCO_ASSETS_API ROCOCO_API_EXPORT
#include <assets/assets.texture.impl.h>
#include <vector>

using namespace Rococo;
using namespace Rococo::Assets;

using TByteArray = std::vector<uint8>;

namespace ANON
{
	struct UninitializedMipMapLevel : IMipMapLevelDescriptor
	{
		bool CanClear() const override
		{
			return true;
		}

		bool CanGPUOperateOnMipMaps(ITextureAsset& asset) const override
		{
			return asset.Index() != (uint32)-1;
		}

		bool CanLoad() const override
		{
			return true;
		}

		bool CanSave() const override
		{
			return false;
		}

		fstring ToString() const override
		{
			return "uninitialized"_fstring;
		}

		bool ReadyForLoad() const override
		{
			return false;
		}
	} s_UninitializedMipMap;

	struct ClearedMipMapLevel : IMipMapLevelDescriptor
	{
		bool CanClear() const override
		{
			return false;
		}

		bool CanGPUOperateOnMipMaps(ITextureAsset& asset) const override
		{
			return asset.Index() != (uint32)-1;
		}

		bool CanLoad() const override
		{
			return true;
		}

		bool CanSave() const override
		{
			// Don't allow the API consumer to save blank files
			return false;
		}

		bool ReadyForLoad() const override
		{
			return false;
		}

		fstring ToString() const override
		{
			return "cleared"_fstring;
		}
	} s_ClearedMipMap;

	struct LoadingMipMapLevel : IMipMapLevelDescriptor
	{
		bool CanClear() const override
		{
			return false;
		}

		bool CanGPUOperateOnMipMaps(ITextureAsset& asset) const override
		{
			UNUSED(asset);
			return false;
		}

		bool CanLoad() const override
		{
			return false;
		}

		bool CanSave() const override
		{
			return false;
		}

		bool ReadyForLoad() const override
		{
			return true;
		}

		fstring ToString() const override
		{
			return "loading"_fstring;
		}
	} s_LoadingMipMap;

	struct LoadedMipMapLevel : IMipMapLevelDescriptor
	{
		bool CanClear() const override
		{
			return true;
		}

		bool CanGPUOperateOnMipMaps(ITextureAsset& asset) const override
		{
			return asset.Index() != (uint32) -1;
		}

		// We prohibit superfluous loading, but...
		// if the API consumer wishes to load again, then he should set the buffer to an uninitialized state first.
		bool CanLoad() const override
		{
			return false;
		}

		// We prohibit redundant saving over data
		bool CanSave() const override
		{
			return false;
		}

		bool ReadyForLoad() const override
		{
			return false;
		}

		fstring ToString() const override
		{
			return "loaded"_fstring;
		}
	} s_LoadedMipMap;

	struct GPUedMipMapLevel : IMipMapLevelDescriptor
	{
		bool CanClear() const override
		{
			return false;
		}

		bool CanGPUOperateOnMipMaps(ITextureAsset& asset) const override
		{
			return asset.Index() != (uint32)-1;
		}

		// We prohibit superfluous loading, but...
		// if the API consumer wishes to load again, then he should set the buffer to an uninitialized state first.
		bool CanLoad() const override
		{
			return false;
		}

		// No mirror for the texture, so nothing to save yet
		bool CanSave() const override
		{
			return false;
		}

		bool ReadyForLoad() const override
		{
			return false;
		}

		fstring ToString() const override
		{
			return "GPUed"_fstring;
		}
	} s_GPUedMipMapLevel;

	struct MirroredMipMapLevel : IMipMapLevelDescriptor
	{
		bool CanClear() const override
		{
			return true;
		}

		bool CanGPUOperateOnMipMaps(ITextureAsset& asset) const override
		{
			return asset.Index() != (uint32)-1;
		}

		// We prohibit superfluous loading, but...
		// if the API consumer wishes to load again, then he should set the buffer to an uninitialized state first.
		bool CanLoad() const override
		{
			return false;
		}

		// Assume the GPU has modified the texture, so saving the data is permitted
		bool CanSave() const override
		{
			return true;
		}

		bool ReadyForLoad() const override
		{
			return false;
		}

		fstring ToString() const override
		{
			return "Mirrored (GPU + Sys-RAM)"_fstring;
		}
	} s_MirroredMipMapLevel;

	struct TextureController : ITextureControllerSupervisor
	{
		ITextureAssetSupervisor& container;
		ITextureAssetsForEngine& engine;
		std::vector<TByteArray> localLevels;

		// Descriptors are currently stateless, but we could add state later if the algorithms needs be
		std::vector<IMipMapLevelDescriptor*> descriptors;
		TexelSpec spec;
		TTextureControllerEvent onLoad;

		enum { LOAD_BEST_SPEC = -1 };

		TextureController(ITextureAssetSupervisor& _container, ITextureAssetsForEngine& _engine, TexelSpec _spec) : container(_container), engine(_engine), spec(_spec)
		{
			localLevels.reserve(ITextureController::MAX_LEVEL_INDEX);
		}

		ITextureAsset& AssetContainer() override
		{
			return container;
		}

		IMipMapLevelDescriptor& operator[](uint32 levelIndex) override
		{
			ValidateLevel(levelIndex);

			if (levelIndex >= descriptors.size())
			{
				Throw(0, "%s: Invalid level index", __FUNCTION__);
			}

			return *descriptors[levelIndex];
		}

		void EnsureLocalLevel(uint32 levelIndex)
		{
			if (spec.bitPlaneCount == 0 || spec.bitsPerBitPlane == 0)
			{
				Throw(0, "[%s]: ITextureController::EnsureLocalLevel(...) requires the colour specification to first be defined", container.Path());
			}

			while (localLevels.size() <= levelIndex)
			{
				TByteArray buffer;
				localLevels.emplace_back(buffer);
				descriptors.push_back(&s_UninitializedMipMap);
			}
		}

		void AllocateLocalLevel(uint32 levelIndex)
		{
			auto& level = localLevels[levelIndex];
			if (level.empty())
			{
				uint32 numberOfBytes = SizeInBytesOf(spec, SpanOf(levelIndex));
				if (numberOfBytes == 0)
				{
					Throw(0, "%s: %s - spec undefined", container.Path(), __FUNCTION__);
				}

				level.resize(numberOfBytes);
			}
		}

		void OnParseFile(TexelSpec imageSpec, Vec2i span, const uint8* texels, int mipMapLevel)
		{
			UNUSED(span);

			if (spec.bitPlaneCount != imageSpec.bitPlaneCount || spec.bitsPerBitPlane != imageSpec.bitsPerBitPlane)
			{
				// Error, the texels contains the error message
				char msg[256];
				SafeFormat(msg, "Error parsing level %d: <spec %u,%u  != imageSpec %u,%u >", mipMapLevel, spec.bitPlaneCount, spec.bitsPerBitPlane, imageSpec.bitPlaneCount, imageSpec.bitsPerBitPlane);
				container.SetError(0, msg);
				onLoad.Invoke(*this, -1);
				return;
			}

			if (span.x != span.y)
			{
				char msg[256];
				SafeFormat(msg, "Error parsing level %d: <span %d, %d was rectangular, and not square>", mipMapLevel, span.x, span.y);
				container.SetError(0, msg);
				onLoad.Invoke(*this, -1);
				return;
			}

			if (mipMapLevel > 0)
			{
				int requiredSpan = SpanOf(mipMapLevel);
				if (span.x != requiredSpan)
				{
					char msg[256];
					SafeFormat(msg, "Error parsing image: <span %d != mipLevelSpan %d>", span.x, requiredSpan);
					container.SetError(0, msg);
					onLoad.Invoke(*this, -1);
					return;
				}

				auto& d = LevelAt(requiredSpan);

				if (!d.ReadyForLoad())
				{
					char msg[256];
					SafeFormat(msg, "The mip map level %d [%s] was marked as not ready for loading.", mipMapLevel, d.ToString());
					container.SetError(0, msg);
					onLoad.Invoke(*this, -1);
					return;
				}

				uint32 nBytes = SizeInBytesOf(spec, requiredSpan);

				EnsureLocalLevel(mipMapLevel);
				localLevels[mipMapLevel].resize(nBytes);
				memcpy(localLevels[mipMapLevel].data(), texels, nBytes);
				descriptors[mipMapLevel] = &s_LoadedMipMap;
				onLoad.Invoke(*this, mipMapLevel);
				return;
			}

			LoadFromTopLevelImage(span, texels);
		}

		static int SizeInBytesOf(TexelSpec _spec, int span)
		{
			uint32 nBytes = Sq(span) * (_spec.bitPlaneCount * _spec.bitsPerBitPlane / 8);
			return nBytes;
		}

		static int SpanOf(int mipMapIndex)
		{
			return 1 << mipMapIndex;
		}

		static int LevelOf(int span)
		{
			int level = 0;
			while (span > 1)
			{
				span = span >> 1;
				level++;
			}
			return level;
		}

		// Arguments have been validated except for span.x as a valid mip map level span
		void LoadFromTopLevelImage(Vec2i span, const uint8* texels)
		{
			int mipMapLevel = LevelOf(span.x);

			if (mipMapLevel > MAX_LEVEL_INDEX)
			{
				char msg[256];
				SafeFormat(msg, "The image [%s] span of %d exceeds the maximum texture span of %d x %d", span.x, SpanOf(MAX_LEVEL_INDEX), SpanOf(MAX_LEVEL_INDEX));
				container.SetError(0, msg);
				onLoad.Invoke(*this, -1);
				return;
			}

			int maxEngineSpan = engine.Factory().GetEngineTextureSpan();
			if (mipMapLevel > maxEngineSpan)
			{
				// The image has a higher resolution than that of the engine quality, this means we should downsample to generate a lower resolution top level mip map. That is if the engine allows it.
				auto onDownsample = [this](Vec2i downSampledSpan, const uint8* downSampledTexels)
				{
					LoadFromTopLevelImage(downSampledSpan, downSampledTexels);
				};

				if (!engine.DownsampleToEngineQuality(spec, span, texels, onDownsample))
				{
					char msg[256];
					SafeFormat(msg, "The engine refused to downsample [%s] of span %d x %d to engine quality %d x %d", span.x, span.x, maxEngineSpan, maxEngineSpan);
					container.SetError(0, msg);
					onLoad.Invoke(*this, -1);
				}

				return;
			}

			size_t nBytes = SizeInBytesOf(spec, span.x);
			EnsureLocalLevel(mipMapLevel);
			localLevels[mipMapLevel].resize(nBytes);
			memcpy(localLevels[mipMapLevel].data(), texels, nBytes);
			descriptors[mipMapLevel] = &s_LoadedMipMap;
			onLoad.Invoke(*this, mipMapLevel);
		}

		void OnLoadFile(const IFileAsset& file, int mipMapLevel) override
		{
			// If this method throws an exception, the texture asset status records the error
			if (file.IsLoaded())
			{
				auto data = file.RawData();

				if (mipMapLevel == LOAD_BEST_SPEC)
				{
					auto onParse = [this, mipMapLevel](TexelSpec spec, Vec2i span, const uint8* texels)->void
					{
						OnParseFile(spec, span, texels, mipMapLevel);
					};

					// This indicates that the file defines the spec					
					container.ParseImage(data, file.Path(), onParse);
				}
			}
		}

		void SetTexelSpec(const TexelSpec& _spec) override
		{
			if (spec.bitPlaneCount != _spec.bitPlaneCount || spec.bitsPerBitPlane != _spec.bitsPerBitPlane)
			{
				Throw(0, "[%s]: ITextureController::SetTexelSpec(%u, %u): The texel specification is already defined as (%u, %u)", container.Path(), _spec.bitPlaneCount, _spec.bitsPerBitPlane, spec.bitPlaneCount, spec.bitsPerBitPlane);
			}

			if (_spec.bitPlaneCount == 0 || _spec.bitPlaneCount > 4)
			{
				Throw(0, "[%s]: ITextureController::SetTexelSpec(_bitPlaneCount = %u, %u): The _bitPlaneCount has domain [1,4]", container.Path(), _spec.bitPlaneCount, _spec.bitsPerBitPlane);
			}

			if ((_spec.bitsPerBitPlane % 8) != 0 || _spec.bitsPerBitPlane < 8 || _spec.bitsPerBitPlane > 32)
			{
				Throw(0, "[%s]: ITextureController::SetTexelSpec(%u, _bitsPerBitPlane = %u): The _bitsPerBitPlane must be one of { 8, 16, 32 }", container.Path(), _spec.bitPlaneCount, _spec.bitsPerBitPlane);
			}
		}

		TexelSpec Spec() const override
		{
			return spec;
		}

		void ValidateLevel(uint32 levelIndex)
		{
			if (levelIndex > ITextureController::MAX_LEVEL_INDEX)
			{
				Throw(0, "[%s]: ITextureController method was invoked in which the [levelIndex] exceeded %u ", container.Path(), ITextureController::MAX_LEVEL_INDEX);
			}
		}

		virtual ~TextureController()
		{
		}

		bool ClearMipMapLevel(uint32 levelIndex) override
		{
			auto& desc = LevelAt(levelIndex);
			if (!desc.CanClear())
			{
				return false;
			}

			auto& byteArray = localLevels[levelIndex];
			memset(byteArray.data(), 0, byteArray.size());
			descriptors[levelIndex] = &s_ClearedMipMap;
			return true;
		}

		void DisposeMipMapLevel(uint32 levelIndex) override
		{
			ValidateLevel(levelIndex);
			descriptors[levelIndex] = &s_UninitializedMipMap;
		}

		// Use the path parameter to identify and load the highest level mip map image.
		// If the texel spec has been set and the image does not match the specification an error is raised in the callback
		// If the texel spec has not been set it will be set by the pixel spec of the image
		bool LoadTopMipMapLevel(TTextureControllerEvent onLoad) override
		{
			this->onLoad = onLoad;
			bool isQueued = container.TryQueueLoadImage(container.Path(), LOAD_BEST_SPEC);
			return isQueued;
		}

		bool AttachToGPU() override
		{
			return engine.AttachToGPU(container);
		}

		void ReleaseFromGPU() override
		{
			engine.ReleaseFromGPU(container);
		}

		// Use the path parameter and the levelIndex to identify the correct mip map level in the file system.
		// If the texel spec has been set and the image does not match the specification an error is raised in the callback
		// If the texel spec has not been set it will be set by the pixel spec of the image
		void LoadMipMapLevel(uint32 levelIndex, TTextureControllerEvent onLoad) override
		{
			this->onLoad = onLoad;

			if (!LevelAt(levelIndex).CanLoad())
			{
				Throw(0, "%s: LoadMipMapLevel - The mip map level #u is in state '%s' and a load cannot be done at this juncture.", container.Path());
			}

			descriptors[levelIndex] = &s_LoadingMipMap;
		}

		// Place the image on the GPU and generate all mips maps of smaller size than that specified. There is no mechanism, other than sampling the GPU buffers, to determine if the operation succeeded
		void GenerateMipMaps(uint32 levelIndex)
		{
			auto& desc = LevelAt(levelIndex);
			if (!desc.CanGPUOperateOnMipMaps(container))
			{
				Throw(0, "%s: GenerateMipMaps - The mip map level #u is in state '%s' and GPU operations on it cannot be done at this juncture. PushToEngine first.", container.Path());
			}

			engine.GenerateMipMaps(levelIndex, container);
		}

		void EnumerateMipMapLevels(TMipMapLevelEnumerator enumerator) override
		{
			uint32 levelSpan = 1;
			for (size_t i = 0; i < localLevels.size(); i++)
			{
				MipMapLevelDesc args(*descriptors[i]);
				args.bytesPerTexel = spec.bitsPerBitPlane * spec.bitPlaneCount >> 3;
				args.levelspan = levelSpan;
				args.mipMapLevel = (uint32) i;
				args.spec = spec;
				args.texelBuffer = localLevels[i].empty() ? nullptr : localLevels[i].data();
				enumerator.Invoke(args);
				levelSpan = levelSpan << 1;
			}
		}

		void FetchAllMipMapLevels() override
		{
			for (int32 i = 0; i < localLevels.size(); i++)
			{
				FetchMipMapLevel(i);
			}
		}

		// Takes the image from the GPU, if available and moves it to the local cache, overwriting any existing data in the cache
		void FetchMipMapLevel(uint32 levelIndex)
		{
			auto& desc = LevelAt(levelIndex);
			if (!desc.CanGPUOperateOnMipMaps(container))
			{
				Throw(0, "%s: FetchMipMapLevel - The mip map level %u is in state '%s' and cannot be overwritten from the GPU.", container.Path(), levelIndex, (cstr) desc.ToString());
			}

			uint32 nBytes = SizeInBytesOf(spec, SpanOf(levelIndex));

			EnsureLocalLevel(levelIndex);
			localLevels[levelIndex].resize(nBytes);

			engine.FetchMipMapLevel(levelIndex, container, localLevels[levelIndex].data());
		}

		// Mirror onto the GPU
		bool PushMipMapLevel(uint32 levelIndex)
		{
			IMipMapLevelDescriptor& desc = LevelAt(levelIndex);
			if (!desc.CanGPUOperateOnMipMaps(container))
			{
				char msg[1024];
				SafeFormat(msg, "%s.CanGPUOperateOnMipMaps() returned false", (cstr) desc.ToString());
				container.SetError(0, msg);
				return false;
			}

			return engine.PushMipMapLevel(levelIndex, container, localLevels[levelIndex].data());
		}

		// Takes the image from the local cache and saves to the file system. Requires the asset path specify a mip map repository
		void SaveMipMapLevel(uint32 levelIndex)
		{
			auto& desc = LevelAt(levelIndex);
			if (!desc.CanSave())
			{
				Throw(0, "%s: FetchMipMapLevel - The mip map level #u is in state '%s' and a load cannot be done at this juncture. PushToGPU first.", container.Path());
			}
		}

		// Allocates local cache for image data, up to the limit of the GPU, the software max mip map level (ITextureController::MAX_LEVEL_INDEX) and the user's graphic detail settings
		// The method returns the maximum mip map level which will not exceed [maxLevelIndex] supplied to the method.
		int ReserveAndReturnReserved(uint32 maxLevelIndex)
		{
			ValidateLevel(maxLevelIndex);
			return maxLevelIndex;
		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo::Assets
{
	ITextureControllerSupervisor* CreateTextureController(ITextureAssetSupervisor& container, ITextureAssetsForEngine& engine, TexelSpec spec)
	{
		return new ANON::TextureController(container, engine, spec);
	}
}