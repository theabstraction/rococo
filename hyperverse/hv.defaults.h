#ifndef HV_DEFAULTS_H
#define HV_DEFAULTS_H

#include "hv.h"

namespace HV
{
   namespace Defaults
   {
      extern const ConfigText appTitle;
      extern const ConfigText appAuthor;
      extern const ConfigText appEmail;
      extern const ConfigText appURL;
      extern const ConfigText appTwitter;
      extern const ConfigInt  appSleep;

      extern const ConfigFloat mouseFpsxScale;
      extern const ConfigFloat mouseFpsyScale;
      extern const ConfigBool mouseYreverse;


      void SetDefaults(IConfig& config);
   }
}

#endif
