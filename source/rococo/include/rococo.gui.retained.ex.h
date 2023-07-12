#pragma once
#include <rococo.gui.retained.h>

// This header does everything rococo.gui.retained.h does, but adds in some templates and other items only of use for gui engine developers rather than the gui consumers

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API void LayoutChildByAnchors(IGRPanel& child, const GuiRect& parentDimensions);
	ROCOCO_GUI_RETAINED_API void LayoutChildrenByAnchors(IGRPanel& parent, const GuiRect& parentDimensions);

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

	// The platform dependent implementation of the custodian handles events and routes to the UI appropriately
	ROCOCO_INTERFACE IGRCustodian
	{
		// The caller will grab the reference to the memento and is responsible for calling IImageMemento->Free() when the memento is no longer used.
		// The debug hint may be used in error message to help narrow down the source of the error. The error message will typically display the imagePath
		virtual IGRImageMemento * CreateImageMemento(cstr debugHint, cstr imagePath) = 0;

		// Takes a platform interpreted key event and translates to an editor delta event
		virtual void TranslateToEditor(const GRKeyEvent& keyEvent, IGREditorMicromanager& manager) = 0;

		// Given a font id and text string, uses the platform font definition to determine the minimam span containing it.
		virtual Vec2i EvaluateMinimalSpan(GRFontId fontId, const fstring& text) const = 0;

		virtual void RaiseError(EGRErrorCode code, cstr function, cstr message) = 0;
	};

	ROCOCO_INTERFACE IGRCustodianSupervisor : IGRCustodian
	{
		virtual void Free() = 0;
	};

	// Rendering functions used by the widget implementations to create a standardized appearance across the UI
	ROCOCO_GUI_RETAINED_API void DrawButton(IGRPanel& panel, bool focused, bool raised, IGRRenderContext& g);
	ROCOCO_GUI_RETAINED_API void DrawMenuButton(IGRPanel& panel, bool focused, bool raised, IGRRenderContext& g);
	ROCOCO_GUI_RETAINED_API void DrawButtonText(IGRPanel& panel, GRAlignmentFlags alignment, Vec2i spacing, const fstring& text, RGBAb colour, IGRRenderContext& g);
	ROCOCO_GUI_RETAINED_API void DrawPanelBackground(IGRPanel& panel, IGRRenderContext& g);
	
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