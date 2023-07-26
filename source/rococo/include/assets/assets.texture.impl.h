#pragma once
#include <assets/assets.texture.h>

// This is the private internal API of the asset texture system. 

namespace Rococo::Assets
{
	struct ITextureAssetsForEngine;

	using TImageLoadEvent = Rococo::Function<void (TexelSpec spec, Vec2i span, const uint8* texels)>;

	ROCOCO_INTERFACE ITextureAssetSupervisor : ITextureAsset
	{
		// Sent by the controller's implementation to the texture loader to load an image for the given mip map level. The index is passed to ITextureControllerSupervisor::OnLoadFile
		// The method returns false if there is already a pending load
		virtual bool TryQueueLoadImage(cstr path, int mipMapLevel) = 0;
		virtual void ParseImage(const FileData& data, cstr path, TImageLoadEvent onParse) = 0;
		virtual void SetError(int statusCode, cstr message) = 0;
	};

	ROCOCO_INTERFACE ITextureControllerSupervisor: ITextureController
	{
		virtual void Free() = 0;
		virtual void OnLoadFile(const IFileAsset& file, int mipMapLevel) = 0;
	};

	ITextureControllerSupervisor* CreateTextureController(ITextureAssetSupervisor& asset, ITextureAssetsForEngine& Engine, TexelSpec spec);

	using TAsyncOnTextureLoadEvent = Rococo::Function<void(ITextureAsset& asset)>;

	ROCOCO_INTERFACE ITextureAssetsForEngine
	{
		virtual bool AttachToGPU(ITextureAsset& asset) = 0;
		virtual bool FetchMipMapLevel(uint32 levelIndex, ITextureAsset& asset, uint8* mipMapLevelDataDestination) = 0;
		virtual bool PushMipMapLevel(uint32 levelIndex, ITextureAsset& asset, const uint8* mipMapLevelData) = 0;
		virtual void GenerateMipMaps(uint32 levelIndex, ITextureAsset& asset) = 0;
		virtual void ReleaseFromGPU(ITextureAsset& asset) = 0;
		virtual bool DownsampleToEngineQuality(TexelSpec imageSpec, Vec2i span, const uint8* texels, Rococo::Function<void(Vec2i downSampledSpan, const uint8* downSampledTexels)> onDownsample) const = 0;
		virtual ITextureAssetFactory& Factory() = 0;
	};
}