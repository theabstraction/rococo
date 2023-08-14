#pragma once
#include <rococo.api.h>
#include <rococo.graphics.types.h>

namespace Rococo::Graphics
{
	enum class MaterialCategory: int;
	struct IRenderer;
}

#include "../../rococo.mplat/code-gen/Rococo.Graphics.MaterialCategory.sxh.h"
#include "../../rococo.mplat/code-gen/Rococo.Graphics.IMaterialBuilder.sxh.h"

namespace Rococo
{
	struct IMathsVenue;
}

namespace Rococo::Events
{
	class IPublisher;
}

namespace Rococo::Graphics
{
	ROCOCO_INTERFACE IMaterialBuilderSupervisor : public IMaterialBuilder
	{
		virtual void Free() = 0;
	};

	namespace Construction
	{
		IMaterialBuilderSupervisor* CreateMaterialsBuilder(IRenderer& renderer, Rococo::Events::IPublisher& publisher);
	}
}