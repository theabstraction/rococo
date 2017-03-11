#ifndef ROCOCO_TEXTURES_H
#define ROCOCO_TEXTURES_H

namespace Rococo
{
   namespace Imaging
   {
      struct F_A8R8G8B8;
   }

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
         virtual void Load(const wchar_t* name, IEventCallback<CompressedTextureBuffer>& onLoad) = 0;
      };

      struct BitmapLocation
      {
         GuiRect txUV;
         int32 textureIndex;
      };

      ROCOCOAPI ITextureArrayBuilder
      {
         virtual void AddBitmap(const wchar_t* name) = 0;
         virtual bool TryGetBitmapLocation(const wchar_t* name, BitmapLocation& location) = 0;
         virtual void BuildTextures(int32 minWidth, int32 priority) = 0;
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
         virtual void WriteSubImage(size_t index, const Imaging::F_A8R8G8B8* pixels, const GuiRect& targetLocation) = 0;    
         virtual int32 MaxWidth() const = 0;
         virtual size_t TextureCount() const = 0;
      };

      ITextureArrayBuilderSupervisor* CreateTextureArrayBuilder(IResourceLoader& loader, ITextureArray& textureArray);
   }
}

#endif