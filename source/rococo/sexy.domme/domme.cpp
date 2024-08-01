#include <rococo.types.h>
#define DOMME_API ROCOCO_API_EXPORT
#include <rococo.domme.h>
#include <rococo.ide.h>
#include <Sexy.S-Parser.h>

namespace Rococo::Sexy
{
	DOMME_API void DommeLog::Write(cstr text)
	{
		scripting.debuggerWindow.Log("%s", text);
	}

	DOMME_API void DommeLog::OnUnhandledException(int errorCode, cstr exceptionType, cstr message, void* exceptionInstance)
	{
		UNUSED(exceptionInstance);

		if (errorCode)
		{
			scripting.debuggerWindow.Log("Unhandled exception: %s: %s\n", exceptionType, message);
		}
		else
		{
			scripting.debuggerWindow.Log("Unhandled exception: (Code %d) %s: %s\n", errorCode, exceptionType, message);
		}
	}

	DOMME_API void DommeLog::OnJITCompileException(Sex::ParseException& ex)
	{		
		cstr sourceName = ex.Source()->Tree().Source().Name();
		scripting.debuggerWindow.Log("JIT exception: (Code %d) %s: %s. Line %d pos %d\n", ex.ErrorCode(), sourceName, ex.Message(), ex.Start().y, ex.Start().x);
	}
}