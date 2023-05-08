#pragma once

#ifndef INCLUDED_ROCOCO_STRINGS
# define INCLUDED_ROCOCO_STRINGS
#endif

#ifdef SEXCHAR_IS_WIDE
# error "Wide characters no longer supported."
#endif

#ifdef ROCOCO_USE_SAFE_V_FORMAT
# include <stdarg.h>
#endif

#ifdef __APPLE__
#define _stricmp strcasecmp
#endif

namespace Rococo
{
	ROCOCO_API [[nodiscard]] fstring to_fstring(cstr const msg);
}

namespace Rococo::Strings
{
	ROCOCO_API cstr FindSubstring(cr_substring bigText, cr_substring searchTerm);
	ROCOCO_API cstr FindSubstring(cr_substring bigText, const fstring& searchTerm);
	ROCOCO_API cstr ForwardFind(char c, cr_substring text);
	ROCOCO_API cstr SkipBlankspace(cr_substring token);
	ROCOCO_API cstr SkipNotBlankspace(cr_substring token);

	ROCOCO_API int SecureFormat(char* buffer, size_t capacity, const char* format, ...);
	ROCOCO_API int SecureFormat(wchar_t* buffer, size_t capacity, const wchar_t* format, ...);
	ROCOCO_API int SafeFormat(char* buffer, size_t capacity, const char* format, ...);
	ROCOCO_API int SafeFormat(wchar_t* buffer, size_t capacity, const wchar_t* format, ...);

	ROCOCO_API [[nodiscard]] uint32 FastHash(cstr text);

#ifdef ROCOCO_USE_SAFE_V_FORMAT
	ROCOCO_API int SafeVFormat(char* buffer, size_t capacity, const char* format, va_list args);
	ROCOCO_API int SafeVFormat(wchar_t* buffer, size_t capacity, const wchar_t* format, va_list args);
# ifndef _WIN32
	ROCOCO_API int sscanf_s(const char* buffer, const char* format, ...);
# endif
#endif

#if USE_VSTUDIO_SAL
	template<size_t CAPACITY, typename... Args>
	inline int SafeFormat(_Out_writes_(CAPACITY) _Null_terminated_ char(&buffer)[CAPACITY], _Printf_format_string_ cstr format, Args... args)
	{
		return SafeFormat(buffer, CAPACITY, format, args...);
	}

	template<size_t CAPACITY, typename... Args>
	inline int SafeFormat(_Out_writes_(CAPACITY) _Null_terminated_ wchar_t(&buffer)[CAPACITY], _Printf_format_string_ const wchar_t* format, Args... args)
	{
		return SafeFormat(buffer, CAPACITY, format, args...);
	}

	template<size_t CAPACITY, typename... Args>
	inline int SecureFormat(_Out_writes_(CAPACITY) _Null_terminated_ char(&buffer)[CAPACITY], _Printf_format_string_ cstr format, Args... args)
	{
		return SecureFormat(buffer, CAPACITY, format, args...);
	}

	template<size_t CAPACITY, typename... Args>
	inline int SecureFormat(_Out_writes_(CAPACITY) _Null_terminated_ wchar_t(&buffer)[CAPACITY], _Printf_format_string_ const wchar_t* format, Args... args)
	{
		return SecureFormat(buffer, CAPACITY, format, args...);
	}
#else
	template<size_t CAPACITY, typename... Args>
	inline int SafeFormat(char(&buffer)[CAPACITY], cstr format, Args... args)
	{
		return SafeFormat(buffer, CAPACITY, format, args...);
	}

	template<size_t CAPACITY, typename... Args>
	inline int SafeFormat(wchar_t(&buffer)[CAPACITY], const wchar_t* format, Args... args)
	{
		return SafeFormat(buffer, CAPACITY, format, args...);
	}

	template<size_t CAPACITY, typename... Args>
	inline int SecureFormat(char(&buffer)[CAPACITY], cstr format, Args... args)
	{
		return SecureFormat(buffer, CAPACITY, format, args...);
	}

	template<size_t CAPACITY, typename... Args>
	inline int SecureFormat(wchar_t(&buffer)[CAPACITY], const wchar_t* format, Args... args)
	{
		return SecureFormat(buffer, CAPACITY, format, args...);
	}
#endif

	struct IStringBuffer
	{
		virtual char* GetBufferStart() = 0;
		virtual size_t Capacity() const = 0;
	};

	ROCOCO_API cstr GetFinalNull(cstr s);
	ROCOCO_API cstr GetRightSubstringAfter(cstr s, char c);
	ROCOCO_API cstr GetFileExtension(cstr s);

	ROCOCO_API const wchar_t* GetFinalNull(const wchar_t* s);
	ROCOCO_API const wchar_t* GetRightSubstringAfter(const wchar_t* s, wchar_t c);
	ROCOCO_API const wchar_t* GetFileExtension(const wchar_t* s);

	ROCOCO_API bool Eq(const wchar_t* a, const wchar_t* b);
	ROCOCO_API bool Eq(cstr a, cstr b);
	ROCOCO_API bool Eq(cr_substring a, cstr b);
	inline bool Eq(cstr a, cr_substring b) { return Eq(b, a); }
	ROCOCO_API bool EqI(cstr a, cstr b);
	ROCOCO_API bool StartsWith(cstr bigString, cstr prefix);
	ROCOCO_API bool EndsWith(cstr bigString, cstr suffix);

	ROCOCO_API bool StartsWith(const wchar_t* bigString, const wchar_t* prefix);
	ROCOCO_API bool EndsWith(const wchar_t* bigString, const wchar_t* suffix);

	ROCOCO_API void SetStringAllocator(IAllocator* a);

	struct HStringData
	{
		cstr currentBuffer;
		size_t length;
		size_t refCount;
	};

	// Heap based persistent string
	class HString
	{
	private:
		HStringData* data;
	public:
		ROCOCO_API HString();
		ROCOCO_API HString(HString&& other);
		ROCOCO_API HString(const HString& s);
		ROCOCO_API HString(cstr s);
		ROCOCO_API HString& operator = (const HString& s);
		ROCOCO_API HString& operator = (cstr s);
		ROCOCO_API ~HString();

		cstr c_str() const
		{
			return data->currentBuffer;
		}

		ROCOCO_API const fstring to_fstring() const;

		operator fstring() const
		{
			return to_fstring();
		}

		operator cstr() const
		{
			return c_str();
		}

		size_t length() const
		{
			return data->length;
		}

		ROCOCO_API size_t ComputeHash() const;
	};

	inline bool operator == (const HString& a, const HString& b)
	{
		return Eq(a, b);
	}

	inline bool operator != (const HString& a, const HString& b)
	{
		return !(a == b);
	}

	struct StringBuilder
	{
#if	USE_VSTUDIO_SAL
		virtual StringBuilder& AppendFormat(_Printf_format_string_ const char* format, ...) = 0;
#else
		virtual StringBuilder& AppendFormat(const char* format, ...) = 0;
#endif
		virtual StringBuilder& operator << (cstr text) = 0;
		virtual StringBuilder& AppendChar(char c) = 0;
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

	struct IDynamicStringBuilder
	{
		virtual StringBuilder& Builder() = 0;
		virtual void Free() = 0;
	};

	ROCOCO_API IDynamicStringBuilder* CreateDynamicStringBuilder(size_t initialCapacity);

	class StackStringBuilder : public StringBuilder
	{
	private:
		char* buffer;
		size_t capacity;
		int32 length;
	public:
		ROCOCO_API StackStringBuilder(char* _buffer, size_t _capacity);
		ROCOCO_API StackStringBuilder(char* _buffer, size_t _capacity, eOpenType type);
		fstring operator * () const override { return fstring{ buffer, length }; }
#if	USE_VSTUDIO_SAL
		ROCOCO_API StringBuilder& AppendFormat(_Printf_format_string_ const char* format, ...) override;
#else
		ROCOCO_API StringBuilder& AppendFormat(const char* format, ...) override;
#endif
		ROCOCO_API StringBuilder& operator << (cstr text) override;
		ROCOCO_API StringBuilder& AppendChar(char c) override;
		ROCOCO_API StringBuilder& operator << (int32 value)  override;
		ROCOCO_API StringBuilder& operator << (uint32 value) override;
		ROCOCO_API StringBuilder& operator << (int64 value)  override;
		ROCOCO_API StringBuilder& operator << (uint64 value) override;
		ROCOCO_API StringBuilder& operator << (float value) override;
		ROCOCO_API StringBuilder& operator << (double value) override;
		ROCOCO_API void Clear() override;
		ROCOCO_API int32 Length() const override;
	};

	ROCOCO_API bool IsCapital(char c);
	ROCOCO_API bool IsLowerCase(char c);
	ROCOCO_API bool IsAlphabetical(char c);
	ROCOCO_API bool IsNumeric(char c);
	ROCOCO_API bool IsAlphaNumeric(char c);
	ROCOCO_API bool IsAlphaNumeric(cr_substring s);

	ROCOCO_API size_t HashArg(cstr text);
	ROCOCO_API int32 HashArg(cstr s, int64 length);
	ROCOCO_API int32 HashArg(int32 x);
	ROCOCO_API int32 HashArg(int64 x);
	ROCOCO_API uint64 XXHash64Arg(const void* buffer, size_t nBytesLength);

	ROCOCO_API int WriteToStandardOutput(const char* text, ...);
	ROCOCO_API int WriteToStandardOutput(cstr text, ...);

	ROCOCO_API int32 StringLength(const char* s);
	ROCOCO_API int32 StringLength(const wchar_t* s);
	ROCOCO_API void CopyString(char* dest, size_t capacity, const char* source);

	ROCOCO_API void StringCat(char* buf, cstr source, int maxChars);
	ROCOCO_API void StringCat(wchar_t* buf, const wchar_t* source, int maxChars);

	ROCOCO_API size_t rlen(cstr s);
	ROCOCO_API int StrCmpN(cstr a, cstr b, size_t len);

	ROCOCO_API int32 Compare(cstr a, cstr b);
	ROCOCO_API int32 CompareI(cstr a, cstr b);
	ROCOCO_API int32 CompareI(cstr a, cstr b, int64 count);
	ROCOCO_API int32 Compare(cstr a, cstr b, int64 count);

	ROCOCO_API int LevenshteinDistance(cstr source, cstr target);

	ROCOCO_API bool StartsWith(cr_substring token, const fstring& prefix);
	ROCOCO_API bool StartsWith(cstr token, cr_substring prefix);
	ROCOCO_API ptrdiff_t Length(cr_substring token);
	ROCOCO_API bool SubstringToString(char* name, size_t sizeofName, cr_substring substring);
	ROCOCO_API bool Eq(const fstring& a, cr_substring b);
	ROCOCO_API bool Eq(cr_substring a, const fstring& b);
	ROCOCO_API bool Eq(cr_substring a, cr_substring b);
	ROCOCO_API bool IsEmpty(cr_substring token);

	ROCOCO_API void ReplaceChar(char* buffer, size_t capacity, char target, char replacement);
	
#if USE_VSTUDIO_SAL
	ROCOCO_API int32 Format(U8FilePath& path, _Printf_format_string_ cstr format, ...);
	ROCOCO_API int32 Format(WideFilePath& path, _Printf_format_string_ const wchar_t* format, ...);
#else
	ROCOCO_API int32 Format(U8FilePath& path, cstr format, ...);
	ROCOCO_API int32 Format(WideFilePath& path, const wchar_t* format, ...);
#endif
	ROCOCO_API void Assign(U8FilePath& dest, const wchar_t* wideSrc);
	ROCOCO_API void Assign(WideFilePath& dest, const char* src);

	ROCOCO_API void ValidateFQNameIdentifier(cstr fqName);

	ROCOCO_API [[nodiscard]] uint32 FastHash(cstr text);

	ROCOCO_API void SplitString(cstr text, size_t length, cstr seperators, IEventCallback<cstr>& onSubString);
} // Rococo::Strings


#ifdef INCLUDED_ROCOCO_REFLECTOR
# include <rococo.strings.reflection.h>
#endif