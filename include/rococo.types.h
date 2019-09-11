#ifndef Rococo_TYPES_H
#define Rococo_TYPES_H

#include <rococo.compiler.options.h>

#ifndef _WIN32
# include <stddef.h>
#endif

#ifndef _PTRDIFF_T_DEFINED
# ifdef  _WIN64
typedef __int64 ptrdiff_t;
# endif
# define _PTRDIFF_T_DEFINED
#endif

#ifdef _WIN32
# define TIGHTLY_PACKED
#else
# define TIGHTLY_PACKED __attribute__((packed))
# endif

namespace Rococo
{
#ifdef _WIN32
	typedef __int8 int8;
	typedef __int16 int16;
	typedef __int32 int32;
	typedef __int64 int64;

	typedef unsigned __int8 uint8;
	typedef unsigned __int16 uint16;
	typedef unsigned __int32 uint32;
	typedef unsigned __int64 uint64;

	typedef size_t ID_BYTECODE;

#define ROCOCO_CONSTEXPR constexpr

#else
	typedef signed char int8;
	typedef signed short int int16;
	typedef signed int int32;
	typedef signed long long int int64;

	typedef unsigned char uint8;
	typedef unsigned short int uint16;
	typedef unsigned int uint32;
	typedef unsigned long long int uint64;

#define ROCOCO_CONSTEXPR

#endif

	typedef const char* cstr;

	typedef float float32;
	typedef double float64;
	typedef void* pointer;
	typedef int32 boolean32; // 32-bit boolean

	struct Vec2;
	struct Vec3;
	struct Vec4;
	struct Matrix4x4;
	struct Radians;
	struct Degrees;
	struct Quat;
	struct Sphere;
	struct Gravity;
	struct Metres;

	typedef const Vec2& cr_vec2;
	typedef const Vec3& cr_vec3;
	typedef const Vec4& cr_vec4;
	typedef const Matrix4x4& cr_m4x4;

	typedef size_t ID_BYTECODE;

	struct fstring
	{
		cstr buffer;
		int32 length;

		operator cstr() const { return buffer; }
	};

	union WindowHandle
	{
		size_t sValue;
		void* pValue;
	};

	typedef WindowHandle WindowRef;

	ROCOCOAPI IStringPopulator
	{
	   virtual void Populate(cstr text) = 0;
	};

	inline ROCOCO_CONSTEXPR fstring operator"" _fstring(cstr msg, size_t length)
	{
		return fstring{ msg, (int32)length };
	}

	struct IDictionary;
	struct StringBuilder;

	struct ILock
	{
		virtual void Lock() = 0;
		virtual void Unlock() = 0;
	};

	namespace Windows
	{
		struct IWindow;
	}

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

	class ThreadLock : public ILock
	{
		int64 implementation[8];
	public:
		ThreadLock();
		~ThreadLock();

		void Lock();
		void Unlock();
	};

	struct Vec2i
	{
		int32 x;
		int32 y;
	};

	namespace Debugging
	{
		struct IStackFrameEnumerator;
	}

	namespace Script
	{
		struct IScriptSystemFactory;
	}

	ROCOCOAPI IException
	{
		virtual cstr Message() const = 0;
		virtual int32 ErrorCode() const = 0;
		virtual Debugging::IStackFrameEnumerator* StackFrames() = 0;
	};

	class RecursionGuard
	{
	private:
		int32& counter;
	public:
		RecursionGuard(int32& _counter) : counter(_counter) { counter++; }
		~RecursionGuard() { counter--; }
	};

	void Throw(int32 errorCode, cstr format, ...);

	template<class T> struct IEventCallback
	{
		virtual void OnEvent(T& arg) = 0;
	};

	template<> struct IEventCallback<cstr>
	{
		virtual void OnEvent(cstr arg) = 0;
	};

	template<> struct IEventCallback<const wchar_t*>
	{
		virtual void OnEvent(const wchar_t* arg) = 0;
	};

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
			if (src != t)
			{
				Rococo::Free(t);
				t = src;
			}
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
		operator bool () const { return t != nullptr; }
	};

	template<class T> ROCOCOAPI IEnumerator
	{
		virtual void operator()(const T& t) = 0;
	};

	template<> ROCOCOAPI IEnumerator<cstr>
	{
		virtual void operator()(cstr t) = 0;
	};

	template<class T> ROCOCOAPI IEnumerable
	{
		virtual const T& operator[](size_t index) = 0;
		virtual size_t Count() const = 0;
		virtual void Enumerate(IEnumerator<T>& cb) = 0;
	};

	template<> ROCOCOAPI IEnumerable<cstr>
	{
		virtual cstr operator[](size_t index) = 0;
		virtual size_t Count() const = 0;
		virtual void Enumerate(IEnumerator<cstr>& cb) = 0;
	};

	template<class T> ROCOCOAPI IMutableEnumerator
	{
		virtual void operator()(T& t) = 0;
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

		RGBA(float _r = 1.0f, float _g = 0.0f, float _b = 0.0f, float _a = 1.0f) : red(_r), green(_g), blue(_b), alpha(_a) {}
	};

	template<class T> ROCOCOAPI IVectorEnumerator
	{
	   virtual T* begin() = 0;
	   virtual T* end() = 0;
	   virtual const T* begin() const = 0;
	   virtual const T* end() const = 0;
	   virtual size_t size() const = 0;
	};

	// Represent a gui rectangle in floating point co-ordinates. top < bottom for most uses.
	struct GuiRectf
	{
		float left;
		float top;
		float right;
		float bottom;

		GuiRectf() {}
		GuiRectf(float _left, float _top, float _right, float _bottom) : left(_left), top(_top), right(_right), bottom(_bottom) {}
	};

	struct AABB2d
	{
		float left;
		float bottom;
		float right;
		float top;

		AABB2d();
		void Empty();
		bool HoldsPoint(cr_vec2 p) const;
		Vec2 Span() const;
		Vec2 Centre() const;

		AABB2d& operator << (cr_vec2 p);
	};

	template<class T> inline T max(T a, T b)
	{
		return a > b ? a : b;
	}

	template<class T> inline T min(T a, T b)
	{
		return a < b ? a : b;
	}

	inline float clamp(float a, float lowestBound, float highestBound)
	{
		return min(highestBound, max(lowestBound, a));
	}

	inline int32 clamp(int32 a, int32 lowestBound, int32 highestBound)
	{
		return min(highestBound, max(lowestBound, a));
	}

	template<class T> inline T Sq(T a)
	{
		return a * a;
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


	inline Metres operator "" _metres(unsigned long long value)
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

	struct GuiRect
	{
		int32 left;
		int32 top;
		int32 right;
		int32 bottom;

		GuiRect() {}
		GuiRect(int32 _left, int32 _top, int32 _right, int32 _bottom) : left(_left), top(_top), right(_right), bottom(_bottom) {}
	};

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

	struct BoundingBox
	{
		struct Layer
		{
			Vec3 nw;
			Vec3 ne;
			Vec3 se;
			Vec3 sw;
		};

		Layer bottom;
		Layer top;

		const Vec3* First() const { return &bottom.nw; }
	};

	/* Axis-Aligned Bounding Box */
	struct AABB
	{
		Vec3 minXYZ;
		Vec3 maxXYZ;

		AABB();
		void Empty();

		AABB& operator << (cr_vec3 p);
		bool HoldsPoint(cr_vec3 p) const;
		bool Intersects(const AABB& other) const;

		Vec3 Centre() const;
		void GetBox(BoundingBox& box) const;
		Vec3 Span() const;

		AABB RotateBounds(const Matrix4x4& Rz) const;
	};

	inline const Vec2& Flatten(const Vec3& a)
	{
		return *reinterpret_cast<const Vec2*>(&a);
	}

	ROCOCOAPI IAllocator
	{
	   virtual void* Allocate(size_t capacity) = 0;
	   virtual void FreeData(void* data) = 0;
	   virtual void* Reallocate(void* ptr, size_t capacity) = 0;
	};

	ROCOCOAPI IAllocatorSupervisor : public IAllocator
	{
		 virtual void Free() = 0;
	};

	ROCOCOAPI IHeapObject
	{
	   virtual void Free() = 0;
	};

	namespace OS
	{
		enum BreakFlag : int32
		{
			BreakFlag_None = 0,
			BreakFlag_STC = 1,
			BreakFlag_VM = 2,
			BreakFlag_SS = 4,
			BreakFlag_All = 0x7FFFFFFF
		};

		typedef int64 ticks;
	}

	template<class T, class U> bool IsFlagged(T flags, U flag) { return (flags & flag) != 0; }

#if !defined(_W64)
# if !defined(__midl) && (defined(_X86_) || defined(_M_IX86))
#  define _W64 __w64
# else
#  define _W64
# endif
#endif

#define IN
#define OUT
#define REF

#ifndef _WIN64
# ifdef _WIN32
#error "Sexy does not supports anything other than 64-bit platforms." 
# endif
#endif

#define POINTERS_ARE_64_BIT

#ifndef _WIN32
	typedef int32 errno_t;
	void memcpy_s(void *dest, size_t destSize, const void *src, size_t count);

	enum { _MAX_PATH = 260 };
#endif

}

#endif