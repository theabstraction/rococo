#ifndef ROCOCO_RENDERER_H
#define ROCOCO_RENDERER_H

#include <rococo.api.h>
#include <rococo.maths.h>
#include <rococo.io.h>
#include <rococo.meshes.h>

#ifndef RENDERER_API
# define RENDERER_API ROCOCO_API_IMPORT
#endif

namespace Rococo
{
	struct IMathsVisitor;
	struct IMathsVenue;

	namespace Windows
	{
		struct IWindow;
	}

	namespace Imaging
	{
		struct IImageLoadEvents;
	}
}

namespace Rococo::Graphics
{
	struct Light;
	struct TextureDesc;
	struct ITextureLoadEnumerator;

	namespace Textures
	{
		struct IBitmapArrayBuilder;
	}

	enum CBUFFER_INDEX
	{
		CBUFFER_INDEX_GLOBAL_STATE = 0,
		CBUFFER_INDEX_CURRENT_SPOTLIGHT = 1,
		CBUFFER_INDEX_AMBIENT_LIGHT = 2,
		CBUFFER_INDEX_DEPTH_RENDER_DESC = 3,
		CBUFFER_INDEX_INSTANCE_BUFFER = 4,
		CBUFFER_INDEX_SELECT_TEXTURE_DESC = 5,
		CBUFFER_INDEX_SUNLIGHT = 6,
		CBUFFER_INDEX_BONE_MATRICES = 7
	};

	enum TXUNIT // The enum values must match the tXXX registers specified in mplat.api.hlsl
	{
		TXUNIT_FONT = 0,
		TXUNIT_SHADOW = 2,
		TXUNIT_ENV_MAP = 3,
		TXUNIT_SELECT = 4,
		TXUNIT_MATERIALS = 6,
		TXUNIT_SPRITES = 7,
		TXUNIT_GENERIC_TXARRAY = 8
	};

	namespace Fonts
	{
		struct FontSpec;
		struct IDrawTextJob;
		struct IHQTextJob;
		struct IFont;
		struct ArrayFontMetrics;
		struct IArrayFontSet;
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

	struct BoneWeight
	{
		float index;
		float weight;
	};

	struct BoneWeights
	{
		BoneWeight bone0;
		BoneWeight bone1;
	};

	struct ParticleVertex
	{
		Vec3 worldPosition;
		RGBAb colour;
		Vec4 geometry;
	};

	struct SkyVertex
	{
		Vec3 position;
	};

	typedef cstr BodyComponentMatClass;

	struct VertexTriangle
	{
		ObjectVertex a;
		ObjectVertex b;
		ObjectVertex c;
	};

	struct GuiTriangle
	{
		GuiVertex a;
		GuiVertex b;
		GuiVertex c;
	};

	struct GuiQuad
	{
		GuiVertex topLeft;
		GuiVertex topRight;
		GuiVertex bottomLeft;
		GuiVertex bottomRight;
	};

	struct IRendererMetrics;

	namespace Textures
	{
		struct BitmapLocation;
		struct IBitmapArrayBuilder;
	}

	struct MaterialArrayMetrics
	{
		int32 Width;
		int32 NumberOfElements;
	};

	enum EWindowCursor
	{
		EWindowCursor_Default = 0,
		EWindowCursor_HDrag,
		EWindowCursor_VDrag,
		EWindowCursor_HandDrag,
		EWindowCursor_BottomRightDrag
	};

	struct GuiMetrics
	{
		Vec2i cursorPosition;
		Vec2i screenSpan;
	};

	ROCOCO_INTERFACE IMaterialPalette
	{
		virtual bool TryGetMaterial(BodyComponentMatClass name, MaterialVertexData & vd) const = 0;
	};

	ROCOCO_INTERFACE IShaders
	{
		virtual ID_GEOMETRY_SHADER CreateGeometryShader(cstr pingPath) = 0;
		virtual ID_VERTEX_SHADER CreateObjectVertexShader(cstr pingPath) = 0;
		virtual ID_VERTEX_SHADER CreateParticleVertexShader(cstr pingPath) = 0;
		virtual ID_PIXEL_SHADER CreatePixelShader(cstr pingPath) = 0;
		virtual void ShowVenue(IMathsVisitor& visitor) = 0;
		virtual void UpdatePixelShader(cstr pingPath) = 0;
		virtual void UpdateVertexShader(cstr pingPath) = 0;
		virtual bool UseGeometryShader(ID_GEOMETRY_SHADER gid) = 0;
		virtual bool UseShaders(ID_VERTEX_SHADER vid, ID_PIXEL_SHADER pid) = 0;
	};

	ROCOCO_INTERFACE IMeshes
	{
		virtual void ClearMeshes() = 0;
		virtual ID_SYS_MESH CreateSkyMesh(const SkyVertex* vertices, uint32 nVertices) = 0;
		virtual ID_SYS_MESH CreateTriangleMesh(const ObjectVertex* vertices, uint32 nVertices, const BoneWeights* weights) = 0;
		virtual void DeleteMesh(ID_SYS_MESH id) = 0;
		virtual void GetMeshDesc(char desc[256], ID_SYS_MESH id) = 0;
		virtual void SetShadowCasting(ID_SYS_MESH id, boolean32 isActive) = 0;
		virtual void ShowVenue(IMathsVisitor& visitor) = 0;
		virtual void UpdateMesh(ID_SYS_MESH id, const ObjectVertex* vertices, uint32 nVertices, const BoneWeights* weights) = 0;
	};

	ROCOCO_INTERFACE ICubeTextures
	{
		virtual ID_CUBE_TEXTURE CreateCubeTexture(cstr path, cstr extension) = 0;
		virtual void SyncCubeTexture(int32 XMaxFace, int32 XMinFace, int32 YMaxFace, int32 YMinFace, int32 ZMaxFace, int32 ZMinFace) = 0;
	};

	ROCOCO_INTERFACE IMipMappedTextureArrayContainer
	{

	};

	ROCOCO_INTERFACE IMipMappedTextureArrayContainerSupervisor: IMipMappedTextureArrayContainer
	{
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE ITextureManager
	{
		virtual ID_TEXTURE CreateDepthTarget(cstr targetName, int32 width, int32 height) = 0;
		virtual ID_TEXTURE CreateRenderTarget(cstr renderTargetName, int32 width, int32 height) = 0;
		virtual void Free() = 0;
		virtual ID_TEXTURE LoadAlphaTextureArray(cstr uniqueName, Vec2i span, int32 nElements, ITextureLoadEnumerator& enumerator) = 0;
		virtual ID_TEXTURE FindTexture(cstr name) const = 0;
		virtual ICubeTextures& CubeTextures() = 0;
		virtual void SetGenericTextureArray(ID_TEXTURE id) = 0;
		virtual void ShowTextureVenue(IMathsVisitor& visitor) = 0;
		virtual int64 Size() const = 0;
		virtual bool TryGetTextureDesc(TextureDesc& desc, ID_TEXTURE id) const = 0;

		// Forward on the jpeg decompression function from the lib-jpeg lib
		virtual bool DecompressJPeg(Imaging::IImageLoadEvents& loadEvents, const unsigned char* sourceBuffer, size_t dataLengthBytes) const = 0;

		// Forward on the tiff decompression function from the lib-tiff lib
		virtual bool DecompressTiff(Imaging::IImageLoadEvents& loadEvents, const unsigned char* sourceBuffer, size_t dataLengthBytes) const = 0;

		virtual IMipMappedTextureArrayContainerSupervisor* DefineRGBATextureArray(uint32 numberOfElements, uint32 span) = 0;
	};

	ROCOCO_INTERFACE IRendererMetrics
	{
		virtual void GetGuiMetrics(GuiMetrics & metrics) const = 0;
	};

	ROCOCO_INTERFACE IHQFontResource
	{
		virtual ID_FONT CreateOSFont(Fonts::IArrayFontSet & glyphs, const Fonts::FontSpec & spec) = 0;
		virtual Vec2i EvalSpan(ID_FONT id, const fstring& text) const = 0;
		virtual const Fonts::ArrayFontMetrics& GetFontMetrics(ID_FONT idFont) = 0;
	};

	ROCOCO_INTERFACE IGuiResources
	{
		virtual const Fonts::ArrayFontMetrics & GetFontMetrics(ID_FONT idFont) = 0;
		virtual Textures::IBitmapArrayBuilder& SpriteBuilder() = 0;
		virtual Fonts::IFont& FontMetrics() = 0;
		virtual IHQFontResource& HQFontsResources() = 0;
		virtual void SetCursorBitmap(const Textures::BitmapLocation& sprite, Vec2i hotspotOffset) = 0;
		virtual void SetSysCursor(EWindowCursor id) = 0;
	};

	struct MaterialTextureArrayBuilderArgs
	{
		IBuffer& buffer;
		cstr name;
	};

	ROCOCO_INTERFACE IMaterialTextureArrayBuilder
	{
		virtual size_t Count() const = 0;
		virtual int32 TexelWidth() const = 0;
		virtual void LoadTextureForIndex(size_t index, IEventCallback<MaterialTextureArrayBuilderArgs>& onLoad) = 0;
	};

	ROCOCO_INTERFACE IMaterials
	{
		virtual void GetMaterialArrayMetrics(MaterialArrayMetrics & metrics) const = 0;
		virtual MaterialId GetMaterialId(cstr name) const = 0;
		virtual cstr GetMaterialTextureName(MaterialId id) const = 0;
		virtual void LoadMaterialTextureArray(IMaterialTextureArrayBuilder& builder) = 0;
	};

	ROCOCO_INTERFACE IGuiRenderContext // Provides draw calls - do not cache
	{
		virtual IMaterials & Materials() = 0;
		virtual void AddTriangle(const GuiVertex triangle[3]) = 0;
		virtual void DrawCustomTexturedMesh(const GuiRect& absRect, ID_TEXTURE id, cstr pixelShader, const GuiVertex* vertices, size_t nCount) = 0;
		virtual void FlushLayer() = 0;
		virtual Vec2i EvalSpan(const Vec2i& pos, Fonts::IDrawTextJob& job, const GuiRect* clipRect = nullptr) = 0;
		virtual void RenderText(const Vec2i& pos, Fonts::IDrawTextJob& job, const GuiRect* clipRect = nullptr) = 0;
		virtual IRendererMetrics& Renderer() = 0;
		virtual void SetGuiShader(cstr pixelShader) = 0;
		virtual void SetScissorRect(const Rococo::GuiRect& rect) = 0;
		virtual void ClearScissorRect() = 0;
		virtual IGuiResources& Gui() = 0;

		// Renders high quality text. To compute span without rendering, pass evaluateSpanOnly as true

		enum EMode { RENDER, EVALUATE_SPAN_ONLY, };
		virtual void RenderHQText(ID_FONT id, Fonts::IHQTextJob& job, EMode mode, const GuiRect& clipRect) = 0;
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
		Matrix4x4 projMatrix;
		GuiScale guiScale;
		Vec4 eye;
		Vec4 viewDir;
		Vec4 aspect;
	};

	ROCOCO_INTERFACE IParticles
	{
		virtual void AddFog(const ParticleVertex & p) = 0;
		void virtual AddPlasma(const ParticleVertex& p) = 0;
		void virtual ClearPlasma() = 0;
		void virtual ClearFog() = 0;
	};

	ROCOCO_INTERFACE IGui3D
	{
		virtual void Add3DGuiTriangles(const VertexTriangle * first, const VertexTriangle * last) = 0;
		virtual void Clear3DGuiTriangles() = 0;
	};

	ROCOCO_INTERFACE IRenderContext // Provides draw calls - do not cache
	{
		virtual IGui3D & Gui3D() = 0;
		virtual void Draw(ID_SYS_MESH id, const ObjectInstance* instance, uint32 nInstances) = 0;
		virtual IRenderer& Renderer() = 0;
		virtual void SetBoneMatrix(uint32 index, cr_m4x4 m) = 0;
	};

	ROCOCO_INTERFACE IUIOverlay
	{
	   virtual void Render(IGuiRenderContext & gc) = 0;
	};

	struct BoneMatrices
	{
		enum { BONE_MATRIX_CAPACITY = 16 }; // This should match that in mplat.api.hlsl
		Matrix4x4 bones[BONE_MATRIX_CAPACITY];
	};

#pragma pack(push,4)
	struct Light
	{
		Matrix4x4 worldToShadowBuffer;
		Vec4 position;
		Vec4 direction;
		Vec4 right;
		Vec4 up;
		RGBA colour = RGBA(1, 1, 1, 1);
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
		boolean32 hasCone = 0;
		Vec3 unused;
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

	ROCOCO_INTERFACE IScene2D
	{
		virtual RGBA GetClearColour() const = 0;
		virtual void OnGuiResize(Vec2i screenSpan) = 0;
		virtual void RenderGui(IGuiRenderContext& grc) = 0;
	};

	ROCOCO_INTERFACE IScene : IScene2D
	{
		virtual void GetCamera(Matrix4x4 & camera, Matrix4x4 & world, Matrix4x4 & proj, Vec4 & eye, Vec4 & viewDir) = 0;
		virtual ID_CUBE_TEXTURE GetSkyboxCubeId() const = 0;
		virtual void RenderObjects(IRenderContext& rc, bool skinned) = 0; // Do not change lights from here
		virtual const Light* GetLights(uint32& nCount) const = 0;	// Called prior to the shadow pass. 
		virtual void RenderShadowPass(const DepthRenderData& drd, IRenderContext& rc, bool skinned) = 0; // Do not change lights from here
	};

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

	enum Alignment : int32
	{
		Alignment_None = 0, 	// 0x0
		Alignment_Left = 1, 	// 0x1
		Alignment_Right = 2, 	// 0x2
		Alignment_Bottom = 4, 	// 0x4
		Alignment_Top = 8, 		// 0x8
		Alignment_Mirror = 16, 	// 0x10
		Alignment_Flip = 32, 	// 0x20
		Alignment_Clipped = 64, // 0x40
	};

	RENDERER_API Vec2i GetTopLeftPos(const GuiRect& rect, Vec2i span, int32 alignmentFlags);
	RENDERER_API Vec2 GetTopLeftPos(Vec2 pos, Vec2 span, int32 alignmentFlags);

	enum TextureFormat
	{
		TextureFormat_32_BIT_FLOAT,
		TextureFormat_RGBA_32_BIT,
		TextureFormat_UNKNOWN
	};

	struct TextureDesc
	{
		uint32 width;
		uint32 height;
		TextureFormat format;
	};

	struct TextureLoadData
	{
		cstr filename;
		const uint8* pData;
		size_t nBytes;
	};

	ROCOCO_INTERFACE ITextureLoadEnumerator
	{
		virtual void ForEachElement(IEventCallback<TextureLoadData> &callback, bool readData) = 0;
	};


	struct IHQFonts;

	ROCOCO_INTERFACE IRenderer : IRendererMetrics
	{
		virtual IGuiResources & Gui() = 0;
		virtual IMaterials& Materials() = 0;
		virtual ITextureManager& Textures() = 0;
		virtual IMeshes& Meshes() = 0;
		virtual IShaders& Shaders() = 0;
		virtual IParticles& Particles() = 0;
		virtual void ExpandViewportToEntireTexture(ID_TEXTURE depthId) = 0;
		virtual void CaptureMouse(bool enable) = 0;
		virtual ID_TEXTURE GetWindowDepthBufferId() const = 0;
		virtual IO::IInstallation& Installation() = 0;
		virtual void Render(Graphics::ENVIRONMENTAL_MAP EnvironmentalMap, IScene& scene) = 0;
		virtual void SetCursorVisibility(bool isVisible) = 0;
		virtual void SetSampler(uint32 index, Samplers::Filter, Samplers::AddressMode u, Samplers::AddressMode v, Samplers::AddressMode w, const RGBA& borderColour) = 0;
		virtual void SetSpecialAmbientShader(ID_SYS_MESH id, cstr vs, cstr ps, bool alphaBlending) = 0;
		virtual void SetSpecialSpotlightShader(ID_SYS_MESH id, cstr vs, cstr ps, bool alphaBlending) = 0;
		virtual void ShowWindowVenue(IMathsVisitor& visitor) = 0;
		virtual void SwitchToWindowMode() = 0;
		virtual IMathsVenue* TextureVenue() = 0;
		virtual Windows::IWindow& CurrentWindow() = 0;
		virtual IMathsVenue* Venue() = 0;
		virtual bool IsFullscreen() = 0;
		virtual void SwitchToFullscreen() = 0;
	};

	struct GlyphCallbackArgs
	{
		GuiRect rect;
		int32 index;
	};

	RENDERER_API void DrawTriangleFacingLeft(IGuiRenderContext& grc, const GuiRect& container, RGBAb colour);
	RENDERER_API void DrawTriangleFacingRight(IGuiRenderContext& grc, const GuiRect& container, RGBAb colour);
	RENDERER_API void DrawTriangleFacingUp(IGuiRenderContext& grc, const GuiRect& container, RGBAb colour);
	RENDERER_API void DrawTriangleFacingDown(IGuiRenderContext& grc, const GuiRect& container, RGBAb colour);

	RENDERER_API Vec2i GetScreenCentre(const GuiMetrics& metrics);
	RENDERER_API Vec2i RenderHorizontalCentredText(IGuiRenderContext& gr, cstr txt, RGBAb colour, int fontSize, const Vec2i& topMiddle);
	RENDERER_API Vec2i RenderVerticalCentredTextWithCallback(IGuiRenderContext& gr, int32 cursorPos, IEventCallback<GlyphCallbackArgs>& cb, cstr txt, RGBAb colour, int fontSize, const Vec2i& topMiddle, const GuiRect& clipRect);
	RENDERER_API Vec2i RenderVerticalCentredText(IGuiRenderContext& grc, cstr text, RGBAb colour, int fontSize, const Vec2i& middleLeft, const GuiRect* clipRect = nullptr);
	RENDERER_API Vec2i RenderTopLeftAlignedText(IGuiRenderContext& grc, cstr text, RGBAb colour, int fontSize, const Vec2i& topLeft);
	RENDERER_API Vec2i RenderTopRightAlignedText(IGuiRenderContext& grc, cstr text, RGBAb colour, int fontSize, const Vec2i& topRight);
	RENDERER_API Vec2i RenderRightAlignedText(IGuiRenderContext& grc, cstr text, RGBAb colour, int fontSize, const GuiRect& rect);
	RENDERER_API void EvalTextSpan(IGuiRenderContext& g, const fstring& text, int32 fontIndex, Vec2& pixelSpan);
	RENDERER_API void DrawRectangle(IGuiRenderContext& grc, const GuiRect& grect, RGBAb diag, RGBAb backdiag);
	RENDERER_API void DrawBorderAround(IGuiRenderContext& grc, const GuiRect& rect, const Vec2i& width, RGBAb diag, RGBAb backdiag);
	RENDERER_API void DrawLine(IGuiRenderContext& grc, int pixelthickness, Vec2i start, Vec2i end, RGBAb colour);
	RENDERER_API void RenderCentred(IGuiRenderContext& grc, ID_FONT fontId, const GuiRect& rect, cstr text, RGBAb colour);

	struct GlyphContext
	{
		GuiRect outputRect;
		uint32 unicode;
	};

	RENDERER_API GuiRect RenderHQText(const GuiRect& clipRect, int32 alignment, IGuiRenderContext& grc, ID_FONT fontId, cstr text, RGBAb colour, Vec2i spacing, IEventCallback<GlyphContext>* glyphCallback = nullptr, int dxShift = 0);
	RENDERER_API Vec2 RenderHQText_LeftAligned_VCentre(IGuiRenderContext& grc, ID_FONT fontId, const GuiRect& rect, cstr text, RGBAb colour);
	RENDERER_API Vec2 RenderHQText_LeftAligned_VCentre_WithCaret(IGuiRenderContext& grc, ID_FONT fontId, const GuiRect& rect, cstr text, RGBAb colour, int caretPos);
	RENDERER_API Vec2 RenderHQParagraph(IGuiRenderContext& grc, ID_FONT fontId, const GuiRect& rect, cstr text, RGBAb colour);
	
	struct alignas(16) StackSpaceGraphics
	{
		char opaque[256];
	};

	RENDERER_API Fonts::IDrawTextJob& CreateHorizontalCentredText(StackSpaceGraphics& ss, int fontIndex, cstr text, RGBAb _colour);
	RENDERER_API Fonts::IDrawTextJob& CreateLeftAlignedText(StackSpaceGraphics& ss, const GuiRect& targetRect, int retzone, int hypzone, int fontHeight, int fontIndex, cstr text, RGBAb colour);
	RENDERER_API float GetAspectRatio(const IRenderer& renderer);
	RENDERER_API void DrawClippedText(IGuiRenderContext& g, const Rococo::GuiRectf& rect, int32 alignmentFlags, const fstring& text, int32 fontIndex, RGBAb colour, const Rococo::GuiRectf& clipRect);
	RENDERER_API void DrawTextWithCaret(IGuiRenderContext& g, const Rococo::GuiRectf& rect, int32 alignmentFlags, const fstring& text, int32 fontIndex, RGBAb colour, const Rococo::GuiRectf& clipRect, int32 caretPos);
	RENDERER_API void DrawLeftAligned(IGuiRenderContext& g, const Rococo::GuiRectf& rect, const fstring& text, int32 fontIndex, int32 fontHeight, RGBAb colour, float32 softRightEdge, float32 hardRightEdge);
	RENDERER_API void DrawText(IGuiRenderContext& g, const GuiRectf& rect, int32 alignmentFlags, const fstring& text, int32 fontIndex, RGBAb colour);
	RENDERER_API void DrawTexture(IGuiRenderContext& grc, ID_TEXTURE id, const GuiRect& absRect);
	ROCOCO_MISC_UTILS_API void RenderBitmap_ShrinkAndPreserveAspectRatio(IGuiRenderContext& rc, MaterialId id, const GuiRect& absRect);
	ROCOCO_MISC_UTILS_API void StretchBitmap(IGuiRenderContext& rc, const Textures::BitmapLocation& location, const GuiRect& absRect);
	ROCOCO_MISC_UTILS_API void DrawSprite(const Vec2i& topLeft, const Textures::BitmapLocation& location, IGuiRenderContext& gc);
	ROCOCO_MISC_UTILS_API void DrawSpriteCentred(const GuiRect& rect, const Textures::BitmapLocation& location, IGuiRenderContext& gc);
}// Rococo::Graphics

namespace Rococo
{
	ROCOCO_INTERFACE IApp
	{
		virtual void Free() = 0;
		virtual void OnCreate() = 0;
		virtual auto OnFrameUpdated(const IUltraClock& clock)->uint32 = 0; // returns number of ms to sleep per frame as hint
		virtual void OnKeyboardEvent(const KeyboardEvent& k) = 0;
		virtual void OnMouseEvent(const MouseEvent& me) = 0;
	};
}

#endif