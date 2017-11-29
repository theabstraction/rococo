#include <mplat.api.hlsl>

struct PixelVertex
{
	float4 position : SV_POSITION0;
	float3 cameraSpacePosition: TEXCOORD3;
	float4 shadowPosition: TEXCOORD2;
	float3 worldPosition: TEXCOORD1;
	float2 uv: TEXCOORD0;
	float4 colour: COLOR0;
};

float4 main(PixelVertex p) : SV_TARGET
{
	float4 shadowXYZW = p.shadowPosition / p.shadowPosition.w;
	float2 shadowUV = float2(1.0f + shadowXYZW.x, 1.0f - shadowXYZW.y) * 0.5f;

	float bias = -0.00001f;
	float shadowDepth = g_ShadowMap.Sample(shadowSampler, shadowUV).x + bias;

	float isLit = shadowDepth > shadowXYZW.z;

	if (isLit)
	{
		float3 lightToPixelVec = p.worldPosition.xyz - light.position.xyz;
		float R2 = dot(lightToPixelVec, lightToPixelVec);

		float r = dot(p.uv, p.uv);
		float intensity = 0.2f * clamp(1 - r, 0, 1);
		float4 texel = float4(p.colour.xyz, intensity * p.colour.w);

		float3 lightToPixelDir = normalize(lightToPixelVec);

		float f = dot(lightToPixelDir, normalize(light.direction.xyz));

		float falloff = 1.0f;

		if (f < light.cutoffCosAngle) falloff = pow(1.0f - (light.cutoffCosAngle - f), light.cutoffPower);

		if (f < 0) f = 0;

		float range = length(p.cameraSpacePosition.xyz);
		float fogging = exp(range * light.fogConstant);

		float diffuse = pow(f, 16.0f) * pow(R2, light.attenuationRate);
		float I = diffuse * falloff * fogging;

		texel.xyz *= I;
		texel.xyz *= light.colour.xyz;

		return texel;
	}
	else
	{
		return float4(0, 0, 0, 0);
	}
}

