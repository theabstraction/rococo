#include <sexy.types.h>
#include <sexy.vm.h>
#include <sexy.vm.cpu.h>
#include <sexy.script.h>

#include <rococo.sexy.ide.h>
#include <rococo.cute.h>

using namespace Rococo;
using namespace Cute;

Rococo::Cute::IMasterWindow* FactoryConstructRococoCuteMasterWindow(IMasterWindowFactory* factory, const fstring& title, int32 x, int32 y, int32 dx, int32 dy)
{
	return factory->CreateMaster(title, Vec2i{ x,y }, Vec2i{ dx, dy });
}

#include "cute.sxh.inl"
#include "cute.functions.inl"

namespace Rococo { namespace Cute
{
	void ExecuteScript(cstr scriptFile, IInstallation& installation, ExecuteScriptSpec& spec, IEventCallback<ScriptCompileArgs>& onCompile, Rococo::Windows::IDE::IScriptExceptionHandler& exHandler)
	{
		if (scriptFile == nullptr || *scriptFile == 0)
		{
			Throw(0, "Rococo::Cute::ExecuteScript - scriptFile was blank");
		}

		if (spec.maxScriptSize > 128_megabytes)
		{
			Throw(0, "Rococo::Cute::ExecuteScript - Failed to execute %s: absolute max script size is 128 megabytes", scriptFile);
		}

		if (spec.parent == nullptr) spec.parent = &(Windows::NoParent());
		AutoFree<IDebuggerWindow> debugger(Rococo::Windows::IDE::CreateDebuggerWindow(*spec.parent));
		AutoFree<ISourceCache> sourceCache(CreateSourceCache(installation));
		Rococo::Windows::IDE::ExecuteSexyScriptLoop(
			spec.stats,
			spec.maxSize,
			*sourceCache,
			*debugger,
			scriptFile,
			0,
			(int32) spec.maxScriptSize,
			onCompile,
			exHandler
		);
	}

	void ExecuteWindowScript(cstr scriptFile, IInstallation& installation, ExecuteScriptSpec& spec, IMasterWindowFactory& factory)
	{
		struct : IEventCallback<ScriptCompileArgs>
		{
			IMasterWindowFactory* factory;

			void OnEvent(ScriptCompileArgs& args) override
			{
				AddNativeCalls_RococoCuteIMasterWindow(args.ss, factory);
				Rococo::Cute::Native::AddNativeCalls_RococoCuteNative(args.ss, nullptr);
				AddNativeCalls_RococoCuteIMenu(args.ss, nullptr);
				AddNativeCalls_RococoCuteISplit(args.ss, nullptr);
			}
		} onCompile;
		onCompile.factory = &factory;

		struct ScriptExceptionHandler : Rococo::Windows::IDE::IScriptExceptionHandler
		{
			Rococo::Cute::IMasterWindowFactory* factory;
			int catchCount = 0;

			ScriptExceptionHandler()
			{

			}

			void Free()
			{

			}

			Rococo::Windows::IDE::EScriptExceptionFlow GetScriptExceptionFlow(cstr source, cstr message) override
			{
				factory->Revert();

				catchCount++;

				return catchCount > 1 ?
					Rococo::Windows::IDE::EScriptExceptionFlow_Terminate : 
					Rococo::Windows::IDE::EScriptExceptionFlow_Retry;
			}
		} onScriptException;
		onScriptException.factory = &factory;

		ExecuteScript(scriptFile, installation, spec, onCompile, onScriptException);

		factory.Commit();
	}
}} // Rococo::Cute