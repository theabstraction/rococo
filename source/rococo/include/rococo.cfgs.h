#pragma once

#include <rococo.types.h>
#include <rococo.editors.h>

#ifndef CFGS_MARSHALLER_API
# define CFGS_MARSHALLER_API ROCOCO_API_IMPORT
#endif

#include <rococo.ids.h>

namespace Rococo::Sex::SEXML
{
	struct ISEXMLBuilder;
	struct ISEXMLDirective;
}

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

	CFGS_MARSHALLER_API [[nodiscard]] bool TryParse(OUT SocketClass& sclass, cstr text);

	struct CFGSSocketType
	{
		cstr Value;
	};

	struct ICFGSNode;

	// socket identifier - no two sockets on a graph will have the same id
	MAKE_UNIQUE_TYPEID(SocketId)

	// cable identifier - no two cables on a graph will have the same id
	MAKE_UNIQUE_TYPEID(CableId)

	// node identifier - no two nodes on a graph will have the same id
	MAKE_UNIQUE_TYPEID(NodeId)

	ROCOCO_INTERFACE ICFGSSocket
	{
		virtual void AddCable(CableId id) = 0;

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

		// A unique id assigned at socket creation that never changes
		virtual [[nodiscard]] SocketId Id() const = 0;

		// Returns the number of cables
		virtual [[nodiscard]] int CableCount() const = 0;

		// returns the cable at the given index. Throws an exception if the index is out of bounds
		virtual [[nodiscard]] CableId GetCable(int32 index) const = 0;

		// Returns the last place that the socket was computed for rendering, along with the edge point through whih its cable protrudes
		virtual [[nodiscard]] void GetLastGeometry(OUT GuiRect& lastCircleRect, OUT Vec2i& lastEdgePoint) const = 0;

		// Sets the last place that the rectangle was computed for rendering.
		// Note we use mutable data and a const function. But since geometry will be const between most frames, its not so bad
		// [circleRect] is the rect bounding the socket circle. 
		// [edgePoint] is the point on the perimeter of the node through which the cable leading to the socket connects
		virtual [[nodiscard]] void SetLastGeometry(const GuiRect& circleRect, Vec2i edgePoint) const = 0;
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

	ROCOCO_INTERFACE ICFGSNode
	{
		// Returns the first socket found with a matching id. If nothing is found null is returned
		virtual ICFGSSocket* FindSocket(SocketId id) = 0;

		// Returns the first socket found with a matching name. If nothing is found null is returned
		virtual ICFGSSocket* FindSocket(cstr name) = 0;

		// The type of node
		virtual [[nodiscard]] CFGSNodeType Type() const = 0;

		// A unique id generated at node construction that is immutable. It is *highly* unlikely any two nodes will have the same id
		virtual [[nodiscard]] NodeId UniqueId() const = 0;

		// Returns the node at the specified index. Throws an exception if [index] is out of bounds
		virtual [[nodiscard]] const ICFGSSocket& operator[](int32 index) const = 0;

		// Number of sockets implemented on the node
		virtual [[nodiscard]] int32 SocketCount() const = 0;

		// The skin settings for the graph
		virtual [[nodiscard]] const IRenderScheme& Scheme() const = 0;

		// Returns a copy of the designer bounds for the graph node.
		virtual [[nodiscard]] Rococo::Editors::DesignerRect GetDesignRectangle() const = 0;

		// Force GetDesignRectangle to return the internal rect offset by the specified value. If makePermanent is set to true the internal rect is set to the old value + offset.
		virtual void SetDesignOffset(const Rococo::Editors::DesignerVec2& offset, bool makePermanent) = 0;
	};

	struct CableConnection
	{
		NodeId node;
		SocketId socket;
	};

	ROCOCO_INTERFACE ICFGSCable
	{
		// Where a cable starts from, always an exit
		virtual [[nodiscard]] CableConnection ExitPoint() const = 0;

		// Where a cable attaches to, always an entrance
		virtual [[nodiscard]] CableConnection EntryPoint() const = 0;

		// The unique id generated when the cable was created. The id never changes for the lifetime of the application, although the id does not get serialized
		virtual [[nodiscard]] CableId Id() const = 0;

		// True if is selected in the visual editor
		virtual [[nodiscard]] bool IsSelected() const = 0;
	};

	ROCOCO_INTERFACE ICFGSNodeBuilder
	{
		virtual void AddSocket(cstr type, SocketClass socketClass, cstr label, SocketId id) = 0;
	};

	ROCOCO_INTERFACE ICFGSNodeSetBuilder
	{
		virtual [[nodiscard]] ICFGSNodeBuilder& AddNode(cstr typeString, const Rococo::Editors::DesignerVec2& topLeft, NodeId id) = 0;
		virtual void DeleteAllNodes() = 0;
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

		virtual [[nodiscard]] ICFGSNodeSetBuilder& Builder() = 0;
	};

	ROCOCO_INTERFACE ICFGSCableEnumerator
	{
		virtual void Add(NodeId startNodeId, SocketId startSocketId, NodeId endNodeId, SocketId endSocketId) = 0;

		// Gives the cable count, which is used to bound operator []
		virtual [[nodiscard]] int32 Count() const = 0;

		// Return the cable at the given index. If the index is out of bounds an IException is thrown
		virtual [[nodiscard]] const ICFGSCable& operator[](int32 index) const = 0;

		// Highlight the given cable by index. The [changed] variable is set to true if and only if this would result in an observable change.
		// If the index does not correspond to a cable then nothing is selected
		virtual [[nodiscard]] void VisuallySelect(int32 index, OUT bool& changed) = 0;
	};

	// A function contains a single graph. We could just as well have named it GraphId, but most use cases graphs are for functions
	MAKE_UNIQUE_TYPEID(FunctionId);

	ROCOCO_INTERFACE ICFGSFunction
	{
		virtual [[nodiscard]] ICFGSCableEnumerator& Cables() = 0;
		virtual [[nodiscard]] ICFGSNodeEnumerator& Nodes() = 0;

		// Once nodes and cables are defined, call this method
		virtual void ConnectCablesToSockets() = 0;
		virtual void DeleteCable(int32 cableIndex) = 0;
		virtual cstr Name() const = 0;
		virtual void SetName(cstr name) = 0;
		virtual FunctionId Id() const = 0;
	};

	// Interface to the control-flow graph system
	ROCOCO_INTERFACE ICFGSDatabase
	{
		virtual void Clear() = 0;
		virtual FunctionId CreateFunction() = 0;
		virtual void DeleteFunction(FunctionId id) = 0;
		virtual void BuildFunction(FunctionId id) = 0;
		virtual ICFGSFunction* CurrentFunction() = 0;
		virtual ICFGSFunction* FindFunction(FunctionId id) = 0;

		// Enumerate functions. Do not delete or append to the database during the enumeration
		virtual void ForEachFunction(Rococo::Function<void(ICFGSFunction& f)> callback) = 0;
	};

	ROCOCO_INTERFACE ICFGSDatabaseSupervisor: ICFGSDatabase
	{
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE ICFGSDesignerSpacePopup
	{
		virtual bool IsVisible() const = 0;
		virtual void ShowAt(Vec2i desktopPosition, Rococo::Editors::DesignerVec2 designPosition) = 0;
		virtual void Hide() = 0;;
	};

	ROCOCO_INTERFACE ICFGSDesignerSpacePopupSupervisor: ICFGSDesignerSpacePopup
	{
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE ICFGArchiver
	{
		virtual void Archiver_OnSaveNavigation(Rococo::Sex::SEXML::ISEXMLBuilder& sb) = 0;
	};

	ROCOCO_INTERFACE ICFGSLoader
	{
		virtual void Loader_OnLoadNavigation(const Rococo::Sex::SEXML::ISEXMLDirective& directive) = 0;
	};

	ROCOCO_INTERFACE ICFGSIntegratedDevelopmentEnvironment
	{
		virtual [[nodiscard]] ICFGSDesignerSpacePopup& DesignerSpacePopup() = 0;
		virtual [[nodiscard]] bool IsConnectionPermitted(const Rococo::CFGS::CableConnection& anchor, const Rococo::CFGS::ICFGSSocket& target) const = 0;
		virtual void LoadNavigation(const Rococo::Sex::SEXML::ISEXMLDirective& directive) = 0;
		virtual void SaveNavigation(Rococo::Sex::SEXML::ISEXMLBuilder& sb) = 0;
		virtual [[nodiscard]] bool TryHandleContextMenuItem(uint16) = 0;
	};

	ROCOCO_INTERFACE ICFGSIntegratedDevelopmentEnvironmentSupervisor : ICFGSIntegratedDevelopmentEnvironment
	{
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE ICFGSGuiEventHandler
	{
		virtual void CFGSGuiEventHandler_OnCableLaying(const CableConnection& anchor) = 0;
		virtual void CFGSGuiEventHandler_OnNodeDragged(const NodeId& id) = 0;
		virtual void CFGSGuiEventHandler_OnNodeHoverChanged(const NodeId& id) = 0;
		virtual void CFGSGuiEventHandler_PopupContextGUI(Vec2i cursorPosition) = 0;
		virtual bool CFGSGuiEventHandler_IsConnectionPermitted(const CableConnection& anchor, const ICFGSSocket& targetSocket) const = 0;
	};


	// An interface for rendering elements and manipulating them with the mouse. 
	ROCOCO_INTERFACE ICFGSGui
	{
		// Respond to cursor move event, returns true if the event is consumed
		virtual [[nodiscard]] bool OnCursorMove(Vec2i cursorPosition) = 0;

		// Respond to cursor click event, returns true if the event is consumed
		virtual [[nodiscard]] bool OnLeftButtonDown(uint32 buttonFlags, Vec2i cursorPosition) = 0;

		// Respond to cursor click event, returns true if the event is consumed
		virtual [[nodiscard]] bool OnLeftButtonUp(uint32 buttonFlags, Vec2i cursorPosition) = 0;

		// Respond to cursor context click event, returns true if the event is consumed
		virtual [[nodiscard]] bool OnRightButtonUp(uint32 buttonFlags, Vec2i cursorPosition) = 0;

		// This is used to paint the RGB elements to the screen. The caller will typically render to a bitmap then periodically blit it to the screen
		virtual void Render(Rococo::Editors::IFlatGuiRenderer & fgr) = 0;

		// This is used to paint indices to an index buffer to enable easy detection of gui objects under the mouse cursor
		virtual void RenderIndices(Rococo::Editors::IFlatGuiRenderer& fgr) = 0;
	};

	ROCOCO_INTERFACE ICFGSGuiSupervisor : ICFGSGui
	{
		virtual void Free() = 0;
	};

	CFGS_MARSHALLER_API [[nodiscard]] cstr ToString(SocketClass sclass);

	// Creates a gui for a CFGS database
	CFGS_MARSHALLER_API [[nodiscard]] ICFGSGuiSupervisor* CreateCFGSGui(ICFGSDatabase& cfgs, Rococo::Editors::IDesignSpace& designSpace, ICFGSGuiEventHandler& eventHandler);

	CFGS_MARSHALLER_API [[nodiscard]] ICFGSDatabaseSupervisor* CreateCFGSDatabase();

	const wchar_t* GetCFGSAppTitle();
}