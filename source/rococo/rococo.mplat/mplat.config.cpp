#include <rococo.mplat.h>
#include <rococo.strings.h>
#include <rococo.hashtable.h>
#include <rococo.io.h>

namespace
{
   using namespace Rococo;

   void ValidateKey(const fstring& name)
   {
       try
       {
           Rococo::ValidateFQNameIdentifier(name);
       }
       catch (IException& ex)
       {
           Throw(ex.ErrorCode(), "Failed to validate config name: '%s'. %s", name.buffer, ex.Message());
       }
   }

   struct Config : public IConfigSupervisor
   {
      mutable stringmap<int32> mapToInt;
      mutable stringmap<float> mapToFloat;
      mutable stringmap<boolean32> mapToBool;
      mutable stringmap<HString> mapToText;

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

      bool TryGetInt(cstr name, int& value, int defaultValue) const override
      {
          auto i = mapToInt.find(name);
          if (i != mapToInt.end())
          {
              value = i->second;
              return true;
          }
          else
          {
              printf("Config element %s not found. Initializing to default value: %d", name, defaultValue);
              value = defaultValue;
              mapToInt[(cstr)name] = defaultValue;
              return false;
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

namespace Rococo::MPlatImpl
{
    Rococo::IInstallationManagerSupervisor* CreateIMS(IO::IInstallation& installation)
    {
        struct Anon : IInstallationManagerSupervisor
        {
            IO::IInstallation& installation;
            Anon(IO::IInstallation& _installation) : installation(_installation) {}

            void Free() override
            {
                delete this;
            }

            void SetPingPathMacro(const fstring& key, const fstring& pingPathValue) override
            {
                if (key.length < 1 || key[0] != '#')
                {
                    Throw(0, "The key must begin with a #");
                }

                if (pingPathValue.length < 1 || pingPathValue[0] != '!')
                {
                    Throw(0, "The value must begin with a !");
                }

                if (!EndsWith(pingPathValue, "/"))
                {
                    Throw(0, "The value must end with a /");
                }

                installation.Macro(key, pingPathValue);
            }
        };
        return new Anon(installation);
    }
}