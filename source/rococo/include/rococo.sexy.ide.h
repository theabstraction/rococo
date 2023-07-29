#ifndef ROCOCO_IDE_H
#define ROCOCO_IDE_H

#include <rococo.api.h>
#include <rococo.sexy.api.h>

namespace Rococo
{
	struct MenuCommand;

	using namespace Rococo::Script;

	namespace Windows
	{
		struct IWindow;

		namespace IDE
		{
			enum class EScriptExceptionFlow
			{
				Terminate,
				Retry,
				Ignore
			};

			ROCOCO_INTERFACE IScriptExceptionHandler
			{
			   virtual void Free() = 0;
			   virtual EScriptExceptionFlow GetScriptExceptionFlow(cstr source, cstr message) = 0;
			};

			ROCOCO_INTERFACE IPersistentScript
			{
			   virtual void ExecuteFunction(ArchetypeCallback bytecodeId, IArgEnumerator& args, IScriptExceptionHandler& exceptionHandler, bool trace) = 0;
			   virtual void ExecuteFunction(cstr name, IArgEnumerator& arg, IScriptExceptionHandler& exceptionHandlers, bool trace) = 0;
			   virtual void Free() = 0;
			};

			ROCOCO_INTERFACE IDebuggerEventHandlerData
			{
				virtual uint32 GetLineNumber() const = 0;
				virtual IWindow& Controller() = 0;
			};

			ROCOCO_INTERFACE IDebuggerEventHandler
			{
				  virtual IEventCallback<MenuCommand>& GetMenuCallback() = 0;
				  virtual void Free() = 0;
			};
		}
	}
}

namespace Rococo
{
	namespace Script
	{
		struct IScriptSystemFactory;
	}

	namespace OS
	{
		struct IAppControl;
	}

	namespace Strings
	{
		struct IVarArgStringFormatter;
		struct IColourOutputControl;
	}

	namespace Windows
	{
		namespace IDE
		{
			struct DebuggerCommandObject
			{
				enum eType { OPEN_SOURCE_FILE, OPEN_SOURCE_FOLDER };
				eType type;
			};

			struct DebuggerCommandObjectFile
			{
				DebuggerCommandObject header;
				char filename[256];
			};

			IDebuggerWindow* GetConsoleAsDebuggerWindow(Strings::IVarArgStringFormatter& formatter, Strings::IColourOutputControl& control);
			Strings::IColourOutputControl& GetConsoleColourController();
			Strings::IVarArgStringFormatter& GetStdoutFormatter();
			IDebuggerWindow* CreateDebuggerWindow(Windows::IWindow& parent, OS::IAppControl& appControl, IO::IInstallation& installation);
			IPersistentScript* CreatePersistentScript(size_t maxBytes, Rococo::Script::IScriptSystemFactory& factory, ISourceCache& sources, IDebuggerWindow& debugger, cstr resourcePath, int32 maxScriptSizeBytes, IScriptCompilationEventHandler& onCompile, IScriptExceptionHandler& exceptionHandler);
			int32 ExecuteSexyScriptLoop(ScriptPerformanceStats& stats, size_t maxBytes, IScriptSystemFactory& factory, ISourceCache& sources, IScriptEnumerator& implicitIncludes, IDebuggerWindow& debugger, cstr resourcePath, int32 param, IScriptCompilationEventHandler& onCompile, IScriptExceptionHandler& exceptionHandler, OS::IAppControl& appControl, bool trace, Strings::StringBuilder* declarationBuilder);
		}
	}
}

#endif