#include <rococo.mplat.h>
#include <array>
#include <rococo.hashtable.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <rococo.strings.h>
#include <rococo.io.h>
#include <rococo.ui.h>

using namespace Rococo;
using namespace Rococo::Strings;

namespace
{
   void InsertCharAtPos(int& caretPos, char* buffer, size_t capacity, char c)
   {
	   int32 len = StringLength(buffer);
	   if (len == capacity - 1)
	   {
		   return;
	   }
	   else if (caretPos >= len)
	   {
		   buffer[len] = c;
		   buffer[len + 1] = 0;
	   }
	   else
	   {
		   for (int32 i = len; i > caretPos; i--)
		   {
			   buffer[i] = buffer[i - 1];
		   }

		   buffer[caretPos] = c;
		   buffer[len + 1] = 0;
	   }

	   caretPos++;
   }

   void DeleteCharAt(int pos, char* buffer, size_t)
   {
	   char* dest = buffer + pos - 1;
	   for (const char* p = buffer + pos; *p != 0; ++p)
	   {
		   *dest++ = *p;
	   }

	   *dest = 0;
   }

   void BackSpaceAtPos(int& caretPos, char* buffer, size_t capacity)
   {
	   if (caretPos > 0)
	   {
		   DeleteCharAt(caretPos, buffer, capacity);
		   caretPos--;
	   }
   }

   void DeleteAtPos(int& caretPos, char* buffer, size_t capacity)
   {
	   int len = StringLength(buffer);
	   if (caretPos < len)
	   {
		   DeleteCharAt(caretPos + 1, buffer, capacity);
	   }
   }

   void Anon_AppendKeyboardInputToEditBuffer(Windows::IWindow& window, int& caretPos, char* buffer, size_t capacity, const KeyboardEventEx& key)
   {
	   if (key.IsUp()) return;

	   if (key.unicode >= 32 && key.unicode < 127)
	   {
		   char c = (char) key.unicode;
		   InsertCharAtPos(caretPos, buffer, capacity, c);
		   return;
	   }

	   switch (key.VKey)
	   {
	   case IO::VirtualKeys::VKCode_HOME:
		   caretPos = 0;
		   break;
	   case IO::VirtualKeys::VKCode_END:
		   caretPos = StringLength(buffer);
		   break;
	   case IO::VirtualKeys::VKCode_LEFT:
		   caretPos--;
		   caretPos = max(0, caretPos);
		   break;
	   case IO::VirtualKeys::VKCode_RIGHT:
		   caretPos++;
		   caretPos = min(StringLength(buffer), caretPos);
		   break;
	   case IO::VirtualKeys::VKCode_BACKSPACE:
		   BackSpaceAtPos(caretPos, buffer, capacity);
		   break;
	   case IO::VirtualKeys::VKCode_DELETE:
		   DeleteAtPos(caretPos, buffer, capacity);
		   break;
	   case IO::VirtualKeys::VKCode_C:
		   if (key.isCtrlHeld)
		   {
			   OS::SaveClipBoardText(buffer, window);
		   }
		   break;
	   case IO::VirtualKeys::VKCode_V:
		   if (key.isCtrlHeld)
		   {
			   IO::PasteFromClipboard(buffer, capacity);
			   caretPos = StringLength(buffer);
		   }
		   break;
	   default:
		   break;
	   }
   }

   class Keyboard : public IKeyboardSupervisor
   {
      std::array<HString, 512> codes;
	  stringmap<int32> mapNameToVkCode;
	  stringmap<HString> actionBinds;
	  Windows::IWindow& window;
   public:
      Keyboard(Windows::IWindow& _window): window(_window)
      {
      }

	  void AppendKeyboardInputToEditBuffer(int& caretPos, char* buffer, size_t capacity, const KeyboardEventEx& key) override
	  {
		  return Anon_AppendKeyboardInputToEditBuffer(window, caretPos, buffer, capacity, key);
	  }

      void ClearActions() override
      {
         actionBinds.clear();
      }

      void BindAction(const fstring& keyName, const fstring& actionName) override
      {
         actionBinds[keyName.buffer] = actionName.buffer;
      }

	  int32 GetVKeyFromName(const fstring& name)
	  {
		  auto i = mapNameToVkCode.find((cstr)name);
		  return i == mapNameToVkCode.end() ? 0 : i->second;
	  }

      cstr GetAction(cstr keyName) override
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

      void Free() override
      {
         delete this;
      }

      Key GetKeyFromEvent(const KeyboardEvent& ke) override
      {
         auto& rke = reinterpret_cast<const RAWKEYBOARD&>(ke);

         if (rke.MakeCode < codes.size())
         {
            USHORT vKeyCode = rke.VKey;
            auto& name = codes[vKeyCode];
            if (name.length() != 0)
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

      void SetKeyName(const fstring& name, int32 vkeyCode) override
      {
         if (vkeyCode < 0 || vkeyCode >= codes.size())
         {
            Throw(0, "Bad scancode. 0 <= scancode < %u", codes.size());
         }

		 mapNameToVkCode[(cstr)name] = vkeyCode;

         codes[vkeyCode] = name.buffer;
      }

	  void SaveCppHeader() override
	  {
		  char text[8192];
		  StackStringBuilder ssb(text, sizeof(text));

		  ssb << "namespace Rococo { namespace IO {\n";
		  ssb << " enum VKCode\n {\n";
		  for (size_t vk = 0; vk < codes.size(); ++vk)
		  {
			  if (codes[vk].length() > 0) ssb.AppendFormat("  VKCode_%s = %u,\n", codes[vk].c_str(), vk);
		  }
		  ssb.AppendFormat("  VKCode_None = 0\n");
		  ssb << " };\n}}\n";
		  
		  IO::SaveUserFile("rococo.vkeys.win32.h", text);
	  }
   };
}

namespace Rococo
{
   IKeyboardSupervisor* CreateKeyboardSupervisor(Windows::IWindow& window)
   {
      return new Keyboard(window);
   }
}