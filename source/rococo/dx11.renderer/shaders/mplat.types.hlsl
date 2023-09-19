#ifndef MPLAT_TYPES_HLSL
#define MPLAT_TYPES_HLSL

#pragma pack_matrix(row_major)

#ifdef _DEBUG
# pragma enable_d3d11_debug_symbols
#endif

struct BoneWeight_2Bones
{
	float index0 : BLENDINDICES0;
	float weight0 : BLENDWEIGHT0;
	float index1 : BLENDINDICES1;
	float weight1 : BLENDWEIGHT1;
};

struct AmbientData
{
	float4 localLight;
	float fogConstant; // light = e(Rk). Where R is distance. k = -0.2218 gives modulation of 1/256 at 25 metres, reducing full brightness to dark
	float a;
	float b;
	float c;
	float4 eye;
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
	float4x4 worldToScreenMatrix;
	float4x4 worldToCameraMatrix;
	float4x4 cameraToScreenMatrix;
	GuiScale guiScale;
	float4 eye;
	float4 viewDir;
	float4 aspect;
};

struct ObjectInstance
{
	float4x4 modelToWorldMatrix;
	float3 scale;
	float unused;
	float4 highlightColour;
};

struct ObjectVertex
{
	float3 modelPosition : POSITION;
	float3 modelNormal : NORMAL;
	float2 uv: TEXCOORD0;
	float4 colour: COLOR0;
	float2 materialIndexAndGloss  : TEXCOORD1;
};

struct BaseVertexData
{
	float2 uv;
	float fontBlend; // 0 -> normal triangle, 1 -> modulate with font texture
};

struct SpriteVertexData
{
	float saturation; // 1.0 -> use colour, 0.0 -> use bitmap texture
	float spriteIndex; // Indexes the texture in the sprite array.
	float matIndex; // Indexes the texture in the material array
	float spriteToMatLerpFactor; // 0 -> use spriteIndex, 1 -> use matIndex, lerping in between
};

struct GuiVertexOpaque
{
    float2 pos : POSITION;
    float3 base : TEXCOORD0;
    float4 sd : TEXCOORD1;
    float4 colour : COLOR;
};

struct GuiVertex
{
	float2 pos : POSITION;
	BaseVertexData base : TEXCOORD0;
	SpriteVertexData sd: TEXCOORD1;
	float4 colour: COLOR;
};

struct GuiPixelVertex
{
	float4 position;
	BaseVertexData base;
	SpriteVertexData sd;
	float4 colour;
};

struct GuiPixelVertexOpaque
{
	float4 position			: SV_POSITION;
	float3 base				: TEXCOORD0;
	float4 sd				: TEXCOORD1;
	float4 colour			: COLOR;
};

struct ObjectPixelVertex
{
	float4 position : SV_POSITION0;
	float4 uv_material_and_gloss: TEXCOORD;
	float3 worldPosition: TEXCOORD1;
	float3 worldNormal : NORMAL;
	float4 shadowPos: TEXCOORD3;
	float4 cameraSpacePosition: TEXCOORD4;
	float4 colour: COLOR0;
};

struct LandPixelVertex
{
	float4 position : SV_POSITION0;
	float4 uv_material_and_gloss: TEXCOORD;
	float4 normal : TEXCOORD1;
};

struct LandVertex
{
	float4 position : SV_POSITION0;
	float4 uv_material_and_gloss: TEXCOORD;
	float4 normal : TEXCOORD1;
};

struct ParticleVertex
{
	float3 position : POSITION;
	float4 colour: COLOR;
	float4 geometry: TEXCOORD;
};

// Used in landscape.ps
struct Sunlight
{
	float4 direction;
};

struct Light
{
	float4x4 worldToShadowBufferMatrix;
	float4 position;
	float4 direction;
	float4 right;
	float4 up;
	float4 colour;
	float4 ambient;
	float fogConstant;
	float cosHalfFov;
	float fov;
	float nearPlane;
	float farPlane;
	float time; // Can be used for animation 0 - ~59.99, cycles every minute
	float cutoffCosAngle; // What angle to trigger cutoff of light
	float cutoffPower; // Exponent of cutoff rate. Range 1 to 64 is cool
	float attenuationRate; // Point lights vary as inverse square, so 0.5 ish
    int hasCone;
    float shadowFudge; // in multisample mode, 0 = jagged but sharp, 0 to 1 = antialiased soft shadows, 1+ is even softer fudgier shadows, >>> 1 penumbra
    float OOShadowTxWidth;
};

struct DepthRenderDesc
{
	float4x4 worldToCamera;
	float4x4 worldToScreen;
	float4 eye;
	float4 direction;
	float4 right;
	float4 up;
	float nearPlane;
	float farPlane;
	float fov; // radians
	float time;
	float4 randoms;
};

struct TextureDescState
{
	float width;
	float height;
	float inverseWidth;
	float inverseHeight;
	float redActive;
	float greenActive;
	float blueActive;
	float alphaActive;
};

// b registers are mapped by CBUFFER_INDEX in dx11.renderer.cpp
cbuffer GlobalState: register(b0)
{
	GlobalState global;
}

cbuffer Spotlight: register(b1)
{
	Light light;
};

cbuffer AmbienceState: register(b2)
{
	AmbientData ambience;
}

cbuffer DepthRenderDesc : register(b3)
{
	DepthRenderDesc drd;
}

cbuffer InstanceState: register(b4)
{
	ObjectInstance instance;
}

cbuffer textureState : register(b5)
{
	TextureDescState state;
}

// Used in landscape.ps
cbuffer SunlightState : register(b6)
{
	Sunlight sunlight;
};

cbuffer BoneMatricesState: register(b7)
{
	float4x4 boneMatrices[16];
};

SamplerState fontSampler: register(s0);
SamplerState shadowSampler: register(s1);
SamplerState envSampler: register(s2);
SamplerState selectSampler: register(s3);
SamplerState matSampler: register(s4);
SamplerState spriteSampler: register(s5);
SamplerState glyphSampler: register(s6);

Texture2D tx_FontSprite: register(t0);
Texture2D tx_ShadowMap: register(t1);
TextureCube tx_cubeMap: register(t2);
Texture2D tx_SelectedTexture : register(t3);
Texture2DArray tx_materials: register(t4);
Texture2DArray tx_BitmapSprite: register(t5);
Texture2DArray tx_GlyphArray: register(t6);

float3 ComputeEyeToWorldDirection(ObjectPixelVertex p)
{
    return normalize(p.worldPosition.xyz - global.eye.xyz);
}

#endif