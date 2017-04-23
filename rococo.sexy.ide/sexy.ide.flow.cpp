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
         debugger.Log("%s", text);
      }

      virtual void OnUnhandledException(int errorCode, csexstr exceptionType, csexstr message, void* exceptionInstance)
      {
         debugger.Log("Unhandled: %s\n%s", exceptionType, message);
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
      AutoFree<Sexy::Script::IPublicScriptSystem> ss;
      size_t maxBytes;
   public:
      PersistentScript(size_t _maxBytes, ISourceCache& _sources, IDebuggerWindow& _debugger, cstr resourcePath, int32 maxScriptSizeBytes, IEventCallback<ScriptCompileArgs>& onCompile, IScriptExceptionHandler& _exceptionHandler) :
         logger(_debugger),
         debugger(_debugger),
         sources(_sources),
         tree(nullptr),
         maxBytes(_maxBytes)
      {
         while (true)
         {
            try
            {
               ss = CreateScriptV_1_2_0_0(Sexy::Compiler::ProgramInitParameters{ maxBytes }, logger);
               if (ss == nullptr)
               {
                  Rococo::Throw(0, "Failed to create script system -> probably an environment problem");
               }
               tree = sources.GetSource(resourcePath);
               InitSexyScript(*tree, debugger, *ss, sources, onCompile);
               break;
            }
            catch (ParseException& ex)
            {
               LogParseException(ex, debugger);

               switch (_exceptionHandler.GetScriptExceptionFlow(ex.Name(), ex.Message()))
               {
               case EScriptExceptionFlow_Retry:
                  break;

               case EScriptExceptionFlow_Ignore:
               case EScriptExceptionFlow_Terminate:
                  Rococo::Throw(ex.ErrorCode(), "%s:\r\n%s", ex.Name(), ex.Message());
                  return;
               }
            }
            catch (IException& ex)
            {
               logger.debugger.Log("%s", ex.Message());

               switch (_exceptionHandler.GetScriptExceptionFlow("--app--", ex.Message()))
               {
               case EScriptExceptionFlow_Ignore:
                  return;
               case EScriptExceptionFlow_Retry:
                  break;

               case EScriptExceptionFlow_Terminate:
                  Rococo::Throw(ex.ErrorCode(), "%s", ex.Message());
                  return;
               }
            }

            DebuggerLoop(*ss, debugger);
            sources.Release(resourcePath);
         }
      }

      virtual void Free()
      {
         delete this;
      }

      virtual void ExecuteFunction(ArchetypeCallback fn, IArgEnumerator& args, IScriptExceptionHandler& exceptionHandler)
      {
         try
         {
            Rococo::ExecuteFunction(fn.byteCodeId, args, *ss, debugger);
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
               Rococo::Throw(ex.ErrorCode(), "%s:\r\n%s", ex.Name(), ex.Message());
               break;
            }
         }
         catch (IException& ex)
         {
            logger.debugger.Log("%s", ex.Message());

            switch (exceptionHandler.GetScriptExceptionFlow("--app--", ex.Message()))
            {
            case EScriptExceptionFlow_Ignore:
               return;
            case EScriptExceptionFlow_Retry:
               break;
            case EScriptExceptionFlow_Terminate:
               Rococo::Throw(ex.ErrorCode(), "%s", ex.Message());
               return;
            }
         }

         DebuggerLoop(*ss, debugger);
      }

      virtual void ExecuteFunction(cstr name, IArgEnumerator& args, IScriptExceptionHandler& exceptionHandler)
      {
         try
         {
            Rococo::ExecuteFunction(name, args, *ss, debugger);
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
               Rococo::Throw(ex.ErrorCode(), "%s:\r\n%s", ex.Name(), ex.Message());
               break;
            }
         }
         catch (IException& ex)
         {
            logger.debugger.Log("Exection thrown: %s", ex.Message());

            switch (exceptionHandler.GetScriptExceptionFlow("--app--", ex.Message()))
            {
            case EScriptExceptionFlow_Ignore:
               return;
            case EScriptExceptionFlow_Retry:
               break;
            case EScriptExceptionFlow_Terminate:
               Rococo::Throw(ex.ErrorCode(), "%s", ex.Message());
               return;
            }
         }

         DebuggerLoop(*ss, debugger);
      }
   };
}

namespace Rococo 
{
   namespace IDE
   {
      int32 ExecuteSexyScriptLoop(size_t maxBytes, ISourceCache& sources, IDebuggerWindow& debugger, cstr resourcePath, int32 param, int32 maxScriptSizeBytes, IEventCallback<ScriptCompileArgs>& onCompile, IScriptExceptionHandler& exceptionHandler)
      {
         ScriptLogger logger(debugger);

         Auto<ISourceCode> src;
         Auto<ISParserTree> tree;

         while (true)
         {
            Script::CScriptSystemProxy ssp(Sexy::Compiler::ProgramInitParameters(maxBytes), logger);
            Script::IPublicScriptSystem& ss = ssp();
            if (&ss == nullptr)
            {
               switch (exceptionHandler.GetScriptExceptionFlow("SexyScriptSystem", "Failed to create script system"))
               {
               case EScriptExceptionFlow_Ignore:
                  return 0;
               case EScriptExceptionFlow_Retry:
               case EScriptExceptionFlow_Terminate:
                  Throw(0, "Failed to create script system");
               }
            }

            try
            {
               ISParserTree* tree = sources.GetSource(resourcePath);
               int32 exitCode = ExecuteSexyScript(*tree, debugger, ss, sources, param, onCompile);
               return exitCode;
            }
            catch (ParseException& ex)
            {
               LogParseException(ex, debugger);

               switch (exceptionHandler.GetScriptExceptionFlow(ex.Name(), ex.Message()))
               {
               case EScriptExceptionFlow_Ignore:
                  return 0;
               case EScriptExceptionFlow_Retry:
                  break;
               case EScriptExceptionFlow_Terminate:
                  Throw(ex.ErrorCode(), "%s:\r\n%s", ex.Name(), ex.Message());
                  break;
               }
            }
            catch (IException& ex)
            {
               switch (exceptionHandler.GetScriptExceptionFlow("--app--", ex.Message()))
               {
               case EScriptExceptionFlow_Ignore:
                  return 0;
               case EScriptExceptionFlow_Retry:
                  break;
               case EScriptExceptionFlow_Terminate:
                  Throw(ex.ErrorCode(), "%s", ex.Message());
                  return 0;
               }
            }

            sources.Release(resourcePath);
            DebuggerLoop(ss, debugger);
         }
      }

      IPersistentScript* CreatePersistentScript(size_t maxBytes, ISourceCache& sources, IDebuggerWindow& debugger, cstr resourcePath, int32 maxScriptSizeBytes, IEventCallback<ScriptCompileArgs>& onCompile, IScriptExceptionHandler& exceptionHandler)
      {
         return new PersistentScript(maxBytes, sources, debugger, resourcePath, maxScriptSizeBytes, onCompile, exceptionHandler);
      }
   }// IDE
} // Rococo