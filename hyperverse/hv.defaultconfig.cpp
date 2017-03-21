#include "hv.defaults.h"

namespace HV
{
   using namespace Rococo;

   namespace Defaults
   {
      const ConfigText appTitle   = { L"app.title"_fstring,   L"Hyperverse"_fstring };
      const ConfigText appAuthor  = { L"app.author"_fstring,  L"Mark Anthony Taylor"_fstring };
      const ConfigText appEmail   = { L"app.email"_fstring,   L"mark.anthony.taylor@gmail.com"_fstring };
      const ConfigText appURL     = { L"app.url"_fstring,     L"www.shyreman.com"_fstring };
      const ConfigText appTwitter = { L"app.twitter"_fstring, L"@Shyreman"_fstring };
      const ConfigInt   appSleep  { L"app.sleep", 5 };

      const ConfigFloat mouseFpsxScale = { L"mouse.fps.xscale"_fstring, 	4.0f };
      const ConfigFloat mouseFpsyScale = { L"mouse.fps.yscale"_fstring, 	4.0f };
      const ConfigBool  mouseYreverse = { L"mouse.fps.yreverse"_fstring, 	false };

      void Add(IConfig& config, const ConfigText& ct)
      {
         config.Text(ct.key, ct.value);
      }

      void Add(IConfig& config, const ConfigBool& ct)
      {
         config.Bool(ct.key, ct.value);
      }

      void Add(IConfig& config, const ConfigInt& ct)
      {
         config.Int(ct.key, ct.value);
      }

      void Add(IConfig& config, const ConfigFloat& ct)
      {
         config.Float(ct.key, ct.value);
      }

      void SetDefaults(IConfig& config)
      {
         Add(config, appTitle);
         Add(config, appAuthor);
         Add(config, appEmail);
         Add(config, appTwitter);
         Add(config, appSleep);
         Add(config, mouseFpsxScale);
         Add(config, mouseFpsyScale);
         Add(config, mouseYreverse);
         Add(config, appSleep);
      }
   }
}