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
            Sprites* sprites;
            U8FilePath containingDir;

            void OnEvent(IO::FileItemData& item) override
            {
               U8FilePath contentRelativePath;
               cstr sep = EndsWith(containingDir, "/") ? "" : "/";
               cstr sep2 = item.containerRelRoot[0] == 0 || EndsWith(item.containerRelRoot, L"/") ? "" : "/";
               Format(contentRelativePath, "%s%s%ls%s%ls", containingDir.buf, sep, item.containerRelRoot, sep2, item.itemRelContainer);
               sprites->AddSprite(to_fstring(contentRelativePath));
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
         renderer.Gui().SpriteBuilder().BuildTextures(256);
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