#pragma once
#include <sexy.types.h>

#ifndef DOMME_API 
# define DOMME_API ROCOCO_API_EXPORT
#endif

namespace Rococo
{
	DECLARE_ROCOCO_INTERFACE IDebuggerWindow;
	DECLARE_ROCOCO_INTERFACE ISourceCache;
}

namespace Rococo::Script
{
	DECLARE_ROCOCO_INTERFACE IScriptSystemFactory;
}

namespace Rococo::Sexy
{
	struct ScriptingResources
	{
		// Script source cache
		ISourceCache& sourceCache;

		// Script debugger window
		IDebuggerWindow& debuggerWindow;

		Rococo::Script::IScriptSystemFactory& ssFactory;
	};

	struct DommeLog: Rococo::ILog
	{
		ScriptingResources& scripting;

		DommeLog(ScriptingResources& _scripting) : scripting(_scripting)
		{

		}

		DOMME_API void Write(cstr text) override;
		DOMME_API void OnUnhandledException(int errorCode, cstr exceptionType, cstr message, void* exceptionInstance) override;
		DOMME_API void OnJITCompileException(Sex::ParseException& ex) override;
	};
}