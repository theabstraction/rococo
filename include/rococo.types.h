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

   typedef char rchar; // the Rococo character type
   typedef const rchar* cstr;

	typedef float float32;
	typedef double float64;
	typedef void* pointer;
	typedef int32 boolean32; // 32-bit boolean

   size_t rlen(cstr s);
   int StrCmpN(cstr a, cstr b, size_t len);

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

   ROCOCOAPI IStringPopulator
   {
      virtual void Populate(cstr text) = 0;
   };

   inline ROCOCO_CONSTEXPR fstring operator"" _fstring(cstr msg, size_t length)
   {
      return fstring{ msg, (int32)length };
   }

   struct StringBuilder
   {
      virtual StringBuilder& AppendFormat(const char* format, ...) = 0;
      virtual StringBuilder& operator << (cstr text) = 0;
      virtual StringBuilder& operator << (int32 value) = 0;
      virtual StringBuilder& operator << (uint32 value) = 0;
      virtual StringBuilder& operator << (int64 value) = 0;
      virtual StringBuilder& operator << (uint64 value) = 0;
      virtual StringBuilder& operator << (float value) = 0;
      virtual StringBuilder& operator << (double value) = 0;
      virtual fstring operator * () const = 0;
      virtual void Clear() = 0;
      virtual int32 Length() const = 0;

      enum eOpenType { BUILD_EXISTING = 0 };
   };

   bool IsPointerValid(const void* ptr);

   class StackStringBuilder : public StringBuilder
   {
   private:
      char* buffer;
      size_t capacity;
      int32 length;
   public:
      StackStringBuilder(char* _buffer, size_t _capacity);
      StackStringBuilder(char* _buffer, size_t _capacity, eOpenType type);
      fstring operator * () const override { return fstring{ buffer, length }; }
      StringBuilder& AppendFormat(const char* format, ...) override;
      StringBuilder& operator << (cstr text) override;
      StringBuilder& operator << (int32 value)  override;
      StringBuilder& operator << (uint32 value) override;
      StringBuilder& operator << (int64 value)  override;
      StringBuilder& operator << (uint64 value) override;
      StringBuilder& operator << (float value) override;
      StringBuilder& operator << (double value) override;
      void Clear() override;
      int32 Length() const override;
   };

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

	ROCOCOAPI IException
	{
		virtual cstr Message() const = 0;
		virtual int32 ErrorCode() const = 0;
	};

   class RecursionGuard
   {
   private:
      int32& counter;
   public:
      RecursionGuard(int32& _counter): counter(_counter) { counter++; }
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
   };

   AABB2d EmptyAABB2dBox();

   template<class T> inline T max(T a, T b)
   {
      return a > b ? a : b;
   }

   template<class T> inline T min(T a, T b)
   {
      return a < b ? a : b;
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

   void ExpandZoneToContain(GuiRect& rect, const Vec2i& p);
   void ExpandZoneToContain(GuiRectf& rect, const Vec2& p);

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

   ROCOCOAPI IAllocatorSupervisor: public IAllocator
   {
		virtual void Free() = 0;
   };

   ROCOCOAPI IHeapObject
   {
      virtual void Free() = 0;
   };

   namespace Memory
   {
      IAllocator& CheckedAllocator();
      IAllocatorSupervisor* CreateBlockAllocator(size_t kilobytes, size_t maxkilobytes);
   }

   void LoadAsciiTextFile(char* data, size_t capacity, const char* filename);

   namespace OS
   {
      enum BreakFlag : int32;

      typedef int64 ticks;
      void PrintDebug(const char* format, ...);
      void TripDebugger();
      void ShowErrorBox(Windows::IWindow& parent, IException& ex, cstr caption);
      bool IsDebugging();
      void BreakOnThrow(BreakFlag flag);
      ticks CpuTicks();
      ticks CpuHz();
	  ticks UTCTime();
	  void FormatTime(ticks utcTime, char* buffer, size_t nBytes);
      bool StripLastSubpath(rchar* fullpath);
      bool IsFileExistant(cstr path);
      void Format_C_Error(int errorCode, rchar* buffer, size_t capacity);
      int OpenForAppend(void** fp, cstr name);
      int OpenForRead(void** fp, cstr name);
      void UILoop(uint32 milliseconds);
	  void ToSysPath(char* path);
	  void ToUnixPath(char* path);
   }

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