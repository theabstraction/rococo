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

	struct BaseVertexData
	{
		Vec2 uv;
		float fontBlend; // 0 -> normal triangle, 1 -> modulate with font texture
	};

	struct SpriteVertexData
	{
		float lerpBitmapToColour; // 1.0 -> use colour, 0.0 -> use bitmap texture
		float textureIndex; // When used in sprite calls, this indexes the texture in the texture array.
		float matIndex; // index the texture in the material array
		float textureToMatLerpFactor; // 0 -> use textureIndex, 1 -> use matIndex, lerping in between
	};

	struct GuiVertex
	{
		Vec2 pos;
		BaseVertexData vd; // 3 floats
		SpriteVertexData sd; // 4 floats
		RGBAb colour;
	};

	typedef float MaterialId;

	struct MaterialVertexData
	{
		RGBAb colour;
		MaterialId materialId;
		float gloss = 0;
	};

	struct ObjectVertex
	{
		Vec3 position;
		Vec3 normal;
		Vec2 uv;
		MaterialVertexData material;
	};

	struct ParticleVertex
	{
		Vec3 worldPosition;
		RGBAb colour;
		Vec4 geometry;
	};

	typedef cstr BodyComponentMatClass;

	ROCOCOAPI IMaterialPalette
	{
		virtual bool TryGetMaterial(BodyComponentMatClass name, MaterialVertexData& vd) const = 0;
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
	ROCOCO_ID(ID_GEOMETRY_SHADER, size_t, -1)
	ROCOCO_ID(ID_TEXTURE, size_t, -1)

	namespace Textures
	{
		struct BitmapLocation;
	}

	ROCOCOAPI IGuiRenderContext // Provides draw calls - do not cache
	{
		virtual void AddTriangle(const GuiVertex triangle[3]) = 0;
		virtual void DrawCustomTexturedMesh(const GuiRect& absRect, ID_TEXTURE id, cstr pixelShader, const GuiVertex* vertices, size_t nCount) = 0;
		virtual void FlushLayer() = 0;
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

	struct GuiScale
	{
		float OOScreenWidth;
		float OOScreenHeight;
		float OOFontWidth;
		float OOSpriteWidth;
	};

	struct GlobalState
	{
		Matrix4x4 worldMatrixAndProj;
		Matrix4x4 worldMatrix;
		GuiScale guiScale;
		Vec4 eye;
		Vec4 viewDir;
		Vec4 aspect;
	};

	ROCOCOAPI IRenderContext // Provides draw calls - do not cache
	{
		virtual void Draw(ID_SYS_MESH id, const ObjectInstance* instance, uint32 nInstances) = 0;
		virtual IRenderer& Renderer() = 0;
	};

	ROCOCOAPI IUIOverlay
	{
	   virtual void Render(IGuiRenderContext& gc) = 0;
	};

#pragma pack(push,4)
	struct Light
	{
		Matrix4x4 worldToShadowBuffer;
		Vec4 position;
		Vec4 direction;
		Vec4 right;
		Vec4 up;
		RGBA colour = RGBA(1,1,1,1);
		RGBA ambient = RGBA(0, 0, 0, 0);
		float fogConstant;
		Vec3 randoms; // 3 random quotients 0.0 - 1.0
		float cosHalfFov;
		Radians fov;
		Metres nearPlane;
		Metres farPlane;
		Seconds time; // Can be used for animation 0 - ~59.99, cycles every minute
		float cutoffCosAngle; // Angle beyond which light is severely diminished, 0.5ish to 1.0
		float cutoffPower = 16.0f; // Exponent of cutoff rate. Range 1 to 16 is cool
		float attenuationRate = -0.15f; // Point lights vary as inverse square, so 0.5 ish
	};

	struct DepthRenderData // N.B if size is not multiple of 16 bytes this will crash DX11 renderer
	{
		Matrix4x4 worldToCamera;
		Matrix4x4 worldToScreen;
		Vec4 eye;
		Vec4 direction;
		Vec4 right;
		Vec4 up;
		Metres nearPlane;
		Metres farPlane;
		Radians fov;
		Seconds time; // Can be used for animation 0 - ~59.99, cycles every minute
		Vec4 randoms; // 4 random quotients 0.0 - 1.0
	};
#pragma pack(pop)

	ROCOCOAPI IScene
	{
		virtual void GetCamera(Matrix4x4& camera, Matrix4x4& world, Vec4& eye, Vec4& viewDir) = 0;
		virtual RGBA GetClearColour() const = 0;
		virtual void RenderGui(IGuiRenderContext& grc) = 0;
		virtual void RenderObjects(IRenderContext& rc) = 0; // Do not change lights from here
		virtual const Light* GetLights(size_t& nCount) const = 0;	// Called prior to the shadow pass. 
		virtual void RenderShadowPass(const DepthRenderData& drd, IRenderContext& rc) = 0; // Do not change lights from here
	};

	ROCOCOAPI IApp
	{
		virtual void Free() = 0;
		virtual void OnCreate() = 0;
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

	struct MaterialTextureArrayBuilderArgs
	{
		IBuffer& buffer;
		cstr name;
	};

	ROCOCOAPI IMaterialTextureArrayBuilder
	{
		virtual size_t Count() const = 0;
		virtual int32 TexelWidth() const = 0;
		virtual void LoadTextureForIndex(size_t index, IEventCallback<MaterialTextureArrayBuilderArgs>& onLoad) = 0;
	};

	struct MaterialArrayMetrics
	{
		int32 Width;
		int32 NumberOfElements;
	};

	namespace Fonts
	{
		struct IFont;
	}

	namespace Samplers
	{
		enum Filter : int32
		{
			Filter_Point = 0,
			Filter_Linear = 1
		};
	
		enum AddressMode : int32
		{
			AddressMode_Border = 0,
			AddressMode_Mirror = 1,
			AddressMode_Wrap = 2,
		};
	}

	namespace Graphics
	{
		enum ENVIRONMENTAL_MAP
		{
			ENVIRONMENTAL_MAP_PROCEDURAL,
			ENVIRONMENTAL_MAP_FIXED_CUBE,
			ENVIRONMENTAL_MAP_DYNAMIC_CUBE
		};

		struct RenderPhaseConfig
		{
			ENVIRONMENTAL_MAP EnvironmentalMap;
			ID_TEXTURE renderTarget;
			ID_TEXTURE depthTarget;
			ID_TEXTURE shadowBuffer;
		};
	}

	ROCOCOAPI IRenderer
	{
	  virtual void AddOverlay(int zorder, IUIOverlay* overlay) = 0;
	  virtual void AddFog(const ParticleVertex& fog) = 0;
	  virtual void AddPlasma(const ParticleVertex& p) = 0;
	  virtual ID_TEXTURE CreateDepthTarget(int32 width, int32 height) = 0;
	  virtual ID_TEXTURE CreateRenderTarget(int32 width, int32 height) = 0;
	  virtual void ClearMeshes() = 0;
	  virtual void ClearFog() = 0;
	  virtual void ClearPlasma() = 0;
	  virtual ID_SYS_MESH CreateTriangleMesh(const ObjectVertex* vertices, uint32 nVertices) = 0;
	  virtual void DeleteMesh(ID_SYS_MESH id) = 0;
	  virtual ID_TEXTURE FindTexture(cstr name) const = 0;
	  virtual Fonts::IFont& FontMetrics() = 0;
	  virtual void GetGuiMetrics(GuiMetrics& metrics) const = 0;
	  virtual void GetMaterialArrayMetrics(MaterialArrayMetrics& metrics) const = 0;
	  virtual void GetMeshDesc(char desc[256], ID_SYS_MESH id) = 0;
	  virtual IInstallation& Installation() = 0;
	  virtual void LoadMaterialTextureArray(IMaterialTextureArrayBuilder& builder) = 0;
	  virtual MaterialId GetMaterialId(cstr name) const = 0;
	  virtual cstr GetMaterialTextureName(MaterialId id) const = 0;
	  virtual ID_TEXTURE LoadTexture(IBuffer& rawImageBuffer, cstr uniqueName) = 0;
	  virtual Textures::ITextureArrayBuilder& SpriteBuilder() = 0;
	  virtual void Render(Graphics::RenderPhaseConfig& config, IScene& scene) = 0;
	  virtual void RemoveOverlay(IUIOverlay* overlay) = 0;
	  virtual void SetCursorBitmap(const Textures::BitmapLocation& sprite, Vec2i hotspotOffset) = 0;
	  virtual void SetCursorVisibility(bool isVisible) = 0;
	  virtual void SetSampler(uint32 index, Samplers::Filter, Samplers::AddressMode u, Samplers::AddressMode v, Samplers::AddressMode w, const RGBA& borderColour) = 0;
	  virtual void SetShadowCasting(ID_SYS_MESH id, boolean32 isActive) = 0;
	  virtual void SetSpecialShader(ID_SYS_MESH id, cstr psSpotlightPingPath, cstr psAmbientPingPath, bool alphaBlending) = 0;
	  virtual void ShowWindowVenue(IMathsVisitor& visitor) = 0;
	  virtual void SyncCubeTexture(int32 XMaxFace, int32 XMinFace, int32 YMaxFace, int32 YMinFace, int32 ZMaxFace, int32 ZMinFace) = 0;
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
		struct GlyphCallbackArgs
		{
			GuiRect rect;
			int32 index;
		};

		void DrawTriangleFacingLeft(IGuiRenderContext& grc, const GuiRect& container, RGBAb colour);
		void DrawTriangleFacingRight(IGuiRenderContext& grc, const GuiRect& container, RGBAb colour);

		Vec2i GetScreenCentre(const GuiMetrics& metrics);
		Vec2i RenderHorizontalCentredText(IGuiRenderContext& gr, cstr txt, RGBAb colour, int fontSize, const Vec2i& topMiddle);
		Vec2i RenderVerticalCentredTextWithCallback(IGuiRenderContext& gr, int32 cursorPos, IEventCallback<GlyphCallbackArgs>& cb, cstr txt, RGBAb colour, int fontSize, const Vec2i& topMiddle, const GuiRect& clipRect);
		Vec2i RenderVerticalCentredText(IGuiRenderContext& grc, cstr text, RGBAb colour, int fontSize, const Vec2i& middleLeft, const GuiRect* clipRect = nullptr);
		Vec2i RenderTopLeftAlignedText(IGuiRenderContext& grc, cstr text, RGBAb colour, int fontSize, const Vec2i& topLeft);
		Vec2i RenderTopRightAlignedText(IGuiRenderContext& grc, cstr text, RGBAb colour, int fontSize, const Vec2i& topRight);
		Vec2i RenderRightAlignedText(IGuiRenderContext& grc, cstr text, RGBAb colour, int fontSize, const GuiRect& rect);

		Vec2i RenderCentredText(IGuiRenderContext& grc, cstr text, RGBAb colour, int fontSize, const Vec2i& middle, const GuiRect* clipRect = nullptr);
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

		void RenderBitmap_ShrinkAndPreserveAspectRatio(IGuiRenderContext& rc, MaterialId id, const GuiRect& absRect);
		void StretchBitmap(IGuiRenderContext& rc, const GuiRect& absRect);
		void DrawSprite(const Vec2i& topLeft, const Textures::BitmapLocation& location, IGuiRenderContext& gc);
		void DrawSpriteCentred(const GuiRect& rect, const Textures::BitmapLocation& location, IGuiRenderContext& gc);
	} // Graphics
} // Rococo

#endif