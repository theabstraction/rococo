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

float4 GetFontPixel(float3 uv_blend, float4 vertexColour)
{
	float fontIntensity = lerp(1.0f, g_FontSprite.Sample(fontSampler, uv_blend.xy).x, uv_blend.z);
	return float4(vertexColour.xyz, fontIntensity);
}

float4 per_pixel_lighting(PixelVertex p)
{
	float4 shadowXYZW = p.shadowPos / p.shadowPos.w;
	float2 shadowUV = float2(1.0f + shadowXYZW.x, 1.0f - shadowXYZW.y) * 0.5f;

	float bias = -0.00001f;
	float shadowDepth = g_ShadowMap.Sample(shadowSampler, shadowUV).x + bias;

	float isLit = shadowDepth > shadowXYZW.z;

	float4 texel = GetFontPixel(p.uv_material_and_gloss.xyw, p.colour);

	if (isLit)
	{
		float3 lightToPixelVec = p.worldPosition.xyz - light.position.xyz;
		float R2 = dot(lightToPixelVec, lightToPixelVec);
	
		float3 lightToPixelDir = normalize(lightToPixelVec);

		float f = dot(lightToPixelDir, normalize(light.direction.xyz));

		float falloff = 1.0f;

		if (f < light.cutoffCosAngle) falloff = pow(1.0f - (light.cutoffCosAngle - f), light.cutoffPower);

		if (f < 0) f = 0;

		float3 normal = normalize(p.normal.xyz);
		float g = -dot(lightToPixelDir, normal);

		float range = length(p.cameraSpacePosition.xyz);
		float fogging = exp(range * light.fogConstant);

		float diffuse = pow(f, 16.0f) * g * pow(R2, light.attenuationRate);
		float intensity = diffuse * falloff * fogging;

		return float4 (texel.xyz * intensity * light.colour.xyz, texel.w);
	}
	else
	{
		return float4(0, 0, 0, texel.w);
	}
}

float4 main(PixelVertex p) : SV_TARGET
{
	return per_pixel_lighting(p);
}
