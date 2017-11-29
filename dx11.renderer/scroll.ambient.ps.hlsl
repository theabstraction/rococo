struct PixelVertex
{
	float4 position : SV_POSITION0;
	float4 normal: NORMAL;
	float4 uv_material_and_gloss: TEXCOORD; // In scroll rendering, gloss controls solid colour to font lerp
	float4 cameraSpacePosition: TEXCOORD1;
	float4 worldPosition: TEXCOORD2;
	float4 colour: COLOR0;	// w component gives lerpColourToTexture
};

#pragma pack_matrix(row_major)

Texture2DArray g_materials: register(t6);
TextureCube g_cubeMap: register(t3);
Texture2D g_FontSprite: register(t0);
Texture2DArray g_BitmapSprite: register(t7);

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

cbuffer GlobalState: register(b0)
{
	float4x4 worldMatrixAndProj;
	float4x4 worldMatrix;
	GuiScale guiScale;
	float4 eye;
	float4 viewDir;
	float4 aspect;
}

cbuffer AmbientState : register(b2)
{
	AmbientData ambience;
}

SamplerState fontSampler: register(s0);
SamplerState spriteSampler: register(s1);
SamplerState matSampler: register(s2);
SamplerState envSampler: register(s3);
SamplerState shadowSampler: register(s4);

float4 GetFontPixel(float3 uv_blend, float4 vertexColour)
{
	float fontIntensity = lerp(1.0f, g_FontSprite.Sample(fontSampler, uv_blend.xy).x, uv_blend.z);
	return float4(vertexColour.xyz, fontIntensity);
}

float4 per_pixel_lighting(PixelVertex p)
{
	float4 texel = GetFontPixel(p.uv_material_and_gloss.xyw, p.colour);

	float range = length(p.cameraSpacePosition.xyz);
	float fogging = exp(range * ambience.fogConstant);

	texel.xyz *= fogging;

	return float4(texel.xyz * ambience.localLight.xyz, texel.w * p.colour.w);
}

float4 main(PixelVertex p) : SV_TARGET
{
	return per_pixel_lighting(p);
}
