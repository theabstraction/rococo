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
	if (!IsInShadow(p.shadowPosition))
	{
		float3 lightToPixelVec = p.worldPosition.xyz - light.position.xyz;
		float R2 = dot(lightToPixelVec, lightToPixelVec);

		float4 texel = GetPointSpriteTexel(p.uv, p.colour);

		float3 lightToPixelDir = normalize(lightToPixelVec);

		float intensity = GetSpotlightIntensity(lightToPixelDir);

		float clarity = GetClarity(p.cameraSpacePosition.xyz);

		float diffuse = pow(R2, light.attenuationRate);
		float I = diffuse * intensity * clarity;

		texel.xyz *= I;
		texel.xyz *= light.colour.xyz;

		return texel;
	}
	else
	{
		return float4(0, 0, 0, 0);
	}
}

