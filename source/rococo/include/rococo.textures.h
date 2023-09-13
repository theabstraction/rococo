#ifndef ROCOCO_TEXTURES_H
#define ROCOCO_TEXTURES_H

#ifndef RENDERER_API
# define RENDERER_API ROCOCO_API_IMPORT
#endif

namespace Rococo::Graphics
{
    namespace Textures
    {
        enum COMPRESSED_TYPE
        {
            COMPRESSED_TYPE_TIF,
            COMPRESSED_TYPE_JPG
        };

        struct CompressedTextureBuffer
        {
            const uint8* data;
            const size_t nBytes;
            COMPRESSED_TYPE type;
        };

        ROCOCO_INTERFACE ICompressedResourceLoader
        {
           virtual void Load(cstr pingPath, IEventCallback<CompressedTextureBuffer>&onLoad) = 0;
        };

        struct BitmapLocation
        {
            GuiRect txUV;
            int32 textureIndex;
            Vec2 pixelSpan;

            static BitmapLocation None()
            {
                return { {0,0,0,0}, -1, {0,0} };
            }
        };

        struct BitmapUpdate
        {
            cstr name;
            int hr;
            cstr msg;
        };

        ROCOCO_INTERFACE IBitmapArrayBuilder
        {
           virtual void AddBitmap(cstr name) = 0;
           virtual bool TryGetBitmapLocation(cstr name, BitmapLocation& location) = 0;
           virtual void BuildBitmaps(int32 minWidth, IEventCallback<BitmapUpdate>* onUpdate = nullptr) = 0;
           virtual void Clear() = 0;
        };

        ROCOCO_INTERFACE IBitmapArrayBuilderSupervisor : public IBitmapArrayBuilder
        {
           virtual void Free() = 0;
        };

        ROCOCO_INTERFACE IGraphicsMemoryContext
        {

        };

        ROCOCO_INTERFACE IBitmapArray
        {
            virtual void AddTexture() = 0;
            virtual void ResetWidth(int32 width) = 0;
            virtual void ResetWidth(int32 width, int32 height) = 0;
            virtual void WriteSubImage(size_t index, const RGBAb* pixels, const GuiRect& targetLocatio) = 0;
            virtual void WriteSubImage(size_t index, const uint8* grayScalePixels, Vec2i span) = 0;
            virtual int32 MaxWidth() const = 0;
            virtual size_t TextureCount() const = 0;
        };

        ROCOCO_MISC_UTILS_API void StandardLoadFromCompressedTextureBuffer(cstr name, IEventCallback<CompressedTextureBuffer>& onLoad, IO::IInstallation& installation, IExpandingBuffer& buffer);
        ROCOCO_MISC_UTILS_API IBitmapArrayBuilderSupervisor* CreateBitmapArrayBuilder(ICompressedResourceLoader& loader, IBitmapArray& textureArray);

        ROCOCO_INTERFACE IMipMappedTextureArray
        {
            virtual uint32 TexelSpan() const = 0;
            virtual uint32 NumberOfElements() const = 0;
            virtual uint32 NumberOfMipLevels() const = 0;
            virtual bool ReadSubImage(uint32 index, uint32 mipMaplevel, uint32 bytesPerTexel, uint8* mipMapLevelDataDestination) = 0;
            virtual void WriteSubImage(uint32 index, uint32 mipMapLevel, const RGBAb* pixels, const GuiRect& targetLocation) = 0;
            virtual void WriteSubImage(uint32 index, uint32 mipMapLevel, const uint8* alphaTexels, const GuiRect& targetLocation) = 0;
            virtual void GenerateMipMappedSubLevels(uint32 index, uint32 mipMapLevel) = 0;
        };

        ROCOCO_INTERFACE IMipMappedTextureArraySupervisor: IMipMappedTextureArray
        {
            virtual void Free() = 0;
        };
    } // Rococo::Graphics::Textures
} // Rococo::Graphics

#endif