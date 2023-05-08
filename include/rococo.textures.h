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
         virtual void Load(cstr pingPath, IEventCallback<CompressedTextureBuffer>& onLoad) = 0;
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

      ROCOCO_INTERFACE ITextureArrayBuilder
      {
         virtual void AddBitmap(cstr name) = 0;
         virtual bool TryGetBitmapLocation(cstr name, BitmapLocation& location) = 0;
         virtual void BuildTextures(int32 minWidth, IEventCallback<BitmapUpdate>* onUpdate = nullptr) = 0;
         virtual void Clear() = 0;
      };

      ROCOCO_INTERFACE ITextureArrayBuilderSupervisor: public ITextureArrayBuilder
      {
         virtual void Free() = 0;
      };

      ROCOCO_INTERFACE IGraphicsMemoryContext
      {

      };

      ROCOCO_INTERFACE ITextureArray
      {
         virtual void AddTexture() = 0;
         virtual void ResetWidth(int32 width) = 0;   
         virtual void ResetWidth(int32 width, int32 height) = 0;
         virtual void WriteSubImage(size_t index, const RGBAb* pixels, const GuiRect& targetLocatio) = 0;
         virtual void WriteSubImage(size_t index, const uint8* grayScalePixels, Vec2i span) = 0;
         virtual int32 MaxWidth() const = 0;
         virtual size_t TextureCount() const = 0;
      };

      RENDERER_API void StandardLoadFromCompressedTextureBuffer(cstr name, IEventCallback<CompressedTextureBuffer>& onLoad, IInstallation& installation, IExpandingBuffer& buffer);
      RENDERER_API ITextureArrayBuilderSupervisor* CreateTextureArrayBuilder(ICompressedResourceLoader& loader, ITextureArray& textureArray);
   } // Rococo::Graphics::Textures
} // Rococo::Graphics

#endif