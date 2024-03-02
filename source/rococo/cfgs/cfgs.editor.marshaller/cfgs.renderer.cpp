#include <rococo.cfgs.h>

using namespace Rococo::Editors;

namespace Rococo::CFGS::Internal
{
	class CFGSRenderer : public ICFGSRenderer
	{
	private:
	public:
		CFGSRenderer()
		{

		}

		void Free() override
		{
			delete this;
		}

		void RenderNode(IFlatGuiRenderer& fgr, const ICFGSNode& node, IDesignTransformations& transforms)
		{
			auto rect = node.GetDesignRectangle();

			GuiRect screenRect = WorldToScreen(rect, transforms);
			fgr.FillRect(screenRect);
		}

		void Render(IFlatGuiRenderer& fgr, ICFGS& cfgs, IDesignTransformations& transforms) override
		{
			fgr.SetFillOptions(RGBAb(192, 120, 120));

			auto& nodes = cfgs.Nodes();
			for (int32 i = 0; i < nodes.Count(); ++i)
			{
				RenderNode(fgr, nodes[i], transforms);
			}
		}
	};
}

namespace Rococo::CFGS
{
	CFGS_MARSHALLER_API ICFGSRenderer* CreateCFGSRenderer()
	{
		return new Internal::CFGSRenderer();
	}
}

#ifdef _WIN32
# pragma comment(lib, "rococo.windows.lib")
#endif