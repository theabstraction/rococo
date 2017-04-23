#include "hv.h"
#include <array>
#include <unordered_map>

#include <rococo.os.win32.h>

namespace
{
   using namespace HV;

   class Keyboard : public IKeyboardSupervisor
   {
      std::array<std::string,512> codes;
      std::unordered_map<std::string, std::string> actionBinds;
      HKL hLocale;
   public:
      Keyboard()
      {
         hLocale = GetKeyboardLayout(0);
      }

      virtual void ClearActions()
      {
         actionBinds.clear();
      }

      virtual void BindAction(const fstring& keyName, const fstring& actionName)
      {
         actionBinds[keyName.buffer] = actionName.buffer;
      }

      virtual cstr GetAction(cstr keyName)
      {
         if (keyName != nullptr)
         {
            auto i = actionBinds.find(keyName);
            if (i != actionBinds.end())
            {
               return i->second.c_str();
            }
         }

         return nullptr;
      }

      virtual void Free()
      {
         delete this;
      }

      virtual Key GetKeyFromEvent(const KeyboardEvent& ke)
      {
         auto& rke = reinterpret_cast<const RAWKEYBOARD&>(ke);

         if (rke.MakeCode < codes.size())
         {
            auto& name = codes[rke.MakeCode];
            if (!name.empty())
            {
               return Key
               {
                  name.c_str(),
                  (rke.Flags & RI_KEY_BREAK) == 0 ? true : false
               };
            }
         }
         
         return Key
         {
            nullptr,
            false
         };
      }

      virtual void SetKeyName(const fstring& name, int32 scancode)
      {
         if (scancode < 0 || scancode >= codes.size())
         {
            Throw(0, "Bad scancode. 0 <= scancode < %u", codes.size());
         }

         codes[scancode] = name.buffer;
      }
   };
}

namespace HV
{
   IKeyboardSupervisor* CreateKeyboardSupervisor()
   {
      return new Keyboard();
   }
}