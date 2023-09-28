#pragma once

#include <rococo.types.h>

#ifndef INCLUDED_STRINGS_REFLECTION
# define INCLUDED_STRINGS_REFLECTION
#endif

#ifndef INCLUDED_ROCOCO_STRINGS
# include <rococo.strings.h>
#endif

#ifndef INCLUDED_ROCOCO_REFLECTOR
# include <rococo.reflector.h>
#endif

namespace Rococo::Reflection
{
	struct Reflected_HString : public IReflectedString
	{
		Strings::HString& src;

		Reflected_HString(Strings::HString& _src) : src(_src)
		{

		}

		operator Reflection::IReflectedString& () { return *this; }

		uint32 Capacity() const override
		{
			// TODO, allow meta data to set this explicitly
			return 4096;
		}

		cstr ReadString() const override
		{
			return src.c_str();
		}

		void WriteString(cstr s) override
		{
			src = s;
		}
	};

	inline Reflected_HString Reflect(Strings::HString& a)
	{
		return Reflected_HString(a);
	}

	struct Reflected_StackString : public Reflection::IReflectedString
	{
		char* src;
		size_t sizeofSrc;

		Reflected_StackString(char* _src, size_t _sizeofSrc) : src(_src), sizeofSrc(_sizeofSrc)
		{
			if (_sizeofSrc > 0xFFFF'FFFFULL)
			{
				Throw(0, "%s: capacity too large", __FUNCTION__);
			}
		}

		operator Reflection::IReflectedString& () { return *this; }

		uint32 Capacity() const override
		{
			return (uint32) sizeofSrc;
		}

		cstr ReadString() const override
		{
			return src;
		}

		void WriteString(cstr s) override
		{
			Strings::SafeFormat(src, sizeofSrc, "%s", s);
		}
	};

	template<size_t CAPACITY>
	inline Reflected_StackString Reflect(char buffer[CAPACITY]) // inline int SafeFormat(char(&buffer)[CAPACITY])
	{
		return Reflected_StackString(buffer, CAPACITY);
	}

	template<uint64 CAPACITY>
	struct Reflected_WideStackString : public Reflection::IReflectedString
	{
		char buffer[CAPACITY];
		wchar_t* src;

		Reflected_WideStackString(wchar_t (&_src)[CAPACITY]) : src(_src)
		{
			if constexpr (CAPACITY > 0xFFFF'FFFFULL)
			{
				Throw(0, "%s: capacity too large", __FUNCTION__);
			}

			SafeFormat(buffer, "%ws", src);
		}

		operator Reflection::IReflectedString& () { return *this; }

		uint32 Capacity() const override
		{
			return (uint32) CAPACITY;
		}

		cstr ReadString() const override
		{
			return buffer;
		}

		void WriteString(cstr s) override
		{
			SafeFormat(buffer, CAPACITY, "%s", s);
			SafeFormat(src, CAPACITY, L"%ws", s);
		}
	};

	template<uint64 CAPACITY>
	inline Reflected_WideStackString<CAPACITY> Reflect(wchar_t (&buffer)[CAPACITY]) // inline int SafeFormat(char(&buffer)[CAPACITY])
	{
		return Reflected_WideStackString<CAPACITY>(buffer);
	}

	ROCOCO_API void ReflectStackFormat(IReflectionVisitor& v, cstr variableName, const char* format, ...);
	ROCOCO_API void SetSection(IReflectionVisitor& v, const char* format, ...);
	ROCOCO_API void EnterElement(IReflectionVisitor& v, const char* format, ...);
}