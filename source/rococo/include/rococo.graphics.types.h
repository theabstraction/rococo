#pragma once

#include <rococo.types.h>

namespace Rococo
{
	ROCOCO_ID(ID_FONT, int32, -1);
	ROCOCO_ID(ID_TEXTURE, size_t, (size_t)-1LL);
	ROCOCO_ID(ID_VERTEX_SHADER, size_t, (size_t)-1LL);
	ROCOCO_ID(ID_GEOMETRY_SHADER, size_t, (size_t)-1LL);
	ROCOCO_ID(ID_CUBE_TEXTURE, size_t, 0);
	ROCOCO_ID(ID_PIXEL_SHADER, size_t, (size_t)-1LL);
}

namespace Rococo::Graphics
{
	class MaterialId
	{		
	private:
		// The value is a float, because it is passed to GPU shaders that generally do not have raw int values
		float value = 0.0f; 

	public:
		MaterialId()
		{

		}

		MaterialId(const MaterialId& id)
		{
			this->value = id.value;
		}

		MaterialId operator = (MaterialId id)
		{
			this->value = id.value;
			return id;
		}

		explicit MaterialId(int materialIndex)
		{
			value = float(materialIndex);
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
}