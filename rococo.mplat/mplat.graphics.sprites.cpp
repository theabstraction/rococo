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

      void AddEachSpriteInDirectory(const fstring& directoryName)
      {
         struct : IEventCallback<cstr>
         {
            Sprites* sprites;
            char shortdir[IO::MAX_PATHLEN];
            char directory[IO::MAX_PATHLEN];

            virtual void OnEvent(cstr filename)
            {
               char contentRelativePath[IO::MAX_PATHLEN];
               SafeFormat(contentRelativePath, IO::MAX_PATHLEN, "%s%s", shortdir, filename);
               sprites->AddSprite(to_fstring(contentRelativePath));
            }
         } onFileFound;
         onFileFound.sprites = this;

         if (directoryName[0] != L'!')
         {
            Throw(0, "Sprite directories must be inside the content directory. Use the '!<directory>' notation");
         }

         StackStringBuilder sb(onFileFound.shortdir, _MAX_PATH);
         sb << directoryName;

         EndDirectoryWithSlash(onFileFound.shortdir, IO::MAX_PATHLEN);

		 char sysPath[IO::MAX_PATHLEN];
		 renderer.Installation().OS().ConvertUnixPathToSysPath(directoryName, sysPath, IO::MAX_PATHLEN);

         SafeFormat(onFileFound.directory, IO::MAX_PATHLEN, "%s%s", (cstr) renderer.Installation().Content(), (sysPath + 1));
         IO::ForEachFileInDirectory(onFileFound.directory, onFileFound);
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