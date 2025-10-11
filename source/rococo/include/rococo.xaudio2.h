// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#pragma once

#include <rococo.types.h>
#include <xaudio2.h>
#include <rococo.audio.h>
#include <rococo.release.h>
#include <x3daudio.h>

namespace Rococo::Audio
{
	template<class T>
	struct AutoVoice
	{
		T* src;

		AutoVoice() : src(nullptr)
		{

		}

		AutoVoice(T* pSrc) : src(pSrc)
		{

		}

		~AutoVoice()
		{
			if (src)
			{
				src->DestroyVoice();
			}
		}

		T& operator* ()
		{
			return *src;
		}

		T** operator& ()
		{
			return &src;
		}

		T* operator ->()
		{
			return src;
		}
	};

	inline const X3DAUDIO_VECTOR& To(cr_vec3 source)
	{
		static_assert(sizeof(X3DAUDIO_VECTOR) == sizeof(source));
		return reinterpret_cast<const X3DAUDIO_VECTOR&>(source);
	}

	cstr GetX2Err(HRESULT x2err);
}

#define VALIDATE(errorCode, operation)\
{ \
	if FAILED(operation) \
		Throw(errorCode, "%s: %s", __ROCOCO_FUNCTION__, #operation); \
}
