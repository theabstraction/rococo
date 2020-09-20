#include <rococo.mplat.h>
#include <rococo.strings.h>
#include <rococo.hashtable.h>

namespace
{
   using namespace Rococo;

   void ValidateKey(const fstring& name)
   {
      Rococo::ValidateFQNameIdentifier(name);
   }

   struct Config : public IConfigSupervisor
   {
      stringmap<int32> mapToInt;
      stringmap<float> mapToFloat;
      stringmap<boolean32> mapToBool;
      stringmap<HString> mapToText;

      void Int(const fstring& name, int32 value) override
      {
         ValidateKey(name);
         mapToInt[(cstr) name] = value;
      }

      void Float(const fstring& name, float value) override
      {
         ValidateKey(name);
         mapToFloat[(cstr)name] = value;
      }

      void Bool(const fstring& name, boolean32 value) override
      {
         ValidateKey(name);
         mapToBool[(cstr)name] = value;
      }

      void Text(const fstring& name, const fstring& value) override
      {
         ValidateKey(name);
         mapToText[(cstr)name] = value;
      }

      int32 GetInt(const fstring& name) override
      {
         auto i = mapToInt.find(name);
         return (i != mapToInt.end()) ? i->second : 0;
      }

      float GetFloat(const fstring& name) override
      {
         auto i = mapToFloat.find(name);
         return (i != mapToFloat.end()) ? i->second : 0;
      }

      boolean32 GetBool(const fstring& name) override
      {
         auto i = mapToBool.find(name);
         return (i != mapToBool.end()) ? i->second : false;
      }

      void GetText(const fstring& name, Rococo::IStringPopulator& text) override
      {
         auto i = mapToText.find(name);
         if (i != mapToText.end() && i->second.length() > 0)
         {
            text.Populate(i->second.c_str());
         }
      }

      cstr GetText(cstr name) const override
      {
         auto i = mapToText.find(name);
         if (i != mapToText.end() && i->second.length() > 0)
         {
            return i->second.c_str();
         }
         else
         {
            return "";
         }
      }

      void Free() override
      {
         delete this;
      }
   };
}

namespace Rococo
{
   IConfigSupervisor* CreateConfig()
   {
      return new Config;
   }
}