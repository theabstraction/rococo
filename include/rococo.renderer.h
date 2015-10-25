#ifndef ROCOCO_RENDERER_H
#define ROCOCO_RENDERER_H

#include <rococo.types.h>
#include <rococo.maths.h>
#include <rococo.io.h>

namespace Rococo
{
	namespace Fonts
	{
		struct IDrawTextJob;
	}

	void ShowErrorBox(IException& ex, const wchar_t* caption);

	struct GuiVertex
	{
		float x;
		float y;
		float saturation; // currently ignored, meant to weigh between texture and colour
		float fontBlend; // 0 -> normal triangle, 1 -> modulate with font texture
		RGBAb colour;
		float u;
		float v;
		float unused;
	};

	struct ObjectVertex
	{
		Vec3 position;
		Vec3 normal;
		RGBAb emissiveColour;
		RGBAb diffuseColour;
	};

	struct IRenderer;

	ROCOCOAPI IGuiRenderContext // Provides draw calls - do not cache
	{
		virtual void AddTriangle(const GuiVertex triangle[3]) = 0;
		virtual void FlushLayer() = 0;

		virtual Vec2i EvalSpan(const Vec2i& pos, Fonts::IDrawTextJob& job) = 0;
		virtual void RenderText(const Vec2i& pos, Fonts::IDrawTextJob& job) = 0;
		virtual IRenderer& Renderer() = 0;
	};

	typedef size_t ID_VERTEX_SHADER;
	typedef size_t ID_PIXEL_SHADER;
	typedef int32 ID_MESH;

	ROCOCOAPI IUltraClock
	{
		virtual ticks Hz() const = 0;			// Number of ticks per seconds
		virtual ticks FrameStart() const = 0;	// The time of the current render frame
		virtual ticks Start() const = 0;		// The time at which the mainloop started
		virtual ticks FrameDelta() const = 0;	// The time between the previous frame and the current frame.
	};

	struct ObjectInstance
	{
		Matrix4x4 orientation;
		RGBA highlightColour;
	};	
	
	struct GlobalState
	{
		Matrix4x4 worldMatrixAndProj;
		Matrix4x4 worldMatrix;
	};

	ROCOCOAPI IRenderContext // Provides draw calls - do not cache
	{
		virtual void Draw(ID_MESH id, const ObjectInstance* instance, uint32 nInstances) = 0;
		virtual IRenderer& Renderer() = 0;
		virtual void SetGlobalState(const GlobalState& gs) = 0;
	};

	ROCOCOAPI IScene
	{
		virtual RGBA GetClearColour() const = 0;
		virtual void RenderGui(IGuiRenderContext& grc) = 0;
		virtual void RenderObjects(IRenderContext& rc) = 0;
	};

	ROCOCOAPI IApp
	{
		virtual void Free() = 0;
		virtual void OnCreated() = 0; // called just after construction - a 'post-constructor'
		virtual auto OnFrameUpdated(const IUltraClock& clock) -> uint32 = 0; // returns number of ms to sleep per frame as hint
		virtual void OnKeyboardEvent(const KeyboardEvent& k) = 0;
		virtual void OnMouseEvent(const MouseEvent& me) = 0;
	};

	struct GuiMetrics
	{
		Vec2i cursorPosition;
		Vec2i screenSpan;
	};

	namespace Windows
	{
		struct IWindow;
	}

	ROCOCOAPI IRenderer
	{
		virtual ID_VERTEX_SHADER CreateGuiVertexShader(const wchar_t* name, const uint8* shaderCode, size_t shaderLength) = 0;
		virtual ID_VERTEX_SHADER CreateObjectVertexShader(const wchar_t* name, const uint8* shaderCode, size_t shaderLength) = 0;
		virtual ID_PIXEL_SHADER CreatePixelShader(const wchar_t* name, const uint8* shaderCode, size_t shaderLength) = 0;

		virtual Windows::IWindow& Window() = 0;
		virtual void Render(IScene& scene) = 0;
		virtual void UseShaders(ID_VERTEX_SHADER vid, ID_PIXEL_SHADER pid) = 0;
		virtual void GetGuiMetrics(GuiMetrics& metrics) const = 0;
		
		virtual ID_MESH CreateTriangleMesh(const ObjectVertex* vertices, uint32 nVertices) = 0;
		virtual void UpdateMesh(ID_MESH rendererId, const ObjectVertex* vertices, uint32 nVertices) = 0;
	};

	ROCOCOAPI IAppFactory
	{
		virtual IApp* CreateApp(IRenderer& renderer) = 0;
	};
}

#endif