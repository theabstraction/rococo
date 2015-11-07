#ifndef Rococo_TYPES_H
#define Rococo_TYPES_H

#ifndef Rococo_COMPILER_OPTIONS_ARE_SET
# include <rococo.compiler.options.h>
#endif

#define SecureFormat swprintf_s // Needs include <wchar.h>. If the output buffer is exhausted it will throw an exception
#define SafeFormat _snwprintf_s // Needs include <wchar.h>. With _TRUNCATE in the MaxCount position, it will truncate buffer overruns, rather than throw
#define SafeVFormat _vsnwprintf_s // Needs include <wchar.h>. With _TRUNCATE in the MaxCount position, it will truncate buffer overruns, rather than throw
#define SecureCopy wcscpy_s // Needs include <wchar.h>. If the output buffer is exhausted it will throw an exception
#define SafeCopy wcsncpy_s // Needs include <wchar.h>. With _TRUNCATE in the MaxCount position, it will truncate buffer overruns, rather than throw
#define SecureCat wcscat_s // Needs include <wchar.h>.  If the output buffer is exhausted it will throw an exception
#define SafeCat wcsncat_s // Needs include <wchar.h>.  With _TRUNCATE in the MaxCount position, it will truncate buffer overruns, rather than throw

// The following could be done with a template, but that results in bloated error messages, and our ids are ubiquitous
#define ROCOCO_ID(DEFINED_ID_NAME,TYPE,INVALID_VALUE)										\
struct DEFINED_ID_NAME																		\
{																							\
	DEFINED_ID_NAME() : value(INVALID_VALUE) {}												\
	explicit DEFINED_ID_NAME(TYPE _value) : value(_value) {}								\
	TYPE value;																				\
	bool operator == (const DEFINED_ID_NAME& obj) { return value == obj.value; }			\
	bool operator != (const DEFINED_ID_NAME& obj) { return value != obj.value; }			\
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
	typedef __int8 int8;
	typedef __int16 int16;
	typedef __int32 int32;
	typedef __int64 int64;

	typedef unsigned __int8 uint8;
	typedef unsigned __int16 uint16;
	typedef unsigned __int32 uint32;
	typedef unsigned __int64 uint64;

	typedef float float32;
	typedef double float64;
	typedef void* pointer;
	typedef int32 boolean32; // 32-bit boolean

	typedef int64 ticks;
	
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
	};

	typedef const Vec3& cr_vec3;

	struct Vec4;
	struct Matrix4x4;

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

	ROCOCOAPI IStringBuilder
	{
		virtual int AppendFormat(const wchar_t* format, ...) = 0;
		virtual operator const wchar_t* () const = 0;
		virtual void Free() = 0;
	};

	IStringBuilder* CreateSafeStringBuilder(size_t capacity);

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

	ROCOCOAPI IException
	{
		virtual const wchar_t* Message() const = 0;
		virtual int32 ErrorCode() const = 0;
	};

	void Throw(int32 errorCode, const wchar_t* format, ...);
	void ShowErrorBox(IException& ex, const wchar_t* caption);

	template<class T> inline void Free(T* t)
	{
		if (t) t->Free();
	}

	template<class T> class AutoFree
	{
	private:
		T* t;
		AutoFree(const AutoFree& src) = delete;
	public:
		AutoFree(T* _t = nullptr) : t(_t) {}

		AutoFree& operator = (T* src)
		{
			Rococo::Free(t);
			t = src;
			return *this;
		}

		~AutoFree()
		{
			Rococo::Free(t);
		}

		operator T* () { return t; }
		T* operator -> () { return t; }
		T& operator * () { return *t; }
		operator const T* () const { return t; }
		const T* operator -> () const { return t; }
		const T& operator * () const { return *t; }
	};

	template<class T> inline T max(T a, T b) 
	{ 
		return a > b ? a : b;
	}

	template<class T> inline T min(T a, T b)
	{
		return a < b ? a : b; 
	}

	template<class T> struct IEventCallback
	{
		virtual void OnEvent(T& arg) = 0;
	};

	template<> struct IEventCallback<const wchar_t*>
	{
		virtual void OnEvent(const wchar_t* arg) = 0;
	};

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

	enum CALLBACK_CONTROL
	{
		CALLBACK_CONTROL_CONTINUE,
		CALLBACK_CONTROL_BREAK
	};

	struct NULL_CONTEXT {};

	template<class T, class CONTEXT = NULL_CONTEXT> struct ICallback
	{
		virtual CALLBACK_CONTROL operator()(T& value, CONTEXT context) = 0;
	};

	template<class T> struct ICallback<T, NULL_CONTEXT>
	{
		virtual CALLBACK_CONTROL operator()(T& value) = 0;
	};

	template<> struct ICallback<const wchar_t*, NULL_CONTEXT>
	{
		virtual CALLBACK_CONTROL operator()(const wchar_t* value) = 0;
	};

	template<class T> ROCOCOAPI IVectorEnumerator
	{
		virtual T* begin() = 0;
		virtual T* end() = 0;
		virtual const T* begin() const = 0;
		virtual const T* end() const = 0;
		virtual size_t size() const = 0;
	};

	void TripDebugger();

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

	namespace Windows
	{
		struct IWindow;
	}

	namespace Visitors
	{
		struct IUIList;
		struct IUITree;
	}

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
	//    ids 0          to 0x1FFFFFFF are defined in script files and level editors
	//    ids 0x20000000 to 0x20FFFFFF are procedurally generated paths, roads and rivers created in C++
	//    ids 0x21000000 t0 0x21FFFFFF are procedurally generated houses created in C++.
	//    ids 0x40000000 to 0x41000000 are gameplay generated meshes such as explosions created in C++.
	ROCOCO_ID(ID_MESH, int32, 0)

	// ID_SYS_MESH are renderer defined indices that are generated when meshes are loaded into the renderer
	ROCOCO_ID(ID_SYS_MESH, size_t, (size_t) -1)

	struct fstring
	{
		const wchar_t* buffer;
		const int32 length;

		operator const wchar_t*() const { return buffer; }
	};

	uint32 FastHash(const wchar_t* text);

	fstring to_fstring(const wchar_t* const msg);

	inline constexpr fstring operator"" _fstring(const wchar_t* msg, size_t length)
	{
		return fstring{ msg, (int32) length };
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
	}
}

#endif