#pragma once
#include <rococo.gui.retained.h>

// This header does everything rococo.gui.retained.h does, but adds in some templates and other items only of use for gui engine developers rather than the gui consumers

namespace Rococo::Gui
{
	ROCOCO_INTERFACE IGREventHandler
	{
		virtual EGREventRouting OnGREvent(GRWidgetEvent & ev) = 0;
	};

	// Lifetime manager for a gui retained instance
	ROCOCO_INTERFACE IGRSystemSupervisor : IGRSystem
	{
		// Assigns a new event handler, and returns the old one
		virtual IGREventHandler* SetEventHandler(IGREventHandler* eventHandler) = 0;
		virtual void NotifyPanelDeleted(int64 uniqueId) = 0;
		virtual EGREventRouting OnGREvent(GRWidgetEvent& ev) = 0;
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE IGRPanelRootSupervisor : IGRPanelRoot
	{
		virtual void DeferRendering(IGRPanelSupervisor& panel) = 0;
	};

	// This is the key factory function that creates the Gui system. The custodian handles the platform dependent side of the GUI.
	ROCOCO_GUI_RETAINED_API IGRSystemSupervisor* CreateGRSystem(GRConfig& config, IGRCustodian& custodian);

	// Implemented by an editor widget, IGRCustodian will map key events to the method calls here to adjust the content of an editor 
	ROCOCO_INTERFACE IGREditorMicromanager
	{
		virtual void AddToCaretPos(int32 delta) = 0;
		virtual void AppendCharAtCaret(char c) = 0;
		virtual void BackspaceAtCaret() = 0;
		virtual int32 CaretPos() const = 0;
		virtual void DeleteAtCaret() = 0;
		virtual void Return() = 0;
		virtual int32 GetTextAndLength(char* buffer, int32 receiveCapacity) const = 0;
	};

	ROCOCO_INTERFACE IGRKeyState
	{
		virtual bool IsCtrlPressed() const = 0;
	};

	// The platform dependent implementation of the custodian handles events and routes to the UI appropriately
	ROCOCO_INTERFACE IGRCustodian
	{
		virtual IGRImageSupervisor* CreateImageFromPath(cstr debugHint, cstr imagePath) = 0;

		// Takes a platform interpreted key event and translates to an editor delta event
		virtual void TranslateToEditor(const GRKeyEvent& keyEvent, IGREditorMicromanager& manager) = 0;

		// Given a font id and text string, uses the platform font definition to determine the minimam span containing it.
		virtual Vec2i EvaluateMinimalSpan(GRFontId fontId, const fstring& text) const = 0;

		// Implementation specific error handling. Further in the Rococo libs we just throw an exception,
		// but not everyone likes exceptions. Generally the error handler should invoke a breakpoint, issue a report and terminate the app.
		virtual void RaiseError(const Rococo::Sex::ISExpression* associatedSExpression, EGRErrorCode code, cstr function, cstr format, ...) = 0;

		virtual IGRFonts& Fonts() = 0;

		virtual IGRKeyState& Keys() = 0;
	};

	ROCOCO_INTERFACE IGRCustodianSupervisor : IGRCustodian
	{
		virtual void Free() = 0;
	};

	// Rendering functions used by the widget implementations to create a standardized appearance across the UI
	ROCOCO_GUI_RETAINED_API void DrawButton(IGRPanel& panel, bool focused, bool raised, IGRRenderContext& g);
	ROCOCO_GUI_RETAINED_API void DrawMenuButton(IGRPanel& panel, const GuiRect& rect, bool focused, bool raised, IGRRenderContext& g);
	ROCOCO_GUI_RETAINED_API void DrawButtonText(IGRPanel& panel, GRAlignmentFlags alignment, Vec2i spacing, const fstring& text, RGBAb colour, RGBAb shadowColour, GRFontId fontId, IGRRenderContext& g, Vec2i shadowOffset = {1,1});
	ROCOCO_GUI_RETAINED_API void DrawPanelBackground(IGRPanel& panel, IGRRenderContext& g);
	ROCOCO_GUI_RETAINED_API void DrawPanelBackgroundEx(IGRPanel& panel, IGRRenderContext& g, EGRSchemeColourSurface back, EGRSchemeColourSurface leftEdge, EGRSchemeColourSurface rightEdge, float alphaScale = 1.0f);
	ROCOCO_GUI_RETAINED_API void DrawGameOptionBackground(IGRWidgetText& title, IGRPanel& panel, IGRRenderContext& rc);
	// Dynamic casting methods
	ROCOCO_GUI_RETAINED_API [[nodiscard]] bool DoInterfaceNamesMatch(cstr a, cstr b);

	// Query to see if the particular interface is part of the supplied instance. Will only compile if there is an elementary derivation of GR_TARGET_INTERFACE from GRBASED_CLASS.
	template<typename GR_TARGET_INTERFACE, class GRBASED_CLASS> inline EGRQueryInterfaceResult QueryForParticularInterface(GRBASED_CLASS* instance, IGRBase** ppOutputArg, cstr interfaceId)
	{
		if (ppOutputArg) *ppOutputArg = nullptr;

		if (!interfaceId || *interfaceId == 0) return EGRQueryInterfaceResult::INVALID_ID;

		if (DoInterfaceNamesMatch(interfaceId, GR_TARGET_INTERFACE::InterfaceId()))
		{
			auto* target = static_cast<GR_TARGET_INTERFACE*>(instance);
			if (target)
			{
				if (ppOutputArg) *ppOutputArg = target;
				return EGRQueryInterfaceResult::SUCCESS;
			}
		}

		return EGRQueryInterfaceResult::NOT_IMPLEMENTED;
	}
}