#include <rococo.mplat.h>

#include <rococo.io.h>
#include <rococo.renderer.h>
#include <rococo.textures.h>
#include <rococo.strings.h>
#include <rococo.hashtable.h>

namespace
{
   using namespace Rococo;
   using namespace Rococo::Graphics;

   struct Sprites : public ISpriteBuilderSupervisor
   {
      IRenderer &renderer;
      stringmap<int> names;

      Sprites(IRenderer& _renderer) : renderer(_renderer)
      {

      }

      void Clear()
      {
         renderer.Gui().SpriteBuilder().Clear();
         names.clear();
      }

      void AddSprite(const fstring& resourceName)
      {
         auto* ext = GetFileExtension(resourceName);

         if (ext == nullptr) Throw(0, "%s failed. [resourceName='%s'] had no extension", __FUNCTION__, resourceName.buffer);

         if (Eq(ext, ".tif") || Eq(ext, ".tiff") || Eq(ext, ".jpg") || Eq(ext, ".jpeg"))
         {
            if (names.find(resourceName.buffer) == names.end())
            {
               renderer.Gui().SpriteBuilder().AddBitmap(resourceName);
               names.insert(resourceName.buffer, 0);
            }
         }
      }

      void AddEachSpriteInDirectory(const fstring& pingNameForDirectory)
      {
         struct : IEventCallback<IO::FileItemData>
         {
            Sprites* sprites = nullptr;
            U8FilePath containingDir;

            void OnEvent(IO::FileItemData& item) override
            {
                if (item.isDirectory)
                {
                    return;
                }

                U8FilePath contentRelativePath;
                cstr sep = EndsWith(containingDir, "/") ? "" : "/";
                cstr sep2 = item.containerRelRoot[0] == 0 || EndsWith(item.containerRelRoot, L"\\") || EndsWith(item.containerRelRoot, L"/") ? "" : "/";
                Format(contentRelativePath, "%s%s%ls%s%ls", containingDir.buf, sep, item.containerRelRoot, sep2, item.itemRelContainer);

                IO::ToUnixPath(contentRelativePath.buf);

                auto* ext = GetFileExtension(contentRelativePath);
                if (ext == nullptr)
                {
                    // an unknown file type - which we ignore
                }
                else if (CompareI(ext, ".tiff") == 0 || CompareI(ext, ".tif") == 0 || CompareI(ext, ".jpg") == 0 || CompareI(ext, ".jpeg"))
                {
                    sprites->AddSprite(to_fstring(contentRelativePath));
                }
            }
         } addSprite;
         addSprite.sprites = this;

         if (pingNameForDirectory[0] != '!' && pingNameForDirectory[0] != '#')
         {
            Throw(0, "Sprite directories must be inside the content directory. Use the '!<directory>' ping path");
         }

		 Format(addSprite.containingDir, "%s", pingNameForDirectory.buffer);

		 WideFilePath sysDirectory;
		 renderer.Installation().ConvertPingPathToSysPath(pingNameForDirectory, sysDirectory);
         IO::ForEachFileInDirectory(sysDirectory, addSprite, true);
      }

      void LoadAllSprites()
      {
         renderer.Gui().SpriteBuilder().BuildBitmaps(256);
      }

      void Free() override
      {
         delete this;
      }
   };
}

namespace Rococo
{
   namespace Graphics
   {
      ISpriteBuilderSupervisor* CreateSpriteBuilderSupervisor(IRenderer & renderer)
      {
         return new Sprites(renderer);
      }
   }
}