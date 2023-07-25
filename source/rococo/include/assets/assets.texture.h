#pragma once
#include <assets/assets.files.h>

namespace Rococo::IO
{
	struct IInstallation;
}

namespace Rococo::Graphics
{
	struct ITextureManager;
}

namespace Rococo::Assets
{
	enum class ETextureDesignation
	{
		Alpha_8Bit,	// The texture is expected to consist of 256 greyscales, 0 to 1.0f
		RGBA_32Bit	// The texture is expected to consist of 4 colour planes of 8 bits each
	};

	struct ITextureAsset;
	struct ITextureController;
	struct IMipMapLevelDescriptor;
	struct ITextureAssetFactory;
	struct ITextureAssetsForEngine;

	using TTextureControllerEvent = Rococo::Function<void(ITextureController& controller, int32 levelIndex)>;

	// Specification of the bit pattern for each texel entry in a texture
	struct TexelSpec
	{
		uint32 bitPlaneCount = 0;
		uint32 bitsPerBitPlane = 0;
	};

	// Represents a texture outside of the graphics engine, so will persist even if engine textures are flushed.
	ROCOCO_INTERFACE ITextureController
	{
		// This gives 8192x8192.
		enum { MAX_LEVEL_INDEX = 14 };

		// Returns a reference to the asset object that contains this interface
		virtual ITextureAsset& AssetContainer() = 0;

		virtual IMipMapLevelDescriptor& operator[](uint32 index) = 0;

		inline IMipMapLevelDescriptor& LevelAt(uint32 index)
		{
			return this->operator[](index);
		}

		// Use the path parameter to identify and load the highest level mip map image.
		// Returns false if there is an outstanding file load for the texture
		virtual bool LoadTopMipMapLevel(TTextureControllerEvent onLoad) = 0;

		// Loads the mip map level at the specified index. The pixel span of the level is 2^levelIndex.
		// So 0 is 1x1 and 10 is 1024x1024.
		// Returns false if there is an outstanding file load for the texture
		virtual void LoadMipMapLevel(uint32 levelIndex, TTextureControllerEvent onLoad) = 0;

		// For a given level Index, uses the GPU to generate lower level mip maps.
		// If necessary a texture resource is created for the GPU to mirror the CPU/sys memory version of the texture
		// Note that the CPU copies' mip map levels are not modified, the generation takes place only on GPU.
		// Call FetchMipMapLevel to retrieve CPU copies of the GPU's computated results
		virtual void GenerateMipMaps(uint32 levelIndex) = 0;

		// Zeroes out the local cache without deallocating memory. Returns false if the cache cannot be cleared at this time
		virtual bool ClearMipMapLevel(uint32 levelIndex) = 0;

		// Marks mip map level memory for disposal. The state of the CPU memory is set to uninitialized.
		virtual void DisposeMipMapLevel(uint32 levelIndex) = 0;

		// Creates a texture resource on the GPU to mirror the CPU/sys memory version if needs be
		virtual void AttachToGPU() = 0;

		// Removes the GPU representation of the texture.
		virtual void ReleaseFromGPU() = 0;

		// Asks the graphics engine to fetch the mip map level at the specified index into system memory
		virtual void FetchMipMapLevel(uint32 levelIndex) = 0;

		// Persists the mip map level at the specified index to a subdirectory of the texture path. 
		// This requires the texture path to be a subdirectory or image archive file
		virtual void SaveMipMapLevel(uint32 levelIndex) = 0;

		// Sets the specification for each colour component in the texture
		virtual void SetTexelSpec(const TexelSpec& spec) = 0;

		virtual TexelSpec Spec() const = 0;

		// Attempts to reserve space for the texture with a mip map level of maximum span of 2^levelIndex
		// The method returns the actual levelIndex allowed.
		virtual int ReserveAndReturnReserved(uint32 maxLevelIndex) = 0;
	};

	ROCOCO_INTERFACE IMipMapLevelDescriptor
	{
		virtual bool CanClear() const = 0;
		virtual bool CanLoad() const = 0;
		virtual bool CanSave() const = 0;
		virtual bool CanGPUOperateOnMipMaps() const = 0;
		virtual bool ReadyForLoad() const = 0;
		virtual fstring ToString() const = 0;
	};

	ROCOCO_INTERFACE ITextureAsset : IAsset
	{
		virtual ITextureController & Tx() = 0;

		// Gets the status code (HRESULT on Win32, otherwise OS dependent) of the file error, populates and truncates the supplied buffer with an error message
		// and returns the length of the full error string with terminating null character. If buffer or nBytes in buffer are null, the buffer is ignored.
		virtual size_t GetErrorAndStatusLength(int& statusCode, char* buffer = nullptr, size_t nBytesInBuffer = 0) const = 0;

		// Throws an exception if the asset is in an error state and adds the filename and/or the function name to the error message if booleans are set to true
		virtual void Validate(bool addFilename = true, bool addFunction = true) = 0;

		virtual cstr Path() const = 0;
	};

	using TImageLoadEvent = Rococo::Function<void (TexelSpec spec, Vec2i span, const uint8* texels)>;

	ROCOCO_INTERFACE ITextureAssetSupervisor : ITextureAsset
	{
		// Sent by the controller's implementation to the texture loader to load an image for the given mip map level. The index is passed to ITextureControllerSupervisor::OnLoadFile
		// The method returns false if there is already a pending load
		virtual bool TryQueueLoadImage(cstr path, int mipMapLevel) = 0;
		virtual void ParseImage(const FileData& data, cstr path, TImageLoadEvent onParse) = 0;
		virtual void SetError(int statusCode, cstr message) = 0;
		virtual ITextureAssetsForEngine& Engine() = 0;
	};

	ROCOCO_INTERFACE ITextureControllerSupervisor: ITextureController
	{
		virtual void Free() = 0;
		virtual void OnLoadFile(const IFileAsset& file, int mipMapLevel) = 0;
	};

	ITextureControllerSupervisor* CreateTextureController(ITextureAssetSupervisor& asset);

	using TAsyncOnTextureLoadEvent = Rococo::Function<void(ITextureAsset& asset)>;

	ROCOCO_ASSETS_API void NoTextureCallback(ITextureAsset& asset);

	ROCOCO_INTERFACE ITextureAssetsForEngine
	{
		virtual void AttachToGPU(ITextureControllerSupervisor & controller) = 0;
		virtual bool DownsampleToEngineQuality(TexelSpec imageSpec, Vec2i span, const uint8* texels, Rococo::Function<void(Vec2i downSampledSpan, const uint8* downSampledTexels)> onDownsample) const = 0;
		virtual ITextureAssetFactory& Factory() = 0;
	};

	ROCOCO_INTERFACE ITextureAssetFactory
	{
		virtual AssetRef<ITextureAsset> CreateTextureAsset(const char* utf8Path, TAsyncOnTextureLoadEvent onLoad = NoTextureCallback) = 0;

		// Creates an array of square textures of given span and length. If the array exists already it is destroyed and the new array takes its place
		// This method generally invoked once per instance, but may change if the user selects different texture qualities. 
		// Texture quality is specified in span, so 2048x2048 is clearly higher quality than 1024x1024.
		// Span must be a power of 2 with a miminum of 256 and maximum of 8192. The maximum number of elements in the array is 1024.
		// If the graphics card cannot handle the parameters, or another error occurs the method will throw an exception.
		// Other API methods may throw an exception if the span is not set before use.
		virtual void SetEngineTextureArray(uint32 spanInPixels, int32 numberOfElementsInArray) = 0;

		// Returns the parameter set in SetEngineTextureArray([spanInPixels], ...)
		// This defines the engine quality of textures in terms of pixel span. [spanInPixels] will be a power of 2. The array top level textures all have this span
		// Span can be changed mid-application in the graphic settings.
		// High quality textures are [2048x2048], mid are [1024x1024], low [512x512], very low [256x256]. The maximum quality is 8192x8192
		virtual int GetEngineTextureQualitySpan() const = 0;
	};

	ROCOCO_INTERFACE ITextureAssetFactorySupervisor : ITextureAssetFactory
	{
		virtual void Free() = 0;
	};

	ROCOCO_ASSETS_API ITextureAssetFactorySupervisor* CreateTextureAssetFactory(Graphics::ITextureManager& engineTextures, IFileAssetFactory& fileManager);
}