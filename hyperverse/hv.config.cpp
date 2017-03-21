#include "hv.h"
#include <unordered_map>

#include <sexy.types.h>

namespace
{
   using namespace HV;

   void ValidateKey(const fstring& name)
   {
      Strings::ValidateFQNameIdentifier(name);
   }

   struct Config : public IConfigSupervisor
   {
      std::unordered_map<std::wstring, int32> mapToInt;
      std::unordered_map<std::wstring, float> mapToFloat;
      std::unordered_map<std::wstring, boolean32> mapToBool;
      std::unordered_map<std::wstring, std::wstring> mapToText;

      virtual void Int(const fstring& name, int32 value)
      {
         ValidateKey(name);
         mapToInt[name.buffer] = value;
      }

      virtual void Float(const fstring& name, float value)
      {
         ValidateKey(name);
         mapToFloat[name.buffer] = value;
      }

      virtual void Bool(const fstring& name, boolean32 value)
      {
         ValidateKey(name);
         mapToBool[name.buffer] = value;
      }

      virtual void Text(const fstring& name, const fstring& value)
      {
         ValidateKey(name);
         mapToText[name.buffer] = value;
      }

      virtual int32 GetInt(const fstring& name)
      {
         auto i = mapToInt.find(name.buffer);
         return (i != mapToInt.end()) ? i->second : 0;
      }

      virtual float GetFloat(const fstring& name)
      {
         auto i = mapToFloat.find(name.buffer);
         return (i != mapToFloat.end()) ? i->second : 0;
      }

      virtual boolean32 GetBool(const fstring& name)
      {
         auto i = mapToBool.find(name.buffer);
         return (i != mapToBool.end()) ? i->second : false;
      }

      virtual void GetText(const fstring& name, Sexy::IStringPopulator& text)
      {
         auto i = mapToText.find(name.buffer);
         if (i != mapToText.end() && !i->second.empty())
         {
            text.Populate(i->second.c_str());
         }
      }

      virtual const wchar_t* GetText(const wchar_t* name) const
      {
         auto i = mapToText.find(name);
         if (i != mapToText.end() && !i->second.empty())
         {
            return i->second.c_str();
         }
         else
         {
            return L"";
         }
      }

      virtual void Free()
      {
         delete this;
      }
   };
}

namespace HV
{
   IConfigSupervisor* CreateConfig()
   {
      return new Config;
   }
}