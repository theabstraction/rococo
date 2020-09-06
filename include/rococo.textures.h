#ifndef ROCOCO_TEXTURES_H
#define ROCOCO_TEXTURES_H

namespace Rococo
{
   struct IExpandingBuffer;

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

      ROCOCOAPI IResourceLoader
      {
         virtual void Load(cstr pingPath, IEventCallback<CompressedTextureBuffer>& onLoad) = 0;
      };

      struct BitmapLocation
      {
         GuiRect txUV;
         int32 textureIndex;
         Vec2 pixelSpan;
      };

      ROCOCOAPI ITextureArrayBuilder
      {
         virtual void AddBitmap(cstr name) = 0;
         virtual bool TryGetBitmapLocation(cstr name, BitmapLocation& location) = 0;
         virtual void BuildTextures(int32 minWidth) = 0;
         virtual void Clear() = 0;
      };

      ROCOCOAPI ITextureArrayBuilderSupervisor: public ITextureArrayBuilder
      {
         virtual void Free() = 0;
      };

      ROCOCOAPI ITextureArray
      {
         virtual void AddTexture() = 0;
         virtual void ResetWidth(int32 width) = 0;     
         virtual void WriteSubImage(size_t index, const RGBAb* pixels, const GuiRect& targetLocation) = 0;
         virtual void WriteSubImage(size_t index, const uint8* grayScalePixels, Vec2i span) = 0;
         virtual int32 MaxWidth() const = 0;
         virtual size_t TextureCount() const = 0;
      };

      void StandardLoadFromCompressedTextureBuffer(cstr name, IEventCallback<CompressedTextureBuffer>& onLoad, IInstallation& installation, IExpandingBuffer& buffer);
      ITextureArrayBuilderSupervisor* CreateTextureArrayBuilder(IResourceLoader& loader, ITextureArray& textureArray);
   }
}

#endif