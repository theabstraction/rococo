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
	[[nodiscard]] fstring to_fstring(cstr const msg);
}

namespace Rococo::Strings
{
	cstr FindSubstring(cr_substring bigText, cr_substring searchTerm);
	cstr FindSubstring(cr_substring bigText, const fstring& searchTerm);
	cstr ForwardFind(char c, cr_substring text);
	cstr SkipBlankspace(cr_substring token);
	cstr SkipNotBlankspace(cr_substring token);

	int SecureFormat(char* buffer, size_t capacity, const char* format, ...);
	int SecureFormat(wchar_t* buffer, size_t capacity, const wchar_t* format, ...);
	int SafeFormat(char* buffer, size_t capacity, const char* format, ...);
	int SafeFormat(wchar_t* buffer, size_t capacity, const wchar_t* format, ...);

	[[nodiscard]] uint32 FastHash(cstr text);

#ifdef ROCOCO_USE_SAFE_V_FORMAT
	int SafeVFormat(char* buffer, size_t capacity, const char* format, va_list args);
	int SafeVFormat(wchar_t* buffer, size_t capacity, const wchar_t* format, va_list args);
# ifndef _WIN32
	int sscanf_s(const char* buffer, const char* format, ...);
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

	cstr GetFinalNull(cstr s);
	cstr GetRightSubstringAfter(cstr s, char c);
	cstr GetFileExtension(cstr s);

	const wchar_t* GetFinalNull(const wchar_t* s);
	const wchar_t* GetRightSubstringAfter(const wchar_t* s, wchar_t c);
	const wchar_t* GetFileExtension(const wchar_t* s);

	bool Eq(const wchar_t* a, const wchar_t* b);
	bool Eq(cstr a, cstr b);
	bool Eq(cr_substring a, cstr b);
	inline bool Eq(cstr a, cr_substring b) { return Eq(b, a); }
	bool EqI(cstr a, cstr b);
	bool StartsWith(cstr bigString, cstr prefix);
	bool EndsWith(cstr bigString, cstr suffix);

	bool StartsWith(const wchar_t* bigString, const wchar_t* prefix);
	bool EndsWith(const wchar_t* bigString, const wchar_t* suffix);

	void SetStringAllocator(IAllocator* a);

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
		HString();
		HString(HString&& other);
		HString(const HString& s);
		HString(cstr s);
		HString& operator = (const HString& s);
		HString& operator = (cstr s);
		~HString();

		cstr c_str() const
		{
			return data->currentBuffer;
		}

		const fstring to_fstring() const;

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

		size_t ComputeHash() const;
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

	IDynamicStringBuilder* CreateDynamicStringBuilder(size_t initialCapacity);

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
#if	USE_VSTUDIO_SAL
		virtual StringBuilder& AppendFormat(_Printf_format_string_ const char* format, ...) override;
#else
		virtual StringBuilder& AppendFormat(const char* format, ...) override;
#endif
		StringBuilder& operator << (cstr text) override;
		StringBuilder& AppendChar(char c) override;
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
	bool IsAlphaNumeric(cr_substring s);

	size_t HashArg(cstr text);
	int32 HashArg(cstr s, int64 length);
	int32 HashArg(int32 x);
	int32 HashArg(int64 x);
	uint64 XXHash64Arg(const void* buffer, size_t nBytesLength);

	int WriteToStandardOutput(const char* text, ...);
	int WriteToStandardOutput(cstr text, ...);

	int32 StringLength(const char* s);
	int32 StringLength(const wchar_t* s);
	void CopyString(char* dest, size_t capacity, const char* source);

	void StringCat(char* buf, cstr source, int maxChars);
	void StringCat(wchar_t* buf, const wchar_t* source, int maxChars);

	size_t rlen(cstr s);
	int StrCmpN(cstr a, cstr b, size_t len);

	int32 Compare(cstr a, cstr b);
	int32 CompareI(cstr a, cstr b);
	int32 CompareI(cstr a, cstr b, int64 count);
	int32 Compare(cstr a, cstr b, int64 count);

	int LevenshteinDistance(cstr source, cstr target);

	bool StartsWith(cr_substring token, const fstring& prefix);
	bool StartsWith(cstr token, cr_substring prefix);
	ptrdiff_t Length(cr_substring token);
	bool SubstringToString(char* name, size_t sizeofName, cr_substring substring);
	bool Eq(const fstring& a, cr_substring b);
	bool Eq(cr_substring a, const fstring& b);
	bool Eq(cr_substring a, cr_substring b);
	bool IsEmpty(cr_substring token);

	void ReplaceChar(char* buffer, size_t capacity, char target, char replacement);
} // Rococo::Strings


#ifdef INCLUDED_ROCOCO_REFLECTOR
# include <rococo.strings.reflection.h>
#endif