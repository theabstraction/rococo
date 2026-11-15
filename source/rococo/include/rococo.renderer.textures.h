#pragma once

#include <rococo.graphics.types.h>
#include <rococo.renderer.formats.h>

namespace Rococo::Imaging
{
	DECLARE_ROCOCO_INTERFACE IImageLoadEvents;
}

namespace Rococo::RAL
{
	DECLARE_ROCOCO_INTERFACE IGBuffers;
	DECLARE_ROCOCO_INTERFACE IRenderTargetSupervisor;
}

namespace Rococo::Graphics::Textures
{
	DECLARE_ROCOCO_INTERFACE IMipMappedTextureArraySupervisor;
}

namespace Rococo
{
	DECLARE_ROCOCO_INTERFACE IMathsVisitor;
}

namespace Rococo::Graphics
{
	DECLARE_ROCOCO_INTERFACE ITextureLoadEnumerator;
	DECLARE_ROCOCO_INTERFACE ICubeTextures;
	
	struct TextureArrayCreationFlags
	{
		boolean32 allowMipMapGeneration : 1;
		boolean32 allowCPUread : 1;
	};

	struct TextureDesc
	{
		uint32 width;
		uint32 height;
		TextureFormat format;
	};

	ROCOCO_INTERFACE ITextureManager
	{
		virtual void AssignToPS(uint32 unitId, ID_TEXTURE texture) = 0;
		virtual void AssignMaterialsToPS() = 0;
		virtual void SetRenderTarget(ID_TEXTURE depthTarget, ID_TEXTURE renderTarget) = 0;
		virtual void SetRenderTarget(RAL::IGBuffers& g, ID_TEXTURE depthTarget) = 0;

		virtual ID_TEXTURE CreateDepthTarget(cstr targetName, int32 width, int32 height) = 0;
		virtual ID_TEXTURE CreateRenderTarget(cstr renderTargetName, int32 width, int32 height, TextureFormat format) = 0;

		virtual RAL::IRenderTargetSupervisor* CreateDynamicRenderTarget(cstr name) = 0;
		virtual RAL::IRenderTargetSupervisor* CreateDynamicDepthTarget(cstr name) = 0;
		virtual RAL::IRenderTargetSupervisor* CreateDynamicNormalTarget(cstr name) = 0;
		virtual RAL::IRenderTargetSupervisor* CreateDynamicVec4Target(cstr name) = 0;

		virtual ID_VOLATILE_BITMAP CreateVolatileBitmap(cstr pingPath) = 0;
		virtual void Free() = 0;
		virtual ID_TEXTURE LoadAlphaTextureArray(cstr uniqueName, Vec2i span, int32 nElements, ITextureLoadEnumerator& enumerator) = 0;
		virtual ID_TEXTURE FindTexture(cstr name) const = 0;
		virtual ICubeTextures& CubeTextures() = 0;
		virtual void SetGenericTextureArray(ID_TEXTURE id) = 0;
		virtual void ShowTextureVenue(IMathsVisitor& visitor) = 0;
		virtual int64 Size() const = 0;
		virtual bool TryGetTextureDesc(TextureDesc& desc, ID_TEXTURE id) const = 0;
		virtual Vec2i GetRenderTargetSpan(ID_TEXTURE id) const = 0;
		virtual Vec2i GetTextureSpan(ID_TEXTURE id) const = 0;

		virtual void CompressJPeg(const RGBAb* data, Vec2i span, cstr filename, int quality) const = 0;
		virtual void CompressTiff(const RGBAb* data, Vec2i span, cstr filename) const = 0;

		// Forward on the jpeg decompression function from the lib-jpeg lib
		virtual bool DecompressJPeg(Imaging::IImageLoadEvents& loadEvents, const unsigned char* sourceBuffer, size_t dataLengthBytes) const = 0;

		// Forward on the tiff decompression function from the lib-tiff lib
		virtual bool DecompressTiff(Imaging::IImageLoadEvents& loadEvents, const unsigned char* sourceBuffer, size_t dataLengthBytes) const = 0;

		virtual Textures::IMipMappedTextureArraySupervisor* DefineRGBATextureArray(uint32 numberOfElements, uint32 span, TextureArrayCreationFlags flags) = 0;
	};
}