struct PixelVertex
{
	float4 position : SV_POSITION0;
	float4 normal: NORMAL;
	float4 uv_material_and_gloss: TEXCOORD;
    float4 cameraSpacePosition: TEXCOORD1;
	float4 worldPosition: TEXCOORD2;
	float4 colour: COLOR0;	// w component gives lerpColourToTexture
};

#pragma pack_matrix(row_major)

Texture2DArray g_materials: register(t6);

SamplerState fontSampler: register(s0);
SamplerState spriteSampler: register(s1);
SamplerState matSampler: register(s2);
SamplerState envSampler: register(s3);
SamplerState shadowSampler: register(s4);

TextureCube g_cubeMap: register(t3);

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

cbuffer AmbienceState: register(b2)
{
	AmbientData ambience;
}

float4 per_pixel_lighting(PixelVertex p)
{
	float4 texel = g_materials.Sample(matSampler, p.uv_material_and_gloss.xyz);
	texel.xyz = lerp(p.colour.xyz, texel.xyz, p.colour.w);

	float3 incident = normalize(p.worldPosition.xyz - eye.xyz);
	float3 reflectionVector = reflect(incident.xyz, normalize(p.normal.xyz));
	float4 reflectionColor = g_cubeMap.Sample(envSampler, reflectionVector);

	texel.xyz = lerp(texel.xyz, reflectionColor.xyz, p.uv_material_and_gloss.w);

	float range = length(p.cameraSpacePosition.xyz);	
	float fogging = exp( range * ambience.fogConstant);
	
	texel.xyz *= fogging;

	return texel * ambience.localLight;
}

float4 main(PixelVertex p) : SV_TARGET
{
	return per_pixel_lighting(p);
}
