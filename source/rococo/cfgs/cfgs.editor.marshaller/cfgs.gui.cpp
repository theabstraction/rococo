#include <rococo.cfgs.h>

using namespace Rococo::Editors;
using namespace Rococo::CFGS;

namespace Rococo::CFGS::Internal
{
	class CFGSGui : public ICFGSGui
	{
	private:
		ICFGS& cfgs;
		IDesignTransformations& transforms;
		ICFGSGuiEventHandler& eventHandler;

		NodeId lastHoveredNode;
	public:
		CFGSGui(ICFGS& _cfgs, Rococo::Editors::IDesignTransformations& _transforms, ICFGSGuiEventHandler& _eventHandler):
			cfgs(_cfgs), transforms(_transforms), eventHandler(_eventHandler)
		{

		}

		void SetHovered(NodeId id)
		{
			if (lastHoveredNode != id)
			{
				lastHoveredNode = id;
				eventHandler.CFGSGuiEventHandler_OnNodeHoverChanged(id);
			}
		}

		void Free() override
		{
			delete this;
		}

		void RenderNode(IFlatGuiRenderer& fgr, const ICFGSNode& node, IDesignTransformations& transforms)
		{
			auto rect = node.GetDesignRectangle();
			GuiRect nodeRect = WorldToScreen(rect, transforms);

			bool isLit = lastHoveredNode == node.UniqueId();

			ColourSchemeQuantum nodeRectColours;
			node.Scheme().GetFillColours(OUT nodeRectColours);

			fgr.SetFillOptions(isLit ? nodeRectColours.litBackColour : nodeRectColours.dullBackColour);

			fgr.FillRect(nodeRect);
		}

		static const ICFGSNode* FindTopMostNodeContainingPoint(DesignerVec2 point, ICFGSNodeEnumerator& nodes)
		{
			for (int32 i = 0; i < nodes.Count(); ++i)
			{
				auto& node = nodes.GetByZOrderDescending(i);
				DesignerRect rect = node.GetDesignRectangle();
				if (rect.Contains(point))
				{
					return &node;
				}
			}

			return nullptr;
		}

		void OnCursorMove(Vec2i pixelPosition) override
		{
			DesignerVec2 designerPos = transforms.ScreenToWorld(pixelPosition);

			const ICFGSNode* topMostNode = FindTopMostNodeContainingPoint(designerPos, cfgs.Nodes());
			if (topMostNode)
			{
				SetHovered(topMostNode->UniqueId());	
			}
			else
			{
				SetHovered(NodeId());
			}
		}

		void Render(IFlatGuiRenderer& fgr) override
		{
			auto& nodes = cfgs.Nodes();
			for (int32 i = 0; i < nodes.Count(); ++i)
			{
				RenderNode(fgr, nodes.GetByZOrderAscending(i), transforms);
			}
		}
	};
}

namespace Rococo::CFGS
{
	CFGS_MARSHALLER_API ICFGSGui* CreateCFGSGui(ICFGS& cfgs, IDesignTransformations& transforms, ICFGSGuiEventHandler& eventHandler)
	{
		return new Internal::CFGSGui(cfgs, transforms, eventHandler);
	}
}

#ifdef _WIN32
# pragma comment(lib, "rococo.windows.lib")
#endif