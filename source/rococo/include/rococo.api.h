#ifndef ROCOCO_API_H
#define ROCOCO_API_H

#pragma once

#include <rococo.types.h>

namespace DirectX
{
	struct XMFLOAT4X4;
	struct XMFLOAT4;
}

namespace Rococo
{
	ROCOCO_API bool IsPointerValid(const void* ptr);

	struct Quat;

	ROCOCO_ID(ID_FONT, int32, -1);
	ROCOCO_ID(ID_TEXTURE, size_t, (size_t)-1LL);
	ROCOCO_ID(ID_VERTEX_SHADER, size_t, (size_t)-1LL);
	ROCOCO_ID(ID_GEOMETRY_SHADER, size_t, (size_t)-1LL);
	ROCOCO_ID(ID_CUBE_TEXTURE, size_t, 0);
	ROCOCO_ID(ID_PIXEL_SHADER, size_t, (size_t)-1LL);
								     
	struct Sphere
	{
		Vec3 centre;
		float radius;
	};

	struct Platform;
	struct Gravity;
	struct Metres;
	struct Quat;

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

namespace Rococo
{
	namespace Memory
	{
		ROCOCO_API [[nodiscard]] IAllocator& CheckedAllocator();
		ROCOCO_API [[nodiscard]] IAllocatorSupervisor* CreateBlockAllocator(size_t kilobytes, size_t maxkilobytes, const char* const name);
		ROCOCO_API void* AlignedAlloc(size_t nBytes, int32 alignment, void* allocatorFunction(size_t));
		ROCOCO_API void AlignedFree(void* buffer, void deleteFunction(void*));
	}
}


#endif