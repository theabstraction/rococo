#include <assets/assets.texture.h>
#include <vector>

using namespace Rococo;
using namespace Rococo::Assets;

namespace ANON
{
	struct UninitializedMipMapLevel : IMipMapLevelDescriptor
	{
		bool CanClear() const override
		{
			return true;
		}

		bool CanGPUOperateOnMipMaps() const override
		{
			return false;
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
	} s_UninitializedMipMap;

	struct ClearedMipMapLevel : IMipMapLevelDescriptor
	{
		bool CanClear() const override
		{
			return false;
		}

		bool CanGPUOperateOnMipMaps() const override
		{
			return false;
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

		bool CanGPUOperateOnMipMaps() const override
		{
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

		bool CanGPUOperateOnMipMaps() const override
		{
			return false;
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

		bool CanGPUOperateOnMipMaps() const override
		{
			return true;
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

		bool CanGPUOperateOnMipMaps() const override
		{
			return true;
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

		fstring ToString() const override
		{
			return "Mirrored (GPU + Sys-RAM)"_fstring;
		}
	} s_MirroredMipMapLevel;

	struct TextureController : ITextureControllerSupervisor
	{
		using TByteArray = std::vector<uint8>;
		ITextureAssetSupervisor& container;
		std::vector<TByteArray> localLevels;
		std::vector<IMipMapLevelDescriptor*> descriptors;
		TexelSpec spec;
		TTextureControllerEvent onLoad;

		enum { LOAD_AND_DEFINE_SPEC = -1 };

		TextureController(ITextureAssetSupervisor& _container): container(_container)
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
				uint32 levelSpan = 1 << localLevels.size();
				uint32 bytesPerTexel = spec.bitPlaneCount * spec.bitsPerBitPlane >> 3;
				uint32 numberOfBytes = Sq(levelSpan) * bytesPerTexel;
				if (numberOfBytes == 0)
				{
					Throw(0, "%s: %s - spec undefined", container.Path(), __FUNCTION__);
				}

				level.resize(numberOfBytes);
			}
		}

		void OnParseFile(TexelSpec spec, Vec2i span, const uint8* texels, int mipMapLevel)
		{

		}

		void OnLoadFile(const IFileAsset& file, int mipMapLevel) override
		{
			// If this method throws an exception, the texture asset status records the error
			if (file.IsLoaded())
			{
				auto data = file.RawData();

				if (mipMapLevel == LOAD_AND_DEFINE_SPEC)
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
		void LoadTopMipMapLevel(TTextureControllerEvent onLoad) override
		{
			container.QueueLoadImage(container.Path(), LOAD_AND_DEFINE_SPEC);
		}

		void AttachToGPU(uint32 levelIndex) override
		{

		}

		void ReleaseFromGPU() override
		{

		}

		// Use the path parameter and the levelIndex to identify the correct mip map level in the file system.
		// If the texel spec has been set and the image does not match the specification an error is raised in the callback
		// If the texel spec has not been set it will be set by the pixel spec of the image
		void LoadMipMapLevel(uint32 levelIndex, TTextureControllerEvent onLoad) override
		{
			if (!LevelAt(levelIndex).CanLoad())
			{
				Throw(0, "%s: LoadMipMapLevel - The mip map level #u is in state '%s' and a load cannot be done at this juncture.", container.Path());
			}

			descriptors[levelIndex] = &s_LoadingMipMap;
		}

		// Place the image on the GPU and generate all mips maps of smaller size than that specified. 
		void GenerateMipMaps(uint32 levelIndex)
		{
			auto& desc = LevelAt(levelIndex);
			if (!desc.CanGPUOperateOnMipMaps())
			{
				Throw(0, "%s: GenerateMipMaps - The mip map level #u is in state '%s' and GPU operations on it cannot be done at this juncture. PushToEngine first.", container.Path());
			}
		}

		// Takes the image from the GPU, if available and moves it to the local cache, overwriting any existing data in the cache
		void FetchMipMapLevel(uint32 levelIndex)
		{
			auto& desc = LevelAt(levelIndex);
			if (!desc.CanGPUOperateOnMipMaps())
			{
				Throw(0, "%s: FetchMipMapLevel - The mip map level #u is in state '%s' and a load cannot be done at this juncture. PushToGPU first.", container.Path());
			}

			EnsureLocalLevel(levelIndex);
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
	ITextureControllerSupervisor* CreateTextureController(ITextureAssetSupervisor& container)
	{
		return new ANON::TextureController(container);
	}
}