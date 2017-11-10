struct PixelVertex
{
	float4 position : SV_POSITION0;
	float3 uv_material: TEXCOORD;
    float4 cameraSpacePosition: TEXCOORD1;
	float4 colour: COLOR0;	// w component gives lerpColourToTexture
};

#pragma pack_matrix(row_major)

Texture2DArray g_materials: register(t6);
SamplerState txSampler;

struct AmbientData
{
	float4 localLight;
	float fogConstant; // light = e(Rk). Where R is distance. k = -0.2218 gives modulation of 1/256 at 25 metres, reducing full brightness to dark
	float a;
	float b;
	float c;
};

cbuffer ambience
{
	AmbientData ambience;
}

float4 per_pixel_lighting(PixelVertex p)
{
	float4 texel = g_materials.Sample(txSampler, p.uv_material);
	texel.xyz = lerp(p.colour.xyz, texel.xyz, p.colour.w);

	float range = length(p.cameraSpacePosition.xyz);	
	float fogging = exp( range * ambience.fogConstant);
	
	texel.xyz *= fogging;

	return texel * ambience.localLight;
}

float4 main(PixelVertex p) : SV_TARGET
{
	return per_pixel_lighting(p);
}
