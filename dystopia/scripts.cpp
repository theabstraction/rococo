#include "dystopia.h"
#include <rococo.io.h>
#include <rococo.visitors.h>

#include <rococo.sexy.ide.h>

using namespace Dystopia;
using namespace Rococo;
using namespace Sexy;
using namespace Sexy::Sex;
using namespace Sexy::VM;

namespace
{
   struct DialogExceptionHandler: public IDE::IScriptExceptionHandler
   {
      Windows::IWindow& parent;

      DialogExceptionHandler(Windows::IWindow& _parent): parent(_parent)
      {
      }

      virtual void Free()
      {
         delete this;
      }

      virtual IDE::EScriptExceptionFlow GetScriptExceptionFlow(const wchar_t* source, const wchar_t* message)
      {
         auto id = Dystopia::ShowContinueBox(parent, L"Failed to create script system");
         switch (id)
         {
         case CMD_ID_IGNORE:
            return IDE::EScriptExceptionFlow_Ignore;
         case CMD_ID_RETRY:
            return IDE::EScriptExceptionFlow_Retry;
         default:
         case CMD_ID_EXIT:
            return IDE::EScriptExceptionFlow_Terminate;
         }
      } 
   };
}
	
namespace Dystopia
{
   IDE::IScriptExceptionHandler* UseDialogBoxForScriptException(Windows::IWindow& parent)
   {
      return new DialogExceptionHandler(parent);
   }
}
