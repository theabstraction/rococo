#pragma once

#include <rococo.types.h>
#include <rococo.editors.h>
#include <rococo.eventargs.h>

#ifndef CFGS_MARSHALLER_API
# define CFGS_MARSHALLER_API ROCOCO_API_IMPORT
#endif

#include <rococo.ids.h>

namespace Rococo::Reflection
{
	struct IEnumDescriptor;
}

namespace Rococo::Sex::SEXML
{
	struct ISEXMLBuilder;
	struct ISEXMLDirective;
}

namespace Rococo::CFGS
{
	struct Colours
	{
		RGBAb normal;
		RGBAb hilight;
	};

	enum class SocketPlacement
	{
		Left = 0,
		Right = 1,
		Top = 2,
		Bottom = 3
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
	CFGS_MARSHALLER_API [[nodiscard]] bool IsInputClass(SocketClass x);
	CFGS_MARSHALLER_API [[nodiscard]] bool IsOutputClass(SocketClass x);
	CFGS_MARSHALLER_API [[nodiscard]] SocketClass FlipInputOutputClass(SocketClass x);

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

		[[nodiscard]] virtual RGBAb GetSocketColour(bool isLit) const = 0;
	
		// Return the face or vertex where the socket is placed. Conceptually the cable to the socket extends outwards through the specified face
		[[nodiscard]] virtual SocketPlacement Placement() const = 0;

		// Returns the designer bounds for the graph node socket, relative to the top left of the node rectangle.
		[[nodiscard]] virtual Rococo::Editors::DesignerRect GetDesignRectangle() const = 0;

		// Returns the unique typename URL for the socket
		[[nodiscard]] virtual CFGSSocketType Type() const = 0;

		[[nodiscard]] virtual SocketClass SocketClassification() const = 0;

		[[nodiscard]] virtual ICFGSNode& ParentNode() const = 0;

		// Typically the variable name
		[[nodiscard]] virtual cstr Name() const = 0;

		// A unique id assigned at socket creation that never changes
		[[nodiscard]] virtual SocketId Id() const = 0;

		// Returns the number of cables
		[[nodiscard]] virtual int CableCount() const = 0;

		// returns the cable at the given index. Throws an exception if the index is out of bounds
		[[nodiscard]] virtual CableId GetCable(int32 index) const = 0;

		// Returns the last place that the socket was computed for rendering, along with the edge point through whih its cable protrudes
		[[nodiscard]] virtual void GetLastGeometry(OUT GuiRect& lastCircleRect, OUT Vec2i& lastEdgePoint) const = 0;

		virtual void SetColours(const Colours& colours) = 0;

		// Sets the last place that the rectangle was computed for rendering.
		// [circleRect] is the rect bounding the socket circle. 
		// [edgePoint] is the point on the perimeter of the node through which the cable leading to the socket connects
		[[nodiscard]] virtual void SetLastGeometry(const GuiRect& circleRect, Vec2i edgePoint) = 0;

		// Tries to assign an opauque string keyed by the [fieldName].
		[[nodiscard]] virtual void SetField(cstr fieldName, cstr fieldValue) = 0;

		virtual void SetType(CFGSSocketType type) = 0;

		// Tries to retrieve an opaque string by field name.
		virtual bool TryGetField(cstr fieldName, Strings::IStringPopulator& populator) const = 0;

		virtual size_t EnumerateFields(Rococo::Function<void(cstr name, cstr value, size_t index)> callback) const = 0;
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
		virtual ICFGSSocket& AddSocket(cstr type, SocketClass socketClass, cstr label, SocketId id) = 0;

		virtual void AddField(cstr name, cstr value, SocketId socketId) = 0;

		virtual void SetColours(const Colours& colours, const Colours& tabColours) = 0;

		// Returns the first socket found with a matching id. If nothing is found null is returned
		[[nodiscard]] virtual ICFGSSocket* FindSocket(SocketId id) = 0;

		// Returns the first socket found with a matching name. If nothing is found null is returned
		[[nodiscard]] virtual ICFGSSocket* FindSocket(cstr name) = 0;

		// Deletes the first socket with matching id. May invalidate iteration of sockets, be careful
		virtual void DeleteSocket(SocketId id) = 0;

		// The type of node
		[[nodiscard]] virtual CFGSNodeType Type() const = 0;

		// A unique id generated at node construction that is immutable. It is *highly* unlikely any two nodes will have the same id
		[[nodiscard]] virtual NodeId Id() const = 0;

		// Returns the node at the specified index. Throws an exception if [index] is out of bounds
		[[nodiscard]] virtual ICFGSSocket& operator[](int32 index) = 0;

		// Number of sockets implemented on the node
		[[nodiscard]] virtual int32 SocketCount() const = 0;

		// The skin settings for the graph
		[[nodiscard]] virtual const IRenderScheme& Scheme() const = 0;

		// Returns a copy of the designer bounds for the graph node.
		[[nodiscard]] virtual Rococo::Editors::DesignerRect GetDesignRectangle() const = 0;

		// Force GetDesignRectangle to return the internal rect offset by the specified value. If makePermanent is set to true the internal rect is set to the old value + offset.
		virtual void SetDesignOffset(const Rococo::Editors::DesignerVec2& offset, bool makePermanent) = 0;

		[[nodiscard]] virtual Colours GetBackColours() const = 0;

		[[nodiscard]] virtual Colours GetTabColours() const = 0;

		virtual void SetLoc(const Rococo::Editors::DesignerVec2& absPosition) = 0;

		virtual void SetType(cstr type) = 0;

		virtual void SetId(NodeId id) = 0;
	};

	struct CableConnection
	{
		NodeId node;
		SocketId socket;
	};

	ROCOCO_INTERFACE ICFGSCable
	{
		// Where a cable starts from, always an exit
		[[nodiscard]] virtual CableConnection ExitPoint() const = 0;

		// Where a cable attaches to, always an entrance
		[[nodiscard]] virtual CableConnection EntryPoint() const = 0;

		// The unique id generated when the cable was created. The id never changes for the lifetime of the application, although the id does not get serialized
		[[nodiscard]] virtual CableId Id() const = 0;

		// True if is selected in the visual editor
		[[nodiscard]] virtual bool IsSelected() const = 0;
	};

	ROCOCO_INTERFACE ICFGSNodeSetBuilder
	{
		[[nodiscard]] virtual ICFGSNode& AddNode(cstr typeString, const Rococo::Editors::DesignerVec2& topLeft, NodeId id) = 0;
		virtual void DeleteNode(NodeId id) = 0;
		virtual void DeleteAllNodes() = 0;
	};

	ROCOCO_INTERFACE ICFGSNodeEnumerator
	{
		// gets a node by index. The order does not change unless nodes are added, inserted or removed.
		[[nodiscard]] virtual ICFGSNode & operator[](int32 index) = 0;

		// gets a node by zorder ascending starting with the bottom node. The order can be changed using the MakeTopMost method
		[[nodiscard]] virtual ICFGSNode& GetByZOrderAscending(int32 index) = 0;

		// gets a node by zorder descending starting with the top most node. The order can be changed using the MakeTopMost method
		[[nodiscard]] virtual ICFGSNode& GetByZOrderDescending(int32 index) = 0;

		// gives the node count, which is used in operator [] and GetByZOrderDescending
		[[nodiscard]] virtual int32 Count() const = 0;

		// finds a node with the given UniqueId
		[[nodiscard]] virtual ICFGSNode* FindNode(NodeId id) const = 0;

		// If the node argument is a member of the node set, then make it top most in z order
		virtual void MakeTopMost(const ICFGSNode& node) = 0;

		[[nodiscard]] virtual ICFGSNodeSetBuilder& Builder() = 0;
	};

	ROCOCO_INTERFACE ICFGSCableEnumerator
	{
		virtual void Add(NodeId startNodeId, SocketId startSocketId, NodeId endNodeId, SocketId endSocketId) = 0;

		// Gives the cable count, which is used to bound operator []
		[[nodiscard]] virtual int32 Count() const = 0;

		// Return the cable at the given index. If the index is out of bounds an IException is thrown
		[[nodiscard]] virtual ICFGSCable& operator[](int32 index) = 0;

		// Highlight the given cable by index. The [changed] variable is set to true if and only if this would result in an observable change.
		// If the index does not correspond to a cable then nothing is selected
		[[nodiscard]] virtual void VisuallySelect(int32 index, OUT bool& changed) = 0;
	};

	// A function contains a single graph. We could just as well have named it GraphId, but most use cases graphs are for functions
	MAKE_UNIQUE_TYPEID(FunctionId);

	ROCOCO_INTERFACE ICFGSFunction
	{
		[[nodiscard]] virtual ICFGSCableEnumerator& Cables() = 0;
		[[nodiscard]] virtual ICFGSNodeEnumerator& Nodes() = 0;

		[[nodiscard]] virtual ICFGSNode& BeginNode() = 0;
		[[nodiscard]] virtual ICFGSNode& ReturnNode() = 0;

		// Once nodes and cables are defined, call this method
		virtual void ConnectCablesToSockets() = 0;
		virtual void DeleteCable(int32 cableIndex) = 0;
		[[nodiscard]] virtual cstr Name() const = 0;
		virtual void SetName(cstr name) = 0;

		// Set which options are available for the beginNode socket types. The [typeOptions] pointer must be valid for the lifetime of property manipulation
		virtual void SetInputTypeOptions(Rococo::Reflection::IEnumDescriptor* typeOptions) = 0;

		// Set which options are available for the returnNode socket types. The [typeOptions] pointer must be valid for the lifetime of property manipulation
		virtual void SetOutputTypeOptions(Rococo::Reflection::IEnumDescriptor* typeOptions) = 0;

		[[nodiscard]] virtual FunctionId Id() const = 0;
		virtual Rococo::Reflection::IPropertyVenue& PropertyVenue() = 0;
	};

	// Interface to the control-flow graph system
	ROCOCO_INTERFACE ICFGSDatabase
	{
		virtual void Clear() = 0;
		[[nodiscard]] virtual FunctionId CreateFunction() = 0;
		virtual void DeleteFunction(FunctionId id) = 0;
		virtual void BuildFunction(FunctionId id) = 0;
		[[nodiscard]] virtual ICFGSFunction* CurrentFunction() = 0;
		[[nodiscard]] virtual ICFGSFunction* FindFunction(FunctionId id) = 0;
		virtual cstr Origin() const = 0;
		virtual void SetOrigin(cstr originName) = 0;

		// Enumerate functions. Do not delete or append to the database during the enumeration
		virtual void ForEachFunction(Rococo::Function<void(ICFGSFunction& f)> callback) = 0;
	};

	ROCOCO_INTERFACE ICFGSControllerConfig
	{
		// Retrieves the full path of the currently active cfgs file, or nullptr if none are active
		[[nodiscard]] virtual cstr ActiveFile() = 0;

		virtual bool TryLoadActiveFile(cstr filename) = 0;
	};

	ROCOCO_INTERFACE ICFGSDatabaseSupervisor : ICFGSDatabase
	{
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE ICFGSDesignerSpacePopup
	{
		[[nodiscard]] virtual bool IsVisible() const = 0;
		virtual void ShowAt(Vec2i desktopPosition, Rococo::Editors::DesignerVec2 designPosition) = 0;
		virtual void SetTemplate(FunctionId id) = 0;
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

	ROCOCO_INTERFACE ICFGSIDENavigation
	{
		virtual bool TryHandleContextMenuItem(uint16 id) = 0;
		virtual void LoadNavigation(const Rococo::Sex::SEXML::ISEXMLDirective& directive) = 0;
		virtual void SaveNavigation(Rococo::Sex::SEXML::ISEXMLBuilder& sb) = 0;
	};

	ROCOCO_INTERFACE ICFGSIntegratedDevelopmentEnvironment
	{
		virtual void Compile() = 0;
		[[nodiscard]] virtual ICFGSIDENavigation& Navigation() = 0;
		[[nodiscard]] virtual ICFGSDesignerSpacePopup& DesignerSpacePopup() = 0;
		[[nodiscard]] virtual bool IsConnectionPermitted(const Rococo::CFGS::CableConnection& anchor, const Rococo::CFGS::ICFGSSocket& target) const = 0;
	};

	ROCOCO_INTERFACE INamespaceValidator
	{
		[[nodiscard]] virtual bool IsLegalNamespace(cstr ns) const = 0;
	};

	ROCOCO_INTERFACE ICFGSIntegratedDevelopmentEnvironmentSupervisor : ICFGSIntegratedDevelopmentEnvironment
	{
		// Tells the IDE we are about to load something new, so delete all cached data
		virtual void Clear() = 0;

		virtual void Free() = 0;

		// Called just after the mainloop finishes. At this point the application is ready to exit, though objects will not have been destroyed
		// The IDE will typically implement the function to save a configuration file ready for OnInitComplete() in the next session
		virtual void OnExit() = 0;

		// Called just before main loop is invoked. At this points all of the key objects that the ide relies upon should have been initialized
		// The IDE will typically implement the function to load a configuration file containing window placement and such
		virtual void OnInitComplete() = 0;

		// Called just after a file is successfully loaded
		virtual void OnLoaded(cstr filename) = 0;
	};

	struct CableDropped : Events::EventArgs
	{
		FunctionId functionId;
		CableConnection anchor;
		Vec2i dropPoint;
		Editors::DesignerVec2 designPoint;
	};

	ROCOCO_INTERFACE ICFGSGuiEventHandler
	{
		virtual void CFGSGuiEventHandler_OnCableDropped(const CableDropped & dropInfo) = 0;
		virtual void CFGSGuiEventHandler_OnCableLaying(const CableConnection& anchor) = 0;
		virtual void CFGSGuiEventHandler_OnNodeDragged(const NodeId& id) = 0;
		virtual void CFGSGuiEventHandler_OnNodeHoverChanged(const NodeId& id) = 0;
		virtual void CFGSGuiEventHandler_OnNodeSelected(const NodeId& id) = 0;
		virtual void CFGSGuiEventHandler_PopupContextGUI(Vec2i cursorPosition) = 0;
		virtual bool CFGSGuiEventHandler_IsConnectionPermitted(const CableConnection& anchor, const ICFGSSocket& targetSocket) const = 0;
	};

	using WasHandled = bool;

	// An interface for rendering elements and manipulating them with the mouse. 
	ROCOCO_INTERFACE ICFGSGui
	{
		// Respond to cursor move event, returns true if the event is consumed
		[[nodiscard]] virtual WasHandled OnCursorMove(Vec2i cursorPosition) = 0;

		// Respond to cursor click event, returns true if the event is consumed
		[[nodiscard]] virtual WasHandled OnLeftButtonDown(uint32 buttonFlags, Vec2i cursorPosition) = 0;

		// Respond to cursor click event, returns true if the event is consumed
		[[nodiscard]] virtual WasHandled OnLeftButtonUp(uint32 buttonFlags, Vec2i cursorPosition) = 0;

		// Respond to cursor context click event, returns true if the event is consumed
		[[nodiscard]] virtual WasHandled OnRightButtonUp(uint32 buttonFlags, Vec2i cursorPosition) = 0;

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

	CFGS_MARSHALLER_API [[nodiscard]] cstr ToString(SocketPlacement placement);

	// Creates a gui for a CFGS database
	CFGS_MARSHALLER_API [[nodiscard]] ICFGSGuiSupervisor* CreateCFGSGui(ICFGSDatabase& cfgs, Rococo::Editors::IDesignSpace& designSpace, ICFGSGuiEventHandler& eventHandler);

	CFGS_MARSHALLER_API [[nodiscard]] ICFGSDatabaseSupervisor* CreateCFGSDatabase(Rococo::Events::IPublisher& publisher);

	[[nodiscard]] cstr GetCFGSAppTitle();
}