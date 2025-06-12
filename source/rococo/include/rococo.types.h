// For copyright, see copyright.txt in the Rococo root folder. All files in the Rococo repo that include this header are covered by it.

#ifndef Rococo_TYPES_H
#define Rococo_TYPES_H

#ifndef ROCOCO_API
# error "define ROCOCO_API"
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

// This is a new definition. Long term goal is to apply all forward declarations of ROCOCO_INTERFACES with this prefix.
#ifndef DECLARE_ROCOCO_INTERFACE
#define DECLARE_ROCOCO_INTERFACE struct
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
# ifdef USE_VSTUDIO_SAL
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
    [[nodiscard]] FORCE_INLINE static DEFINED_ID_NAME Invalid() noexcept { return DEFINED_ID_NAME(); }			\
	FORCE_INLINE size_t operator() (const DEFINED_ID_NAME& obj) const { return size_t(obj.value); }				\
    FORCE_INLINE operator bool () const noexcept { return value != INVALID_VALUE; }								\
};																												\
inline bool operator == (const DEFINED_ID_NAME& a, const DEFINED_ID_NAME& b) { return a.value == b.value; }		\
inline bool operator != (const DEFINED_ID_NAME& a, const DEFINED_ID_NAME& b) { return !(a == b); }              \
inline bool operator <  (const DEFINED_ID_NAME& a, const DEFINED_ID_NAME& b) { return a.value < b.value; }

#define UNUSED(x) (void)(x);
#define HIDE_COMPILER_WARNINGS(x)  (void)(x);

namespace DirectX
{
	struct XMFLOAT4;
	struct XMFLOAT4X4;
}

#ifndef __ROCOCO_WIDECHAR__
#error - define __ROCOCO_WIDECHAR__ in the compiler environment
#endif

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

	// Some bytecode identifier. In some contexts, such as virtual tables for interfaces, they encode a function addresses
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

	typedef __ROCOCO_WIDECHAR__ ROCOCO_WIDECHAR;
	typedef const ROCOCO_WIDECHAR* crwstr;

#ifdef __FUNCTION__
# define __ROCOCO_FUNCTION__ __FUNCTION__
#else
# define __ROCOCO_FUNCTION__ __func__
#endif

	enum class ErrorCode : int
	{
		None = 0
	};

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

	DECLARE_ROCOCO_INTERFACE IPackage;

	typedef const Vec2& cr_vec2;
	typedef const Vec3& cr_vec3;
	typedef const Vec4& cr_vec4;
	typedef const Matrix4x4& cr_m4x4;

	typedef size_t ID_BYTECODE;

	DECLARE_ROCOCO_INTERFACE IExpandingBuffer;

	struct fstring
	{
		const char* buffer;
		int32 length;

		FORCE_INLINE constexpr operator const char* () const noexcept { return buffer; }
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

	template<int OPTIMAL_SIZE, typename TYPENAME, typename ... ARGS>
	class ArbitraryFunction;

	template<typename RETURNTYPE, typename ... ARGS>
	using Function = ArbitraryFunction<64, RETURNTYPE, ARGS ...>;

	enum class Limits: size_t { FSTRING_LENGTH_LIMIT = 0x020000000LL };

	// Maximum fully qualified name length. Names categories include variables a.b.c.d and functions A.B.C.D and methods a.b.c.D
	enum { MAX_FQ_NAME_LEN = 127 };

	enum class EFlowLogic { CONTINUE, BREAK };

	namespace OS
	{
		ROCOCO_API bool IsDebugging();
		ROCOCO_API void TripDebugger();
	}

	namespace IO
	{
		namespace VirtualKeys
		{
			enum VKCode : int;
		}
	}

	namespace Script
	{
		DECLARE_ROCOCO_INTERFACE IPublicScriptSystem;
		struct InterfacePointerToStringBuilder
		{
			void* pSexyInterfacePointer;
		};
		struct ISxyExpressionRef
		{
			void* internal;
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
	typedef FilePath<ROCOCO_WIDECHAR>  WideFilePath;
	typedef FilePath<char>	   U8FilePath;

	union WindowHandle
	{
		size_t sValue;
		void* pValue;
	};

	namespace Events
	{
		class IPublisher;
	}

	namespace Windows
	{
		DECLARE_ROCOCO_INTERFACE IWindow;
	}

	namespace Strings
	{
		DECLARE_ROCOCO_INTERFACE IVarArgStringFormatter;
		DECLARE_ROCOCO_INTERFACE IColourOutputControl;
	}

	struct BoneAngles;

	typedef WindowHandle WindowRef;

	FORCE_INLINE constexpr fstring operator"" _fstring (const char* msg, size_t length) noexcept
	{
		return fstring{ msg, (int32)length };
	}

	struct IDictionary;

	namespace UI
	{
		// The system dependent widget id. On Windows each specifies the control id for child window
		struct SysWidgetId
		{
			uint16 value;
		};
	}

	namespace Strings
	{
		DECLARE_ROCOCO_INTERFACE StringBuilder;

		// A substring of a larger string. If start < end start points to the first valid char, and end just after the last valid char, otherwise the string is empty
		struct Substring
		{
			const char* start;
			const char* finish;

			[[nodiscard]] FORCE_INLINE const char* begin() const { return start; }
			[[nodiscard]] FORCE_INLINE const char* end() const { return finish; }

			[[nodiscard]] FORCE_INLINE bool empty() const { return finish <= start; }

			[[nodiscard]] FORCE_INLINE operator bool() const { return !empty(); }
			[[nodiscard]] FORCE_INLINE int64 Length() const { return finish - start; }

			[[nodiscard]] FORCE_INLINE static Substring Null() { return { nullptr,nullptr }; }

			[[nodiscard]] ROCOCO_API static Substring ToSubstring(const char* text);

			// Copies the item into the buffer, truncating data if required, and terminating with a nul character
			ROCOCO_API void CopyWithTruncate(char* buffer, size_t capacity) const;

			// Try to copy everything to the string buffer, including a terminating null. If the buffer is not large enough the method returns false and the buffer supplied is unchanged
			[[nodiscard]] ROCOCO_API bool TryCopyWithoutTruncate(char* name, size_t sizeofName) const;
		};

		// An immutable substring
		typedef const Substring& cr_substring;

		// An interface that allows a method to populate a buffer from a cstr pointer without having to return the pointer. Rather it calls the populator's populate method
		// The aim is to eliminate the need for temporary heap allocation of string data. 
		// Imagine a function 'const char* ToString(Int32 i)' that converts integers to strings. A function that returns a cstr would need to populate a temporary buffer, and return a reference to it, creating lifetime issues.
		// Instead we would write 'void ToString(int i, IStringPopulator& populator)'. The internal buffer would be removed from the stack before the function returns, eliminating lifetime issue.
		ROCOCO_INTERFACE IStringPopulator
		{
		   virtual void Populate(const char* text) = 0;
		};
	}

	struct ILock
	{
		virtual void Lock() = 0;
		virtual void Unlock() = 0;
	};

	namespace Windows
	{
		DECLARE_ROCOCO_INTERFACE IWindow;
	}

	namespace Compiler
	{
		DECLARE_ROCOCO_INTERFACE IStructure;
	}

	struct Vec2i
	{
		int32 x;
		int32 y;
	};

	namespace Debugging
	{
		DECLARE_ROCOCO_INTERFACE IStackFrameEnumerator;
	}

	namespace Reflection
	{
		DECLARE_ROCOCO_INTERFACE IReflectionTarget;
		DECLARE_ROCOCO_INTERFACE IReflectionVisitor;
		DECLARE_ROCOCO_INTERFACE IReflectionVisitation;
	}

	struct ISubsystemMonitor;

	template<typename... Args>
	ROCOCO_INTERFACE IEventImpressario
	{
		virtual void Add(Rococo::Function<void(Args... )> handler) = 0;
	};

	typedef uint8 GRAYSCALE;

	namespace Script
	{
		DECLARE_ROCOCO_INTERFACE IScriptSystemFactory;
	}

	namespace Sex
	{
		DECLARE_ROCOCO_INTERFACE ISExpression;
	}

	namespace Strings
	{
		class HString;
	}

	struct IScriptEnumerator;
	struct ScriptCompileArgs;

	ROCOCO_INTERFACE IScriptCompilationEventHandler
	{
		virtual void OnCompile(ScriptCompileArgs& args) = 0;
		virtual IScriptEnumerator* ImplicitIncludes() = 0;
	};

	ROCOCO_INTERFACE IException
	{
		virtual const char* Message() const = 0;
		virtual int32 ErrorCode() const = 0;
		virtual Debugging::IStackFrameEnumerator* StackFrames() = 0;
	};

#ifdef USE_VSTUDIO_SAL
	[[noreturn]] ROCOCO_API void Throw(int32 errorCode, _Printf_format_string_ const char* format, ...);

	namespace Sex
	{
		[[noreturn]] ROCOCO_API void Throw(const Sex::ISExpression& e, _Printf_format_string_ cstr format, ...);
	}
#else
	[[noreturn]] ROCOCO_API void Throw(int32 errorCode, const char* format, ...);

	namespace Sex
	{
		[[noreturn]] ROCOCO_API void Throw(const Sex::ISExpression& e, const char* format, ...);
	}
#endif

	template<class T> struct IEventCallback
	{
		virtual void OnEvent(T& arg) = 0;
	};

	template<> struct IEventCallback<const wchar_t*>
	{
		virtual void OnEvent(const wchar_t* arg) = 0;
	};

	template<class T>
	ROCOCO_INTERFACE IDesignator
	{
		virtual void Designate(T* t) = 0;
	};

	template<class T>
	struct Deallocator
	{
		void operator()(T* t)
		{
			if (t) t->Free();
		}
	};

	template<class T>
	void Free(T* t)
	{
		if (t) t->Free();
	}

	template<class T, class DEALLOCATOR = Deallocator<T>> class AutoFree
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
				DEALLOCATOR d;
				d(t);
			}
			t = src;
			return *this;
		};

		FORCE_INLINE AutoFree<T>& operator = (const AutoFree<T>& src)
		{
			DEALLOCATOR d;
			d(t);
			t = src.t;
			src.t = nullptr;
			return *this;
		}

		FORCE_INLINE AutoFree<T>& operator = (const AutoFree<T>&& src)
		{
			DEALLOCATOR d;
			d(t);
			t = src.t;
			src.t = nullptr;
		}

		FORCE_INLINE ~AutoFree()
		{
			DEALLOCATOR d;
			d(t);
		}

		// Release our hold on the pointer, but does not free it. Then returns it
		FORCE_INLINE T* Detach()
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

	template<> ROCOCO_INTERFACE IEnumerator<const char*>
	{
		virtual void operator()(const char* t) = 0;
	};

	template<class T> ROCOCO_INTERFACE IEnumerable
	{
		[[nodiscard]] virtual const T& operator[](size_t index) = 0;
		[[nodiscard]] virtual size_t Count() const = 0;
		virtual void Enumerate(IEnumerator<T>& cb) = 0;
	};

	template<class T> ROCOCO_INTERFACE IValueTypeVectorReader
	{
		[[nodiscard]] virtual T operator[](size_t index) const = 0;
		[[nodiscard]] virtual size_t Count() const = 0;
	};


	template<> ROCOCO_INTERFACE IEnumerable<const char*>
	{
		[[nodiscard]] virtual const char* operator[](size_t index) = 0;
		[[nodiscard]] virtual size_t Count() const = 0;
		virtual void Enumerate(IEnumerator<const char*>& cb) = 0;
	};

	template<class T> ROCOCO_INTERFACE IMutableEnumerator
	{
		virtual void operator()(T& t) = 0;
	};

	struct RGBb
	{
		uint8 red;
		uint8 green;
		uint8 blue;
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

	template<class T> FORCE_INLINE void swap_args(T& a, T& b)
	{
		T c = a;
		a = b;
		b = c;
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

	typedef const char* cstr;

	struct Vec2
	{
		float x;
		float y;
	};

	struct Vec2F64
	{
		double x;
		double y;
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

		[[nodiscard]] FORCE_INLINE const Vec2& Flatten() const
		{
			return *reinterpret_cast<const Vec2*>(this);
		}
	};

	struct alignas(4) Vec4 // was 16, but sexy does not align on 16 byte boundaries yet
	{
		float x;
		float y;
		float z;
		float w;

		inline static Vec4 FromVec3(cr_vec3& v, float w)
		{
			return{ v.x, v.y, v.z, w };
		}

		inline operator DirectX::XMFLOAT4* () { return reinterpret_cast<DirectX::XMFLOAT4*> (this); }
		inline operator const DirectX::XMFLOAT4* () const { return reinterpret_cast<const DirectX::XMFLOAT4*> (this); }

		inline operator const Vec3& () const { return *reinterpret_cast<const Vec3*> (this); }
		inline operator Vec3& () { return *reinterpret_cast<Vec3*> (this); }
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

	struct Sphere
	{
		Vec3 centre;
		float radius;
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

		[[nodiscard]] AABB RotateBounds(const Matrix4x4& m) const;
	};

	ROCOCO_INTERFACE IAllocator
	{
		typedef void (*FN_AllocatorReleaseFunction)();

		// Throws a std::bad_alloc on failure for all implementations
		[[nodiscard]] virtual void* Allocate(size_t capacity) = 0;
		virtual void FreeData(void* data) = 0;
		virtual void* Reallocate(void* ptr, size_t capacity) = 0;

		// Add a function to be invoked when the allocator is about to be destructed
		virtual void AtRelease(FN_AllocatorReleaseFunction fn) = 0;

		// Returns the number of bytes in the allocator heap, or 0 if not known
		virtual size_t EvaluateHeapSize() = 0;
	};

	ROCOCO_INTERFACE IAllocatorSupervisor : public IAllocator
	{
		 virtual void Free() = 0;
	};

	ROCOCO_INTERFACE IHeapObject
	{
	   virtual void Free() = 0;
	};

	ROCOCO_ID(ID_SUBSYSTEM, int32, 0);

	namespace OS
	{
		namespace Flags
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

namespace Rococo::Validators
{
	enum class EValidationPurpose;

	template<class VALUE_TYPE>
	struct IValueValidator;

	template<class VALUE_TYPE>
	struct IValueFormatter;
}

#endif

#ifndef _WIN32
int _i64toa_s(int64 value, char* buffer, size_t capacity, int radix);
int _itoa_s(int value, char* buffer, size_t capacity, int radix);
int _ui64toa_s(int64 value, char* buffer, size_t capacity, int radix);
#endif
