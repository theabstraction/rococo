#include <rococo.mplat.h>

#include <rococo.io.h>
#include <rococo.renderer.h>
#include <rococo.textures.h>
#include <rococo.strings.h>
#include <unordered_set>
#include <string>

namespace
{
   using namespace Rococo;
   using namespace Rococo::Graphics;

   struct Sprites : public ISpriteSupervisor
   {
      IRenderer &renderer;
      std::unordered_set<std::string> names;

      Sprites(IRenderer& _renderer) : renderer(_renderer)
      {

      }

      void Clear()
      {
         renderer.SpriteBuilder().Clear();
         names.clear();
      }

      void AddSprite(const fstring& resourceName)
      {
         auto* ext = GetFileExtension(resourceName);
         if (Eq(ext, ".tif") || Eq(ext, ".tiff") || Eq(ext, ".jpg") || Eq(ext, ".jpeg"))
         {
            if (names.find(resourceName.buffer) == names.end())
            {
               renderer.SpriteBuilder().AddBitmap(resourceName);
               names.insert(resourceName.buffer);
            }
         }
      }

      void AddEachSpriteInDirectory(const fstring& pingNameForDirectory)
      {
         struct : IEventCallback<IO::FileItemData>
         {
            Sprites* sprites;
            char containingDir[IO::MAX_PATHLEN];

            virtual void OnEvent(IO::FileItemData& item)
            {
               char contentRelativePath[IO::MAX_PATHLEN];
               cstr sep = EndsWith(containingDir, "/") ? "" : "/";
               cstr sep2 = item.containerRelRoot[0] == 0 || EndsWith(item.containerRelRoot, L"/") ? "" : "/";
               SafeFormat(contentRelativePath, IO::MAX_PATHLEN, "%s%s%ls%s%ls", containingDir, sep, item.containerRelRoot, sep2, item.itemRelContainer);
               sprites->AddSprite(to_fstring(contentRelativePath));
            }
         } addSprite;
         addSprite.sprites = this;

         if (pingNameForDirectory[0] != '!')
         {
            Throw(0, "Sprite directories must be inside the content directory. Use the '!<directory>' ping path");
         }

		 SafeFormat(addSprite.containingDir, IO::MAX_PATHLEN, "%s", pingNameForDirectory.buffer);

		 WideFilePath sysDirectory;
		 renderer.Installation().ConvertPingPathToSysPath(pingNameForDirectory, sysDirectory);
         IO::ForEachFileInDirectory(sysDirectory, addSprite, true);
      }

      void LoadAllSprites()
      {
         renderer.SpriteBuilder().BuildTextures(256);
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
      ISpriteSupervisor* CreateSpriteSupervisor(IRenderer & renderer)
      {
         return new Sprites(renderer);
      }
   }
}