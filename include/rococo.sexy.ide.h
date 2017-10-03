#ifndef ROCOCO_IDE_H
#define ROCOCO_IDE_H

#include <rococo.api.h>

namespace Rococo
{
   namespace Windows
   {
      struct IWindow;

      namespace IDE
      {
         enum EScriptExceptionFlow
         {
            EScriptExceptionFlow_Terminate,
            EScriptExceptionFlow_Retry,
            EScriptExceptionFlow_Ignore
         };

         ROCOCOAPI IScriptExceptionHandler
         {
            virtual void Free() = 0;
            virtual EScriptExceptionFlow GetScriptExceptionFlow(cstr source, cstr message) = 0;
         };

         ROCOCOAPI IPersistentScript
         {
            virtual void ExecuteFunction(ArchetypeCallback bytecodeId, IArgEnumerator& args, IScriptExceptionHandler& exceptionHandler) = 0;
            virtual void ExecuteFunction(cstr name, IArgEnumerator& arg, IScriptExceptionHandler& exceptionHandlers) = 0;
            virtual void Free() = 0;
         };
      }
   }
}

namespace Rococo
{
   namespace Windows
   {
      namespace IDE
      {
         IDebuggerWindow* CreateDebuggerWindow(Windows::IWindow& parent);
         IPersistentScript* CreatePersistentScript(size_t maxBytes, ISourceCache& sources, IDebuggerWindow& debugger, cstr resourcePath, int32 maxScriptSizeBytes, IEventCallback<ScriptCompileArgs>& onCompile, IScriptExceptionHandler& exceptionHandler);
         int32 ExecuteSexyScriptLoop(size_t maxBytes, ISourceCache& sources, IDebuggerWindow& debugger, cstr resourcePath, int32 param, int32 maxScriptSizeBytes, IEventCallback<ScriptCompileArgs>& onCompile, IScriptExceptionHandler& exceptionHandler);
      }
   }
}

#endif