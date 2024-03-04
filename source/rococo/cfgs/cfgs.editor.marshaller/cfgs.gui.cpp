#include <rococo.cfgs.h>
#include <rococo.maths.h>

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

	void ShrinkRect(REF GuiRect& rect, int nPixelsEachBorder)
	{
		rect.left += nPixelsEachBorder;
		rect.top += nPixelsEachBorder;
		rect.right -= nPixelsEachBorder;
		rect.bottom -= nPixelsEachBorder;
	}

	static bool IsLeftSide(const ICFGSSocket& socket)
	{
		switch (socket.SocketClassification())
		{
		case SocketClass::InputRef:
		case SocketClass::InputVar:
		case SocketClass::ConstInputRef:
		case SocketClass::Trigger:
			return true;
		default:
			return false;
		}
	}

	RGBAb GetSocketColour(const ICFGSSocket& socket, bool isLit)
	{
		switch (socket.SocketClassification())
		{
		case SocketClass::Trigger:
		case SocketClass::Exit:
			return isLit ? RGBAb(0, 255, 0) : RGBAb(0, 192, 0);
		case SocketClass::InputRef:
		case SocketClass::InputVar:
		case SocketClass::ConstInputRef:
			return isLit ? RGBAb(0, 255, 255) : RGBAb(0, 192, 192);
		default:
			return isLit ? RGBAb(128, 128, 255) : RGBAb(96, 96, 192);
		}
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
			if (lastHoveredNode != id || lastHoveredNode != NodeId())
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


		void RenderLeftSocket(IFlatGuiRenderer& fgr, const ICFGSNode& node, const ICFGSSocket& socket, IDesignTransformations& transforms, int yIndex)
		{
			DesignerRect designerParentRect = node.GetDesignRectangle();
			GuiRect parentRect = WorldToScreen(designerParentRect, transforms);

			int32 socketTop = parentRect.top + 30;

			GuiRect circleRect{ parentRect.left + 20, socketTop + yIndex * 20, parentRect.left + 40, socketTop + (yIndex+1) * 20 };
			GuiRect socketTextRect{ circleRect.right, circleRect.top, (parentRect.right + parentRect.left) / 2, circleRect.bottom };

			Vec2i cursorPos = fgr.CursorPosition();
			bool isLit = IsPointInRect(cursorPos, circleRect);

			ShrinkRect(circleRect, 2);
			fgr.DrawCircle(circleRect, GetSocketColour(socket, isLit), 2, RGBAb(0, 0, 0, 0));

			fgr.SetTextOptions(RGBAb(0, 0, 0, 0), RGBAb(255, 255, 255, 255));

			fgr.DrawText(socketTextRect, socket.Name(), EFGAF_Left | EFGAF_VCentre);
		}

		void RenderRightSocket(IFlatGuiRenderer& fgr, const ICFGSNode& node, const ICFGSSocket& socket, IDesignTransformations& transforms, int yIndex)
		{
			DesignerRect designerParentRect = node.GetDesignRectangle();
			GuiRect parentRect = WorldToScreen(designerParentRect, transforms);

			int32 socketTop = parentRect.top + 30;
			
			GuiRect circleRect{ parentRect.right - 40, socketTop + yIndex * 20, parentRect.right - 20, socketTop + (yIndex + 1) * 20 };
			GuiRect socketTextRect{ (parentRect.right + parentRect.left) / 2, circleRect.top, circleRect.left, circleRect.bottom };

			Vec2i cursorPos = fgr.CursorPosition();
			bool isLit = IsPointInRect(cursorPos, circleRect);

			ShrinkRect(circleRect, 2);
			fgr.DrawCircle(circleRect, GetSocketColour(socket, isLit), 2, RGBAb(0, 0, 0, 0));

			fgr.SetTextOptions(RGBAb(0, 0, 0, 0), RGBAb(255, 255, 255, 255));

			fgr.DrawText(socketTextRect, socket.Name(), EFGAF_Right | EFGAF_VCentre);
		}

		void RenderSockets(IFlatGuiRenderer& fgr, const ICFGSNode& node, IDesignTransformations& transforms)
		{
			int32 leftIndex = 0;
			int32 rightIndex = 0;

			for (int i = 0; i < node.SocketCount(); ++i)
			{				
				auto& socket = node[i];
				if (IsLeftSide(socket))
				{
					RenderLeftSocket(fgr, node, socket, transforms, leftIndex++);
				}
				else
				{
					RenderRightSocket(fgr, node, socket, transforms, rightIndex++);
				}
			}
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

			RGBAb lineColour = isLit ? RGBAb(192, 192, 0) : RGBAb(128, 128, 0);

			fgr.DrawRoundedRect(nodeRect, 16, lineColour);

			GuiRect namePlateRect = ComputeNamePlateRect(nodeRect);

			ColourSchemeQuantum typeNamePlateColours;
			node.Scheme().GetTypeNamePlateColours(OUT typeNamePlateColours);

			fgr.SetFillOptions(isLit ? typeNamePlateColours.litColour : typeNamePlateColours.dullColour);
			fgr.SetTextOptions(isLit ? typeNamePlateColours.litColour : typeNamePlateColours.dullColour, isLit ? typeNameColours.litColour : typeNameColours.dullColour);

			fgr.DrawRoundedRect(namePlateRect, 16, lineColour);

			GuiRect namePlateTextRect = namePlateRect;

			namePlateRect.top = (namePlateRect.bottom + namePlateRect.top) >> 1;

			fgr.DrawFilledRect(namePlateRect);

			namePlateTextRect.left += 2;

			fgr.DrawText(namePlateTextRect, node.Type().Value, EFGAF_Left | EFGAF_VCentre);

			RenderSockets(fgr, node, transforms);
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