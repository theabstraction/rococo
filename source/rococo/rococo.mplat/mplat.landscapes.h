#pragma once

namespace Rococo::Graphics
{
	ROCOCO_INTERFACE ILandscapeTesselatorSupervisor : ILandscapeTesselator
	{
		virtual void Free() = 0;
	};

	ILandscapeTesselatorSupervisor* CreateLandscapeTesselator(IMeshBuilderSupervisor& meshes);
}