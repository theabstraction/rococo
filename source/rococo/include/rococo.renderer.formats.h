#pragma once

#include <rococo.types.h>

namespace Rococo::Graphics
{
	enum class TextureFormat: uint32
	{
		F_32_BIT_FLOAT,
		F_RGBA_32_BIT,
		F_8_BIT_UINT,
		F_24_BIT_BUMPMAP,
		F_24_BIT_SINT,
		F_24_BIT_UINT,
		F_16_BIT_FLOAT,
		F_UNKNOWN
	};
}