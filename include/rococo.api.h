#ifndef ROCOCO_API_H
#define ROCOCO_API_H

#include <rococo.types.h>

#ifndef ROCOCO_API
# define ROCOCO_API __declspec(dllimport)
#endif

#ifndef ROCOCO_WINDOWS_API 
# define ROCOCO_WINDOWS_API __declspec(dllimport)
#endif

namespace DirectX
{
	struct XMFLOAT4X4;
	struct XMFLOAT4;
}

namespace Rococo
{
	ROCOCO_API bool IsPointerValid(const void* ptr);

	namespace OS
	{
		ROCOCO_API void SetCursorVisibility(bool isVisible, Rococo::Windows::IWindow& captureWindow);
		ROCOCO_API void ShellOpenDocument(cstr path);
		ROCOCO_API void TripDebugger();
		ROCOCO_API void PrintDebug(const char* format, ...);
		ROCOCO_WINDOWS_API void ShowErrorBox(Windows::IWindow& parent, IException& ex, cstr caption);
		ROCOCO_API [[nodiscard]] bool IsDebugging();
		ROCOCO_API void BreakOnThrow(BreakFlag flag);
		ROCOCO_API void SetBreakPoints(int flags);
		ROCOCO_API [[nodiscard]] ticks CpuTicks();
		ROCOCO_API [[nodiscard]] ticks CpuHz();
		ROCOCO_API [[nodiscard]] ticks UTCTime();
		ROCOCO_API void FormatErrorMessage(char* message, size_t sizeofBuffer, int errorCode);
		ROCOCO_API void BuildExceptionString(char* buffer, size_t capacity, IException& ex, bool appendStack);
		ROCOCO_API cstr GetCommandLineText();
	}

	namespace Maths::IEEE475
	{
		ROCOCO_API float BinaryToFloat(uint32 binaryRepresentation);
		ROCOCO_API double BinaryToDouble(uint64 binaryRepresentation);
		ROCOCO_API uint32 FloatToBinary(float f);
		ROCOCO_API uint64 DoubleToBinary(double d);
	}

	struct Quat;

	ROCOCO_ID(ID_FONT, int32, -1);
	ROCOCO_ID(ID_TEXTURE, size_t, -1)
	ROCOCO_ID(ID_VERTEX_SHADER, size_t, -1)
	ROCOCO_ID(ID_PIXEL_SHADER, size_t, -1)
	ROCOCO_ID(ID_GEOMETRY_SHADER, size_t, -1)
	ROCOCO_ID(ID_CUBE_TEXTURE, size_t, 0);

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

	namespace Strings
	{
#if USE_VSTUDIO_SAL
		ROCOCO_API int32 Format(U8FilePath& path, _Printf_format_string_ cstr format, ...);
		ROCOCO_API int32 Format(WideFilePath& path, _Printf_format_string_ const wchar_t* format, ...);
#else
		ROCOCO_API int32 Format(U8FilePath& path, cstr format, ...);
		ROCOCO_API int32 Format(WideFilePath& path, const wchar_t* format, ...);
#endif
		ROCOCO_API void Assign(U8FilePath& dest, const wchar_t* wideSrc);
		ROCOCO_API void Assign(WideFilePath& dest, const char* src);

		ROCOCO_API void ValidateFQNameIdentifier(cstr fqName);

		ROCOCO_API [[nodiscard]] uint32 FastHash(cstr text);

		ROCOCO_API void SplitString(cstr text, size_t length, cstr seperators, IEventCallback<cstr>& onSubString);
	}

	namespace Windows
	{
		ROCOCO_WINDOWS_API IWindow& NoParent();
		ROCOCO_WINDOWS_API int ShowMessageBox(IWindow& window, cstr text, cstr caption, uint32 uType);
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

	struct IInstallation;
	struct IOS;
	
	struct IBuffer;
	struct KeyboardEvent;
	struct MouseEvent;

	ROCOCO_API void GetTimestamp(char str[26]);

	namespace Post
	{
		struct IPostbox;
	}

	// ID_MESH are artist defined indices. The artist chooses a number to associate with a mesh.
	// Rough convention:
	//    id  0          invalid
	//    ids 1          to 0x1FFFFFFF are defined in script files and level editors
	//    ids 0x20000000 to 0x20FFFFFF are procedurally generated paths, roads and rivers created in C++
	//    ids 0x21000000 t0 0x21FFFFFF are procedurally generated houses created in C++.
	//    ids 0x40000000 to 0x41000000 are gameplay generated meshes such as explosions created in C++.
	//	  ids 0x41000001 to 0x42000000 are skeletal animation meshes
	ROCOCO_ID(ID_MESH, int32, 0)

	// ID_SYS_MESH are renderer defined indices that are generated when meshes are loaded into the renderer
	ROCOCO_ID(ID_SYS_MESH, size_t, -1)

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
	void ExpandZoneToContain(GuiRect& rect, const Vec2i& p);
	void ExpandZoneToContain(GuiRectf& rect, const Vec2& p);

	namespace Memory
	{
		ROCOCO_API [[nodiscard]] IAllocator& CheckedAllocator();
		ROCOCO_API [[nodiscard]] IAllocatorSupervisor* CreateBlockAllocator(size_t kilobytes, size_t maxkilobytes);
	}
}


#endif