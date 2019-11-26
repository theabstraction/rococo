#include <rococo.sexy.ide.h>

#include <sexy.types.h>
#include <sexy.compiler.public.h>
#include <sexy.debug.types.h>
#include <sexy.vm.h>
#include <sexy.vm.cpu.h>
#include <sexy.script.h>
#include <sexy.s-parser.h>

#include <rococo.strings.h>

using namespace Rococo;
using namespace Rococo::Windows;
using namespace Rococo::Windows::IDE;


using namespace Rococo::Sex;
using namespace Rococo::VM;

namespace
{
	struct ScriptLogger : ILog
	{
		IDebuggerWindow& debugger;
		ScriptLogger(IDebuggerWindow& _debugger) : debugger(_debugger) {}
		char lastError[1024] = { 0 };
		virtual void Write(cstr text)
		{
			SafeFormat(lastError, sizeof(lastError), "%s", text);
			debugger.Log("%s", text);
		}

		virtual void OnUnhandledException(int errorCode, cstr exceptionType, cstr message, void* exceptionInstance)
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
		AutoFree<Rococo::Script::IPublicScriptSystem> ss;
		size_t maxBytes;
	public:
		PersistentScript(size_t _maxBytes, IScriptSystemFactory& factory, ISourceCache& _sources, IDebuggerWindow& _debugger, cstr resourcePath, int32 maxScriptSizeBytes, IEventCallback<ScriptCompileArgs>& onCompile, IScriptExceptionHandler& _exceptionHandler) :
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
					ss = factory.CreateScriptSystem(Rococo::Compiler::ProgramInitParameters{ maxBytes }, logger);
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

		virtual void ExecuteFunction(ArchetypeCallback fn, IArgEnumerator& args, IScriptExceptionHandler& exceptionHandler, bool trace)
		{
			try
			{
				Rococo::ExecuteFunction(fn.byteCodeId, args, *ss, debugger, trace);
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

		virtual void ExecuteFunction(cstr name, IArgEnumerator& args, IScriptExceptionHandler& exceptionHandler, bool trace)
		{
			try
			{
				Rococo::ExecuteFunction(name, args, *ss, debugger, trace);
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
	namespace Windows
	{
		namespace IDE
		{
			int32 ExecuteSexyScriptLoop(ScriptPerformanceStats& stats, size_t maxBytes, IScriptSystemFactory& factory, ISourceCache& sources, IDebuggerWindow& debugger, cstr resourcePath, int32 param, int32 maxScriptSizeBytes, IEventCallback<ScriptCompileArgs>& onCompile, IScriptExceptionHandler& exceptionHandler, OS::IAppControl& appControl, bool trace)
			{
				ScriptLogger logger(debugger);

				Auto<ISourceCode> src;
				Auto<ISParserTree> tree;

				while (appControl.IsRunning())
				{
					logger.lastError[0] = 0;
					Rococo::Compiler::ProgramInitParameters pip;
					pip.addCoroutineLib = true;
					pip.MaxProgramBytes = maxBytes;
					pip.NativeSourcePath = nullptr;

#ifdef _DEBUG
					pip.useDebugLibs = true;
#else
					pip.useDebugLibs = false;
#endif

					AutoFree<IPublicScriptSystem> pSS (factory.CreateScriptSystem(pip, logger));
					Script::IPublicScriptSystem& ss = *pSS;
					if (!IsPointerValid(&ss))
					{
						switch (exceptionHandler.GetScriptExceptionFlow("SexyScriptSystem", logger.lastError))
						{
						case EScriptExceptionFlow_Ignore:
							return 0;
						case EScriptExceptionFlow_Retry:
						case EScriptExceptionFlow_Terminate:
							Throw(0, "Failed to create script system: %s", logger.lastError);
						}
					}

					try
					{
						OS::ticks start = OS::CpuTicks();
						ISParserTree* tree = sources.GetSource(resourcePath);
						stats.loadTime = OS::CpuTicks() - start;
						int32 exitCode = ExecuteSexyScript(stats, *tree, debugger, ss, sources, param, onCompile, trace);
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
						if (ex.ErrorCode() != 0)
						{
							debugger.Log("Exception thrown in script: %s\nError code 0x%x (%d)", ex.Message(), ex.ErrorCode(), ex.ErrorCode());
						}
						else
						{
							debugger.Log("Exception thrown in script: %s", ex.Message());
						}

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

					sources.ReleaseAll();

					DebuggerLoop(ss, debugger);
				}

				return 0;
			}

			IPersistentScript* CreatePersistentScript(size_t maxBytes, IScriptSystemFactory& factory, ISourceCache& sources, IDebuggerWindow& debugger, cstr resourcePath, int32 maxScriptSizeBytes, IEventCallback<ScriptCompileArgs>& onCompile, IScriptExceptionHandler& exceptionHandler)
			{
				return new PersistentScript(maxBytes, factory, sources, debugger, resourcePath, maxScriptSizeBytes, onCompile, exceptionHandler);
			}
		}// IDE
	}
} // Rococo