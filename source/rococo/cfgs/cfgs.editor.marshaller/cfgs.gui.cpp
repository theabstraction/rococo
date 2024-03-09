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

	enum class RenderPhase
	{
		RGB,
		Indices
	};

	class CFGSGui : public ICFGSGui
	{
	private:
		ICFGSDatabase& cfgs;
		IDesignSpace& designSpace;
		ICFGSGuiEventHandler& eventHandler;

		NodeId lastHoveredNode;

		NodeId dragId;
		Vec2i dragStart{ 0,0 };
	public:
		CFGSGui(ICFGSDatabase& _cfgs, Rococo::Editors::IDesignSpace& _designSpace, ICFGSGuiEventHandler& _eventHandler):
			cfgs(_cfgs), designSpace(_designSpace), eventHandler(_eventHandler)
		{

		}

		virtual ~CFGSGui()
		{

		}

		void SetHovered(NodeId id)
		{
			if (lastHoveredNode || lastHoveredNode != id)
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

		void RenderLeftSocket(IFlatGuiRenderer& fgr, const ICFGSNode& node, const ICFGSSocket& socket, IDesignSpace& designSpace, int yIndex)
		{
			DesignerRect designerParentRect = node.GetDesignRectangle();
			GuiRect parentRect = WorldToScreen(designerParentRect, designSpace);

			int32 socketTop = parentRect.top + 30;

			GuiRect circleRect{ parentRect.left + 6, socketTop + yIndex * 20, parentRect.left + 26, socketTop + (yIndex+1) * 20 };
			GuiRect socketTextRect{ circleRect.right, circleRect.top, (parentRect.right + parentRect.left) / 2, circleRect.bottom };

			Vec2i cursorPos = fgr.CursorPosition();
			bool isLit = IsPointInRect(cursorPos, circleRect);

			ShrinkRect(circleRect, 2);
			fgr.DrawCircle(circleRect, GetSocketColour(socket, isLit), 2, RGBAb(0, 0, 0, 0));

			Vec2i edgePoint = { parentRect.left, (circleRect.top + circleRect.bottom) / 2 };
			socket.SetLastGeometry(circleRect, edgePoint);

			fgr.SetTextOptions(RGBAb(0, 0, 0, 0), RGBAb(255, 255, 255, 255));

			fgr.DrawText(socketTextRect, socket.Name(), EFGAF_Left | EFGAF_VCentre);

			if (socket.CableCount() > 0)
			{
				fgr.MoveLineStartTo(Centre(circleRect));
				fgr.DrawLineTo({ parentRect.left, Centre(circleRect).y }, GetSocketColour(socket, false), 2);

				ShrinkRect(circleRect, 4);
				fgr.DrawCircle(circleRect, GetSocketColour(socket, isLit), 2, RGBAb(0, 0, 0, 0));
			}
		}

		void RenderRightSocket(IFlatGuiRenderer& fgr, const ICFGSNode& node, const ICFGSSocket& socket, IDesignSpace& designSpace, int yIndex)
		{
			DesignerRect designerParentRect = node.GetDesignRectangle();
			GuiRect parentRect = WorldToScreen(designerParentRect, designSpace);

			int32 socketTop = parentRect.top + 30;
			
			GuiRect circleRect{ parentRect.right - 26, socketTop + yIndex * 20, parentRect.right - 6, socketTop + (yIndex + 1) * 20 };
			GuiRect socketTextRect{ (parentRect.right + parentRect.left) / 2, circleRect.top, circleRect.left, circleRect.bottom };

			Vec2i cursorPos = fgr.CursorPosition();
			bool isLit = IsPointInRect(cursorPos, circleRect);

			ShrinkRect(circleRect, 2);
			fgr.DrawCircle(circleRect, GetSocketColour(socket, isLit), 2, RGBAb(0, 0, 0, 0));

			Vec2i edgePoint = { parentRect.right, (circleRect.top + circleRect.bottom) / 2 };
			socket.SetLastGeometry(circleRect, edgePoint);

			fgr.SetTextOptions(RGBAb(0, 0, 0, 0), RGBAb(255, 255, 255, 255));

			fgr.DrawText(socketTextRect, socket.Name(), EFGAF_Right | EFGAF_VCentre);

			if (socket.CableCount() > 0)
			{
				fgr.MoveLineStartTo(Centre(circleRect));
				fgr.DrawLineTo({ parentRect.right, Centre(circleRect).y }, GetSocketColour(socket, false), 2);

				ShrinkRect(circleRect, 4);
				fgr.DrawCircle(circleRect, GetSocketColour(socket, isLit), 2, RGBAb(0, 0, 0, 0));
			}
		}

		void RenderSockets(IFlatGuiRenderer& fgr, const ICFGSNode& node, IDesignSpace& designSpace)
		{
			int32 leftIndex = 0;
			int32 rightIndex = 0;

			for (int i = 0; i < node.SocketCount(); ++i)
			{				
				auto& socket = node[i];
				if (IsLeftSide(socket))
				{
					RenderLeftSocket(fgr, node, socket, designSpace, leftIndex++);
				}
				else
				{
					RenderRightSocket(fgr, node, socket, designSpace, rightIndex++);
				}
			}
		}

		bool TryGetSocketGeometry(const ICFGSNode& node, SocketId socketId, OUT GuiRect& circleRect, OUT Vec2i& edgePoint, RGBAb& lineColour, bool isLit)
		{
			for (int i = 0; i < node.SocketCount(); ++i)
			{
				auto& socket = node[i];
				if (socket.Id() == socketId)
				{
					socket.GetLastGeometry(OUT circleRect, OUT edgePoint);
					lineColour = GetSocketColour(socket, isLit);
					return circleRect.left < circleRect.right;
				}
			}

			return false;
		}

		void RenderNode(IFlatGuiRenderer& fgr, const ICFGSNode& node, IDesignSpace& designSpace, RenderPhase phase)
		{
			auto rect = node.GetDesignRectangle();
			GuiRect nodeRect = WorldToScreen(rect, designSpace);

			if (phase == RenderPhase::Indices)
			{
				fgr.DrawFilledRect(nodeRect, RGBAb(255,255,255,255));
				return;
			}

			bool isLit = lastHoveredNode == node.UniqueId();

			ColourSchemeQuantum nodeRectColours;
			node.Scheme().GetFillColours(OUT nodeRectColours);

			RGBAb backColour = isLit ? nodeRectColours.litColour : nodeRectColours.dullColour;

			ColourSchemeQuantum typeNameColours;
			node.Scheme().GetTypeNameColours(OUT typeNameColours);

			RGBAb lineColour = isLit ? RGBAb(192, 192, 0) : RGBAb(128, 128, 0);

			fgr.DrawRoundedRect(nodeRect, 16, backColour, lineColour);

			GuiRect namePlateRect = ComputeNamePlateRect(nodeRect);

			ColourSchemeQuantum typeNamePlateColours;
			node.Scheme().GetTypeNamePlateColours(OUT typeNamePlateColours);

			RGBAb backPlateColour = isLit ? typeNamePlateColours.litColour : typeNamePlateColours.dullColour;

			fgr.SetTextOptions(backPlateColour, isLit ? typeNameColours.litColour : typeNameColours.dullColour);

			fgr.DrawRoundedRect(namePlateRect, 16, backPlateColour, lineColour);

			GuiRect namePlateTextRect = namePlateRect;

			namePlateRect.top = (namePlateRect.bottom + namePlateRect.top) >> 1;

			fgr.DrawFilledRect(namePlateRect, backPlateColour);

			namePlateTextRect.left += 2;

			fgr.DrawText(namePlateTextRect, node.Type().Value, EFGAF_Left | EFGAF_VCentre);

			RenderSockets(fgr, node, designSpace);
		}

		void RenderCable(IFlatGuiRenderer& fgr, const ICFGSNode& start, SocketId startSocket, const ICFGSNode& end, SocketId endSocket, int cableIndex, RenderPhase phase, bool isLit)
		{		
			GuiRect circle;
			RGBAb colour;
			Vec2i startPixelPos;
			if (!TryGetSocketGeometry(start, startSocket, OUT circle, OUT startPixelPos, OUT colour, isLit))
			{
				return;
			}

			Vec2i endPixelPos;
			if (!TryGetSocketGeometry(end, endSocket, OUT circle, OUT endPixelPos, OUT colour, isLit))
			{
				return;
			}

			if (phase == RenderPhase::RGB)
			{
				fgr.DrawSpline(2, startPixelPos, { 120, 0 }, endPixelPos, { -120, 0 }, colour);
				return;
			}
			
			if (cableIndex > 65535)
			{
				// The very definition of spaghetti coding
				return;
			}

			uint32 fakeRed = cableIndex & 0x000000FF;
			uint32 fakeGreen = (cableIndex >> 8) & 0x000000FF;
			uint8 fakeBlue = 128; // 128 indicates a cable index

			fgr.DrawSpline(6, startPixelPos, { 120, 0 }, endPixelPos, { -120, 0 }, RGBAb((uint8)fakeRed, (uint8)fakeGreen, fakeBlue, 255));
		}

		void RenderCablesUnderneath(IFlatGuiRenderer& fgr, RenderPhase phase)
		{
			auto& cables = cfgs.Cables();
			for (int32 i = 0; i < cables.Count(); i++)
			{
				auto& cable = cables[i];
				auto start = cable.ExitPoint();
				auto end = cable.EntryPoint();

				auto* nodeStart = cfgs.Nodes().FindNode(start.node);
				auto* nodeEnd = cfgs.Nodes().FindNode(end.node);

				if (nodeStart && nodeEnd)
				{
					RenderCable(fgr, *nodeStart, start.socket, *nodeEnd, end.socket, i, phase, cable.IsSelected());
				}
			}
		}

		void RenderConnectionVoyage(IFlatGuiRenderer& fgr)
		{
			if (!connectionAnchor.node)
			{
				return;
			}

			auto* node = cfgs.Nodes().FindNode(connectionAnchor.node);
			if (!node)
			{
				return;
			}

			GuiRect socketRect = { -1,-1,-1,-1 };
			Vec2i edgePoint = { -1, -1 };
			RGBAb lineColour{ 0,0,0,0 };
			if (!TryGetSocketGeometry(*node, connectionAnchor.socket, OUT socketRect, OUT edgePoint, OUT lineColour, true))
			{
				return;
			}

			GuiRect parentRect = WorldToScreen(node->GetDesignRectangle(), designSpace);

			Vec2i cursorPosition = fgr.CursorPosition();

			int32 delta = 1;

			Vec2i socketCentre = Centre(socketRect);

			if (cursorPosition.x > edgePoint.x)
			{
				delta = max(120, cursorPosition.x - edgePoint.x);
			}
			else if (cursorPosition.x > socketCentre.x)
			{
				delta = IsPointInRect(cursorPosition, parentRect) ? -1 : max(120, cursorPosition.x - socketCentre.x);
			}
			else
			{
				delta = IsPointInRect(cursorPosition, parentRect) ? -1 : max(-120, socketCentre.x - cursorPosition.x);
			}

			fgr.DrawSpline(3, socketCentre, { delta, 0}, cursorPosition, {-delta,0}, lineColour);
		}

		void Render(IFlatGuiRenderer& fgr) override
		{
			RenderCablesUnderneath(fgr, RenderPhase::RGB);

			auto& nodes = cfgs.Nodes();
			for (int32 i = 0; i < nodes.Count(); ++i)
			{
				RenderNode(fgr, nodes.GetByZOrderAscending(i), designSpace, RenderPhase::RGB);
			}

			RenderConnectionVoyage(fgr);
		}

		void RenderIndices(IFlatGuiRenderer& fgr) override
		{
			RenderCablesUnderneath(fgr, RenderPhase::Indices);

			auto& nodes = cfgs.Nodes();
			for (int32 i = 0; i < nodes.Count(); ++i)
			{
				RenderNode(fgr, nodes.GetByZOrderAscending(i), designSpace, RenderPhase::Indices);
			}
		}

		bool OnCursorMove(Vec2i pixelPosition) override
		{
			if (dragId)
			{
				Vec2i delta = pixelPosition - dragStart;
				DesignerVec2 designerDelta = designSpace.ScreenDeltaToWorldDelta(delta);
				
				auto* node = cfgs.Nodes().FindNode(dragId);
				if (node)
				{
					node->SetDesignOffset(designerDelta, false);
					eventHandler.CFGSGuiEventHandler_OnNodeDragged(dragId);
				}

				return true;
			}

			if (connectionAnchor.node)
			{
				eventHandler.CFGSGuiEventHandler_OnCableLaying(connectionAnchor);
				return true;
			}

			DesignerVec2 designerPos = designSpace.ScreenToWorld(pixelPosition);

			const ICFGSNode* topMostNode = FindTopMostNodeContainingPoint(designerPos, cfgs.Nodes());
			if (topMostNode)
			{
				SetHovered(topMostNode->UniqueId());	
			}
			else
			{
				SetHovered(NodeId());
			}

			return false;
		}

		const ICFGSSocket* FindSocketAt(Vec2i screenPosition, const ICFGSNode& node) const
		{
			for (int i = 0; i < node.SocketCount(); ++i)
			{
				auto& socket = node[i];
				OUT GuiRect rect = { -1,-1,-1,-1 };
				OUT Vec2i edgePt = { -1, -1 };
				socket.GetLastGeometry(OUT rect, OUT edgePt);

				if (IsPointInRect(screenPosition, rect))
				{
					return &socket;
				}
			}

			return nullptr;
		}

		// Where the connection is anchored when placing a new cable
		CableConnection connectionAnchor;

		const ICFGSNode* FindNodeAt(Vec2i screenPosition)
		{
			auto& nodes = cfgs.Nodes();
			for (int32 i = 0; i < nodes.Count(); ++i)
			{
				auto& node = nodes.GetByZOrderDescending(i);
				auto rect = node.GetDesignRectangle();
				GuiRect nodeRect = WorldToScreen(rect, designSpace);

				if (IsPointInRect(screenPosition, nodeRect))
				{
					return &node;
				}
			}

			return nullptr;
		}

		bool OnLeftButtonDown(uint32 gridEventWheelFlags, Vec2i cursorPosition) override
		{
			UNUSED(gridEventWheelFlags);

			const ICFGSNode* node = FindNodeAt(cursorPosition);

			if (node)
			{
				cfgs.Nodes().MakeTopMost(*node);

				const ICFGSSocket* socket = FindSocketAt(cursorPosition, *node);
				if (!socket)
				{
					dragId = node->UniqueId();
					dragStart = cursorPosition;
					return true;
				}
				else
				{
					connectionAnchor.node = node->UniqueId();
					connectionAnchor.socket = socket->Id();
					return true;
				}
			}

			auto& cables = cfgs.Cables();

			RGBAb indices;
			if (designSpace.TryGetIndicesAt(cursorPosition, indices))
			{
				if (indices.blue == 128)
				{
					// Indicates a cable index
					uint32 cableIndexRed = (uint32)(uint8) indices.red;
					uint32 cableIndexGreen = (uint32)(uint8)indices.green;
					uint32 cableIndex = cableIndexRed | (cableIndexGreen << 8);
					
					bool wasChanged = false;
					cables.VisuallySelect(cableIndex, OUT wasChanged);
					
					return wasChanged;
				}

				bool wasChanged = false;
				cables.VisuallySelect(-1, OUT wasChanged);

				return wasChanged;
			}

			return false;
		}

		bool OnLeftButtonUp(uint32 gridEventWheelFlags, Vec2i cursorPosition)override
		{
			UNUSED(gridEventWheelFlags);

			if (dragId)
			{
				Vec2i delta = cursorPosition - dragStart;
				DesignerVec2 designerDelta = designSpace.ScreenDeltaToWorldDelta(delta);

				auto* node = cfgs.Nodes().FindNode(dragId);
				if (node)
				{
					node->SetDesignOffset(designerDelta, true);					
					eventHandler.CFGSGuiEventHandler_OnNodeDragged(dragId);
				}

				dragId = NodeId();

				return true;
			}

			if (connectionAnchor.node)
			{
				const ICFGSNode* entranceNode = FindNodeAt(cursorPosition);

				if (entranceNode)
				{
					auto* entranceSocket = FindSocketAt(cursorPosition, *entranceNode);
					if (entranceSocket)
					{
						cfgs.Cables().Add(connectionAnchor.node, connectionAnchor.socket, entranceNode->UniqueId(), entranceSocket->Id());
						cfgs.ConnectCablesToSockets();
					}
				}

				connectionAnchor = CableConnection();
				return true;
			}

			return false;
		}
	};
}

#include <rococo.hashtable.h>

static Rococo::stringmap<Rococo::CFGS::SocketClass> mapStringToClass;
static std::unordered_map<Rococo::CFGS::SocketClass,const char*> mapClassToString;

namespace Rococo::CFGS
{
	void PopulateSocketClass()
	{
		if (mapStringToClass.empty())
		{
			mapStringToClass.insert("None", SocketClass::None);
			mapStringToClass.insert("Trigger", SocketClass::Trigger);
			mapStringToClass.insert("Exit", SocketClass::Exit);
			mapStringToClass.insert("InputVar", SocketClass::InputVar);
			mapStringToClass.insert("OutputValue", SocketClass::OutputValue);
			mapStringToClass.insert("InputRef", SocketClass::InputRef);
			mapStringToClass.insert("ConstInputRef", SocketClass::ConstInputRef);
			mapStringToClass.insert("OutputRef", SocketClass::OutputRef);
			mapStringToClass.insert("ConstOutputRef", SocketClass::ConstOutputRef);

			for (auto& i : mapStringToClass)
			{
				mapClassToString[i.second] = i.first;
			}
		}
	}

	CFGS_MARSHALLER_API bool TryParse(OUT SocketClass& sclass, cstr text)
	{
		PopulateSocketClass();

		auto i = mapStringToClass.find(text);
		if (i != mapStringToClass.end())
		{
			OUT sclass = i->second;
			return true;
		}
		else
		{
			OUT sclass = SocketClass::None;
			return false;
		}
	}

	CFGS_MARSHALLER_API cstr ToString(SocketClass sclass)
	{
		PopulateSocketClass();

		auto i = mapClassToString.find(sclass);
		if (i != mapClassToString.end())
		{
			return i->second;
		}
		else
		{
			Throw(0, "Unknown sclass: %d", (int32)sclass);
		}
	}

	CFGS_MARSHALLER_API ICFGSGui* CreateCFGSGui(ICFGSDatabase& cfgs, IDesignSpace& designSpace, ICFGSGuiEventHandler& eventHandler)
	{
		return new Internal::CFGSGui(cfgs, designSpace, eventHandler);
	}
}

#ifdef _WIN32
# pragma comment(lib, "rococo.windows.lib")
#endif