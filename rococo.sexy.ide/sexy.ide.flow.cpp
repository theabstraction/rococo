#include <rococo.sexy.ide.h>

#include <sexy.types.h>
#include <sexy.compiler.public.h>
#include <sexy.debug.types.h>
#include <sexy.script.h>
#include <Sexy.S-Parser.h>

#include <windows.h>
#include <rococo.window.h>

#include <rococo.strings.h>

#include <rococo.visitors.h>
#include <commctrl.h>

using namespace Rococo;
using namespace Rococo::IDE;
using namespace Rococo::Windows;

using namespace Sexy;
using namespace Sexy::Sex;
using namespace Sexy::VM;

namespace
{
   struct ScriptLogger : ILog
   {
      IDebuggerWindow& debugger;
      ScriptLogger(IDebuggerWindow& _debugger) : debugger(_debugger) {}

      virtual void Write(csexstr text)
      {
         debugger.Log(L"%s", text);
      }

      virtual void OnUnhandledException(int errorCode, csexstr exceptionType, csexstr message, void* exceptionInstance)
      {
         debugger.Log(L"Unhandled: %s\n%s", exceptionType, message);
      }

      virtual void OnJITCompileException(Sex::ParseException& ex)
      {
         LogParseException(ex, debugger);
      }
   };

   class PersistentScript : public IPersistentScript
   {
      ScriptLogger logger;
      IDebuggerWindow& debugger;
      ISourceCache& sources;
      ISParserTree* tree;
      Script::CScriptSystemProxy ssp;
   public:
      PersistentScript(size_t maxBytes, ISourceCache& _sources, IDebuggerWindow& _debugger, const wchar_t* resourcePath, int32 maxScriptSizeBytes, IEventCallback<ScriptCompileArgs>& onCompile, IScriptExceptionHandler& _exceptionHandler) :
         logger(_debugger),
         debugger(_debugger),
         sources(_sources),
         tree(nullptr),
         ssp(ProgramInitParameters(maxBytes), logger)
      {
         Script::IPublicScriptSystem& ss = ssp();
         if (&ss == nullptr)
         {
            Throw(0, L"Failed to create script system -> probably an environment problem");
         }

         while (true)
         {
            try
            {
               tree = sources.GetSource(resourcePath);
               InitSexyScript(*tree, debugger, ssp(), sources, onCompile);
               break;
            }
            catch (ParseException& ex)
            {
               LogParseException(ex, debugger);

               switch (_exceptionHandler.GetScriptExceptionFlow(ex.Name(), ex.Message()))
               {
               case EScriptExceptionFlow_Retry:
                  DebuggerLoop(ssp(), debugger);
                  break;

               case EScriptExceptionFlow_Ignore:
               case EScriptExceptionFlow_Terminate:
                  Throw(ex.ErrorCode(), L"%s:\r\n%s", ex.Name(), ex.Message());
                  return;
               }
            }
         }
      }

      virtual void Free()
      {
         delete this;
      }

      virtual void ExecuteFunction(ArchetypeCallback fn, IArgEnumerator& args, IScriptExceptionHandler& exceptionHandler)
      {
         while (true)
         {
            try
            {
               Rococo::ExecuteFunction(fn.byteCodeId, args, ssp(), debugger);
               return;
            }
            catch (ParseException& ex)
            {
               LogParseException(ex, debugger);

               switch (exceptionHandler.GetScriptExceptionFlow(ex.Name(), ex.Message()))
               {
               case EScriptExceptionFlow_Ignore:
                  return;
               case EScriptExceptionFlow_Retry:
                  DebuggerLoop(ssp(), debugger);
                  break;
               case EScriptExceptionFlow_Terminate:
                  Throw(ex.ErrorCode(), L"%s:\r\n%s", ex.Name(), ex.Message());
                  break;
               }
            }
            catch (IException& ex)
            {
               logger.debugger.Log(L"%s", ex.Message());

               switch (exceptionHandler.GetScriptExceptionFlow(L"--app--", ex.Message()))
               {
               case EScriptExceptionFlow_Ignore:
                  return;
               case EScriptExceptionFlow_Retry:
                  break;
               case EScriptExceptionFlow_Terminate:
                  Throw(ex.ErrorCode(), L"%s", ex.Message());
                  return;
               }
            }

            DebuggerLoop(ssp(), debugger);
         }
      }

      virtual void ExecuteFunction(const wchar_t* name, IArgEnumerator& args, IScriptExceptionHandler& exceptionHandler)
      {
         while (true)
         {
            try
            {
               Rococo::ExecuteFunction(name, args, ssp(), debugger);
               return;
            }
            catch (ParseException& ex)
            {
               LogParseException(ex, debugger);

               switch (exceptionHandler.GetScriptExceptionFlow(ex.Name(), ex.Message()))
               {
               case EScriptExceptionFlow_Ignore:
                  return;
               case EScriptExceptionFlow_Retry:
                  DebuggerLoop(ssp(), debugger);
                  break;
               case EScriptExceptionFlow_Terminate:
                  Throw(ex.ErrorCode(), L"%s:\r\n%s", ex.Name(), ex.Message());
                  break;
               }
            }
            catch (IException& ex)
            {
               logger.debugger.Log(L"Exection thrown: %s", ex.Message());

               switch (exceptionHandler.GetScriptExceptionFlow(L"--app--", ex.Message()))
               {
               case EScriptExceptionFlow_Ignore:
                  return;
               case EScriptExceptionFlow_Retry:
                  break;
               case EScriptExceptionFlow_Terminate:
                  Throw(ex.ErrorCode(), L"%s", ex.Message());
                  return;
               }
            }

            DebuggerLoop(ssp(), debugger);
            Throw(0, L"Script failed");
         }
      }
   };
}

namespace Rococo 
{
   namespace IDE
   {
      void ExecuteSexyScriptLoop(size_t maxBytes, ISourceCache& sources, IDebuggerWindow& debugger, const wchar_t* resourcePath, int32 param, int32 maxScriptSizeBytes, IEventCallback<ScriptCompileArgs>& onCompile, IScriptExceptionHandler& exceptionHandler)
      {
         ScriptLogger logger(debugger);

         Auto<ISourceCode> src;
         Auto<ISParserTree> tree;

         while (true)
         {
            Script::CScriptSystemProxy ssp(ProgramInitParameters(maxBytes), logger);
            Script::IPublicScriptSystem& ss = ssp();
            if (&ss == nullptr)
            {
               switch (exceptionHandler.GetScriptExceptionFlow(L"SexyScriptSystem", L"Failed to create script system"))
               {
               case EScriptExceptionFlow_Ignore:
                  return;
               case EScriptExceptionFlow_Retry:
               case EScriptExceptionFlow_Terminate:
                  Throw(0, L"Failed to create script system");
               }
            }

            try
            {
               ISParserTree* tree = sources.GetSource(resourcePath);
               ExecuteSexyScript(*tree, debugger, ss, sources, param, onCompile);
               return;
            }
            catch (ParseException& ex)
            {
               LogParseException(ex, debugger);

               switch (exceptionHandler.GetScriptExceptionFlow(ex.Name(), ex.Message()))
               {
               case EScriptExceptionFlow_Ignore:
                  return;
               case EScriptExceptionFlow_Retry:
                  break;
               case EScriptExceptionFlow_Terminate:
                  Throw(ex.ErrorCode(), L"%s:\r\n%s", ex.Name(), ex.Message());
                  break;
               }
            }
            catch (IException& ex)
            {
               switch (exceptionHandler.GetScriptExceptionFlow(L"--app--", ex.Message()))
               {
               case EScriptExceptionFlow_Ignore:
                  return;
               case EScriptExceptionFlow_Retry:
                  DebuggerLoop(ssp(), debugger);
                  break;
               case EScriptExceptionFlow_Terminate:
                  Throw(ex.ErrorCode(), L"%s", ex.Message());
                  return;
               }
            }

            sources.Release(resourcePath);
            DebuggerLoop(ss, debugger);
         }
      }

      IPersistentScript* CreatePersistentScript(size_t maxBytes, ISourceCache& sources, IDebuggerWindow& debugger, const wchar_t* resourcePath, int32 maxScriptSizeBytes, IEventCallback<ScriptCompileArgs>& onCompile, IScriptExceptionHandler& exceptionHandler)
      {
         return new PersistentScript(maxBytes, sources, debugger, resourcePath, maxScriptSizeBytes, onCompile, exceptionHandler);
      }
   }// IDE
} // Rococo