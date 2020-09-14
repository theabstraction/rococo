#include <rococo.mplat.h>
#include <rococo.strings.h>
#include <unordered_map>

namespace
{
   using namespace Rococo;

   void ValidateKey(const fstring& name)
   {
      Rococo::ValidateFQNameIdentifier(name);
   }

   struct Config : public IConfigSupervisor
   {
      std::unordered_map<StringKey, int32, StringKey::Hash> mapToInt;
      std::unordered_map<StringKey, float, StringKey::Hash> mapToFloat;
      std::unordered_map<StringKey, boolean32, StringKey::Hash> mapToBool;
      std::unordered_map<StringKey, HString, StringKey::Hash> mapToText;

      void Int(const fstring& name, int32 value) override
      {
         ValidateKey(name);
         StringKey key(name);
         mapToInt[key] = value;
      }

      void Float(const fstring& name, float value) override
      {
         ValidateKey(name);
         StringKey key(name);
         mapToFloat[key] = value;
      }

      void Bool(const fstring& name, boolean32 value) override
      {
         ValidateKey(name);
         StringKey key(name);
         mapToBool[key] = value;
      }

      void Text(const fstring& name, const fstring& value) override
      {
         ValidateKey(name);
         StringKey key(name);
         mapToText[key] = value;
      }

      int32 GetInt(const fstring& name) override
      {
         StringKey key(name);
         auto i = mapToInt.find(key);
         return (i != mapToInt.end()) ? i->second : 0;
      }

      float GetFloat(const fstring& name) override
      {
         StringKey key(name);
         auto i = mapToFloat.find(key);
         return (i != mapToFloat.end()) ? i->second : 0;
      }

      boolean32 GetBool(const fstring& name) override
      {
         StringKey key(name);
         auto i = mapToBool.find(key);
         return (i != mapToBool.end()) ? i->second : false;
      }

      void GetText(const fstring& name, Rococo::IStringPopulator& text) override
      {
         StringKey key(name);
         auto i = mapToText.find(key);
         if (i != mapToText.end() && i->second.length() > 0)
         {
            text.Populate(i->second.c_str());
         }
      }

      cstr GetText(cstr name) const override
      {
         StringKey key(name);
         auto i = mapToText.find(key);
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