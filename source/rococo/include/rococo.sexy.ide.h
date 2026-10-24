// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#ifndef ROCOCO_IDE_H
#define ROCOCO_IDE_H

#include <rococo.api.h>
#include <rococo.sexy.api.h>

#ifndef SEXYIDE_API
# error first define SEXYIDE_API
#endif

namespace Rococo
{
	struct MenuCommand;

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
			   virtual void ExecuteFunction(Script::ArchetypeCallback bytecodeId, IArgEnumerator& args, IScriptExceptionHandler& exceptionHandler, bool trace) = 0;
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

			SEXYIDE_API IDebuggerWindow* GetConsoleAsDebuggerWindow(Strings::IVarArgStringFormatter& formatter, Strings::IColourOutputControl& control);
			SEXYIDE_API Strings::IColourOutputControl& GetConsoleColourController();
			SEXYIDE_API Strings::IVarArgStringFormatter& GetStdoutFormatter();
			SEXYIDE_API IDebuggerWindow* CreateDebuggerWindow(Windows::IWindow& parent, OS::IAppControl& appControl, IO::IInstallation& installation);
			SEXYIDE_API IPersistentScript* CreatePersistentScript(size_t maxBytes, Rococo::Script::IScriptSystemFactory& factory, ISourceCache& sources, IDebuggerWindow& debugger, cstr resourcePath, int32 maxScriptSizeBytes, IScriptCompilationEventHandler& onCompile, IScriptExceptionHandler& exceptionHandler);
			SEXYIDE_API int32 ExecuteSexyScriptLoop(ScriptPerformanceStats& stats, size_t maxBytes, Script::IScriptSystemFactory& factory, ISourceCache& sources, IScriptEnumerator& implicitIncludes, IDebuggerWindow& debugger, cstr resourcePath, int32 param, IScriptCompilationEventHandler& onCompile, IScriptExceptionHandler& exceptionHandler, OS::IAppControl& appControl, bool trace, Strings::StringBuilder* declarationBuilder);
		}
	}
}

#endif