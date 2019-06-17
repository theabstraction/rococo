#pragma once

#ifdef SEXCHAR_IS_WIDE
# error "Wide characters no longer supported."
#endif

#ifdef ROCOCO_USE_SAFE_V_FORMAT
# include <stdarg.h>
#endif

#ifndef _WIN32
int _stricmp(const char* a, const char* b);
#endif

namespace Rococo
{
   int SecureFormat(char* buffer, size_t capacity, const char* format, ...);
   int SafeFormat(char* buffer, size_t capacity, const char* format, ...);

#ifdef ROCOCO_USE_SAFE_V_FORMAT
   int SafeVFormat(char* buffer, size_t capacity, const char* format, va_list args);

# ifndef _WIN32
   int sscanf_s(const char* buffer, const char* format, ...);
# endif
#endif

	struct IStringBuffer
	{
		virtual char* GetBufferStart() = 0;
		virtual size_t Capacity() const = 0;
	};

   cstr GetFinalNull(cstr s);
   cstr GetRightSubstringAfter(cstr s, char c);
   cstr GetFileExtension(cstr s);

   bool Eq(cstr a, cstr b);
   bool EqI(cstr a, cstr b);
   bool StartsWith(cstr bigString, cstr prefix);
   bool EndsWith(cstr bigString, cstr suffix);

   void SetStringAllocator(IAllocator* a);

   struct HStringData
   {
	   cstr currentBuffer;
	   size_t length;
	   size_t refCount;
   };

   class HString
   {
   private:
	   HStringData* data;
   public:
	   HString();
	   HString(const HString& s);
	   HString(cstr s);
	   HString& operator = (const HString& s);
	   HString& operator = (cstr s);
	   ~HString();

	   cstr c_str() const
	   {
		   return data->currentBuffer;
	   }

	   operator cstr() const
	   {
		   return data->currentBuffer;
	   }

	   size_t length() const
	   {
		   return data->length;
	   }

	   size_t ComputeHash() const;
   };

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

   struct IStringBuilder
   {
	   virtual StringBuilder& Builder() = 0;
	   virtual void Free() = 0;
   };

   IStringBuilder* CreateDynamicStringBuilder(size_t initialCapacity);

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

   bool IsCapital(char c);
   bool IsLowerCase(char c);
   bool IsAlphabetical(char c);
   bool IsNumeric(char c);
   bool IsAlphaNumeric(char c);

   size_t Hash(cstr text);
   int32 Hash(cstr s, int64 length);
   int32 Hash(int32 x);
   int32 Hash(int64 x);

   int WriteToStandardOutput(const char* text, ...);
   int WriteToStandardOutput(cstr text, ...);

   int32 StringLength(const char* s);
   int32 StringLength(cstr s);
   void CopyString(char* dest, size_t capacity, const char* source);

   void StringCat(char* buf, cstr source, int maxChars);

   size_t rlen(cstr s);
   int StrCmpN(cstr a, cstr b, size_t len);

   int32 Compare(cstr a, cstr b);
   int32 CompareI(cstr a, cstr b);
   int32 CompareI(cstr a, cstr b, int64 count);
   int32 Compare(cstr a, cstr b, int64 count);
}
