#ifndef BLOKE_UI_H
#define BLOKE_UI_H

namespace Sexy
{
	namespace Script
	{
		struct IPublicScriptSystem;
	}
}

namespace Rococo
{
	// Generic OS independent window handle. For operating systems without windows, should normally be null
	struct WindowHandle
	{
		WindowHandle() : ptr(nullptr) {}
		void* ptr;
	};

	struct SourceFileSet;

	struct NO_VTABLE IScriptExecutionTarget
	{
		virtual void ExecuteAppScript(const wchar_t* sourceName, ICallback<Sexy::Script::IPublicScriptSystem>* onPostCompile) = 0;
	};
}

#endif // BLOKE_UI_H