// rococo.carpenter.test.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include <rococo.api.h>
#include <..\tables\rococo.periodic-table.h>
#include <..\tables\localized-text-table.h>
#include <..\tables\rococo.quotes.h>
#include <cstdio>
#include <rococo.io.h>
#include <rococo.strings.h>

# ifdef _DEBUG
#  pragma comment(lib, "rococo.util.debug.lib")
# else
#  pragma comment(lib, "rococo.util.lib")
#endif

#include <codecvt>

using namespace Rococo;
using namespace Rococo::OS;
using namespace Rococo::Science::Materials;
using namespace Rococo::Strings;

void validate(bool condition, cstr expression, cstr location, int line)
{
    if (!condition)
    {
       Throw(0, "Error: %s, %s #%d\n", expression, location, line);
    }
}

#define VALIDATE(expression) validate((expression), #expression, __FILE__, __LINE__)


int main()
{
    try
    {
        AutoFree<IPeriodicTableSupervisor> periodicTable = GetPeriodicTable();
        VALIDATE(Eq(periodicTable->Meta().GetTitle(), "Periodic Table of the Elements"));

        AutoFree<IOSSupervisor> os = GetOS();
        AutoFree<IInstallationSupervisor> installation = CreateInstallation(L"content.indicator.txt", *os);

        periodicTable->Load(*installation);

        if (periodicTable->NumberOfRows() != 118)
        {
            auto& helium = periodicTable->GetRow((int32)Rococo::Science::Materials::ElementName::Helium);
            if (helium.atomicMass < 4.0f || helium.atomicMass >= 4.1f)
            {
                Throw(0, "Bad helium");
            }

            VALIDATE(Eq("Helium"_fstring, ToString(helium.element)));
        }

        ILocalizedText& lt = LocalizedText();
        VALIDATE(Eq(lt.Meta().GetTitle(), "Localization Table"));
        auto& marcus = lt.GetRow((int32)TextId::Introduction_MarcusAndronicus);
        VALIDATE(StartsWith(marcus.english, "Princes,"));
        puts("All tests succeeded");
        return 0;
    }
    catch (IException& ex)
    {
        char buf[4096];
        Rococo::OS::BuildExceptionString(buf, sizeof buf, ex, true);
        fprintf_s(stderr, "%s", buf);
        return -1;
    }
}

