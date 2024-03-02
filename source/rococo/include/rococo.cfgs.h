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

	ROCOCO_INTERFACE ICFGSNode
	{
		virtual [[nodiscard]] CFGSNodeType Type() const = 0;
		virtual [[nodiscard]] const ICFGSSocket& operator[](int32 index) = 0;
		virtual [[nodiscard]] int32 SocketCount() const = 0;

		// Returns a copy of the designer bounds for the graph node.
		virtual [[nodiscard]] Rococo::Editors::DesignerRect GetDesignRectangle() const = 0;
	};

	ROCOCO_INTERFACE ICFGSNodeEnumerator
	{
		virtual [[nodiscard]] const ICFGSNode & operator[](int32 index) = 0;
		virtual [[nodiscard]] int32 Count() const = 0;
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

	ROCOCO_INTERFACE ICFGSRenderer
	{
		virtual void Render(Rococo::Editors::IFlatGuiRenderer & fgr, ICFGS& cfgs, Rococo::Editors::IDesignTransformations& transforms) = 0;
		virtual void Free() = 0;
	};

	CFGS_MARSHALLER_API ICFGSRenderer* CreateCFGSRenderer();
	CFGS_MARSHALLER_API ICFGSSupervisor* CreateCFGSTestSystem();
}