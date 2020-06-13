#pragma pack_matrix(row_major)

#ifdef _DEBUG
# pragma enable_d3d11_debug_symbols
#endif

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
	float4 highlightColour;
};

struct ObjectVertex
{
	float4 position : POSITION;
	float4 normal : NORMAL;
	float2 uv: TEXCOORD0;
	float4 colour: COLOR0;
	float2 materialIndexAndGloss  : TEXCOORD1;
};

struct GuiPixelVertex
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
	float4 worldPosition: TEXCOORD1;
	float4 normal : TEXCOORD2;
	float4 shadowPos: TEXCOORD3;
	float4 cameraSpacePosition: TEXCOORD4;
	float4 colour: COLOR0;	// w component gives lerpColourToTexture
};

struct ParticleVertex
{
	float3 position : POSITION;
	float4 colour: COLOR;
	float4 geometry: TEXCOORD;
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
	float3 randoms; // 4 random quotients 0.0 - 1.0
	float cosHalfFov;
	float fov;
	float nearPlane;
	float farPlane;
	float time; // Can be used for animation 0 - ~59.99, cycles every minute
	float cutoffCosAngle; // What angle to trigger cutoff of light
	float cutoffPower; // Exponent of cutoff rate. Range 1 to 64 is cool
	float attenuationRate; // Point lights vary as inverse square, so 0.5 ish
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