#ifndef ROCOCO_RENDERER_H
# define ROCOCO_RENDERER_H

#include <rococo.api.h>
#include <rococo.maths.h>
//#include <rococo.io.h>
#include <rococo.meshes.h>
#include <rococo.renderer.types.h>

#ifndef RENDERER_API
# define RENDERER_API ROCOCO_API_IMPORT
#endif

#ifndef ROCOCO_GRAPHICS_API
# define ROCOCO_GRAPHICS_API ROCOCO_API_IMPORT
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

namespace Rococo::RAL
{
	struct IRAL;
	struct IPipeline;
	struct RALMeshBuffer;
}

namespace Rococo::Graphics
{
	enum class TextureFormat : uint32;

	ROCOCO_INTERFACE IMaterialPalette
	{
		virtual bool TryGetMaterial(BodyComponentMatClass name, MaterialVertexData & vd) const = 0;
	};

	// A mapping between interface names and the class names that implement them
	// For example 'refShaderModel' might map to 'ShaderModel16Samples' for high quality soft shadows
	ROCOCO_INTERFACE IShaderOptions
	{
		virtual size_t NumberOfOptions() const = 0;

		// Retrieve the options. Do not cache the pointers, consume them before significant API calls that may change the options
		virtual void GetOption(size_t index, OUT cstr& interfaceName, OUT cstr& className) = 0;
	};

	enum class VertexElementFormat : uint32
	{
		// One 32-bit float
		Float1,

		// Two 32-bit floats
		Float2,

		// Three 32-bit floats
		Float3,

		// Four 32-bit floats
		Float4,

		// 32-bit rgba colour value with four unsigned 8-bit components
		RGBA8U
	};

	struct VertexElement
	{
		cstr semanticName;
		uint32 semanticIndex;
		VertexElementFormat format;
		uint32 slot;
	};

	ROCOCO_INTERFACE IShaders
	{
		virtual ID_GEOMETRY_SHADER CreateGeometryShader(cstr pingPath) = 0;
		// Create a vertex shader based on the vertex element description. The terminating element must have SemanticName of null
		virtual ID_VERTEX_SHADER CreateVertexShader(cstr pingPath, const VertexElement* elements) = 0;
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
		virtual RAL::RALMeshBuffer& GetBuffer(ID_SYS_MESH id) = 0;
	};

	ROCOCO_INTERFACE ICubeTextures
	{
		virtual ID_CUBE_TEXTURE CreateCubeTexture(cstr path, cstr extension) = 0;
		virtual ID_CUBE_TEXTURE CreateSkyboxCubeIdFromMaterialArrayIndices(int32 XMaxFace, int32 XMinFace, int32 YMaxFace, int32 YMinFace, int32 ZMaxFace, int32 ZMinFace) = 0;
	};

	struct TextureArrayCreationFlags
	{
		boolean32 allowMipMapGeneration : 1;
		boolean32 allowCPUread : 1;
	};

	ROCOCO_INTERFACE ITextureManager
	{
		virtual void AssignToPS(uint32 unitId, ID_TEXTURE texture) = 0;
		virtual void SetRenderTarget(ID_TEXTURE depthTarget, ID_TEXTURE renderTarget) = 0;

		virtual ID_TEXTURE CreateDepthTarget(cstr targetName, int32 width, int32 height) = 0;
		virtual ID_TEXTURE CreateRenderTarget(cstr renderTargetName, int32 width, int32 height, TextureFormat format) = 0;
		virtual void Free() = 0;
		virtual ID_TEXTURE LoadAlphaTextureArray(cstr uniqueName, Vec2i span, int32 nElements, ITextureLoadEnumerator& enumerator) = 0;
		virtual ID_TEXTURE FindTexture(cstr name) const = 0;
		virtual ICubeTextures& CubeTextures() = 0;
		virtual void SetGenericTextureArray(ID_TEXTURE id) = 0;
		virtual void ShowTextureVenue(IMathsVisitor& visitor) = 0;
		virtual int64 Size() const = 0;
		virtual bool TryGetTextureDesc(TextureDesc& desc, ID_TEXTURE id) const = 0;

		virtual void CompressJPeg(const RGBAb* data, Vec2i span, cstr filename, int quality) const = 0;
		virtual void CompressTiff(const RGBAb* data, Vec2i span, cstr filename) const = 0;

		// Forward on the jpeg decompression function from the lib-jpeg lib
		virtual bool DecompressJPeg(Imaging::IImageLoadEvents& loadEvents, const unsigned char* sourceBuffer, size_t dataLengthBytes) const = 0;

		// Forward on the tiff decompression function from the lib-tiff lib
		virtual bool DecompressTiff(Imaging::IImageLoadEvents& loadEvents, const unsigned char* sourceBuffer, size_t dataLengthBytes) const = 0;

		virtual Textures::IMipMappedTextureArraySupervisor* DefineRGBATextureArray(uint32 numberOfElements, uint32 span, TextureArrayCreationFlags flags) = 0;
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
		virtual void AddTriangle(const GuiVertex triangle[3]) = 0;
		virtual void DrawCustomTexturedMesh(const GuiRect& absRect, ID_TEXTURE id, cstr pixelShader, const GuiVertex* vertices, size_t nCount) = 0;
		virtual void FlushLayer() = 0;
		virtual Vec2i EvalSpan(const Vec2i& pos, Fonts::IDrawTextJob& job, const GuiRect* clipRect = nullptr) = 0;
		virtual void RenderText(const Vec2i& pos, Fonts::IDrawTextJob& job, const GuiRect* clipRect = nullptr) = 0;		
		virtual void SetGuiShader(cstr pixelShader) = 0;
		virtual void SetScissorRect(const Rococo::GuiRect& rect) = 0;
		virtual void ClearScissorRect() = 0;
		virtual GuiScale GetGuiScale() const = 0;

		// Renders high quality text. To compute span without rendering, pass evaluateSpanOnly as true
		enum EMode { RENDER, EVALUATE_SPAN_ONLY, };
		virtual void RenderHQText(ID_FONT id, Fonts::IHQTextJob& job, EMode mode, const GuiRect& clipRect) = 0;

		virtual IMaterials& Materials() = 0;
		virtual IGuiResources& Resources() = 0;
		virtual IRendererMetrics& Renderer() = 0;
	};

	ROCOCO_INTERFACE IParticles
	{
		virtual void AddFog(const ParticleVertex & p) = 0;
		void virtual AddPlasma(const ParticleVertex& p) = 0;
		void virtual ClearPlasma() = 0;
		void virtual ClearFog() = 0;
	};

	ROCOCO_INTERFACE IParticlesSupervisor: IParticles
	{
		virtual void RenderFogWithAmbient() = 0;
		virtual void RenderFogWithSpotlight() = 0;
		virtual void RenderPlasma() = 0;
		virtual void DrawParticles(const ParticleVertex* particles, size_t nParticles, ID_PIXEL_SHADER psID, ID_VERTEX_SHADER vsID, ID_GEOMETRY_SHADER gsID) = 0;
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE IGui3D
	{
		virtual void Add3DGuiTriangles(const VertexTriangle * first, const VertexTriangle * last) = 0;
		virtual void Clear3DGuiTriangles() = 0;
	};

	ROCOCO_INTERFACE IGui3DSupervisor : IGui3D
	{
		virtual void Free() = 0;
		virtual void Render3DGui() = 0;
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
	struct LightConstantBuffer
	{
		Matrix4x4 worldToShadowBuffer;
		Vec4 position;
		Vec4 direction;
		Vec4 right;
		Vec4 up;
		RGBA colour = RGBA(1, 1, 1, 1);
		RGBA ambient = RGBA(0, 0, 0, 0);
		float fogConstant;
		float cosHalfFov;
		Radians fov;
		Metres nearPlane;
		Metres farPlane;
		Seconds time; // Can be used for animation 0 - ~59.99, cycles every minute
		float cutoffCosAngle; // Angle beyond which light is severely diminished, 0.5ish to 1.0
		float cutoffPower = 16.0f; // Exponent of cutoff rate. Range 1 to 16 is cool
		float attenuationRate = -0.15f; // Point lights vary as inverse square, so 0.5 ish
		boolean32 hasCone = 0;
		float shadowFudge = 1.0f; // 0 will create sharp but jags in multisampled mode, 1 will antialias jags, and > 1 add some fudge. 1.0 to 3.0 is about right for most light sources
		float OOShadowTxWidth; // 1 over shadow texture width
	};

	// Data associated with a depth-only render phase, such as shadow generation
	struct DepthRenderData 
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
	};

	using ShadowRenderData = DepthRenderData;
#pragma pack(pop)

	ROCOCO_INTERFACE IScene2D
	{
		virtual RGBA GetClearColour() const = 0;
		virtual void OnGuiResize(Vec2i screenSpan) = 0;
		virtual void RenderGui(IGuiRenderContext& grc) = 0;
	};

	// Simple struct giving the light array and the element count
	struct Lights
	{
		// Points to an array of LightConstantBuffer elements. So lightArray[i] is the ith light-constant-buffer
		const LightConstantBuffer* lightArray;

		// The number of elements in the lightArray
		uint32 count;
	};

	enum class EShadowCasterFilter
	{
		SkinnedCastersOnly,
		UnskinnedCastersOnly
	};

	ROCOCO_INTERFACE IScene : IScene2D
	{
		virtual void GetCamera(Matrix4x4 & camera, Matrix4x4 & world, Matrix4x4 & proj, Vec4 & eye, Vec4 & viewDir) = 0;
		virtual ID_CUBE_TEXTURE GetEnvironmentMap() const = 0;
		virtual ID_CUBE_TEXTURE GetSkyboxCubeId() const = 0;
		virtual void RenderObjects(IRenderContext& rc, EShadowCasterFilter filter) = 0; // Do not change lights from here
		virtual Lights GetLights() const = 0;	// Called prior to the shadow pass. 
		virtual void RenderShadowPass(const DepthRenderData& drd, IRenderContext& rc, EShadowCasterFilter filter) = 0; // Do not change lights from here
	};

	ROCOCO_INTERFACE IGuiRenderContextSupervisor : IGuiRenderContext
	{
		virtual void RenderGui(IScene & scene, const GuiMetrics & metrics) = 0;
	};

	namespace Samplers
	{
		enum class Filter : int32
		{
			Point = 0,
			Linear = 1,
			Anisotropic = 2
		};

		enum class AddressMode : int32
		{
			Border = 0,
			Mirror = 1,
			Wrap = 2,
			Clamp = 3,
			MirrorOnce = 4
		};
	}

	enum class ENVIRONMENTAL_MAP_TYPE
	{
		PROCEDURAL,
		FIXED_CUBE,
		DYNAMIC_CUBE
	};

	struct RenderOutputTargets
	{
		// When rendering to a texture the renderTarget != -1, otherwise we are rendering to the window buffers
		ID_TEXTURE renderTarget;
		ID_TEXTURE depthTarget;
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

	ROCOCO_GRAPHICS_API Vec2i GetTopLeftPos(const GuiRect& rect, Vec2i span, int32 alignmentFlags);
	ROCOCO_GRAPHICS_API Vec2 GetTopLeftPos(Vec2 pos, Vec2 span, int32 alignmentFlags);

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

	struct ScreenMode
	{
		int DX;
		int DY;
	};

	ROCOCO_INTERFACE IRenderer : IRendererMetrics
	{
		virtual IGuiResources & GuiResources() = 0;
		virtual IMaterials& Materials() = 0;
		virtual ITextureManager& Textures() = 0;
		virtual IMeshes& Meshes() = 0;
		virtual IShaders& Shaders() = 0;
		virtual IParticles& Particles() = 0;
		virtual size_t EnumerateScreenModes(Rococo::Function<void(const ScreenMode&)> onMode) = 0;
		virtual void ExpandViewportToEntireTexture(ID_TEXTURE depthId) = 0;
		virtual void CaptureMouse(bool enable) = 0;
		virtual ID_TEXTURE GetWindowDepthBufferId() const = 0;
		virtual IO::IInstallation& Installation() = 0;
		virtual void Render(IScene& scene) = 0;
		virtual void SetCursorVisibility(bool isVisible) = 0;
		virtual void SetEnvironmentMap(ID_CUBE_TEXTURE envId) = 0;
		virtual void SetFullscreenMode(const ScreenMode& mode) = 0;
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
		virtual void RegisterSubsystem(ISubsystemMonitor& monitor, ID_SUBSYSTEM platformId) = 0;
	};

	struct GlyphCallbackArgs
	{
		GuiRect rect;
		int32 index;
	};

	ROCOCO_GRAPHICS_API void DrawTriangleFacingLeft(IGuiRenderContext& grc, const GuiRect& container, RGBAb colour);
	ROCOCO_GRAPHICS_API void DrawTriangleFacingRight(IGuiRenderContext& grc, const GuiRect& container, RGBAb colour);
	ROCOCO_GRAPHICS_API void DrawTriangleFacingUp(IGuiRenderContext& grc, const GuiRect& container, RGBAb colour);
	ROCOCO_GRAPHICS_API void DrawTriangleFacingDown(IGuiRenderContext& grc, const GuiRect& container, RGBAb colour);

	ROCOCO_GRAPHICS_API Vec2i GetScreenCentre(const GuiMetrics& metrics);
	ROCOCO_GRAPHICS_API Vec2i RenderHorizontalCentredText(IGuiRenderContext& gr, cstr txt, RGBAb colour, int fontSize, const Vec2i& topMiddle);
	ROCOCO_GRAPHICS_API Vec2i RenderVerticalCentredTextWithCallback(IGuiRenderContext& gr, int32 cursorPos, IEventCallback<GlyphCallbackArgs>& cb, cstr txt, RGBAb colour, int fontSize, const Vec2i& topMiddle, const GuiRect& clipRect);
	ROCOCO_GRAPHICS_API Vec2i RenderVerticalCentredText(IGuiRenderContext& grc, cstr text, RGBAb colour, int fontSize, const Vec2i& middleLeft, const GuiRect* clipRect = nullptr);
	ROCOCO_GRAPHICS_API Vec2i RenderTopLeftAlignedText(IGuiRenderContext& grc, cstr text, RGBAb colour, int fontSize, const Vec2i& topLeft);
	ROCOCO_GRAPHICS_API Vec2i RenderTopRightAlignedText(IGuiRenderContext& grc, cstr text, RGBAb colour, int fontSize, const Vec2i& topRight);
	ROCOCO_GRAPHICS_API Vec2i RenderRightAlignedText(IGuiRenderContext& grc, cstr text, RGBAb colour, int fontSize, const GuiRect& rect);
	ROCOCO_GRAPHICS_API void EvalTextSpan(IGuiRenderContext& g, const fstring& text, int32 fontIndex, Vec2& pixelSpan);
	ROCOCO_GRAPHICS_API void DrawRectangle(IGuiRenderContext& grc, const GuiRect& grect, RGBAb diag, RGBAb backdiag);
	ROCOCO_GRAPHICS_API void DrawBorderAround(IGuiRenderContext& grc, const GuiRect& rect, const Vec2i& width, RGBAb diag, RGBAb backdiag);
	ROCOCO_GRAPHICS_API void DrawLine(IGuiRenderContext& grc, int pixelthickness, Vec2i start, Vec2i end, RGBAb colour);
	ROCOCO_GRAPHICS_API void RenderCentred(IGuiRenderContext& grc, ID_FONT fontId, const GuiRect& rect, cstr text, RGBAb colour);

	struct GlyphContext
	{
		GuiRect outputRect;
		uint32 unicode;
	};

	ROCOCO_GRAPHICS_API GuiRect RenderHQText(const GuiRect& clipRect, int32 alignment, IGuiRenderContext& grc, ID_FONT fontId, cstr text, RGBAb colour, Vec2i spacing, IEventCallback<GlyphContext>* glyphCallback = nullptr, int dxShift = 0);
	ROCOCO_GRAPHICS_API Vec2 RenderHQText_LeftAligned_VCentre(IGuiRenderContext& grc, ID_FONT fontId, const GuiRect& rect, cstr text, RGBAb colour);
	ROCOCO_GRAPHICS_API Vec2 RenderHQText_CentreAligned(IGuiRenderContext& grc, ID_FONT fontId, const GuiRect& rect, cstr text, RGBAb colour);
	ROCOCO_GRAPHICS_API Vec2 RenderHQText_LeftAligned_VCentre_WithCaret(IGuiRenderContext& grc, ID_FONT fontId, const GuiRect& rect, cstr text, RGBAb colour, int caretPos);
	ROCOCO_GRAPHICS_API Vec2 RenderHQParagraph(IGuiRenderContext& grc, ID_FONT fontId, const GuiRect& rect, cstr text, RGBAb colour);
	
	struct alignas(16) StackSpaceGraphics
	{
		char opaque[256];
	};

	ROCOCO_GRAPHICS_API Fonts::IDrawTextJob& CreateHorizontalCentredText(StackSpaceGraphics& ss, int fontIndex, cstr text, RGBAb _colour);
	ROCOCO_GRAPHICS_API Fonts::IDrawTextJob& CreateLeftAlignedText(StackSpaceGraphics& ss, const GuiRect& targetRect, int retzone, int hypzone, int fontHeight, int fontIndex, cstr text, RGBAb colour);
	ROCOCO_GRAPHICS_API float GetAspectRatio(const IRenderer& renderer);
	ROCOCO_GRAPHICS_API void DrawClippedText(IGuiRenderContext& g, const Rococo::GuiRectf& rect, int32 alignmentFlags, const fstring& text, int32 fontIndex, RGBAb colour, const Rococo::GuiRectf& clipRect);
	ROCOCO_GRAPHICS_API void DrawTextWithCaret(IGuiRenderContext& g, const Rococo::GuiRectf& rect, int32 alignmentFlags, const fstring& text, int32 fontIndex, RGBAb colour, const Rococo::GuiRectf& clipRect, int32 caretPos);
	ROCOCO_GRAPHICS_API void DrawLeftAligned(IGuiRenderContext& g, const Rococo::GuiRectf& rect, const fstring& text, int32 fontIndex, int32 fontHeight, RGBAb colour, float32 softRightEdge, float32 hardRightEdge);
	ROCOCO_GRAPHICS_API void DrawText(IGuiRenderContext& g, const GuiRectf& rect, int32 alignmentFlags, const fstring& text, int32 fontIndex, RGBAb colour);
	ROCOCO_GRAPHICS_API void DrawTexture(IGuiRenderContext& grc, ID_TEXTURE id, const GuiRect& absRect);
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