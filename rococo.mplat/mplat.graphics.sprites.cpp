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
         struct : IEventCallback<const wchar_t*>
         {
            Sprites* sprites;
            char containingDir[IO::MAX_PATHLEN];

            virtual void OnEvent(const wchar_t* filename)
            {
               char contentRelativePath[IO::MAX_PATHLEN];
               SafeFormat(contentRelativePath, IO::MAX_PATHLEN, "%s/%S", containingDir, filename);
               sprites->AddSprite(to_fstring(contentRelativePath));
            }
         } onFileFound;
         onFileFound.sprites = this;

         if (pingNameForDirectory[0] != '!')
         {
            Throw(0, "Sprite directories must be inside the content directory. Use the '!<directory>' ping path");
         }

		 SafeFormat(onFileFound.containingDir, IO::MAX_PATHLEN, "%s", pingNameForDirectory.buffer);

		 wchar_t sysDirectory[_MAX_PATH];
		 renderer.Installation().ConvertPingPathToSysPath(pingNameForDirectory, sysDirectory, IO::MAX_PATHLEN);
         IO::ForEachFileInDirectory(sysDirectory, onFileFound);
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