#include "mplat.api.hlsl"

struct PixelVertex
{
	float4 position : SV_POSITION0;
	float4 normal: NORMAL;
	float4 uv_material_and_gloss: TEXCOORD;
    float4 cameraSpacePosition: TEXCOORD1;
	float4 worldPosition: TEXCOORD2;
	float4 colour: COLOR0;	// w component gives lerpColourToTexture
};

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
