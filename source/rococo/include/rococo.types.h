#ifndef Rococo_TYPES_H
#define Rococo_TYPES_H

#ifndef ROCOCO_API 
# define ROCOCO_API __declspec(dllimport)
#endif

#ifndef ROCOCO_WINDOWS_API 
# define ROCOCO_WINDOWS_API __declspec(dllimport)
#endif

#ifndef ROCOCO_API_EXPORT 
# define ROCOCO_API_EXPORT __declspec(dllexport)
#endif

#ifndef ROCOCO_API_IMPORT 
# define ROCOCO_API_IMPORT __declspec(dllimport)
#endif

#ifndef ROCOCO_MISC_UTILS_API 
# define ROCOCO_MISC_UTILS_API ROCOCO_API_IMPORT
#endif

#ifndef ROCOCO_UTILS_EX_API
#define ROCOCO_UTILS_EX_API ROCOCO_API_IMPORT
#endif

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
# if USE_VSTUDIO_SAL
#  include <sal.h>
# endif
#endif

#ifdef _WIN32
# define TIGHTLY_PACKED
//# include <sal.h>
#else
# define TIGHTLY_PACKED __attribute__((packed))
# endif

// The following could be done with a template, but that results in bloated error messages, and our ids are ubiquitous
#define ROCOCO_ID(DEFINED_ID_NAME,TYPE,INVALID_VALUE)															\
struct DEFINED_ID_NAME																							\
{																												\
	FORCE_INLINE DEFINED_ID_NAME() : value(INVALID_VALUE) {}												    \
	FORCE_INLINE explicit DEFINED_ID_NAME(TYPE _value) : value(_value) {}										\
	TYPE value;																									\
    FORCE_INLINE [[nodiscard]] static DEFINED_ID_NAME Invalid() noexcept { return DEFINED_ID_NAME(); }			\
	FORCE_INLINE size_t operator() (const DEFINED_ID_NAME& obj) const { return size_t(obj.value); }				\
    FORCE_INLINE operator bool () const noexcept { return value != INVALID_VALUE; }								\
};																												\
inline bool operator == (const DEFINED_ID_NAME& a, const DEFINED_ID_NAME& b) { return a.value == b.value; }		\
inline bool operator != (const DEFINED_ID_NAME& a, const DEFINED_ID_NAME& b) { return !(a == b); }              \
inline bool operator <  (const DEFINED_ID_NAME& a, const DEFINED_ID_NAME& b) { return a.value < b.value; }

#define UNUSED(x) (x);
#define HIDE_COMPILER_WARNINGS(x) (x);

namespace DirectX
{
	struct XMFLOAT4;
	struct XMFLOAT4X4;
}

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

#else
	typedef signed char int8;
	typedef signed short int int16;
	typedef signed int int32;
	typedef signed long long int int64;

	typedef unsigned char uint8;
	typedef unsigned short int uint16;
	typedef unsigned int uint32;
	typedef unsigned long long int uint64;

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
	struct FPSAngles;

	struct IPackage;

	typedef const Vec2& cr_vec2;
	typedef const Vec3& cr_vec3;
	typedef const Vec4& cr_vec4;
	typedef const Matrix4x4& cr_m4x4;

	typedef size_t ID_BYTECODE;
	typedef float MaterialId;

	struct IExpandingBuffer;

	struct fstring
	{
		cstr buffer;
		int32 length;

		FORCE_INLINE constexpr operator cstr() const noexcept { return buffer; }
	};

	template<class TYPE>
	struct SearchResult
	{
		TYPE value;
		boolean32 wasFound;
	};

	template<class T>
	struct Hash
	{
	};

	template <class _Ty>
	struct RemoveReference
	{
		using Type = _Ty;
	};

#ifdef _WIN32
	typedef size_t lsize_t;
#else
	typedef unsigned long long lsize_t;
#endif

	FORCE_INLINE constexpr lsize_t operator "" _megabytes(lsize_t mb)
	{
		return mb * 1024 * 1024;
	}

	FORCE_INLINE constexpr lsize_t operator "" _gigabytes(lsize_t mb)
	{
		return mb * 1024ULL * 1024ULL * 1024ULL;
	}

	FORCE_INLINE constexpr lsize_t operator "" _kilobytes(lsize_t kb)
	{
		return kb * 1024;
	}

	// Maximum fully qualified name length. Names categories include variables a.b.c.d and functions A.B.C.D and methods a.b.c.D
	enum { MAX_FQ_NAME_LEN = 127 };

	enum class EFlowLogic { CONTINUE, BREAK };

	ROCOCO_INTERFACE IFieldEnumerator
	{
		virtual void OnMemberVariable(cstr name, cstr type) = 0;
	};

	namespace OS
	{
		ROCOCO_API bool IsDebugging();
		ROCOCO_API void TripDebugger();
	}

	namespace Script
	{
		struct IPublicScriptSystem;
		struct InterfacePointerToStringBuilder
		{
			void* pSexyInterfacePointer;
		};
	}

	template<class T> struct FilePath
	{
		FilePath() { buf[0] = 0; }
		enum { CAPACITY = 260 };
		T buf[CAPACITY];
		FORCE_INLINE constexpr operator const T* () const noexcept { return buf; }
	};

	typedef FilePath<char32_t> U32FilePath;
	typedef FilePath<wchar_t>  WideFilePath;
	typedef FilePath<char>	   U8FilePath;

	union WindowHandle
	{
		size_t sValue;
		void* pValue;
	};

	namespace Windows
	{
		struct IWindow;
	}

	namespace Strings
	{
		struct IVarArgStringFormatter;
		struct IColourOutputControl;
	}

	struct BoneAngles;

	typedef WindowHandle WindowRef;

	FORCE_INLINE constexpr fstring operator"" _fstring (cstr msg, size_t length) noexcept
	{
		return fstring{ msg, (int32)length };
	}

	template<typename COLOUR_STRUCT>
	ROCOCO_INTERFACE IImagePopulator
	{
		virtual void OnImage(const COLOUR_STRUCT * pixelBuffer, int width, int height) = 0;
	};

	struct IDictionary;

	namespace Strings
	{
		struct StringBuilder;

		// A substring of a larger string. If start == end, length is zero, otherwise start points to the first valid char, and end just after the last valid char.
		struct Substring
		{
			cstr start;
			cstr finish;

			FORCE_INLINE cstr begin() const { return start; }
			FORCE_INLINE cstr end() const { return finish; }

			FORCE_INLINE bool empty() const { return finish <= start; }

			FORCE_INLINE operator bool() const { return !empty(); }
			FORCE_INLINE int64 Length() const { return finish - start; }

			FORCE_INLINE static Substring Null() { return { nullptr,nullptr }; }
		};

		ROCOCO_API Substring ToSubstring(cstr text);

		inline Substring Substring_Null() { return { nullptr,nullptr }; }

		// An immutable substring
		typedef const Substring& cr_substring;

		ROCOCO_INTERFACE IStringPopulator
		{
		   virtual void Populate(cstr text) = 0;
		};

		// Copies the item into the buffer, truncating data if required, and terminating with a nul character
		ROCOCO_API void CopyWithTruncate(cr_substring item, char* buffer, size_t capacity);

		// Duplicates the item as a null terminated string on the stack, then invokes the populator with a reference to the string pointer
		ROCOCO_API void Populate(Strings::cr_substring item, IStringPopulator& populator);

		ROCOCO_API Substring RightOfFirstChar(char c, cr_substring token);
		ROCOCO_API cstr ReverseFind(char c, cr_substring token);
		ROCOCO_API cstr FindChar(cstr token, char c);
	}

	namespace Sexy
	{
		using namespace Rococo::Strings;
		// Type inference API
		// TODO - move functions to their own header
		ROCOCO_MISC_UTILS_API void ForEachFieldOfClassDef(cr_substring className, cr_substring classDef, IFieldEnumerator& cb);
		ROCOCO_MISC_UTILS_API Substring GetClassDefinition(cr_substring className, cr_substring doc);
		ROCOCO_API bool IsSexyKeyword(cr_substring candidate);
		ROCOCO_API bool IsNotTokenChar(char c);
		ROCOCO_API cstr GetFirstNonTokenPointer(cr_substring s);
		ROCOCO_API cstr GetFirstNonTypeCharPointer(cr_substring s);
		ROCOCO_API Substring GetFirstTokenFromLeft(cr_substring s);
		// Given a document and a position to the right of the start of the doc, return first pointer of none type char found, or null if everything was of type until doc.start
		ROCOCO_API cstr GetFirstNonTokenPointerFromRight(cr_substring doc, cstr startPosition);
	}

	struct ILock
	{
		virtual void Lock() = 0;
		virtual void Unlock() = 0;
	};

	namespace Windows
	{
		struct IWindow;
	}

	namespace Compiler
	{
		struct IStructure;
	}

	class Sync
	{
		ILock& lock;
	public:
		FORCE_INLINE Sync(ILock& _lock) : lock(_lock)
		{
			lock.Lock();
		}

		FORCE_INLINE ~Sync()
		{
			lock.Unlock();
		}
	};

	class ThreadLock : public ILock
	{
		int64 implementation[8];
	public:
		ROCOCO_API ThreadLock();
		ROCOCO_API ~ThreadLock();

		ROCOCO_API void Lock();
		ROCOCO_API void Unlock();
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

	typedef uint8 GRAYSCALE;

	namespace Script
	{
		struct IScriptSystemFactory;
	}

	ROCOCO_INTERFACE IException
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
		FORCE_INLINE RecursionGuard(int32& _counter) : counter(_counter) { counter++; }
		FORCE_INLINE ~RecursionGuard() { counter--; }
	};

#if USE_VSTUDIO_SAL
	[[ noreturn ]]
	ROCOCO_API void Throw(int32 errorCode, _Printf_format_string_ cstr format, ...);
#else
	[[ noreturn ]]
	ROCOCO_API void Throw(int32 errorCode, cstr format, ...);
#endif

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
		mutable T* t;
	public:
		FORCE_INLINE AutoFree(T* _t = nullptr) : t(_t) {}
		FORCE_INLINE AutoFree(AutoFree<T>&& src)
		{
			t = src.t;
			src.t = nullptr;
		}

		FORCE_INLINE AutoFree<T>& operator = (T* src)
		{
			if (t != nullptr && t != src)
			{
				Rococo::Free(t);
			}
			t = src;
			return *this;
		};

		FORCE_INLINE AutoFree<T>& operator = (const AutoFree<T>& src)
		{
			Rococo::Free(t);
			t = src.t;
			src.t = nullptr;
			return *this;
		}

		FORCE_INLINE AutoFree<T>& operator = (const AutoFree<T>&& src)
		{
			Rococo::Free(t);
			t = src.t;
			src.t = nullptr;
		}

		FORCE_INLINE ~AutoFree()
		{
			Rococo::Free(t);
		}

		// Release our hold on the pointer, but does not free it. Then returns it
		FORCE_INLINE T* Release()
		{
			T* output = t;
			t = nullptr;
			return output;
		}

		FORCE_INLINE operator T* () { return t; }
		FORCE_INLINE T* operator -> () { return t; }
		FORCE_INLINE T& operator * () { return *t; }
		FORCE_INLINE operator const T* () const { return t; }
		FORCE_INLINE const T* operator -> () const { return t; }
		FORCE_INLINE const T& operator * () const { return *t; }
		FORCE_INLINE operator bool () const { return t != nullptr; }
	};

	template<class T> ROCOCO_INTERFACE IEnumerator
	{
		virtual void operator()(const T& t) = 0;
	};

	template<> ROCOCO_INTERFACE IEnumerator<cstr>
	{
		virtual void operator()(cstr t) = 0;
	};

	template<class T> ROCOCO_INTERFACE IEnumerable
	{
		[[nodiscard]] virtual const T& operator[](size_t index) = 0;
		[[nodiscard]] virtual size_t Count() const = 0;
		virtual void Enumerate(IEnumerator<T>& cb) = 0;
	};

	template<> ROCOCO_INTERFACE IEnumerable<cstr>
	{
		[[nodiscard]] virtual cstr operator[](size_t index) = 0;
		[[nodiscard]] virtual size_t Count() const = 0;
		virtual void Enumerate(IEnumerator<cstr>& cb) = 0;
	};

	template<class T> ROCOCO_INTERFACE IMutableEnumerator
	{
		virtual void operator()(T& t) = 0;
	};

	struct RGBAb
	{
		uint8 red = 0;
		uint8 green = 0;
		uint8 blue = 0;
		uint8 alpha = 0;

		RGBAb() {}
		FORCE_INLINE RGBAb(uint32 x) { RGBAb* pCol = (RGBAb*)&x; *this = *pCol; }
		FORCE_INLINE RGBAb(uint8 _red, uint8 _green, uint8 _blue, uint8 _alpha = 255) : red(_red), green(_green), blue(_blue), alpha(_alpha) {}
	};

	struct RGBA
	{
		float red;
		float green;
		float blue;
		float alpha;

		FORCE_INLINE RGBA(float _r = 1.0f, float _g = 0.0f, float _b = 0.0f, float _a = 1.0f) : red(_r), green(_g), blue(_b), alpha(_a) {}
	};

	template<class T> ROCOCO_INTERFACE IVectorEnumerator
	{
		[[nodiscard]] virtual T* begin() = 0;
		[[nodiscard]] virtual T* end() = 0;
		[[nodiscard]] virtual const T* begin() const = 0;
		[[nodiscard]] virtual const T* end() const = 0;
		[[nodiscard]] virtual size_t size() const = 0;
	};

	// Represent a gui rectangle in floating point co-ordinates. top < bottom for most uses.
	struct GuiRectf
	{
		float left = 0;
		float top = 0;
		float right = 0;
		float bottom = 0;

		GuiRectf() {}
		FORCE_INLINE GuiRectf(float _left, float _top, float _right, float _bottom) : left(_left), top(_top), right(_right), bottom(_bottom) {}
		FORCE_INLINE bool IsNormalized() const { return right > left && bottom > top; }
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

	template<class T> [[nodiscard]] FORCE_INLINE T max(T a, T b)
	{
		return a > b ? a : b;
	}

	template<class T> [[nodiscard]] FORCE_INLINE T min(T a, T b)
	{
		return a < b ? a : b;
	}

	template<class T> [[nodiscard]] FORCE_INLINE T clamp(T a, T lowestBound, T highestBound)
	{
		return min(highestBound, max(lowestBound, a));
	}

	template<class T> [[nodiscard]] FORCE_INLINE T Sq(T a)
	{
		return a * a;
	}

	struct Kilograms
	{
		float value;
		FORCE_INLINE operator float() const { return value; }
	};

	FORCE_INLINE inline Kilograms operator "" _kg(long double value)
	{
		return Kilograms{ (float)value };
	}

	struct Metres
	{
		float value;
		FORCE_INLINE operator float() const { return value; }
	};

	FORCE_INLINE Metres operator "" _metres(long double value)
	{
		return Metres{ (float)value };
	}


	FORCE_INLINE Metres operator "" _metres(unsigned long long value)
	{
		return Metres{ (float)value };
	}

	struct Seconds
	{
		float value;
		FORCE_INLINE operator float() const { return value; }
	};

	FORCE_INLINE Seconds operator "" _seconds(long double value)
	{
		return Seconds{ (float)value };
	}

	struct MetresPerSecond
	{
		float value;
		FORCE_INLINE operator float() const { return value; }
	};

	FORCE_INLINE MetresPerSecond operator "" _mps(long double value)
	{
		return MetresPerSecond{ (float)value };
	}

	struct GuiRect
	{
		int32 left = 0;
		int32 top = 0;
		int32 right = 0;
		int32 bottom = 0;

		GuiRect() {}
		FORCE_INLINE GuiRect(int32 _left, int32 _top, int32 _right, int32 _bottom) : left(_left), top(_top), right(_right), bottom(_bottom) {}

		FORCE_INLINE bool IsNormalized() const { return right > left && bottom > top; }
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

		FORCE_INLINE static Vec3 FromVec2(const Vec2& pos, float z)
		{
			return Vec3{ pos.x, pos.y, z };
		}
	};

	struct alignas(4) Quat
	{
		Vec3 v;
		float s;

		Quat() : v{ 0,0,0 }, s(1.0f) {}
		Quat(cr_vec3 _v, float _s) : v(_v), s(_s) {}

		FORCE_INLINE operator DirectX::XMFLOAT4* () { return reinterpret_cast<DirectX::XMFLOAT4*> (this); }
		FORCE_INLINE operator const DirectX::XMFLOAT4* () const { return reinterpret_cast<const DirectX::XMFLOAT4*> (this); }
	};

	typedef const Quat& cr_quat;

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

		FORCE_INLINE const Vec3* First() const { return &bottom.nw; }
	};

	/* Axis-Aligned Bounding Box */
	struct AABB
	{
		Vec3 minXYZ;
		Vec3 maxXYZ;

		AABB();
		void Empty();

		AABB& operator << (cr_vec3 p);
		[[nodiscard]] bool HoldsPoint(cr_vec3 p) const;
		[[nodiscard]] bool Intersects(const AABB& other) const;

		[[nodiscard]] Vec3 Centre() const;
		void GetBox(BoundingBox& box) const;
		[[nodiscard]] Vec3 Span() const;

		[[nodiscard]] AABB RotateBounds(const Matrix4x4& Rz) const;
	};

	FORCE_INLINE  [[nodiscard]] inline const Vec2& Flatten(const Vec3& a)
	{
		return *reinterpret_cast<const Vec2*>(&a);
	}

	ROCOCO_INTERFACE IAllocator
	{
	   [[nodiscard]] virtual void* Allocate(size_t capacity) = 0;
	   virtual void FreeData(void* data) = 0;
	   virtual void* Reallocate(void* ptr, size_t capacity) = 0;
	};

	ROCOCO_INTERFACE IAllocatorSupervisor : public IAllocator
	{
		 virtual void Free() = 0;
	};

	ROCOCO_INTERFACE IHeapObject
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
			BreakFlag_IllFormed_SExpression = 8,
			BreakFlag_All = 0x7FFFFFFF
		};
	}

	namespace Time
	{
		typedef int64 ticks;
	}

	template<class T, class U> [[nodiscard]] constexpr bool IsFlagged(T flags, U flag) { return (flags & flag) != 0; }

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

namespace Rococo
{
	template<int OPTIMAL_SIZE, class RETURN_TYPE, typename ... ARGS>
	class ArbitraryFunction;
}

namespace Rococo
{
	template<class U, class V> struct tuple
	{
		U first;
		V second;
	};

	template<typename T, typename U> [[nodiscard]] inline bool HasFlag(T flag, U flags)
	{
		return ((int)flags & (int)flag) != 0;
	}
}

namespace Rococo::IO
{
	struct IInstallation;
}

#endif