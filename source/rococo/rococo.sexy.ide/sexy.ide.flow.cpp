#include <rococo.sexy.ide.h>

#include <sexy.types.h>
#include <sexy.compiler.public.h>
#include <sexy.debug.types.h>
#include <sexy.vm.h>
#include <sexy.vm.cpu.h>
#include <sexy.script.h>
#include <sexy.s-parser.h>

#include <rococo.strings.h>
#include <rococo.ide.h>
#include <rococo.os.h>

#include <rococo.debugging.h>
#include <rococo.maths.i32.h>
#include <rococo.time.h>

using namespace Rococo;
using namespace Rococo::Windows;
using namespace Rococo::Windows::IDE;

using namespace Rococo::Strings;
using namespace Rococo::Sex;
using namespace Rococo::VM;

namespace
{
	void SaveInCriticalErrorLog(Sex::ParseException& ex)
	{
		char msg[1024];
		SafeFormat(msg, "\nLine %d col %d to Line %d pos %d - %s\n", ex.Start().y+1, ex.Start().x+1, ex.End().y+1, ex.End().x+1, ex.Name());
		Rococo::Debugging::AddCriticalLog(msg);

		SafeFormat(msg, "%s\n", ex.Message());
		Rococo::Debugging::AddCriticalLog(msg);

		SafeFormat(msg, "Specimen: %s\n", ex.Specimen());
		Rococo::Debugging::AddCriticalLog(msg);
	}

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
			UNUSED(exceptionInstance);
			debugger.Log("Unhandled: %s\n%s. Coded %d", exceptionType, message, errorCode);
		}

		virtual void OnJITCompileException(Sex::ParseException& ex)
		{
			// Ensure we can capture the error
			SaveInCriticalErrorLog(ex);
			LogParseException(ex, debugger, true);
		}
	};

	void ThrowScriptError(ParseException& ex)
	{
		Rococo::Throw(ex.ErrorCode(), "%s (%d,%d) to (%d,%d). %s:\r\n%s", ex.Name(), ex.Start().x, ex.Start().y, ex.End().x, ex.End().y, ex.Name(), ex.Message());
	}

	void InitDebugger(IDebuggerWindow& debugger, ParseException& ex)
	{
		debugger.ClearSourceCode();
		auto* src = ex.Source();
		if (src)
		{
			Vec2i start = ex.Start() - Vec2i { 1, 0};
			Vec2i end = ex.End() - Vec2i { 1, 0};
			debugger.SetCodeHilight(ex.Name(), start, end, ex.Message());
			debugger.AddSourceCode(ex.Name(), src->Tree().Source().SourceStart());
		}
		LogParseException(ex, debugger);
	}

	class PersistentScript : public IPersistentScript
	{
		ScriptLogger logger;
		IDebuggerWindow& debugger;
		ISourceCache& sources;
		ISParserTree* tree;
		AutoFree<Rococo::Script::IPublicScriptSystem> ss;
		size_t maxBytes;
	public:
		PersistentScript(size_t _maxBytes, IScriptSystemFactory& factory, ISourceCache& _sources, IScriptEnumerator& implicitIncludes, IDebuggerWindow& _debugger, cstr resourcePath, IScriptCompilationEventHandler& onCompile, IScriptExceptionHandler& _exceptionHandler) :
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
					InitSexyScript(*tree, debugger, *ss, sources, implicitIncludes, onCompile, nullptr);
					break;
				}
				catch (ParseException& ex)
				{
					InitDebugger(debugger, ex);

					switch (_exceptionHandler.GetScriptExceptionFlow(ex.Name(), ex.Message()))
					{
					case EScriptExceptionFlow::Retry:
						break;

					case EScriptExceptionFlow::Ignore:
					case EScriptExceptionFlow::Terminate:
						ThrowScriptError(ex);
						return;
					}
				}
				catch (IException& ex)
				{
					debugger.ClearSourceCode();
					logger.debugger.Log("%s", ex.Message());

					switch (_exceptionHandler.GetScriptExceptionFlow("--app--", ex.Message()))
					{
					case EScriptExceptionFlow::Ignore:
						return;
					case EScriptExceptionFlow::Retry:
						break;

					case EScriptExceptionFlow::Terminate:
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
				InitDebugger(debugger, ex);

				switch (exceptionHandler.GetScriptExceptionFlow(ex.Name(), ex.Message()))
				{
				case EScriptExceptionFlow::Ignore:
					return;
				case EScriptExceptionFlow::Retry:
					break;
				case EScriptExceptionFlow::Terminate:
					ThrowScriptError(ex);
					break;
				}
			}
			catch (IException& ex)
			{
				debugger.ClearSourceCode();
				logger.debugger.Log("%s", ex.Message());

				switch (exceptionHandler.GetScriptExceptionFlow("--app--", ex.Message()))
				{
				case EScriptExceptionFlow::Ignore:
					return;
				case EScriptExceptionFlow::Retry:
					break;
				case EScriptExceptionFlow::Terminate:
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
				InitDebugger(debugger, ex);

				switch (exceptionHandler.GetScriptExceptionFlow(ex.Name(), ex.Message()))
				{
				case EScriptExceptionFlow::Ignore:
					return;
				case EScriptExceptionFlow::Retry:
					break;
				case EScriptExceptionFlow::Terminate:
					ThrowScriptError(ex);
					break;
				}
			}
			catch (IException& ex)
			{
				debugger.ClearSourceCode();
				logger.debugger.Log("Exection thrown: %s", ex.Message());

				switch (exceptionHandler.GetScriptExceptionFlow("--app--", ex.Message()))
				{
				case EScriptExceptionFlow::Ignore:
					return;
				case EScriptExceptionFlow::Retry:
					break;
				case EScriptExceptionFlow::Terminate:
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
			void LogStack(IException& ex, ILogger& logger)
			{
				auto* sf = ex.StackFrames();
				if (!sf) return;

				using namespace Rococo::Debugging;

				struct Formatter: public IStackFrameFormatter
				{
					ILogger& logger;
					Formatter(ILogger& argLogger) : logger(argLogger)
					{

					}

					void Format(const StackFrame& frame) override
					{
						logger.Log("%64s - %s line %d", frame.functionName, frame.sourceFile, frame.lineNumber);
					}
				} sfFormatter(logger);

				sf->FormatEachStackFrame(sfFormatter);
			}

			int32 ExecuteSexyScriptLoop(
				ScriptPerformanceStats& stats,
				size_t maxBytes,
				IScriptSystemFactory& factory,
				ISourceCache& sources,
				IScriptEnumerator& implicitIncludes,
				IDebuggerWindow& debugger,
				cstr resourcePath, 
				int32 param, 
				IScriptCompilationEventHandler& onCompile, 
				IScriptExceptionHandler& exceptionHandler,
				OS::IAppControl& appControl, 
				bool trace, 
				StringBuilder* declarationBuilder
			)
			{
				ScriptLogger logger(debugger);

				while (appControl.IsRunning())
				{
					logger.lastError[0] = 0;
					Rococo::Compiler::ProgramInitParameters pip;
					pip.addCoroutineLib = true;
					pip.MaxProgramBytes = maxBytes;
					pip.NativeSourcePath = nullptr;
					pip.addIO = true;

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
						case EScriptExceptionFlow::Ignore:
							return 0;
						case EScriptExceptionFlow::Retry:
						case EScriptExceptionFlow::Terminate:
							Throw(0, "Failed to create script system: %s", logger.lastError);
						}
					}

					try
					{
						Time::ticks start = Time::TickCount();
						auto* tree = sources.GetSource(resourcePath);
						stats.loadTime = Time::TickCount() - start;
						debugger.ResetJitStatus();
						int32 exitCode = ExecuteSexyScript(stats, *tree, debugger, ss, sources, implicitIncludes, param, onCompile, trace, declarationBuilder);
						return exitCode;
					}
					catch (ParseException& ex)
					{
						InitDebugger(debugger, ex);
						debugger.Log("Caught exception during execution of %s", resourcePath);

						//LogStack(ex, debugger);

						switch (exceptionHandler.GetScriptExceptionFlow(ex.Name(), ex.Message()))
						{
						case EScriptExceptionFlow::Ignore:
							return 0;
						case EScriptExceptionFlow::Retry:
							break;
						case EScriptExceptionFlow::Terminate:
							ThrowScriptError(ex);
							break;
						}
					}
					catch (IException& ex)
					{
						debugger.ClearSourceCode();

						if (ex.ErrorCode() != 0)
						{
							char errorMessage[256];
							Rococo::OS::FormatErrorMessage(errorMessage, sizeof(errorMessage), ex.ErrorCode());
							debugger.Log("Exception thrown in script: %s\nError code 0x%x (%d). %s\nMain file: %s", ex.Message(), ex.ErrorCode(), ex.ErrorCode(), errorMessage, resourcePath);
						}
						else
						{
							debugger.Log("Exception thrown in script %s: %s", resourcePath, ex.Message());
						}

						//LogStack(ex, debugger);

						switch (exceptionHandler.GetScriptExceptionFlow("--app--", ex.Message()))
						{
						case EScriptExceptionFlow::Ignore:
							return 0;
						case EScriptExceptionFlow::Retry:
							break;
						case EScriptExceptionFlow::Terminate:
							Throw(ex.ErrorCode(), "%s", ex.Message());
							return 0;
						}
					}

					sources.ReleaseAll();

					DebuggerLoop(ss, debugger);
				}

				return 0;
			}

			IPersistentScript* CreatePersistentScript(size_t maxBytes, IScriptSystemFactory& factory, ISourceCache& sources, IScriptEnumerator& implicitIncludes, IDebuggerWindow& debugger, cstr resourcePath, IScriptCompilationEventHandler& onCompile, IScriptExceptionHandler& exceptionHandler)
			{
				return new PersistentScript(maxBytes, factory, sources, implicitIncludes, debugger, resourcePath, onCompile, exceptionHandler);
			}
		}// IDE
	}
} // Rococo