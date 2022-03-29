// rococo.carpenter.test.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include <rococo.api.h>
#include <..\tables\rococo.periodic-table.h>
#include <..\tables\localized-text-table.h>
#include <cstdio>
#include <rococo.io.h>

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

int main()
{
    try
    {
        AutoFree<IPeriodicTableSupervisor> periodicTable = GetPeriodicTable();
        puts(periodicTable->Meta().GetTitle());

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

            puts(ToString(helium.element));
        }

        ILocalizedText& lt = LocalizedText();
        puts("\n");
        puts(lt.Meta().GetTitle());
        puts("\n");
        auto& marcus = lt.GetRow((int32)TextId::Introduction_MarcusAndronicus);
        puts(marcus.english);

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

