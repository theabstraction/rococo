#pragma once

#include <rococo.types.h>
#include <rococo.graphics.types.h>
#include <rococo.maths.h>

namespace Rococo::Graphics
{
	struct LightConstantBuffer;
	struct TextureDesc;
	struct ITextureLoadEnumerator;

	namespace Textures
	{
		struct IBitmapArrayBuilder;
		struct IMipMappedTextureArray;
		struct IMipMappedTextureArraySupervisor;
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
		TXUNIT_SHADOW = 1,
		TXUNIT_ENV_MAP = 2,
		TXUNIT_SELECT = 3,
		TXUNIT_MATERIALS = 4,
		TXUNIT_SPRITES = 5,
		TXUNIT_GENERIC_TXARRAY = 6,
		TXUNIT_NOISE_PERMUTATION = 7,
		TXUNIT_NOISE_GRADIENT_LOOKUP = 8,
		TXUNIT_GBUFFER_START = 9
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

#pragma pack(push,1)
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
		int64 frameIndex;
	};

	struct ObjectInstance
	{
		Matrix4x4 orientation;
		Vec3 scale;
		float unused;
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
		// TODO - delete this line: float OOShadowTxWidth;
	};

	struct AmbientData
	{
		RGBA localLight;
		float fogConstant = -0.2218f; // light = e(Rk). Where R is distance. k = -0.2218 gives modulation of 1/256 at 25 metres, reducing full brightness to dark
		float a = 0;
		float b = 0;
		float c = 0;
	};
#pragma pack(pop)
}