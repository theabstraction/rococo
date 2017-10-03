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
      virtual void AddDisassembly(RGBAb colour, cstr text, RGBAb bkColor = RGBAb(255, 255, 255), bool bringToView = false)
      {
         printf("%s\n", text);
      }

      virtual void BeginStackUpdate()
      {
         printf("Begin-Stack:\n");
      }

      virtual void EndStackUpdate()
      {
         printf("End-Stack:\n");
      }

      virtual void InitDisassembly(size_t codeId)
      {
         printf("Disassembly for id %llu\n", (unsigned long long) codeId);
      }

      virtual void AddSourceCode(cstr name, cstr sourceCode)
      {
         printf("Source: %s\n%s\n", name, sourceCode);
      }

      virtual void Free()
      {
         delete this;
      }

      virtual Windows::IWindow& GetDebuggerWindowControl()
      {
         return Windows::NoParent();
      }

      virtual void PopulateStackView(Visitors::ITreePopulator& populator)
      {  
         using namespace Rococo::Visitors;

         struct anon : IUITree
         {
            virtual TREE_NODE_ID AddChild(TREE_NODE_ID parentId, cstr text, CheckState state)
            {
               printf("%s\n", text);
               return TREE_NODE_ID();
            }

            virtual TREE_NODE_ID AddRootItem(cstr text, CheckState state)
            {
               printf("%s\n", text);
               return TREE_NODE_ID();
            }

            virtual void ResetContent()
            {

            }

            virtual void SetId(TREE_NODE_ID nodeId, int64 id)
            {

            }

         } anonTree;

         populator.Populate(anonTree);
      }

      virtual void PopulateRegisterView(Visitors::IListPopulator& populator)
      {
         using namespace Rococo::Visitors;

         struct anon : IUIList
         {
            virtual void AddRow(cstr values[])
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

            virtual void ClearRows()
            {

            }

            virtual void SetColumns(cstr columnNames[], int widths[])
            {

            }

            virtual int NumberOfRows() const
            {
               return 0;
            }

            virtual void DeleteRow(int rowIndex)
            {

            }

         } anonList;
         populator.Populate(anonList);
      }

      virtual void Run(IDebuggerPopulator& populator, IDebugControl& control)
      {
         populator.Populate(*this);

         OS::UILoop(1000);
      }

      virtual void SetCodeHilight(cstr source, const Vec2i& start, const Vec2i& end, cstr message)
      {
         printf("Code Highlight: %s (%d,%d) to (%d,%d)\n\t%s\n", source, start.x, start.y, end.x, end.y, message);
      }

      virtual void ShowWindow(bool show, IDebugControl* debugControl)
      {
         
      }

      virtual void AddLogSection(RGBAb colour, cstr format, ...)
      {
         char msg[4096];

         va_list args;
         va_start(args, format);
         SafeVFormat(msg, sizeof(msg), format, args);
         printf("%s\n", msg);
      }

      virtual void ClearLog()
      {

      }

      virtual int Log(cstr format, ...)
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
   namespace IDE
   {
      IDebuggerWindow* CreateDebuggerWindow(Windows::IWindow& parent)
      {
         return new StdoutDebugger();
      }
   }
}