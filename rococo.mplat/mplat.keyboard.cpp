#include <rococo.mplat.h>
#include <array>
#include <unordered_map>

#include <rococo.os.win32.h>

#include <rococo.strings.h>

namespace
{
   using namespace Rococo;

   class Keyboard : public IKeyboardSupervisor
   {
      std::array<std::string, 512> codes;
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
            USHORT vKeyCode = rke.VKey;
            auto& name = codes[vKeyCode];
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

      virtual void SetKeyName(const fstring& name, int32 vkeyCode)
      {
         if (vkeyCode < 0 || vkeyCode >= codes.size())
         {
            Throw(0, "Bad scancode. 0 <= scancode < %u", codes.size());
         }

         codes[vkeyCode] = name.buffer;
      }

	  virtual void SaveCppHeader()
	  {
		  char text[8192];
		  StackStringBuilder ssb(text, sizeof(text));

		  ssb << "namespace Rococo { namespace IO {\n";
		  ssb << " enum VKCode\n {\n";
		  for (size_t vk = 0; vk < codes.size(); ++vk)
		  {
			  if (codes[vk].size() > 0) ssb.AppendFormat("  VKCode_%s = %u,\n", codes[vk].c_str(), vk);
		  }
		  ssb.AppendFormat("  VKCode_None = 0\n");
		  ssb << " };\n}}\n";
		  
		  IO::SaveUserFile("rococo.vkeys.win32.h", text);
	  }
   };
}

namespace Rococo
{
   IKeyboardSupervisor* CreateKeyboardSupervisor()
   {
      return new Keyboard();
   }
}