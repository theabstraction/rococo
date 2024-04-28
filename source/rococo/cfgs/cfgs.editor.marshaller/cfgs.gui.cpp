#include <rococo.cfgs.h>
#include <rococo.maths.h>
#include <rococo.strings.h>

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

	enum class RenderPhase
	{
		RGB,
		Indices
	};

	// The implementation is designed to be minimalist, it merely renders the nodes and cables and allows the mouse to drag cables and nodes.
	class CFGSGui : public ICFGSGuiSupervisor
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

		void ComputeLeftSocketGeometry(const ICFGSNode& node, const ICFGSSocket& socket, IDesignSpace& designSpace, int yIndex)
		{
			DesignerRect designerParentRect = node.GetDesignRectangle();
			GuiRect parentRect = WorldToScreen(designerParentRect, designSpace);

			DesignerVec2 designerSocketTopLeft { designerParentRect.left, designerParentRect.top + yIndex * 20 };
			Vec2i topLeft = designSpace.WorldToScreen(designerSocketTopLeft) + Vec2i{ 6, 30 };

			int diameter = 20;

			GuiRect circleRect{ topLeft.x, topLeft.y, topLeft.x + diameter, topLeft.y + diameter };
			GuiRect socketTextRect{ circleRect.right, circleRect.top, (parentRect.right + parentRect.left) / 2, circleRect.bottom };

			Vec2i edgePoint = { parentRect.left, (circleRect.top + circleRect.bottom) / 2 };
			socket.SetLastGeometry(circleRect, edgePoint);
		}

		enum
		{
			PADDING_SOCKET_TEXT_RECT_CENTRE = 12
		};

		void RenderLeftSocket(IFlatGuiRenderer& fgr, const ICFGSNode& node, const ICFGSSocket& socket, IDesignSpace& designSpace)
		{
			GuiRect circleRect{ -1,-1,-1,-1 };
			Vec2i edgePoint{ -1, -1 };
			socket.GetLastGeometry(OUT circleRect, OUT edgePoint);

			DesignerRect designerParentRect = node.GetDesignRectangle();
			GuiRect parentRect = WorldToScreen(designerParentRect, designSpace);

			GuiRect socketTextRect{ circleRect.right, circleRect.top, (parentRect.right + parentRect.left) / 2 - PADDING_SOCKET_TEXT_RECT_CENTRE, circleRect.bottom };

			Vec2i cursorPos = fgr.CursorPosition();
			bool isLit = IsPointInRect(cursorPos, circleRect);

			ShrinkRect(circleRect, 2);
			fgr.DrawCircle(circleRect, socket.GetSocketColour(isLit), 2, RGBAb(0, 0, 0, 0));

			fgr.SetTextOptions(RGBAb(0, 0, 0, 0), RGBAb(255, 255, 255, 255));

			cstr name = socket.Name();

			int pixelWidth = fgr.MeasureText(name);
			if (pixelWidth >= Width(socketTextRect))
			{
				char subname[8];
				Strings::SafeFormat(subname, "%.3s...", name);
				fgr.DrawText(socketTextRect, subname, EFGAF_Left | EFGAF_VCentre);
			}
			else
			{
				fgr.DrawText(socketTextRect, socket.Name(), EFGAF_Left | EFGAF_VCentre);
			}

			if (socket.CableCount() > 0)
			{
				fgr.MoveLineStartTo(Centre(circleRect));
				fgr.DrawLineTo({ parentRect.left, Centre(circleRect).y }, socket.GetSocketColour(false), 2);

				ShrinkRect(circleRect, 4);
				fgr.DrawCircle(circleRect, socket.GetSocketColour(isLit), 2, RGBAb(0, 0, 0, 0));
			}
		}

		void ComputeRightSocketGeometry(const ICFGSNode& node, const ICFGSSocket& socket, IDesignSpace& designSpace, int yIndex)
		{
			DesignerRect designerParentRect = node.GetDesignRectangle();
			GuiRect parentRect = WorldToScreen(designerParentRect, designSpace);

			int32 socketTop = parentRect.top + 30;

			GuiRect circleRect{ parentRect.right - 26, socketTop + yIndex * 20, parentRect.right - 6, socketTop + (yIndex + 1) * 20 };
			GuiRect socketTextRect{ circleRect.right, circleRect.top, (parentRect.right + parentRect.left) / 2, circleRect.bottom };

			Vec2i edgePoint = { parentRect.right, (circleRect.top + circleRect.bottom) / 2 };
			socket.SetLastGeometry(circleRect, edgePoint);
		}

		void RenderRightSocket(IFlatGuiRenderer& fgr, const ICFGSNode& node, const ICFGSSocket& socket, IDesignSpace& designSpace)
		{
			GuiRect circleRect{ -1,-1,-1,-1 };
			Vec2i edgePoint{ -1, -1 };
			socket.GetLastGeometry(OUT circleRect, OUT edgePoint);

			DesignerRect designerParentRect = node.GetDesignRectangle();
			GuiRect parentRect = WorldToScreen(designerParentRect, designSpace);
			
			GuiRect socketTextRect{ (parentRect.right + parentRect.left) / 2 + PADDING_SOCKET_TEXT_RECT_CENTRE, circleRect.top, circleRect.left, circleRect.bottom };

			Vec2i cursorPos = fgr.CursorPosition();
			bool isLit = IsPointInRect(cursorPos, circleRect);

			ShrinkRect(circleRect, 2);
			fgr.DrawCircle(circleRect, socket.GetSocketColour(isLit), 2, RGBAb(0, 0, 0, 0));

			fgr.SetTextOptions(RGBAb(0, 0, 0, 0), RGBAb(255, 255, 255, 255));

			cstr name = socket.Name();

			int pixelWidth = fgr.MeasureText(name);
			if (pixelWidth >= Width(socketTextRect))
			{
				char subname[8];
				Strings::SafeFormat(subname, "%.3s...", name);
				fgr.DrawText(socketTextRect, subname, EFGAF_Right | EFGAF_VCentre);
			}
			else
			{
				fgr.DrawText(socketTextRect, socket.Name(), EFGAF_Right | EFGAF_VCentre);
			}

			if (socket.CableCount() > 0)
			{
				fgr.MoveLineStartTo(Centre(circleRect));
				fgr.DrawLineTo({ parentRect.right, Centre(circleRect).y }, socket.GetSocketColour(false), 2);

				ShrinkRect(circleRect, 4);
				fgr.DrawCircle(circleRect, socket.GetSocketColour(isLit), 2, RGBAb(0, 0, 0, 0));
			}
		}

		void RenderSockets(IFlatGuiRenderer& fgr, const ICFGSNode& node, IDesignSpace& designSpace)
		{
			for (int i = 0; i < node.SocketCount(); ++i)
			{				
				auto& socket = node[i];
				if (IsLeftSide(socket))
				{
					RenderLeftSocket(fgr, node, socket, designSpace);
				}
				else
				{
					RenderRightSocket(fgr, node, socket, designSpace);
				}
			}
		}

		void ComputeSocketGeometry(const ICFGSNode& node, IDesignSpace& designSpace)
		{
			int32 leftIndex = 0;
			int32 rightIndex = 0;

			for (int i = 0; i < node.SocketCount(); ++i)
			{
				auto& socket = node[i];
				if (IsLeftSide(socket))
				{
					ComputeLeftSocketGeometry(node, socket, designSpace, leftIndex++);
				}
				else
				{
					ComputeRightSocketGeometry(node, socket, designSpace, rightIndex++);
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
					lineColour = socket.GetSocketColour(isLit);
					return circleRect.left < circleRect.right;
				}
			}

			return false;
		}

		cstr GetFirstAlphaNumericOnRight(cstr qualifiedIdentifier)
		{
			cstr end = qualifiedIdentifier + Strings::StringLength(qualifiedIdentifier);
			cstr rhs = end - 1;

			while (rhs > qualifiedIdentifier)
			{
				if (Strings::IsAlphaNumeric(*rhs))
				{
					rhs--;
				}
				else
				{
					rhs++;
					break;
				}
			}

			if (rhs >= qualifiedIdentifier && rhs < end)
			{
				return rhs;
			}
			else
			{
				return qualifiedIdentifier;
			}
		}

		bool Abbreviate(char* buffer, size_t sizeofBuffer, cstr text)
		{
			cstr src = text;

			char* dest = buffer;
			char* end = buffer + sizeofBuffer;

			while (*src != 0 && dest < end)
			{
				if (Strings::IsCapital(*src) || Strings::IsNumeric(*src))
				{
					*dest++ = *src++;
				}
				else
				{
					src++;
				}
			}

			end[-1] = 0;
			if (dest < end)
			{
				*dest = 0;
			}

			return *buffer != 0;
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

			bool isLit = lastHoveredNode == node.Id();

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

			cstr text = node.Type().Value;

			int width = fgr.MeasureText(text);

			if (width < Width(namePlateTextRect))
			{
				fgr.DrawText(namePlateTextRect, node.Type().Value, EFGAF_Left | EFGAF_VCentre);
			}
			else
			{
				cstr rhs = GetFirstAlphaNumericOnRight(text);
				width = fgr.MeasureText(rhs);
				if (width < Width(namePlateTextRect))
				{
					fgr.DrawText(namePlateTextRect, rhs, EFGAF_Left | EFGAF_VCentre);
				}
				else
				{
					char abbreviations[10];
					if (Abbreviate(abbreviations, sizeof abbreviations, rhs))
					{
						width = fgr.MeasureText(abbreviations);
						if (width < Width(namePlateTextRect))
						{
							fgr.DrawText(namePlateTextRect, abbreviations, EFGAF_Left | EFGAF_VCentre);
						}
						else
						{
							fgr.DrawText(namePlateTextRect, "...", EFGAF_Left | EFGAF_VCentre);
						}
					}
				}
			}

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
				fgr.DrawSpline(2, startPixelPos, { 60, 0 }, endPixelPos, { -60, 0 }, colour);
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

			fgr.DrawSpline(6, startPixelPos, { 60, 0 }, endPixelPos, { -60, 0 }, RGBAb((uint8)fakeRed, (uint8)fakeGreen, fakeBlue, 255));
		}

		void RenderCablesUnderneath(IFlatGuiRenderer& fgr, RenderPhase phase)
		{
			auto* f = cfgs.CurrentFunction();
			if (!f)
			{
				return;
			}

			auto& cables = f->Cables();
			for (int32 i = 0; i < cables.Count(); i++)
			{
				auto& cable = cables[i];
				auto start = cable.ExitPoint();
				auto end = cable.EntryPoint();

				auto* nodeStart = f->Nodes().FindNode(start.node);
				auto* nodeEnd = f->Nodes().FindNode(end.node);

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

			auto* f = cfgs.CurrentFunction();
			if (!f)
			{
				return;
			}

			auto* node = f->Nodes().FindNode(connectionAnchor.node);
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
			auto* f = cfgs.CurrentFunction();
			if (!f)
			{
				return;
			}

			auto& nodes = f->Nodes();
			for (int32 i = 0; i < nodes.Count(); ++i)
			{
				ComputeSocketGeometry(nodes[i], designSpace);
			}

			RenderCablesUnderneath(fgr, RenderPhase::RGB);

			for (int32 i = 0; i < nodes.Count(); ++i)
			{
				RenderNode(fgr, nodes.GetByZOrderAscending(i), designSpace, RenderPhase::RGB);
			}

			RenderConnectionVoyage(fgr);
		}

		void RenderIndices(IFlatGuiRenderer& fgr) override
		{
			auto* f = cfgs.CurrentFunction();
			if (!f)
			{
				return;
			}

			RenderCablesUnderneath(fgr, RenderPhase::Indices);

			auto& nodes = f->Nodes();
			for (int32 i = 0; i < nodes.Count(); ++i)
			{
				RenderNode(fgr, nodes.GetByZOrderAscending(i), designSpace, RenderPhase::Indices);
			}
		}

		WasHandled OnCursorMove(Vec2i pixelPosition) override
		{
			auto* f = cfgs.CurrentFunction();
			if (!f)
			{
				return false;
			}

			if (dragId)
			{
				Vec2i delta = pixelPosition - dragStart;
				DesignerVec2 designerDelta = designSpace.ScreenDeltaToWorldDelta(delta);
				
				auto* node = f->Nodes().FindNode(dragId);
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

			const ICFGSNode* topMostNode = FindTopMostNodeContainingPoint(designerPos, f->Nodes());
			if (topMostNode)
			{
				SetHovered(topMostNode->Id());	
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
			auto* f = cfgs.CurrentFunction();
			if (!f)
			{
				return nullptr;
			}

			auto& nodes = f->Nodes();
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

		// Tries to locate the cable at the screen pixel location, and also emits the cable index for the appropriate cable.
		// In the event that no cable lies under the screen co-ordinate the index is set to -1 and nullptr is returned
		const ICFGSCable* FindCableAt(Vec2i screenPosition, OUT int32& index)
		{
			auto* f = cfgs.CurrentFunction();
			if (!f)
			{
				index = -1;
				return nullptr;
			}

			auto& cables = f->Cables();

			RGBAb indices;
			if (designSpace.TryGetIndicesAt(screenPosition, indices))
			{
				if (indices.blue == 128)
				{
					// Indicates a cable index
					uint32 cableIndexRed = (uint32)(uint8)indices.red;
					uint32 cableIndexGreen = (uint32)(uint8)indices.green;
					uint32 cableIndex = cableIndexRed | (cableIndexGreen << 8);

					if (cableIndex < (uint32)cables.Count())
					{
						OUT index = (int32)cableIndex;
						return &cables[cableIndex];
					}
				}
			}

			OUT index = -1;
			return nullptr;
		}

		WasHandled OnLeftButtonDown(uint32 buttonFlags, Vec2i cursorPosition) override
		{
			UNUSED(buttonFlags);

			auto* f = cfgs.CurrentFunction();
			if (!f)
			{
				return false;
			}

			const ICFGSNode* node = FindNodeAt(cursorPosition);

			if (node)
			{
				f->Nodes().MakeTopMost(*node);

				const ICFGSSocket* socket = FindSocketAt(cursorPosition, *node);
				if (!socket)
				{
					dragId = node->Id();
					dragStart = cursorPosition;
					return true;
				}
				else
				{
					connectionAnchor.node = node->Id();
					connectionAnchor.socket = socket->Id();
					return true;
				}
			}

			auto& cables = f->Cables();

			int32 foundIndex = -1;
			const ICFGSCable* cable = FindCableAt(cursorPosition, OUT foundIndex);

			bool wasChanged = false;
			cables.VisuallySelect(cable ? foundIndex : -1, OUT wasChanged);
			return cable != nullptr;
		}

		WasHandled OnLeftButtonUp(uint32 buttonFlags, Vec2i cursorPosition) override
		{
			auto* f = cfgs.CurrentFunction();
			if (!f)
			{
				return false;
			}

			if (connectionAnchor.node)
			{
				const ICFGSNode* entranceNode = FindNodeAt(cursorPosition);

				if (entranceNode)
				{
					auto* entranceSocket = FindSocketAt(cursorPosition, *entranceNode);
					if (entranceSocket)
					{
						bool isPermitted = eventHandler.CFGSGuiEventHandler_IsConnectionPermitted(connectionAnchor, *entranceSocket);
						if (isPermitted)
						{
							f->Cables().Add(connectionAnchor.node, connectionAnchor.socket, entranceNode->Id(), entranceSocket->Id());
							f->ConnectCablesToSockets();
						}
					}
				}
				else
				{
					// Potentially we are dragging some cable that may imply a set of useful endpoints (such as an interface that implies a connection to a method).

					CableDropped cableDrop;
					cableDrop.functionId = f->Id();
					cableDrop.anchor = connectionAnchor;
					cableDrop.dropPoint = cursorPosition;
					cableDrop.designPoint = designSpace.ScreenToWorld(cursorPosition);
					eventHandler.CFGSGuiEventHandler_OnCableDropped(cableDrop);
				}

				connectionAnchor = CableConnection();
				return true;
			}

			if (dragId)
			{
				Vec2i delta = cursorPosition - dragStart;
				DesignerVec2 designerDelta = designSpace.ScreenDeltaToWorldDelta(delta);

				auto* node = f->Nodes().FindNode(dragId);
				if (node)
				{
					node->SetDesignOffset(designerDelta, true);					
					eventHandler.CFGSGuiEventHandler_OnNodeDragged(dragId);
					eventHandler.CFGSGuiEventHandler_OnNodeSelected(dragId);
				}

				dragId = NodeId();

				return true;
			}

			eventHandler.CFGSGuiEventHandler_OnNodeSelected(NodeId());

			if (HasFlag(KEY_HELD_FLAG_CTRL, buttonFlags))
			{
				int32 foundIndex = -1;
				auto* cable = FindCableAt(cursorPosition, OUT foundIndex);
				if (cable)
				{
					f->DeleteCable(foundIndex);
					return true;
				}
			}

			return false;
		}

		WasHandled OnRightButtonUp(uint32 buttonFlags, Vec2i cursorPosition) override
		{
			UNUSED(buttonFlags);

			auto* f = cfgs.CurrentFunction();
			if (!f)
			{
				return false;
			}

			if (dragId || connectionAnchor.node)
			{
				return false;
			}

			const ICFGSNode* node = FindNodeAt(cursorPosition);
			if (node)
			{
				return false;
			}

			int32 foundIndex = -1;
			auto* cable = FindCableAt(cursorPosition, OUT foundIndex);
			if (cable)
			{
				return false;
			}

			eventHandler.CFGSGuiEventHandler_PopupContextGUI(cursorPosition);

			return false;
		}
	};
}

#include <rococo.hashtable.h>

static Rococo::stringmap<Rococo::CFGS::SocketClass> mapStringToClass =
{
	{ "None", SocketClass::None },
	{ "Trigger", SocketClass::Trigger },
	{ "Exit", SocketClass::Exit },
	{ "InputVar", SocketClass::InputVar },
	{ "OutputValue", SocketClass::OutputValue },
	{ "InputRef", SocketClass::InputRef },
	{ "ConstInputRef", SocketClass::ConstInputRef },
	{ "OutputRef", SocketClass::OutputRef },
	{ "ConstOutputRef", SocketClass::ConstOutputRef }
};

static std::unordered_map<Rococo::CFGS::SocketClass,const char*> mapClassToString;

static std::unordered_map<Rococo::CFGS::SocketPlacement, const char*> mapPlacementToString = 
{
	{ Rococo::CFGS::SocketPlacement::Left, "Left" },
	{ Rococo::CFGS::SocketPlacement::Right, "Right" },
	{ Rococo::CFGS::SocketPlacement::Top, "Top" },
	{ Rococo::CFGS::SocketPlacement::Bottom, "Bottom" },
};

namespace Rococo::CFGS
{
	void PopulateSocketClass()
	{
		if (mapClassToString.empty())
		{
			for (auto& i : mapStringToClass)
			{
				mapClassToString[i.second] = i.first;
			}
		}
	}

	CFGS_MARSHALLER_API bool TryParse(OUT SocketClass& sclass, cstr text)
	{
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
		auto i = mapClassToString.find(sclass);
		if (i != mapClassToString.end())
		{
			return i->second;
		}
		else
		{
			PopulateSocketClass();

			i = mapClassToString.find(sclass);
			if (i != mapClassToString.end())
			{
				return i->second;
			}

			Throw(0, "Unknown sclass: %d", (int32)sclass);
		}
	}

	CFGS_MARSHALLER_API cstr ToString(SocketPlacement placement)
	{
		auto i = mapPlacementToString.find(placement);
		if (i != mapPlacementToString.end())
		{
			return i->second;
		}
		else
		{
			Throw(0, "Unknown placement: %d", (int32)placement);
		}
	}

	CFGS_MARSHALLER_API ICFGSGuiSupervisor* CreateCFGSGui(ICFGSDatabase& cfgs, IDesignSpace& designSpace, ICFGSGuiEventHandler& eventHandler)
	{
		return new Internal::CFGSGui(cfgs, designSpace, eventHandler);
	}
}

#ifdef _WIN32
# pragma comment(lib, "rococo.windows.lib")
#endif