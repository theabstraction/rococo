#include "mplat.api.hlsl"

struct PixelVertex
{
	float4 position : SV_POSITION0;
	float4 uv_material_and_gloss: TEXCOORD;
    float4 worldPosition: TEXCOORD1;
	float4 normal : TEXCOORD2;
	float4 shadowPos: TEXCOORD3;
	float4 cameraSpacePosition: TEXCOORD4;
	float4 colour: COLOR0;	// w component gives lerpColourToTexture
};

float4 per_pixel_lighting(PixelVertex p)
{
	if (!IsInShadow(p.shadowPos))
	{
		float3 lightToPixelVec = p.worldPosition.xyz - light.position.xyz;
		float R2 = dot(lightToPixelVec, lightToPixelVec);

		float4 texel = SampleMaterial(p.uv_material_and_gloss.xyz, p.colour);

		float3 incident = normalize(p.worldPosition.xyz - global.eye.xyz);

		texel = ModulateWithEnvMap(texel, incident, p.normal.xyz, p.uv_material_and_gloss.w);

		float3 lightToPixelDir = normalize(lightToPixelVec);

		float intensity = GetSpotlightIntensity(lightToPixelDir);

		float3 normal = p.normal.xyz;
		float g = -dot(lightToPixelDir, normal);

		float clarity = GetClarity(p.cameraSpacePosition.xyz);

		float3 r = reflect(lightToPixelDir.xyz, normal);

		float dotProduct = dot(r, incident);
		float shine = 240.0f;
		float specular = p.uv_material_and_gloss.w * max(pow(dotProduct, shine), 0);
		
		float diffuse = g * pow(R2, light.attenuationRate);
		float I = (diffuse + specular) * intensity * clarity;

		texel.xyz *= I;
		texel.xyz *= light.colour.xyz;

		return float4 (texel.xyz, 1.0f);
	}
	else
	{
		return float4(0,0,0,1);
	}
}

float4 no_lighting(PixelVertex p)
{
	float4 texel = tx_materials.Sample(matSampler, p.uv_material_and_gloss.xyz).xyzw;
	texel = lerp(p.colour, texel, p.colour.w);

	float3 incident = normalize(p.worldPosition.xyz - global.eye.xyz);
	float3 reflectionVector = reflect(incident.xyz, normalize(p.normal.xyz));
	float4 reflectionColor = tx_cubeMap.Sample(envSampler, reflectionVector);

	texel.xyz = lerp(texel.xyz, reflectionColor.xyz, p.uv_material_and_gloss.w);
	return texel;
}

float4 main(PixelVertex p) : SV_TARGET
{
	return per_pixel_lighting(p);
}
