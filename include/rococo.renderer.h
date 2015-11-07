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
		float u;
		float v;
	};

	struct IRenderer;

	ROCOCO_ID(ID_VERTEX_SHADER,size_t,-1)
	ROCOCO_ID(ID_PIXEL_SHADER, size_t, -1)
	ROCOCO_ID(ID_TEXTURE, size_t, -1)
	
	ROCOCOAPI IGuiRenderContext // Provides draw calls - do not cache
	{
		virtual void AddTriangle(const GuiVertex triangle[3]) = 0;
		virtual void FlushLayer() = 0;

		virtual Vec2i EvalSpan(const Vec2i& pos, Fonts::IDrawTextJob& job) = 0;
		virtual void RenderText(const Vec2i& pos, Fonts::IDrawTextJob& job) = 0;
		virtual IRenderer& Renderer() = 0;
		virtual auto SelectTexture(ID_TEXTURE id) -> Vec2i = 0; // select texture and returns span
	};

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
		virtual void Draw(ID_SYS_MESH id, const ObjectInstance* instance, uint32 nInstances) = 0;
		virtual IRenderer& Renderer() = 0;
		virtual void SetGlobalState(const GlobalState& gs) = 0;
		virtual void SetMeshTexture(ID_TEXTURE textureId, int textureIndex) = 0;
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
		
		virtual ID_SYS_MESH CreateTriangleMesh(const ObjectVertex* vertices, uint32 nVertices) = 0;
		virtual void UpdateMesh(ID_SYS_MESH rendererId, const ObjectVertex* vertices, uint32 nVertices) = 0;

		virtual ID_TEXTURE LoadTexture(IBuffer& rawImageBuffer, const wchar_t* uniqueName) = 0;
		virtual void SetCursorBitmap(ID_TEXTURE bitmapId, Vec2i hotspotOffset, Vec2 uvTopLeft, Vec2 uvBottomRight) = 0;
	};

	ROCOCOAPI IAppFactory
	{
		virtual IApp* CreateApp(IRenderer& renderer) = 0;
	};

	namespace Graphics
	{
		Vec2i GetScreenCentre(const GuiMetrics& metrics);
		void RenderHorizontalCentredText(IGuiRenderContext& gr, const wchar_t* txt, RGBAb colour, int fontSize, const Vec2i& topLeft);
		Vec2i RenderVerticalCentredText(IGuiRenderContext& grc, int32 x, int32 top, RGBAb colour, const wchar_t* text, int fontIndex);
		void DrawRectangle(IGuiRenderContext& grc, const GuiRect& grect, RGBAb diag, RGBAb backdiag);
		void DrawBorderAround(IGuiRenderContext& grc, const GuiRect& rect, const Vec2i& width, RGBAb diag, RGBAb backdiag);

		struct alignas(16) StackSpaceGraphics
		{
			char opaque[256];
		};

		Fonts::IDrawTextJob& CreateHorizontalCentredText(StackSpaceGraphics& ss, int fontIndex, const wchar_t* text, RGBAb _colour);
		Fonts::IDrawTextJob& CreateLeftAlignedText(StackSpaceGraphics& ss, const GuiRect& targetRect, int retzone, int hypzone, int fontIndex, const wchar_t* text, RGBAb colour);
		float GetAspectRatio(const IRenderer& renderer);
		Vec2 PixelSpaceToScreenSpace(const Vec2i& v, IRenderer& renderer);
	}
}

#endif