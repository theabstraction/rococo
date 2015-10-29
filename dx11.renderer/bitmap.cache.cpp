#include <rococo.types.h>
#include <rococo.renderer.h>
#include <rococo.io.h>

namespace
{
	using namespace Rococo;

	class Bitmaps: public IBitmapCacheSupervisor
	{
		IInstallation& installation;
		IRenderer& renderer;

		AutoFree<IExpandingBuffer> imageBuffer;
	public:
		Bitmaps(IInstallation& _installation, IRenderer& _renderer):
			installation(_installation),
			renderer(_renderer),
			imageBuffer(CreateExpandingBuffer(1024 * 1024))
		{

		}

		virtual ID_BITMAP Cache(const wchar_t* resourceName)
		{
			installation.LoadResource(resourceName, *imageBuffer, 128 * 1024 * 1024);
			auto id = renderer.LoadTexture(*imageBuffer, resourceName);
			return id; 
		}

		virtual void SetCursorBitmap(ID_BITMAP id, Vec2i hotspotOffset)
		{
			renderer.SetCursorBitmap(id, hotspotOffset,{ 0.0f, 0.0f }, { 1.0f, 1.0f });
		}

		virtual void DrawBitmap(IGuiRenderContext& gc, const GuiRect& targetRect, ID_BITMAP id)
		{
			gc.SelectTexture(id);

			float x0 = (float) targetRect.left;
			float x1 = (float)targetRect.right;
			float y0 = (float)targetRect.top;
			float y1 = (float)targetRect.bottom;

			GuiVertex quad[6]
			{
				GuiVertex { x0, y0, 0.0f, 0.0f, RGBAb(128,0,0), 0.0f, 0.0f, 0.0f },
				GuiVertex { x1, y0, 0.0f, 0.0f, RGBAb(0,128,0), 1.0f, 0.0f, 0.0f },
				GuiVertex { x1, y1, 0.0f, 0.0f, RGBAb(0,0,128), 1.0f, 1.0f, 0.0f },
				GuiVertex { x1, y1, 0.0f, 0.0f, RGBAb(128,0,0), 1.0f, 1.0f, 0.0f },
				GuiVertex { x0, y1, 0.0f, 0.0f, RGBAb(0,128,0), 0.0f, 1.0f, 0.0f },
				GuiVertex { x0, y0, 0.0f, 0.0f, RGBAb(0,0,128), 0.0f, 0.0f, 0.0f }
			};

			gc.AddTriangle(quad);
			gc.AddTriangle(quad + 3);
		}

		virtual void Free()
		{
			delete this;
		}
	};
}

namespace Rococo
{
	IBitmapCacheSupervisor* CreateBitmapCache(IInstallation& installation, IRenderer& renderer)
	{
		return new Bitmaps(installation, renderer);
	}
}