#include <rococo.types.h>
#include <rococo.io.h>
#include <rococo.visitors.h>
#include <sexy.types.h>
#include <sexy.compiler.public.h>
#include <sexy.debug.types.h>
#include <sexy.script.h>
#include <Sexy.S-Parser.h>
#include <sexy.vm.h>
#include <sexy.vm.cpu.h>

#include <malloc.h>
#include <wchar.h>
#include <process.h>

#include <windows.h>

#include "dystopia.h"

#include <string>
#include <unordered_map>

using namespace Rococo;
using namespace Sexy;
using namespace Sexy::Sex;
using namespace Sexy::VM;

namespace Dystopia
{
	struct ScriptLogger : ILog
	{
		ILogger& logger;
		ScriptLogger(ILogger& _logger) : logger(_logger) {}

		virtual void Write(csexstr text)
		{
			logger.Log(L"%s", text);
		}

		virtual void OnUnhandledException(int errorCode, csexstr exceptionType, csexstr message, void* exceptionInstance)
		{

		}

		virtual void OnJITCompileException(Sex::ParseException& ex)
		{
			LogParseException(ex, logger);
		}
	};

	void ExecuteSexyScriptLoop(size_t maxBytes, ISourceCache& sources, IDebuggerWindow& debugger, const wchar_t* resourcePath, int32 param, int32 maxScriptSizeBytes, IEventCallback<ScriptCompileArgs>& onCompile)
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
				auto id = ShowContinueBox(debugger.GetDebuggerWindowControl(), L"Failed to create script system");
				Throw(0, L"Failed to create script system");
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
				auto id = ShowContinueBox(debugger.GetDebuggerWindowControl(), ex.Message());
				if (id == CMD_ID_EXIT) Throw(ex.ErrorCode(), L"%s", ex.Message());
				else if (id == CMD_ID_IGNORE) return;
			}
			catch (IException& ex)
			{
				auto id = ShowContinueBox(debugger.GetDebuggerWindowControl(), ex.Message());
				if (id == CMD_ID_EXIT) Throw(ex.ErrorCode(), L"%s", ex.Message());
				else if (id == CMD_ID_IGNORE) return;
			}

			sources.Release(resourcePath);

			DebuggerLoop(ss, debugger);
		}
	}
}