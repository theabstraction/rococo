#include <rococo.cfgs.h>

using namespace Rococo::Editors;
using namespace Rococo::CFGS;

namespace Rococo::CFGS::Internal
{
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

		GuiRect ComputeNamePlateRect(const GuiRect& nodeRect)
		{
			enum
			{
				LeftMargin = 1,
				TopMargin = 1,
				RightMargin = 1,
				PlateHeight = 24
			};

			GuiRect npRect { nodeRect.left + LeftMargin, nodeRect.top + TopMargin, nodeRect.right - RightMargin, nodeRect.top + PlateHeight };
			return npRect;
		}

		void RenderNode(IFlatGuiRenderer& fgr, const ICFGSNode& node, IDesignTransformations& transforms)
		{
			auto rect = node.GetDesignRectangle();
			GuiRect nodeRect = WorldToScreen(rect, transforms);

			bool isLit = lastHoveredNode == node.UniqueId();

			ColourSchemeQuantum nodeRectColours;
			node.Scheme().GetFillColours(OUT nodeRectColours);

			RGBAb backColour = isLit ? nodeRectColours.litColour : nodeRectColours.dullColour;

			fgr.SetFillOptions(backColour);

			ColourSchemeQuantum typeNameColours;
			node.Scheme().GetTypeNameColours(OUT typeNameColours);

			fgr.SetLineOptions(isLit ? RGBAb(192, 192, 0) : RGBAb(128, 128, 0));

			fgr.DrawRoundedRect(nodeRect, 16);

		//	fgr.MoveLineStartTo({ nodeRect.left, nodeRect.top });
		//	fgr.DrawLineTo({ nodeRect.right, nodeRect.top });
		//	fgr.DrawLineTo({ nodeRect.right, nodeRect.bottom });

		//	fgr.DrawLineTo({ nodeRect.left, nodeRect.bottom });
		//	fgr.DrawLineTo({ nodeRect.left, nodeRect.top });

			GuiRect namePlateRect = ComputeNamePlateRect(nodeRect);

			ColourSchemeQuantum typeNamePlateColours;
			node.Scheme().GetTypeNamePlateColours(OUT typeNamePlateColours);

			fgr.SetFillOptions(isLit ? typeNamePlateColours.litColour : typeNamePlateColours.dullColour);
			fgr.SetTextOptions(isLit ? typeNamePlateColours.litColour : typeNamePlateColours.dullColour, isLit ? typeNameColours.litColour : typeNameColours.dullColour);

			fgr.DrawRoundedRect(namePlateRect, 16);

			GuiRect namePlateTextRect = namePlateRect;

			namePlateRect.top = (namePlateRect.bottom + namePlateRect.top) >> 1;

			fgr.DrawFilledRect(namePlateRect);

			namePlateTextRect.left += 2;

			fgr.DrawText(namePlateTextRect, node.Type().Value);
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