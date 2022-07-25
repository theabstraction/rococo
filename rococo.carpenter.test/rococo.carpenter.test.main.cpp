// rococo.carpenter.test.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include <rococo.api.h>
#include <..\tables\periodic-table.h>
#include <..\tables\localization-text-table.h>
#include <..\tables\quotes-table.h>
#include <..\tables\users.demo.h>

#include <cstdio>
#include <rococo.io.h>
#include <rococo.strings.h>

#include <rococo.libs.inl>

#include <codecvt>

#include <sexy.types.h>
#include <sexy.vm.h>
#include <sexy.vm.cpu.h>
#include <sexy.script.h>
#include <rococo.sexy.api.h>
#include <rococo.sexy.ide.h>
#include <rococo.os.h>
#include <rococo.ide.h>

#include <rococo.package.h>

#include <tables/rococo.carpenter.test.sxh.inl>

using namespace Rococo;
using namespace Rococo::OS;
using namespace Rococo::Science::Materials;
using namespace Rococo::Strings;
using namespace Rococo::Windows;

void validate(bool condition, cstr expression, cstr location, int line)
{
    if (!condition)
    {
       Throw(0, "Error: %s, %s #%d\n", expression, location, line);
    }
}

#define VALIDATE(expression) validate((expression), #expression, __FILE__, __LINE__)

using namespace Rococo;
using namespace Rococo::Quotes;
using namespace Rococo::Test::UserDemo;

struct TableScriptContext : public IEventCallback<ScriptCompileArgs>, public Rococo::Windows::IDE::IScriptExceptionHandler, public IAppControl
{
    IInstallation& installation;

    void Free() override
    {

    }

    IDE::EScriptExceptionFlow GetScriptExceptionFlow(cstr source, cstr message) override
    {
        return IDE::EScriptExceptionFlow_Terminate;
    }

    void OnEvent(ScriptCompileArgs& args) override
    {
        AddNativeCalls_RococoScienceMaterials(args.ss);
        AddNativeCalls_RococoScienceMaterialsIPeriodicTable_Sexy(args.ss, &installation);
        AddNativeCalls_RococoStrings(args.ss);
        AddNativeCalls_RococoStringsILocalizedText_Sexy(args.ss, &installation);
        AddNativeCalls_RococoTestUserDemoIUsers_Sexy(args.ss, &installation);
        AddNativeCalls_RococoQuotes(args.ss);
        AddNativeCalls_RococoQuotesIQuotes_Sexy(args.ss, &installation);
    }

    TableScriptContext(IInstallation& _installation):installation(_installation) {}

    void Execute(cstr pingPath, ScriptPerformanceStats& stats, int32 id)
    {
        AutoFree<IScriptSystemFactory> ssFactory(CreateScriptSystemFactory_1_5_0_0());
        AutoFree<ISourceCache> sourceCache(CreateSourceCache(installation));
        AutoFree<IDebuggerWindow> debuggerWindow(Windows::IDE::GetConsoleAsDebuggerWindow());

        WideFilePath sysPathPackages;
        installation.ConvertPingPathToSysPath("!packages/tables_1000.sxyz", sysPathPackages);
        AutoFree<IPackageSupervisor> tables = OpenZipPackage(sysPathPackages, "tables");

        sourceCache->AddPackage(tables);

        try
        {
            IDE::ExecuteSexyScriptLoop(stats,
                4096_kilobytes,
                *ssFactory,
                *sourceCache,
                *debuggerWindow,
                pingPath,
                id,
                (int32)128_kilobytes,
                *this,
                *this,
                *this,
                false,
                nullptr);
        }
        catch (IException&)
        {
            throw;
        }
    }

    bool isRunning = true;

    bool IsRunning() const override
    {
        return isRunning;
    }

    void ShutdownApp() override
    {
        isRunning = false;
    }
};

void test_sexy(IInstallation& installation)
{
	WideFilePath srcpath;
	Format(srcpath, L"%sscripts\\native\\", installation.Content());

	Rococo::Script::SetDefaultNativeSourcePath(srcpath);

	ScriptPerformanceStats stats;

    TableScriptContext script(installation);
    script.Execute("!scripts/test/tables.sxy", stats, 0);
}

int main()
{
    Rococo::OS::SetBreakPoints(Rococo::OS::BreakFlag_All);

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

        AutoFree<IQuotesSupervisor> quotes = GetQuoteTable(*installation);
        auto& johnson = quotes->GetRow((int32)QuoteId::DoctorJohnson1);
        fstring q = johnson.GetText();

        AutoFree<IUsersSupervisor> users = GetUserTable(*installation);

        int ownerIndex = -1;
        auto* mark = users->FindRowByOwnerId("mark@z.geoff"_fstring, ownerIndex);
        VALIDATE(mark != nullptr && mark->GetPurchaseId() == 1175);
        VALIDATE(ownerIndex == 0);
        auto* stan = users->FindRowByOwnerId("stan@z.geoff"_fstring, ownerIndex);
        VALIDATE(stan == nullptr && ownerIndex == -1);


        VALIDATE(StartsWith(q, "No man"));
        VALIDATE(EndsWith(q, "money."));

        test_sexy(*installation);

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

