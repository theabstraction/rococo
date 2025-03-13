#pragma once

#include <rococo.types.h>
#include <rococo.os.win32.h>

namespace Rococo
{
	ROCOCO_INTERFACE IGR2DSceneRenderContext
	{
		virtual void DrawBackground() = 0;
	};

	ROCOCO_INTERFACE IGR2DScene
	{
		virtual void Render(IGR2DSceneRenderContext& rc) = 0;
	};

	ROCOCO_INTERFACE IGR2DSceneHandler
	{
		virtual void OnPaint(IGR2DScene& scene, HWND hWnd) = 0;
	};

	ROCOCO_INTERFACE IGR2DSceneHandlerSupervisor : IGR2DSceneHandler
	{
		virtual void Free() = 0;
	};
}

#ifndef ROCOCO_GR_GDI_API
# define ROCOCO_GR_GDI_API ROCOCO_API_IMPORT
#endif

namespace Rococo::GR::Win32
{
	ROCOCO_GR_GDI_API IGR2DSceneHandlerSupervisor* CreateSceneHandler();
}