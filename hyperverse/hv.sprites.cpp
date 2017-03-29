#include "hv.h"

#include <rococo.io.h>
#include <rococo.renderer.h>
#include <rococo.textures.h>
#include <rococo.strings.h>
#include <unordered_set>
#include <string>

namespace
{
   using namespace HV;
   using namespace HV::Graphics;

   struct Sprites : public ISpriteSupervisor
   {
      IRenderer &renderer;
      std::unordered_set<std::wstring> names;

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
         if (Eq(ext, L".tif") || Eq(ext, L".tiff") || Eq(ext, L".jpg") || Eq(ext, L".jpeg"))
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
         struct : IEventCallback<const wchar_t*>
         {
            Sprites* sprites;
            wchar_t shortdir[IO::MAX_PATHLEN];
            wchar_t directory[IO::MAX_PATHLEN];

            virtual void OnEvent(const wchar_t* filename)
            {
               wchar_t contentRelativePath[IO::MAX_PATHLEN];
               SafeFormat(contentRelativePath, _TRUNCATE, L"%s%s", shortdir, filename);             
               sprites->AddSprite(to_fstring(contentRelativePath));
            }
         } onFileFound;
         onFileFound.sprites = this;

         if (directoryName[0] != L'!')
         {
            Throw(0, L"Sprite directories must be inside the content directory. Use the '!<directory>' notation");
         }

         SafeCopy(onFileFound.shortdir, directoryName, _TRUNCATE);
         EndDirectoryWithSlash(onFileFound.shortdir, IO::MAX_PATHLEN);
   
         SafeFormat(onFileFound.directory, _TRUNCATE, L"%s%s", renderer.Installation().Content(), (onFileFound.shortdir+1));
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

namespace HV
{
   namespace Graphics
   {
      ISpriteSupervisor* CreateSpriteSupervisor(IRenderer & renderer)
      {
         return new Sprites(renderer);
      }
   }
}