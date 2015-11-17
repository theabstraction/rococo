#ifndef ROCOCO_API_H
#define ROCOCO_API_H

#include <rococo.types.h>

// The following could be done with a template, but that results in bloated error messages, and our ids are ubiquitous
#define ROCOCO_ID(DEFINED_ID_NAME,TYPE,INVALID_VALUE)										\
struct DEFINED_ID_NAME																		\
{																							\
	DEFINED_ID_NAME() : value(INVALID_VALUE) {}												\
	explicit DEFINED_ID_NAME(TYPE _value) : value(_value) {}								\
	TYPE value;																				\
    static DEFINED_ID_NAME Invalid() { return DEFINED_ID_NAME(); }							\
	size_t operator()(const DEFINED_ID_NAME& obj) const { return size_t(obj.value); }		\
};																							\
																							\
inline bool operator == (const DEFINED_ID_NAME& a, const DEFINED_ID_NAME& b) { return a.value == b.value; }				\
inline bool operator != (const DEFINED_ID_NAME& a, const DEFINED_ID_NAME& b) { return !(a == b); }

namespace DirectX
{
	struct XMFLOAT4X4;
	struct XMFLOAT4;
}

namespace Rococo
{
	typedef int64 ticks;

	ticks CpuClock();

	struct Vec2
	{
		float x;
		float y;
	};

	struct Vec3
	{
		float x;
		float y;
		float z;

		static Vec3 FromVec2(const Vec2& pos, float z)
		{
			return Vec3{ pos.x, pos.y, z };
		}
	};

	template<class T> struct Segment
	{
		T a;
		T b;
	};

	typedef const Vec3& cr_vec3;

	struct Vec4;
	struct Matrix4x4;
	struct Quat;

	struct Vec2i
	{
		int32 x;
		int32 y;
	};

	ROCOCO_ID(ID_BITMAP, uint64, -1)

	struct Sphere
	{
		Vec3 centre;
		float radius;
	};

	struct GuiRect
	{
		int32 left;
		int32 top;
		int32 right;
		int32 bottom;

		GuiRect() {}
		GuiRect(int32 _left, int32 _top, int32 _right, int32 _bottom) : left(_left), top(_top), right(_right), bottom(_bottom) {}
	};

	struct Degrees;
	struct Radians;
	struct Gravity;
	struct Metres;
	struct Quat;

	namespace Windows
	{
		struct IWindow;
		IWindow& NoParent();
		int ShowMessageBox(IWindow& window, const wchar_t* text, const wchar_t* caption, uint32 uType);
	}

	namespace Visitors
	{
		enum CheckState : int32;
		struct TREE_NODE_ID;
		struct IUITree;
		struct IUIList;
	}

	void ShowErrorBox(Windows::IWindow& parent, IException& ex, const wchar_t* caption);

	template<class T> inline T max(T a, T b)
	{
		return a > b ? a : b;
	}

	template<class T> inline T min(T a, T b)
	{
		return a < b ? a : b;
	}

	struct Kilograms
	{
		float value;
		operator float() const { return value; }
	};

	inline Kilograms operator "" _kg(long double value)
	{
		return Kilograms{ (float)value };
	}

	struct Metres
	{
		float value;
		operator float() const { return value; }
	};

	inline Metres operator "" _metres(long double value)
	{
		return Metres{ (float)value };
	}

	struct Seconds
	{
		float value;
		operator float() const { return value; }
	};

	inline Seconds operator "" _seconds(long double value)
	{
		return Seconds{ (float)value };
	}

	struct MetresPerSecond
	{
		float value;
		operator float() const { return value; }
	};

	inline MetresPerSecond operator "" _mps(long double value)
	{
		return MetresPerSecond{ (float)value };
	}

	struct Quad
	{
		float left;
		float top;
		float right;
		float bottom;

		Quad() {}
		Quad(float _left, float _top, float _right, float _bottom) : left(_left), top(_top), right(_right), bottom(_bottom) {}
	};

	template<class T> ROCOCOAPI IVectorEnumerator
	{
		virtual T* begin() = 0;
		virtual T* end() = 0;
		virtual const T* begin() const = 0;
		virtual const T* end() const = 0;
		virtual size_t size() const = 0;
	};

	struct RGBAb
	{
		uint8 red;
		uint8 green;
		uint8 blue;
		uint8 alpha;

		RGBAb() {}
		RGBAb(uint32 x) { RGBAb* pCol = (RGBAb*)&x; *this = *pCol; }
		RGBAb(uint8 _red, uint8 _green, uint8 _blue, uint8 _alpha = 255) : red(_red), green(_green), blue(_blue), alpha(_alpha) {}
	};

	struct RGBA
	{
		float red;
		float green;
		float blue;
		float alpha;

		RGBA(float _r, float _g, float _b, float _a = 1.0f) : red(_r), green(_g), blue(_b), alpha(_a) {}
	};

	struct IRenderer;
	struct IInstallation;
	struct IOS;
	struct IRenderContext;
	struct IBuffer;
	struct IUltraClock;
	struct IStringBuilder;

	struct ILock
	{
		virtual void Lock() = 0;
		virtual void Unlock() = 0;
	};

	class Sync
	{
		ILock& lock;
	public:
		Sync(ILock& _lock) : lock(_lock)
		{
			lock.Lock();
		}

		~Sync()
		{
			lock.Unlock();
		}
	};

	struct KeyboardEvent;
	struct MouseEvent;

	void SplitString(const wchar_t* text, size_t length, const wchar_t* seperators, IEventCallback<const wchar_t*>& onSubString);

	struct IGuiRenderContext;

	ROCOCOAPI IBitmapCache
	{
		virtual ID_BITMAP Cache(const wchar_t* resourceName) = 0;
		virtual void DrawBitmap(IGuiRenderContext& gc, const GuiRect& targetRect, ID_BITMAP id) = 0;
		virtual void SetCursorBitmap(ID_BITMAP id, Vec2i hotspotOffset) = 0;
		virtual void SetMeshBitmap(IRenderContext& rc, ID_BITMAP id) = 0;
	};

	ROCOCOAPI IBitmapCacheSupervisor : public IBitmapCache
	{
		virtual void Free() = 0;
	};

	IBitmapCacheSupervisor* CreateBitmapCache(IInstallation& installation, IRenderer& renderer);

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
	ROCOCO_ID(ID_SYS_MESH, size_t, (size_t)-1)

	struct fstring
	{
		const wchar_t* buffer;
		const int32 length;

		operator const wchar_t*() const { return buffer; }
	};

	bool operator == (const fstring& a, const fstring& b);

	uint32 FastHash(const wchar_t* text);

	fstring to_fstring(const wchar_t* const msg);

	inline constexpr fstring operator"" _fstring(const wchar_t* msg, size_t length)
	{
		return fstring{ msg, (int32)length };
	}

	inline size_t operator "" _megabytes(size_t mb)
	{
		return mb * 1024 * 1024;
	}

	inline size_t operator "" _kilobytes(size_t kb)
	{
		return kb * 1024;
	}

	typedef const Matrix4x4& cr_m4x4;

	ROCOCOAPI IDebugControl
	{
		virtual void Continue() = 0;
		virtual void StepOut() = 0;
		virtual void StepOver() = 0;
		virtual void StepNextSymbol() = 0;
		virtual void StepNext() = 0;
	};

	ROCOCOAPI ILogger
	{
		virtual int Log(const wchar_t* format, ...) = 0;
	};

	ROCOCOAPI IDebuggerWindow : public ILogger
	{
		virtual void AddDisassembly(bool clearFirst, const wchar_t* text) = 0;
		virtual void Free() = 0;
		virtual Windows::IWindow& GetDebuggerWindowControl() = 0;
		virtual bool IsVisible() const = 0;
		virtual void ShowWindow(bool show, IDebugControl* debugControl) = 0;
		virtual Visitors::IUITree& StackTree() = 0;
		virtual Visitors::IUIList& RegisterList() = 0;
	};
}

namespace Sexy
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

	enum EXECUTERESULT;
}

namespace Rococo
{
	ROCOCOAPI ISourceCache
	{
		virtual Sexy::Sex::ISParserTree* GetSource(const wchar_t* resourceName) = 0;
		virtual void Free() = 0;
		virtual void Release(const wchar_t* resourceName) = 0;
	};

	void DebuggerLoop(Sexy::Script::IPublicScriptSystem &ss, IDebuggerWindow& debugger);

	struct ScriptCompileArgs
	{
		Sexy::Script::IPublicScriptSystem& ss;
	};

	void ExecuteSexyScript(Sexy::Sex::ISParserTree& mainModule, IDebuggerWindow& debugger, Sexy::Script::IPublicScriptSystem& ss, ISourceCache& sources, int32 param, IEventCallback<ScriptCompileArgs>& onCompile);
	ISourceCache* CreateSourceCache(IInstallation& installation);

	void ThrowSex(Sexy::Sex::cr_sex s, const wchar_t* format, ...);
	void ScanExpression(Sexy::Sex::cr_sex s, const wchar_t* hint, const char* format, ...);
	void ValidateArgument(Sexy::Sex::cr_sex s, const wchar_t* arg);

	Vec3 GetVec3Value(Sexy::Sex::cr_sex sx, Sexy::Sex::cr_sex sy, Sexy::Sex::cr_sex sz);
	RGBAb GetColourValue(Sexy::Sex::cr_sex s);
	Quat GetQuat(Sexy::Sex::cr_sex s);

	void LogParseException(Sexy::Sex::ParseException& ex, ILogger& logger);

	fstring GetAtomicArg(Sexy::Sex::cr_sex s);
}

namespace Rococo
{
	namespace Random
	{
		uint32 Next();
		uint32 Next(uint32 modulus);
		void Seed(uint32 value = 0);
		float NextFloat(float minValue, float maxValue);
		Vec3 NextNormalVector();
	}
}

#endif