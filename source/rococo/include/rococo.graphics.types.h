// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#pragma once

#include <rococo.types.h>

namespace Rococo
{
	ROCOCO_ID(ID_FONT, int32, -1);
	ROCOCO_ID(ID_TEXTURE, size_t, (size_t)-1LL);
	ROCOCO_ID(ID_VOLATILE_BITMAP, size_t, (size_t)-1LL);
	ROCOCO_ID(ID_VERTEX_SHADER, size_t, (size_t)-1LL);
	ROCOCO_ID(ID_GEOMETRY_SHADER, size_t, (size_t)-1LL);
	ROCOCO_ID(ID_CUBE_TEXTURE, size_t, 0);
	ROCOCO_ID(ID_PIXEL_SHADER, size_t, (size_t)-1LL);
}

namespace Rococo::Graphics
{
	ROCOCO_ID(IdSprite, int32, 0);

	template<class T>
	class GPU_ENTITY_ID
	{		
	private:
		// The value is a float, because it is passed to GPU shaders that generally do not have raw int values
		float value = 0.0f; 

	public:
		GPU_ENTITY_ID<T>()
		{

		}

		GPU_ENTITY_ID<T>(const GPU_ENTITY_ID<T>& id)
		{
			this->value = id.value;
		}

		GPU_ENTITY_ID<T> operator = (GPU_ENTITY_ID<T> id)
		{
			this->value = id.value;
			return id;
		}

		explicit GPU_ENTITY_ID<T>(int index)
		{
			value = float(index);
		}

		explicit GPU_ENTITY_ID<T>(float index)
		{
			value = index;
		}

		float ToFloat() const
		{
			return value;
		}

		int ToInt() const
		{
			return (int) value;
		}

		operator float() const
		{
			return value;
		}
	};

	struct MaterialIdProxy;
	using MaterialId = GPU_ENTITY_ID<MaterialIdProxy>;

	struct TextureSliceProxy;
	using TextureSliceId = GPU_ENTITY_ID<TextureSliceProxy>;

	struct MaterialVertexData;
}