#ifndef ROCOCO_RENDERER_H
#define ROCOCO_RENDERER_H

#include <rococo.api.h>
#include <rococo.maths.h>
#include <rococo.io.h>

namespace Rococo
{
	namespace Fonts
	{
		struct IDrawTextJob;
	}

	struct GuiVertex
	{
		float x;
		float y;
		float saturation; // 1.0 -> use colour, 0.0 -> use bitmap texture
		float fontBlend; // 0 -> normal triangle, 1 -> modulate with font texture
		RGBAb colour;
		float u;
		float v;
		float textureIndex; // When used in sprite calls, this indexes the texture in the texture array
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

	struct VertexTriangle
	{
		ObjectVertex a;
		ObjectVertex b;
		ObjectVertex c;
	};

	struct IRenderer;

	ROCOCO_ID(ID_VERTEX_SHADER, size_t, -1)
	ROCOCO_ID(ID_PIXEL_SHADER, size_t, -1)
	ROCOCO_ID(ID_TEXTURE, size_t, -1)

	namespace Textures
	{
		struct BitmapLocation;
	}

	ROCOCOAPI IGuiRenderContext // Provides draw calls - do not cache
	{
		virtual void AddTriangle(const GuiVertex triangle[3]) = 0;
		virtual void FlushLayer() = 0;
		virtual void AddSpriteTriangle(bool alphaBlend, const GuiVertex triangle[3]) = 0;
		virtual Vec2i EvalSpan(const Vec2i& pos, Fonts::IDrawTextJob& job, const GuiRect* clipRect = nullptr) = 0;
		virtual void RenderText(const Vec2i& pos, Fonts::IDrawTextJob& job, const GuiRect* clipRect = nullptr) = 0;
		virtual IRenderer& Renderer() = 0;
		virtual auto SelectTexture(ID_TEXTURE id)->Vec2i = 0; // select texture and returns span
	};

	struct ObjectInstance
	{
		Matrix4x4 orientation;
		RGBA highlightColour;
	};

	struct GlobalLight
	{
		Vec4 lightDir;
		Vec4 lightPos;
	};

	struct GlobalState
	{
		Matrix4x4 worldMatrixAndProj;
		Matrix4x4 worldMatrix;
		GlobalLight lights[2];
	};

	ROCOCOAPI IRenderContext // Provides draw calls - do not cache
	{
		virtual void Draw(ID_SYS_MESH id, const ObjectInstance* instance, uint32 nInstances) = 0;
		virtual IRenderer& Renderer() = 0;
		virtual void SetGlobalState(const GlobalState& gs) = 0;
		virtual void SetMeshTexture(ID_TEXTURE textureId, int textureIndex) = 0;
	};

	ROCOCOAPI IUIOverlay
	{
	   virtual void Render(IGuiRenderContext& gc) = 0;
	};

	struct Light
	{
		Vec3 position;
		Vec3 direction;
		float intensity;
		RGBAb colour;
		Degrees fov;
	};

	struct DepthRenderData
	{
		Matrix4x4 worldToCamera;
		Matrix4x4 worldToScreen;
		Vec3 eye;
		Vec3 direction;
		Vec3 right;
		Vec3 up;
		float nearPlane;
		float farPlane;
		Degrees fov;
	};

	ROCOCOAPI IScene
	{
		virtual RGBA GetClearColour() const = 0;
		virtual void RenderGui(IGuiRenderContext& grc) = 0;
		virtual void RenderObjects(IRenderContext& rc) = 0; // Do not change lights from here
		virtual const Light* GetLights(size_t& nCount) const = 0;	// Called prior to the shadow pass. 
		virtual void RenderShadowPass(const DepthRenderData& drd, IRenderContext& rc) = 0; // Do not change lights from here
	};

	ROCOCOAPI IApp
	{
		virtual void Free() = 0;
		virtual auto OnFrameUpdated(const IUltraClock& clock)->uint32 = 0; // returns number of ms to sleep per frame as hint
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

	namespace Textures
	{
		struct ITextureArrayBuilder;
	}

	struct IMathsVisitor;
	struct IMathsVenue;

	ROCOCOAPI IRenderer
	{
	  virtual void AddOverlay(int zorder, IUIOverlay* overlay) = 0;
	  virtual void ClearMeshes() = 0;
	  virtual ID_SYS_MESH CreateTriangleMesh(const ObjectVertex* vertices, uint32 nVertices) = 0;
	  virtual void DeleteMesh(ID_SYS_MESH id) = 0;
	  virtual void GetGuiMetrics(GuiMetrics& metrics) const = 0;
	  virtual void GetMeshDesc(char desc[256], ID_SYS_MESH id) = 0;
	  virtual IInstallation& Installation() = 0;
	  virtual ID_TEXTURE LoadTexture(IBuffer& rawImageBuffer, cstr uniqueName) = 0;
	  virtual Textures::ITextureArrayBuilder& SpriteBuilder() = 0;
	  virtual void Render(IScene& scene) = 0;
	  virtual void RemoveOverlay(IUIOverlay* overlay) = 0;
	  virtual void SetCursorBitmap(ID_TEXTURE bitmapId, Vec2i hotspotOffset, Vec2 uvTopLeft, Vec2 uvBottomRight) = 0;
	  virtual void SetCursorVisibility(bool isVisible) = 0;
	  virtual void ShowWindowVenue(IMathsVisitor& visitor) = 0;
	  virtual void SwitchToWindowMode() = 0;
	  virtual IMathsVenue* TextureVenue() = 0;
	  virtual void UpdateMesh(ID_SYS_MESH rendererId, const ObjectVertex* vertices, uint32 nVertices) = 0;
	  virtual void UpdatePixelShader(cstr pingPath) = 0;
	  virtual void UpdateVertexShader(cstr pingPath) = 0;
	  virtual Windows::IWindow& Window() = 0;
	  virtual IMathsVenue* Venue() = 0;
	};

	namespace Graphics
	{
		Vec2i GetScreenCentre(const GuiMetrics& metrics);
		Vec2i RenderHorizontalCentredText(IGuiRenderContext& gr, cstr txt, RGBAb colour, int fontSize, const Vec2i& topMiddle);
		Vec2i RenderVerticalCentredText(IGuiRenderContext& grc, cstr text, RGBAb colour, int fontSize, const Vec2i& middleLeft);
		Vec2i RenderTopLeftAlignedText(IGuiRenderContext& grc, cstr text, RGBAb colour, int fontSize, const Vec2i& topLeft);
		Vec2i RenderTopRightAlignedText(IGuiRenderContext& grc, cstr text, RGBAb colour, int fontSize, const Vec2i& topRight);

		Vec2i RenderCentredText(IGuiRenderContext& grc, cstr text, RGBAb colour, int fontSize, const Vec2i& middle);
		void DrawRectangle(IGuiRenderContext& grc, const GuiRect& grect, RGBAb diag, RGBAb backdiag);
		void DrawBorderAround(IGuiRenderContext& grc, const GuiRect& rect, const Vec2i& width, RGBAb diag, RGBAb backdiag);
		void DrawLine(IGuiRenderContext& grc, int pixelthickness, Vec2i start, Vec2i end, RGBAb colour);

		struct alignas(16) StackSpaceGraphics
		{
			char opaque[256];
		};

		Fonts::IDrawTextJob& CreateHorizontalCentredText(StackSpaceGraphics& ss, int fontIndex, cstr text, RGBAb _colour);
		Fonts::IDrawTextJob& CreateLeftAlignedText(StackSpaceGraphics& ss, const GuiRect& targetRect, int retzone, int hypzone, int fontIndex, cstr text, RGBAb colour);
		float GetAspectRatio(const IRenderer& renderer);
		Vec2 PixelSpaceToScreenSpace(const Vec2i& v, IRenderer& renderer);

		void RenderBitmap_ShrinkAndPreserveAspectRatio(IGuiRenderContext& rc, ID_TEXTURE id, const GuiRect& absRect);
		void StretchBitmap(IGuiRenderContext& rc, const GuiRect& absRect);
		void DrawSprite(const Vec2i& position, const Textures::BitmapLocation& location, IGuiRenderContext& gc, bool alphaBlend);
	} // Graphics
} // Rococo

#endif