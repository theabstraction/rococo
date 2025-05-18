#pragma once

#include <rococo.types.h>
#include <rococo.events.h>
#include <rococo.graphics.types.h>

namespace Rococo
{
	struct KeyboardEventEx;
	struct MouseEvent;
	struct Platform;

	namespace Graphics
	{
		struct IGuiRenderContext;
		enum class MaterialCategory: int32;
	}

	ROCOCO_INTERFACE IUIElement
	{
		// Route a keyboard event to the element. Returns false to redirect event to the parent element
		virtual bool OnKeyboardEvent(const KeyboardEventEx & key) = 0;
		virtual void OnRawMouseEvent(const MouseEvent& ev) = 0;
		virtual void OnMouseMove(Vec2i cursorPos, Vec2i delta, int dWheel) = 0;
		virtual void OnMouseLClick(Vec2i cursorPos, bool clickedDown) = 0;
		virtual void OnMouseRClick(Vec2i cursorPos, bool clickedDown) = 0;
		virtual void Render(Graphics::IGuiRenderContext& rc, const GuiRect& absRect) = 0;
	};

	ROCOCO_INTERFACE IBloodyPropertySetEditor : public IUIElement
	{
		virtual void AddBool(cstr name, bool* value) = 0;
		virtual void AddSpacer() = 0;
		virtual void AddFloat(cstr name, float* value, float minValue, float maxValue) = 0;
		virtual void AddFloatRange(cstr name, float* leftValue, float* rightValue, float minValue, float maxValue) = 0;
		virtual void AddInt(cstr name, bool addHexView, int* value) = 0;
		virtual void AddMaterialCategory(cstr name, cstr notifyId, Graphics::MaterialCategory* cat) = 0;
		virtual void AddMessage(cstr message) = 0;
		virtual void AddColour(cstr name, RGBAb* colour) = 0;
		virtual void AddMaterialString(cstr name, REF Graphics::MaterialId& id, cstr notifyId, char* value, size_t valueLen) = 0;
		virtual void AddPingPath(cstr name, char* value, size_t valueLen, cstr defaultSubDir, int32 width) = 0;
		virtual void AddButton(cstr name, cstr eventName) = 0;
		virtual void Clear();
	};

	ROCOCO_INTERFACE IBloodyPropertySetEditorSupervisor : public IBloodyPropertySetEditor
	{
		virtual void Free() = 0;
	};

	struct BloodyNotifyArgs
	{
		IBloodyPropertySetEditorSupervisor& bs;
		cstr sourceName;
		cstr notifyId;
	};

	IBloodyPropertySetEditorSupervisor* CreateBloodyPropertySetEditor(Platform& _platform, IEventCallback<BloodyNotifyArgs>& _onDirty, IScriptCompilationEventHandler& onCompileUIPanel);

	struct UIPopulate : public Rococo::Events::EventArgs
	{
		IUIElement* renderElement;
		cstr name;
	};
}
