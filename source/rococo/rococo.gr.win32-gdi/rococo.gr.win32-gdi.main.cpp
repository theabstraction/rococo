#include <rococo.types.h>

#define ROCOCO_GR_GDI_API ROCOCO_API_EXPORT

#include <rococo.gr.win32-gdi.h>
#include <rococo.strings.h>
#include <rococo.gui.retained.h>
#include <rococo.maths.h>
#include <cstdio>

using namespace Rococo::Gui;

namespace Rococo::GR::Win32::Implementation
{
	struct SceneRenderer: Gui::IGRRenderContext
	{
		HWND hWnd;
		HDC paintDC = 0;
		
		SceneRenderer(HWND _hWnd): hWnd(_hWnd)
		{

		}

		// Get some kind of hover point for the cursor
		Vec2i CursorHoverPoint() const override
		{
			return { 0,0 };
		}

		// It is up to the renderer to decide if a panel is hovered.
		bool IsHovered(IGRPanel& panel) const override
		{
			UNUSED(panel);
			return false;
		}

		// Get the screen dimensions
		GuiRect ScreenDimensions() const override
		{
			RECT clientRect;
			GetClientRect(hWnd, &clientRect);
			return GuiRect(clientRect.left, clientRect.top, clientRect.right, clientRect.bottom);
		}

		/* heading: 0 = N, E = 90 etc */
		void DrawDirectionArrow(const GuiRect& absRect, RGBAb colour, Degrees heading) override
		{
			UNUSED(absRect);
			UNUSED(colour);
			UNUSED(heading);
		}

		void DrawRect(const GuiRect& absRect, RGBAb colour) override
		{
			HBRUSH hBrush = CreateSolidBrush(RGB(colour.red, colour.green, colour.blue));
			FillRect(paintDC, reinterpret_cast<const RECT*>(&absRect), hBrush);
			DeleteObject(hBrush);
		}

		void DrawRectEdge(const GuiRect& absRect, RGBAb colour1, RGBAb colour2) override
		{
			UNUSED(absRect);
			UNUSED(colour1);
			UNUSED(colour2);
		}

		// Queues an edge rect for rendering after everything else of lower priority has been rendered. Used for highlighting
		void DrawRectEdgeLast(const GuiRect& absRect, RGBAb colour1, RGBAb colour2) override
		{
			UNUSED(absRect);
			UNUSED(colour1);
			UNUSED(colour2);
		}

		void DrawEditableText(GRFontId fontId, const GuiRect& clipRect, GRAlignmentFlags alignment, Vec2i spacing, const fstring& text, int32 caretPos, RGBAb colour) override
		{
			UNUSED(fontId);
			UNUSED(clipRect);
			UNUSED(alignment);
			UNUSED(spacing);
			UNUSED(text);
			UNUSED(caretPos);
			UNUSED(colour);
		}

		void DrawText(GRFontId fontId, const GuiRect& targetRect, const GuiRect& clipRect, GRAlignmentFlags alignment, Vec2i spacing, const fstring& text, RGBAb colour) override
		{
			UNUSED(targetRect);
			UNUSED(fontId);
			UNUSED(clipRect);
			UNUSED(alignment);
			UNUSED(spacing);
			UNUSED(text);
			UNUSED(colour);
		}

		// Causes all render operations to complete
		void Flush() override
		{

		}

		GuiRect lastScissorRect = GuiRect(0,0,0,0);

		void EnableScissors(const GuiRect& scissorRect) override
		{
			lastScissorRect = scissorRect;
		}

		void DisableScissors() override
		{
			lastScissorRect = GuiRect{ 0,0,0,0 };
		}

		bool TryGetScissorRect(GuiRect& scissorRect) const override
		{
			scissorRect = lastScissorRect;
			return lastScissorRect.IsNormalized();
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