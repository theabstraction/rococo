#ifndef ROCOCO_API_H
#define ROCOCO_API_H

#pragma once

#include <rococo.types.h>

// This file is deprecated. move stuff out of it when you can. It grew too large...

namespace DirectX
{
	struct XMFLOAT4X4;
	struct XMFLOAT4;
}

namespace Rococo
{
	ROCOCO_API bool IsPointerValid(const void* ptr);
						     
	struct Platform;
	struct Gravity;
	struct Metres;

	struct StringKeyValuePairArg
	{
		cstr key;
		cstr value;
	};

#ifdef _WIN32
	namespace Windows
	{
		struct IWindow; // defined in rococo.windows.h which provides HWND
	}
#else
	ROCOCO_ID(ID_OSWINDOW, uint64, 0);

	namespace Windows
	{
		struct IWindow
		{
			virtual operator ID_OSWINDOW() const = 0;
		};
	}
#endif

	namespace Windows
	{
		// Minimizes app to the task bar, or whatever the OS uses to contain references to invisible apps
		ROCOCO_API void MinimizeApp(IWindow& window);

		// Toggles between fullscreen and nominal app size
		ROCOCO_API void RestoreApp(IWindow& window);

		// Requests a window to close using standard OS closing procedures
		ROCOCO_API void SendCloseEvent(IWindow& window);

		[[nodiscard]]
		ROCOCO_API Vec2i GetDesktopSpan();
		ROCOCO_API Vec2i GetDesktopWorkArea();
	}

	namespace Graphics
	{
		struct IRenderer;
		struct IRenderContext;
		struct IGuiRenderContext;
	}

	namespace Visitors
	{
		enum CheckState : int32;
		struct TREE_NODE_ID;
		struct IUITree;
		struct IUIList;
		struct ITreePopulator;
		struct IListPopulator;
	}

	struct IUltraClock;

	namespace IO
	{
		struct IInstallation;
		struct IOS;
	}
	
	struct IBuffer;
	struct KeyboardEvent;
	struct MouseEvent;

	ROCOCO_API void GetTimestamp(char str[26]);

	namespace Post
	{
		struct IPostbox;
	}

	ROCOCO_ID(ID_WIDGET, int32, 0);
	ROCOCO_ID(ID_UI_EVENT_TYPE, int64, 0);

	ROCOCO_API bool operator == (const fstring& a, const fstring& b);

	ROCOCO_API [[nodiscard]] fstring to_fstring(cstr const msg);

	typedef const Matrix4x4& cr_m4x4;

	struct IDebuggerWindow;
}

namespace Rococo
{
	struct IExpandingBuffer;
	struct ISubsystemMonitor;

	ROCOCO_INTERFACE IPingPathResolver
	{
		virtual void PingPathToSysPath(cstr pingPath, U8FilePath & sysPath) = 0;
		virtual void SysPathToPingPath(cstr sysPath, U8FilePath& pingPath) = 0;
	};

	namespace Script
	{
		struct IPublicScriptSystem;
	}

	namespace Sex
	{
		struct ISParserTree;
		struct ISExpression;
		typedef const ISExpression& cr_sex;
		class ParseException;
		
	}

	namespace VM
	{
		struct IVirtualMachine;
	}

	namespace Cute
	{
		struct IMasterWindowFactory;
		struct IWindowSupervisor;
	}

	namespace Script
	{
		ROCOCO_API void PopulateStringBuilder(InterfacePointerToStringBuilder sb, const fstring& text);
	}

	enum EXECUTERESULT : int32;
}

namespace Rococo
{
	struct IMathsVenue;
}

#endif