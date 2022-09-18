#pragma once

#include <rococo.api.h>

namespace Rococo
{
	namespace Cute
	{
		enum ResizeType;

		ROCOCO_INTERFACE IWindowSupervisor
		{
			virtual void AddChild(IWindowSupervisor* child) = 0;
			virtual void Close() = 0;
			virtual void Free() = 0;
			virtual void OnResize(Vec2i span, ResizeType to) = 0;
			virtual WindowRef Handle() = 0;
		};
	}

	namespace Post
	{
		struct IPostbox;
	}
}

#ifndef ROCOCO_CUTE_SXH_H
# define ROCOCO_CUTE_SXH_H
# include <../rococo.cute/cute.sxh.h>
#endif

namespace Rococo
{
	namespace Script
	{
		struct IScriptSystemFactory;
	}
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
			Rococo::Windows::IWindow* debuggerParentWnd = nullptr;
		};

		enum ResizeType
		{
			ResizeTo_Normal,
			ResizeTo_Full,
			ResizeTo_Minimize
		};

		ROCOCO_INTERFACE ISplitSupervisor : IWindowSupervisor
		{
			virtual ISplit& Split() = 0;
		};

		ROCOCO_INTERFACE IChildSupervisor : IWindowSupervisor
		{
		};

		struct CuteWindowExData
		{
			RGBAb normalBackgroundColour;
			RGBAb hilighBackgroundColour;
		};

		ROCOCO_INTERFACE IMasterWindowFactory
		{
			virtual void Free() = 0;
			virtual void Commit() = 0;
			virtual bool HasInstances() const = 0;
			virtual void Revert() = 0;
			virtual IMasterWindow* CreateMaster(cstr title, const Vec2i& pos, const Vec2i& span) = 0;
			virtual Post::IPostbox& Postbox() = 0;
		};

		void ExecuteScript(cstr scriptFile, IInstallation& installation, Rococo::Script::IScriptSystemFactory& ssFactory, ExecuteScriptSpec& spec, IEventCallback<ScriptCompileArgs>& onCompile, Rococo::Windows::IDE::IScriptExceptionHandler& exHandler, bool trace);
		void ExecuteWindowScript(cstr scriptFile, IInstallation& installation, Rococo::Script::IScriptSystemFactory& ssFactory, ExecuteScriptSpec& spec, IMasterWindowFactory& factory);

		struct IMenu;
		struct ISplit;

		ROCOCO_INTERFACE IMenuSupervisor
		{
			virtual IMenu& Menu() = 0;
			virtual void Free() = 0;
		};

		/// N.B for multiply derived classes, ensure constructor has finished before passing a reference here
		void SetMasterProc(WindowRef ref, IWindowSupervisor* window);

#ifdef _WIN32
# ifdef WINAPI
		IMasterWindowFactory* CreateMasterWindowFactory(HINSTANCE hInstance, HWND hParent, Post::IPostbox& postbox);
		IMenuSupervisor* CreateCuteMenu(HWND hWndOwner);
		ITree* CreateTree(IParentWindow& parent, Post::IPostbox& post, int32 createStyleFlags);
		IParentWindow* CreateParent(IWindowSupervisor& parent, DWORD style, DWORD exStyle, int32 x, int32 y, int32 dx, int32 dy, Post::IPostbox& post);
		ISplitSupervisor* CreateSplit(ATOM atom, IParentWindow& parent, int32 pixelSplit, int32 minLo, int32 maxHi, int32 splitterWidth, boolean32 draggable, bool isLeftAndRight, Post::IPostbox& post);
		IChildSupervisor* CreateChildProxy(HWND hWnd);
		IChildSupervisor* CreateChild(HWND hParentWnd, DWORD style, DWORD exStyle, int32 x, int32 y, int32 dx, int32 dy);
		IChildSupervisor* CreateChild(IWindowBase& window, DWORD style, DWORD exStyle, int32 x, int32 y, int32 dx, int32 dy);
# endif
#endif

		namespace Native
		{
			void GetWindowRect(WindowRef hWnd, Vec2i& pos, Vec2i& span);
			void GetSpan(WindowRef hWnd, Vec2i& span);
			void SetText(WindowRef hWnd, const fstring& text);
			int32 GetText(WindowRef hWnd, IStringPopulator& sb);
			void SetColourTarget(WindowRef hWnd, ColourTarget target, RGBAb colour);
		}

		ROCOCO_INTERFACE ITreeNode
		{
			virtual ITreeNode* AddItem(cstr item) = 0;
		};
	}
}