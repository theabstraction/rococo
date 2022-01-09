#pragma once

namespace Rococo::Graphics
{
	ROCOCOAPI ILandscapeTesselatorSupervisor : ILandscapeTesselator
	{
		virtual void Free() = 0;
	};

	ILandscapeTesselatorSupervisor* CreateLandscapeTesselator(IMeshBuilderSupervisor& meshes);
}