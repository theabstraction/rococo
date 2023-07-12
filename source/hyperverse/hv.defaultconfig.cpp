#include "hv.defaults.h"

namespace HV
{
   using namespace Rococo;

   namespace Defaults
   {
      const ConfigText appTitle   = { "app.title"_fstring,   "Hyperverse"_fstring };
      const ConfigText appAuthor  = { "app.author"_fstring,  "Mark Anthony Taylor"_fstring };
      const ConfigText appEmail   = { "app.emai"_fstring,   "mark.anthony.taylor@gmail.com"_fstring };
      const ConfigText appURL     = { "app.ur"_fstring,     "www.shyreman.com"_fstring };
      const ConfigText appTwitter = { "app.twitter"_fstring, "@Shyreman"_fstring };
      const ConfigInt   appSleep  { "app.sleep", 5 };

      const ConfigFloat mouseFpsxScale = { "mouse.fps.xscale"_fstring, 	4.0f };
      const ConfigFloat mouseFpsyScale = { "mouse.fps.yscale"_fstring, 	4.0f };
      const ConfigBool  mouseYreverse = { "mouse.fps.yreverse"_fstring, 	false };

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