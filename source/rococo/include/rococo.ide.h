#pragma once

#include <rococo.types.h>

namespace Rococo
{
	namespace Visitors
	{
		enum CheckState : int32;
		struct TREE_NODE_ID;
		struct IUITree;
		struct IUIList;
		struct ITreePopulator;
		struct IListPopulator;
	}

	struct IDebuggerWindow;

	ROCOCO_INTERFACE IDebugControl
	{
		virtual void Continue() = 0;
		virtual void StepOut() = 0;
		virtual void StepOver() = 0;
		virtual void StepNextSymbol() = 0;
		virtual void StepNext() = 0;
		virtual void PopulateAPITree(Visitors::IUITree& tree) = 0;
		virtual void RefreshAtDepth(int stackDepth) = 0; // Refresh source and disassembly, but do not refresh the CallStack view
	};

	ROCOCO_INTERFACE ILogger
	{
		virtual void AddLogSection(RGBAb colour, cstr format, ...) = 0;
		virtual void ClearLog() = 0;
		virtual int Log(cstr format, ...) = 0;
	};

	ROCOCO_INTERFACE IDebuggerPopulator
	{
	   virtual void Populate(IDebuggerWindow & debugger) = 0;
	};

	struct MenuCommand
	{
		cstr name;
		const uint8* buffer;
		size_t len;
	};

	enum class DISASSEMBLY_TEXT_TYPE
	{
		HEADER,
		COMMENT,
		MAIN,
		HILIGHT
	};

	ROCOCO_INTERFACE IDebuggerWindow : public ILogger
	{
		virtual void AddDisassembly(DISASSEMBLY_TEXT_TYPE type, cstr text, bool bringToView = false) = 0;
		virtual void InitDisassembly(size_t codeId) = 0;
		virtual void AddSourceCode(cstr name, cstr sourceCode) = 0;
		virtual void ClearSourceCode() = 0;
		virtual void Free() = 0;
		[[nodiscard]] virtual Windows::IWindow& GetDebuggerWindowControl() = 0;
		virtual void PopulateMemberView(Visitors::ITreePopulator& populator) = 0;
		virtual void PopulateRegisterView(Visitors::IListPopulator& populator) = 0;
		virtual void PopulateVariableView(Visitors::IListPopulator& populator) = 0;
		virtual void PopulateCallStackView(Visitors::IListPopulator& populator) = 0;
		virtual void ResetJitStatus() = 0;
		virtual void Run(IDebuggerPopulator& populator, IDebugControl& control) = 0;
		virtual void SetCodeHilight(cstr source, const Vec2i& start, const Vec2i& end, cstr message, bool jitCompileException = false) = 0;
		virtual void ShowWindow(bool show, IDebugControl* debugControl) = 0;
	};
}

namespace Rococo::OS
{
	struct IAppControl;
}

namespace Rococo::Windows::IDE
{
	ROCOCO_API IDebuggerWindow* CreateDebuggerWindowForStdout(Windows::IWindow& parent, OS::IAppControl& appControl);
}