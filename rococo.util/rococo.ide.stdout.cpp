#include <rococo.api.h>
#include <rococo.visitors.h>
#include <stdio.h>

#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>

// Provides limited IDE functionality for stdout.
// Used on non-Win32 based platforms

using namespace Rococo;

namespace
{
   struct StdoutDebugger : public IDebuggerWindow
   {
      void AddDisassembly(RGBAb colour, cstr text, RGBAb bkColor = RGBAb(255, 255, 255), bool bringToView = false) override
      {
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

	  void PopulateMemberView(Visitors::ITreePopulator& populator) override
	  {

	  }

	  void PopulateVariableView(Visitors::IListPopulator& populator) override
	  {

	  }

	  void PopulateCallStackView(Visitors::IListPopulator& populator) override
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

            }

            int NumberOfRows() const override
            {
               return 0;
            }

            void DeleteRow(int rowIndex) override
            {

            }

         } anonList;
         populator.Populate(anonList);
      }

      void Run(IDebuggerPopulator& populator, IDebugControl& control) override
      {
         populator.Populate(*this);

         OS::UILoop(1000);
      }

      void SetCodeHilight(cstr source, const Vec2i& start, const Vec2i& end, cstr message) override
      {
         printf("Code Highlight: %s (%d,%d) to (%d,%d)\n\t%s\n", source, start.x, start.y, end.x, end.y, message);
      }

      void ShowWindow(bool show, IDebugControl* debugControl) override
      {
         
      }

      void AddLogSection(RGBAb colour, cstr format, ...) override
      {
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
   };
}

namespace Rococo
{
	namespace Windows
	{
		namespace IDE
		{
			IDebuggerWindow* CreateDebuggerWindow(Windows::IWindow& parent, IEventCallback<MenuCommand>& menuHandler, OS::IAppControl& appControl)
			{
				return new StdoutDebugger();
			}
		}
	}
}