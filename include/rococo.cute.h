#pragma once

#include <rococo.api.h>

namespace Rococo
{
	namespace Windows
	{
		namespace IDE
		{
			struct IScriptExceptionHandler;
		}
	}
	namespace Cute
	{
		struct IMasterWindow;

		struct ExecuteScriptSpec
		{
			ScriptPerformanceStats stats = { 0 };
			size_t maxSize = 128_kilobytes;
			size_t maxScriptSize = 128_kilobytes;
			Rococo::Windows::IWindow* parent = nullptr;
		};

		ROCOCOAPI IMasterWindowFactory
		{
			virtual void Free() = 0;
			virtual void Commit() = 0;
			virtual bool HasInstances() const = 0;
			virtual void Revert() = 0;
			virtual IMasterWindow* CreateMaster(cstr title, const Vec2i& pos, const Vec2i& span) = 0;
		};

		void ExecuteScript(cstr scriptFile, IInstallation& installation, ExecuteScriptSpec& spec, IEventCallback<ScriptCompileArgs>& onCompile, Rococo::Windows::IDE::IScriptExceptionHandler& exHandler);
		void ExecuteWindowScript(cstr scriptFile, IInstallation& installation, ExecuteScriptSpec& spec, IMasterWindowFactory& factory);

#ifdef _WIN32
# ifdef WINAPI
		IMasterWindowFactory* CreateMasterWindowFactory(HINSTANCE hInstance, HWND hParent);
# endif
#endif
	}
}