#ifndef ROCOCO_API_H
#define ROCOCO_API_H

#include <rococo.types.h>

namespace DirectX
{
	struct XMFLOAT4X4;
	struct XMFLOAT4;
}

namespace Rococo
{
	namespace OS
	{
		void TripDebugger();
		void PrintDebug(const char* format, ...);
		void ShowErrorBox(Windows::IWindow& parent, IException& ex, cstr caption);
		[[nodiscard]] bool IsDebugging();
		void BreakOnThrow(BreakFlag flag);
		void SetBreakPoints(int flags);
		[[nodiscard]] ticks CpuTicks();
		[[nodiscard]] ticks CpuHz();
		[[nodiscard]] ticks UTCTime();
		void FormatErrorMessage(char* message, size_t sizeofBuffer, int errorCode);
		void BuildExceptionString(char* buffer, size_t capacity, IException& ex, bool appendStack);
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

	int32 Format(U8FilePath& path, cstr format, ...);
	int32 Format(WideFilePath& path, const wchar_t* format, ...);

	void Assign(U8FilePath& dest, const wchar_t* wideSrc);
	void Assign(WideFilePath& dest, const char* src);

	enum { MAX_FQ_NAME_LEN = 127 };
	void ValidateFQNameIdentifier(cstr fqName);

	namespace Windows
	{
		IWindow& NoParent();
		int ShowMessageBox(IWindow& window, cstr text, cstr caption, uint32 uType);
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
	struct IRenderer;
	struct IInstallation;
	struct IOS;
	struct IRenderContext;
	struct IBuffer;
	struct KeyboardEvent;
	struct MouseEvent;

	void SplitString(cstr text, size_t length, cstr seperators, IEventCallback<cstr>& onSubString);

	void GetTimestamp(char str[26]);

	struct IGuiRenderContext;

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

	bool operator == (const fstring& a, const fstring& b);

	[[nodiscard]] uint32 FastHash(cstr text);

	[[nodiscard]] fstring to_fstring(cstr const msg);

#ifdef _WIN32
	typedef size_t lsize_t;
#else
	typedef unsigned long long lsize_t;
#endif

	inline constexpr lsize_t operator "" _megabytes(lsize_t mb)
	{
		return mb * 1024 * 1024;
	}

	inline constexpr lsize_t operator "" _kilobytes(lsize_t kb)
	{
		return kb * 1024;
	}

	typedef const Matrix4x4& cr_m4x4;

	struct IDebuggerWindow;
}

namespace Rococo
{
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
		[[nodiscard]] IAllocator& CheckedAllocator();
		[[nodiscard]] IAllocatorSupervisor* CreateBlockAllocator(size_t kilobytes, size_t maxkilobytes);
	}

	template<typename T, typename U> [[nodiscard]] inline bool HasFlag(T flag, U flags)
	{
		return (flags & flag) != 0;
	}
}


#endif