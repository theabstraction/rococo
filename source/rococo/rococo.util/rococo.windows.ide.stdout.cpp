#define ROCOCO_API __declspec(dllexport)
#include <rococo.api.h>
#include <rococo.visitors.h>
#include <stdio.h>

#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>

#include <rococo.ide.h>
#include <rococo.os.h>

// Provides limited IDE functionality for stdout.
// Used on non-Win32 based platforms

using namespace Rococo;
using namespace Rococo::Strings;

namespace
{
    struct StdoutDebugger : public IDebuggerWindow
    {
        virtual ~StdoutDebugger() {}

        void AddDisassembly(DISASSEMBLY_TEXT_TYPE, cstr text, bool bringToView = false) override
        {
            UNUSED(bringToView);
            printf("%s\n", text);
        }

        void InitDisassembly(size_t codeId) override
        {
            printf("Disassembly for id %llu\n", (unsigned long long) codeId);
        }

        void AddSourceCode(cstr name, cstr sourceCode) override
        {
            printf("Source: %s\n%s\n", name, sourceCode);
        }

        void Free() override
        {
            delete this;
        }

        Windows::IWindow& GetDebuggerWindowControl() override
        {
            return Windows::NoParent();
        }

        void PopulateMemberView(Visitors::ITreePopulator&) override
        {

        }

        void PopulateVariableView(Visitors::IListPopulator&) override
        {

        }

        void PopulateCallStackView(Visitors::IListPopulator&) override
        {

        }

        void PopulateRegisterView(Visitors::IListPopulator& populator) override
        {
            using namespace Rococo::Visitors;

            struct anon : IUIList
            {
                void AddRow(cstr values[]) override
                {
                    cstr* value = values;
                    int count = 0;
                    while (*value != nullptr)
                    {
                        if (count++ != 0)
                        {
                            printf("\t");
                        }

                        printf("%s", *value);

                        value++;
                    }

                    printf("\n");
                }

                void ClearRows() override
                {

                }

                void SetColumns(cstr columnNames[], int widths[]) override
                {
                    UNUSED(columnNames);
                    UNUSED(widths);
                }

                int NumberOfRows() const override
                {
                    return 0;
                }

                void DeleteRow(int rowIndex) override
                {
                    UNUSED(rowIndex);
                }

            } anonList;
            populator.Populate(anonList);
        }

        void Run(IDebuggerPopulator& populator, IDebugControl& control) override
        {
            UNUSED(control);
            populator.Populate(*this);

            OS::UILoop(1000);
        }

        void SetCodeHilight(cstr source, const Vec2i& start, const Vec2i& end, cstr message, bool jitCompileException = false) override
        {
            UNUSED(source);
            printf("Code Highlight: %s %s (%d,%d) to (%d,%d)\n\t%s\n", jitCompileException ? "(Bad compilation)" : "", source, start.x, start.y, end.x, end.y, message);
        }

        void ShowWindow(bool show, IDebugControl* debugControl) override
        {
            UNUSED(show);
            UNUSED(debugControl);
        }

        void AddLogSection(RGBAb colour, cstr format, ...) override
        {
            UNUSED(colour);

            char msg[4096];

            va_list args;
            va_start(args, format);
            SafeVFormat(msg, sizeof(msg), format, args);
            printf("%s\n", msg);
        }

        void ClearLog() override
        {

        }

        int Log(cstr format, ...) override
        {
            char msg[4096];

            va_list args;
            va_start(args, format);
            SafeVFormat(msg, sizeof(msg), format, args);
            return printf("%s\n", msg);
        }

        void ClearSourceCode() override
        {

        }

        void ResetJitStatus() override
        {

        }
    };
}

namespace Rococo::Windows::IDE
{
    ROCOCO_API IDebuggerWindow* CreateDebuggerWindowForStdout(Windows::IWindow& parent, OS::IAppControl& appControl)
    {
        UNUSED(parent);
        UNUSED(appControl);
        return new StdoutDebugger();
    }
}
