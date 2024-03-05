#pragma once

#include <rococo.types.h>
#include <rococo.editors.h>

#ifndef CFGS_MARSHALLER_API
# define CFGS_MARSHALLER_API ROCOCO_API_IMPORT
#endif

namespace Rococo::CFGS
{
	enum class SocketPlacement
	{
		Left = 0,
		Right = 1,
		Top = 2,
		bottom = 3
	};

	enum class SocketClass
	{
		// An indeterminate socket. Used by APIs to indicate an invalid or undefined socket
		None,

		// The node is executed by sending a signal on the trigger
		Trigger,

		// When the node terminates the exit socket triggers the cable to the next node
		Exit,

		// A variable that provides input
		InputVar,

		// A variable that provides output
		OutputValue,

		// A mutable primitive variable
		InputRef,

		// An imutable primitive variable
		ConstInputRef,

		// A variable that provides a reference to a mutable output structure
		OutputRef,

		// A variable that provides a reference to an imutable output structure
		ConstOutputRef
	};

	struct CFGSSocketType
	{
		cstr Value;
	};

	struct ICFGSNode;

	ROCOCO_INTERFACE ICFGSSocket
	{
		// Return the face or vertex where the socket is placed. Conceptually the cable to the socket extends outwards through the specified face
		virtual [[nodiscard]] SocketPlacement Placement() const = 0;

		// Returns the designer bounds for the graph node socket, relative to the top left of the node rectangle.
		virtual [[nodiscard]] Rococo::Editors::DesignerRect GetDesignRectangle() const = 0;

		// Returns the unique typename URL for the socket
		virtual [[nodiscard]] CFGSSocketType Type() const = 0;

		virtual [[nodiscard]] SocketClass SocketClassification() const = 0;

		virtual [[nodiscard]] ICFGSNode& ParentNode() const = 0;

		// Typically the variable name
		virtual [[nodiscard]] cstr Name() const = 0;
	};

	struct CFGSNodeType
	{
		cstr Value;
	};

	struct ColourSchemeQuantum
	{
		// The background colour for the entity
		RGBAb dullColour;

		// The background colour for the entity when highlighted
		RGBAb litColour;
	};

	ROCOCO_INTERFACE IRenderScheme
	{
		virtual void GetFillColours(OUT ColourSchemeQuantum & q) const = 0;
		virtual void GetTypeNameColours(OUT ColourSchemeQuantum& q) const = 0;
		virtual void GetTypeNamePlateColours(OUT ColourSchemeQuantum& q) const = 0;
	};

	struct NodeId
	{
		enum { UNIQUE_BUFFER_LEN = 16 };

		NodeId()
		{
			subValues.iValues[0] = subValues.iValues[1] = 0;
		}

		union UTypes
		{
			char bufValue[UNIQUE_BUFFER_LEN];
			int64 iValues[2];
		} subValues;

		bool operator == (const NodeId& other) const
		{
			bool result = this->subValues.iValues[0] == other.subValues.iValues[0] && this->subValues.iValues[1] == other.subValues.iValues[1];
			return result;
		}

		bool operator != (const NodeId& other) const
		{
			return !(*this == other);
		}

		operator bool() const
		{
			return subValues.iValues[0] != 0 || subValues.iValues[1] != 0;
		}
	};

	ROCOCO_INTERFACE ICFGSNode
	{
		virtual [[nodiscard]] CFGSNodeType Type() const = 0;

		// A unique id generated at node construction that is immutable. It is *highly* unlikely any two nodes will have the same id
		virtual [[nodiscard]] NodeId UniqueId() const = 0;
		virtual [[nodiscard]] const ICFGSSocket& operator[](int32 index) const = 0;
		virtual [[nodiscard]] int32 SocketCount() const = 0;
		virtual [[nodiscard]] const IRenderScheme& Scheme() const = 0;

		// Returns a copy of the designer bounds for the graph node.
		virtual [[nodiscard]] Rococo::Editors::DesignerRect GetDesignRectangle() const = 0;

		// Force GetDesignRectangle to return the internal rect offset by the specified value. If makePermanent is set to true the internal rect is set to the old value + offset.
		virtual void SetDesignOffset(const Rococo::Editors::DesignerVec2& offset, bool makePermanent) = 0;
	};

	ROCOCO_INTERFACE ICFGSNodeEnumerator
	{
		// gets a node by index. The order does not change unless nodes are added, inserted or removed.
		virtual [[nodiscard]] const ICFGSNode & operator[](int32 index) = 0;

		// gets a node by zorder ascending starting with the bottom node. The order can be changed using the MakeTopMost method
		virtual [[nodiscard]] const ICFGSNode& GetByZOrderAscending(int32 index) = 0;

		// gets a node by zorder descending starting with the top most node. The order can be changed using the MakeTopMost method
		virtual [[nodiscard]] const ICFGSNode& GetByZOrderDescending(int32 index) = 0;

		// gives the node count, which is used in operator [] and GetByZOrderDescending
		virtual [[nodiscard]] int32 Count() const = 0;

		// finds a node with the given UniqueId
		virtual [[nodiscard]] const ICFGSNode* FindNode(NodeId id) const = 0;

		// finds a node with the given UniqueId
		virtual [[nodiscard]] ICFGSNode* FindNode(NodeId id) = 0;

		// If the node argument is a member of the node set, then make it top most in z order
		virtual void MakeTopMost(const ICFGSNode& node) = 0;
	};

	// Interface to the control-flow graph system
	ROCOCO_INTERFACE ICFGS
	{
		virtual ICFGSNodeEnumerator & Nodes() = 0;
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE ICFGSSupervisor: ICFGS
	{
		virtual ICFGSNodeEnumerator & Nodes() = 0;
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE ICFGSGuiEventHandler
	{
		virtual void CFGSGuiEventHandler_OnNodeDragged(const NodeId& id) = 0;
		virtual void CFGSGuiEventHandler_OnNodeHoverChanged(const NodeId& id) = 0;
	};

	ROCOCO_INTERFACE ICFGSGui
	{
		// Respond to cursor move event, returns true if the event is consumed
		virtual bool OnCursorMove(Vec2i cursorPosition) = 0;

		// Respond to cursor click event, returns true if the event is consumed
		virtual bool OnLeftButtonDown(uint32 gridEventWheelFlags, Vec2i cursorPosition) = 0;

		// Respond to cursor click event, returns true if the event is consumed
		virtual bool OnLeftButtonUp(uint32 gridEventWheelFlags, Vec2i cursorPosition) = 0;

		virtual void Render(Rococo::Editors::IFlatGuiRenderer & fgr) = 0;

		virtual void Free() = 0;
	};

	CFGS_MARSHALLER_API ICFGSGui* CreateCFGSGui(ICFGS& cfgs, Rococo::Editors::IDesignTransformations& transforms, ICFGSGuiEventHandler& eventHandler);
	CFGS_MARSHALLER_API ICFGSSupervisor* CreateCFGSTestSystem();
}