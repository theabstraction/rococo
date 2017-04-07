#ifndef Rococo_TYPES_H
#define Rococo_TYPES_H

#include <rococo.compiler.options.h>


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

   int64 CpuTicks();
   int64 CpuHz();

   struct fstring
   {
      const wchar_t* buffer;
      int32 length;

      operator const wchar_t*() const { return buffer; }
   };

   inline constexpr fstring operator"" _fstring(const wchar_t* msg, size_t length)
   {
      return fstring{ msg, (int32)length };
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
		virtual const wchar_t* Message() const = 0;
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

	void Throw(int32 errorCode, const wchar_t* format, ...);
	void TripDebugger();
	bool IsDebugging();

	template<class T> struct IEventCallback
	{
		virtual void OnEvent(T& arg) = 0;
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

	template<> ROCOCOAPI IEnumerator<const wchar_t*>
	{
		virtual void operator()(const wchar_t* t) = 0;
	};

	template<class T> ROCOCOAPI IEnumerable
	{
		virtual const T& operator[](size_t index) = 0;
		virtual size_t Count() const = 0;
		virtual void Enumerate(IEnumerator<T>& cb) = 0;
	};

	template<> ROCOCOAPI IEnumerable<const wchar_t*>
	{
		virtual const wchar_t* operator[](size_t index) = 0;
		virtual size_t Count() const = 0;
		virtual void Enumerate(IEnumerator<const wchar_t*>& cb) = 0;
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

      RGBA(float _r, float _g, float _b, float _a = 1.0f) : red(_r), green(_g), blue(_b), alpha(_a) {}
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

   ROCOCOAPI IAllocator
   {
      virtual void* Allocate(size_t capacity) = 0;
      virtual void Free(void* data) = 0;
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

   namespace OS
   {
      void PrintDebug(const char* format, ...);
   }
}

#endif