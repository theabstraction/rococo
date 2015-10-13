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

	struct Vec4
	{
		float x;
		float y;
		float z;
		float w;
	};

	struct Matrix4x4
	{
		Vec4 row0;
		Vec4 row1;
		Vec4 row2;
		Vec4 row3;
	};

	struct Vec2i
	{
		int32 x;
		int32 y;
	};

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

	struct Degrees
	{
		float quantity;
		operator float() const { return quantity; }
	};

	struct Radians
	{
		float quantity;
		operator float() const { return quantity; }
	};

	struct NO_VTABLE IException
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
			if (t) t->Free();
			t = src;
			return *this;
		}

		~AutoFree()
		{
			Free(t);
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

	template<class T> struct NO_VTABLE IVectorEnumerator
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

		RGBAb(uint32 x) { RGBAb* pCol = (RGBAb*)&x; *this = *pCol; }
		RGBAb(uint8 _red, uint8 _green, uint8 _blue, uint8 _alpha = 255) : red(_red), green(_green), blue(_blue), alpha(_alpha) {}
	};

	struct IRenderer;
	struct IInstallation;
	struct IOS;
	struct IRenderContext;
	struct IBuffer;

	namespace Windows
	{
		struct IWindow;
	}

	namespace Visitors
	{
		struct IUIList;
		struct IUITree;
	}
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