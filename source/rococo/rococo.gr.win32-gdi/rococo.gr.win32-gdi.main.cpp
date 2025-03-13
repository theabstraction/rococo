#include <rococo.types.h>

#define ROCOCO_GR_GDI_API ROCOCO_API_EXPORT

#include <rococo.gr.win32-gdi.h>
#include <rococo.strings.h>

#include <cstdio>

namespace Rococo::GR::Win32::Implementation
{
	struct SceneRenderer: IGR2DSceneRenderContext
	{
		HWND hWnd;
		HDC paintDC = 0;
		
		SceneRenderer(HWND _hWnd): hWnd(_hWnd)
		{

		}

		void DrawBackground() override
		{
			RECT rect;
			GetClientRect(hWnd, &rect);

			HBRUSH redBrush = CreateSolidBrush(RGB(128, 0, 0));
			auto old = SelectObject(paintDC, redBrush);

			FillRect(paintDC, &rect, redBrush);

			SelectObject(paintDC, old);

			DeleteObject(redBrush);
		}

		void Render(IGR2DScene& scene)
		{
			PAINTSTRUCT ps;
			BeginPaint(hWnd, &ps);

			paintDC = ps.hdc;

			scene.Render(*this);

			EndPaint(hWnd, &ps);
		}
	};

	struct SceneHandler : IGR2DSceneHandlerSupervisor
	{
		void OnPaint(IGR2DScene& scene, HWND hWnd) override
		{
			SceneRenderer renderer(hWnd);
			renderer.Render(scene);
		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo::GR::Win32
{
	ROCOCO_API_EXPORT IGR2DSceneHandlerSupervisor* CreateSceneHandler()
	{
		return new Implementation::SceneHandler();
	}
}