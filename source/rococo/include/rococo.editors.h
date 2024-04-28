#pragma once

#include <rococo.types.h>

namespace Rococo::Reflection
{
	struct IPropertyVenue;
	struct IPropertyEditor;
	struct IEstateAgent;
}

namespace Rococo::Editors
{
	// Interface for controlling an ensemble of property editors. These are generally invoked outside the containing window class to populate and read visual controls from associated properties
	ROCOCO_INTERFACE IUIPropertiesEditor
	{
		// Invoke VisitVenue on the venue using the internal property builder
		virtual void BuildEditorsForProperties(Reflection::IPropertyVenue& venue) = 0;

		// Delete all properties from the editor
		virtual void Clear() = 0;

		// Tells the UI system to attempt to validate and copy data from the visual editor for the specified property to the venue
		virtual void UpdateFromVisuals(Reflection::IPropertyEditor& sourceEditor, Reflection::IPropertyVenue& targetVenue) = 0;

		// Try to get the latest edited string for the given property
		[[nodiscard]] virtual bool TryGetEditorString(cstr propertyIdentifier, OUT Rococo::Strings::HString& value) = 0;

		// Tell the editor that an agent's property has changed and it should update the associated editor/view to reflect the change
		virtual void Refresh(cstr onlyThisPropertyId, Reflection::IEstateAgent& sourceAgent) = 0;
	};

	// Supervisor interface for controlling an ensemble of property editors. Generally these methods are called by the host window (see CreatePropertiesEditor)
	ROCOCO_INTERFACE IUIPropertiesEditorSupervisor : IUIPropertiesEditor
	{
		virtual void AdvanceSelection(UI::SysWidgetId id) = 0;
		virtual void Free() = 0;
		virtual void LayouVertically() = 0;
		virtual void NavigateByTabFrom(UI::SysWidgetId id, int delta) = 0;
		virtual void OnButtonClicked(UI::SysWidgetId id) = 0;
		virtual void OnEditorChanged(UI::SysWidgetId id) = 0;
		virtual void OnEditorLostKeyboardFocus(UI::SysWidgetId id) = 0;
	};

	enum EKeyHeldFlags
	{
		KEY_HELD_FLAG_NONE = 0,
		KEY_HELD_FLAG_CTRL = 0x0008,
		KEY_HELD_FLAG_LBUTTON = 0x0001,
		KEY_HELD_FLAG_R_RBUTTON = 0x0002,
		KEY_HELD_FLAG_M_BUTTON = 0x0010,
		KEY_HELD_FLAG_SHIFT = 0x0004
	};

	enum EFlatGuiAlignmentFlags
	{
		EFGAF_None,
		EFGAF_Left = 0x0000,
		EFGAF_Right = 0x0002,
		EFGAF_Top = 0x0000,
		EFGAF_Bottom = 0x0008,
		EFGAF_VCentre = 0x010
	};

	// Provides methods for rendering to a fairly boring flat shaded GUI system, such as the Win32 GDI API
	ROCOCO_INTERFACE IFlatGuiRenderer
	{
		virtual void DrawCircle(const GuiRect& rect, RGBAb edgeColour, int thickness, RGBAb fillColour) = 0;
		virtual void DrawLineTo(Vec2i pos, RGBAb edgeColour, int thickness) = 0;
		virtual void DrawText(const GuiRect & rect, cstr text, uint32 flatGuiAlignmentFlags) = 0;
		virtual void DrawFilledRect(const GuiRect& rect, RGBAb colour) = 0;
		virtual void DrawRoundedRect(const GuiRect& rect, int border, RGBAb backColour, RGBAb lineColour) = 0;
		virtual void DrawSpline(int thickness, Vec2i start, Vec2i startDirection, Vec2i end, Vec2i endDirection, RGBAb colour) = 0;
		virtual int  MeasureText(cstr text) = 0;
		virtual void MoveLineStartTo(Vec2i pos) = 0;

		virtual void SetTextOptions(RGBAb backColour, RGBAb textColour) = 0;

		virtual Vec2i CursorPosition() const = 0;
		virtual Vec2i Span() const = 0;
	};

	ROCOCO_INTERFACE IUI2DGridEvents
	{
		// Indicates the back button was released while the control had focus. Typically used to delete nodes
		virtual void GridEvent_OnBackReleased() = 0;

		// Triggered when the control wheel (such as the mouse wheel) rotates a definite number of clicks. 
		// The buttonFlags is a combination of EKeyHeldFlags bits
		virtual void GridEvent_OnControlWheelRotated(int32 clicks, uint32 buttonFlags, Vec2i cursorPosition) = 0;

		// Triggered when the cursor moves within the grid (or even outside when the cursor is captured) 
		// The buttonFlags is a combination of EKeyHeldFlags bits
		virtual void GridEvent_OnCursorMove(uint32 buttonFlags, Vec2i cursorPosition) = 0;

		// Triggered when the default activate button of the cursor control (such as the left mouse button) is held down when the cursor is over the grid area
		// The buttonFlags is a combination of EKeyHeldFlags bits
		virtual void GridEvent_OnLeftButtonDown(uint32 buttonFlags, Vec2i cursorPosition) = 0;

		// Triggered when the default activate button of the cursor control (such as the left mouse button) is released when the cursor is over the grid area
		// The buttonFlags is a combination of EKeyHeldFlags bits
		virtual void GridEvent_OnLeftButtonUp(uint32 buttonFlags, Vec2i cursorPosition) = 0;

		// Triggered when the default activate button of the context control (such as the right mouse button) is released when the cursor is over the grid area
		// The buttonFlags is a combination of EKeyHeldFlags bits
		virtual void GridEvent_OnRightButtonUp(uint32 buttonFlags, Vec2i cursorPosition) = 0;

		// Provides a renderer object for painting over the grid
		virtual void GridEvent_PaintForeground(IFlatGuiRenderer& renderer) = 0;

		// Provides a renderer object for painting indices in the index bitmap. This bitmap is not visible, but provides GetPixelAt(...) functionality for detecting entities. Useful for collision detection and item selection
		virtual void GridEvent_PaintForegroundIndices(IFlatGuiRenderer& renderer) = 0;
	};

	// Design vector. Designer grids in this namespace use double precision components
	struct DesignerVec2
	{
		double x;
		double y;
	};

	// Double precision rectangle, with bottom >= top and right >= left. It represents a rectangle in designer co-ordinates
	struct DesignerRect
	{
		double left;
		double top;
		double right;
		double bottom;

		ROCOCO_WINDOWS_API bool Contains(const DesignerVec2& pt);
	};

	// This interface exposes methods for converting between design co-ordinates (with double precision components) to screen co-ordinates (pixels/texels) with int32 components, and getting at the index buffer for the representation
	ROCOCO_INTERFACE IDesignSpace
	{
		virtual	Vec2i WorldToScreen(const DesignerVec2& designPos) const = 0;
		virtual DesignerVec2 ScreenToWorld(Vec2i pixelPos) const = 0;
		virtual DesignerVec2 ScreenDeltaToWorldDelta(Vec2i pixelDelta) const = 0;

		// Retrieve the indices from the index buffer for the screen view. 
		// N.B the index buffer is separate from the colour buffer. The implementation, purpose and behaviour of the index buffer is dependent on the architecture and beyond the scope of documentation here.
		virtual bool TryGetIndicesAt(Vec2i pixelPos, OUT RGBAb& indices) const = 0;
	};

	ROCOCO_INTERFACE IUI2DGridSlate
	{
		virtual double ScaleFactor() const = 0;
		virtual void SetScaleFactor(double newValue) = 0;
		virtual void SetHorizontalDomain(double left, double right) = 0;
		virtual void SetVerticalDomain(double top, double bottom) = 0;
		virtual void SetCentrePosition(const DesignerVec2& pos) = 0;
		virtual void SetSmallestGradation(double gradationDelta) = 0;

		// Begins a drag operation on the grid, using the specified pixel position as the start co-ordinate
		virtual void BeginDrag(Vec2i referencePixelPosition) = 0;

		// Ends a drag operation on the grid, using the specified pixel position as the end co-ordinate
		virtual void EndDrag(Vec2i referencePixelPosition) = 0;

		// Returns true if a dragging operation is in progress
		virtual bool IsDragging() const = 0;

		// Temporarily updates the visuals of the grid, using the specified pixel position as the preview co-ordinate
		virtual void PreviewDrag(Vec2i referencePixelPosition) = 0;

		// Tells the underlying widget system that cursor button and move events should be directed to the grid and not outside of it
		virtual void CaptureCursorInput() = 0;

		// Tells the underlying widget system that cursor events outside of the grid should not be routed to the grid any longer
		virtual void ReleaseCapture() = 0;

		virtual IDesignSpace& DesignSpace() = 0;

		virtual Vec2i GetDesktopPositionFromGridPosition(Vec2i gridPosition) = 0;
	};

	ROCOCO_WINDOWS_API GuiRect WorldToScreen(const DesignerRect& designerRect, IDesignSpace& designSpace);
	ROCOCO_WINDOWS_API DesignerRect ScreenToWorld(const GuiRect& designerRect, IDesignSpace& designSpace);

	ROCOCO_INTERFACE IUI2DGridSlateSupervisor : IUI2DGridSlate
	{
		virtual void Free() = 0;
		virtual void QueueRedraw() = 0;
		virtual void ResizeToParent() = 0;
	};
}

namespace Rococo::Windows
{
	struct IParentWindowSupervisor;

	// Create a properties editor window, hosted by the propertiesPanelArea. The host needs to respond to window events and invoke IUIPropertiesEditorSupervisor method appropriately
	// An example is given in rococo.abstract.editor\abstract.editor.window.cpp
	ROCOCO_WINDOWS_API Editors::IUIPropertiesEditorSupervisor* CreatePropertiesEditor(IParentWindowSupervisor& propertiesPanelArea, Events::IPublisher& publisher);
	ROCOCO_WINDOWS_API Editors::IUI2DGridSlateSupervisor* Create2DGrid(IParentWindowSupervisor& gridArea, uint32 style, Editors::IUI2DGridEvents& eventHandler, bool useDoubleBuffering);
}