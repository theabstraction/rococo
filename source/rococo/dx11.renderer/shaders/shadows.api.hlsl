#include "mplat.types.hlsl"

float SampleShadowWithDelta(float4 pos, float2 offset)
{
	float2 scaledOffset = offset * global.OOShadowTxWidth * light.shadowFudge;
	float3 shadowXYZ = pos.xyz / pos.w;
	float2 shadowUV = (scaledOffset + (float2(1.0f + shadowXYZ.x, 1.0f - shadowXYZ.y))) * 0.5f;

	float bias = -0.00001f;
	float shadowDepth = tx_ShadowMap.Sample(shadowSampler, shadowUV).x + bias;
	
	if (shadowDepth <= shadowXYZ.z)
	{
		return 1.0f;
	}
	else
	{
		return 0.0f;
	}
}

float GetShadowDensity_16Sample(float4 shadowPos)
{
    float shadowDensity = 0.0f;
    float2 delta;
	
    for (delta.y = -1.5f; delta.y <= 1.5f; delta.y += 1.0f)
    {
        for (delta.x = -1.5f; delta.x <= 1.5f; delta.x += 1.0f)
        {
            shadowDensity += SampleShadowWithDelta(shadowPos, delta);
        }
    }
	
    return shadowDensity / 4.0f;
}

float GetShadowDensity_4Sample(float4 shadowPos)
{
	float shadowDensity = 0.0f;
	float2 delta;
	
    float f1 = SampleShadowWithDelta(shadowPos, float2(-1.5f, 0.5f));
    float f2 = SampleShadowWithDelta(shadowPos, float2(0.5f, 0.5f));
    float f3 = SampleShadowWithDelta(shadowPos, float2(-1.5f, -1.5f));
    float f4 = SampleShadowWithDelta(shadowPos, float2(0.5f, -1.5f));
	
    return (f1 + f2 + f3 + f4) * 0.25f;
}

float GetShadowDensity_1Sample(float4 shadowPos)
{
	float3 shadowXYZ = shadowPos.xyz / shadowPos.w;
	float2 shadowUV = float2(1.0f + shadowXYZ.x, 1.0f - shadowXYZ.y) * 0.5f;

	float bias = -0.00001f;
	float shadowDepth = tx_ShadowMap.Sample(shadowSampler, shadowUV).x + bias;

	if (shadowDepth <= shadowXYZ.z)
	{
		return 1.0f;
	}
	else
	{
		return 0.0f;
	}
}

interface IShadowModel
{
    float GetShadowDensityFromPos(float4 shadowPos);
};

class ShadowModel1Sample: IShadowModel
{
    float GetShadowDensityFromPos(float4 shadowPos)
    {
        return GetShadowDensity_1Sample(shadowPos);
    }
};

class ShadowModel4Samples : IShadowModel
{
    float GetShadowDensityFromPos(float4 shadowPos)
    {
        return GetShadowDensity_4Sample(shadowPos);
    }
};

class ShadowModel16Samples : IShadowModel
{
    float GetShadowDensityFromPos(float4 shadowPos)
    {
        return GetShadowDensity_16Sample(shadowPos);
    }
};

IShadowModel refShadowModel;

float GetShadowDensityFromPos(float4 shadowPos)
{
    return GetShadowDensity_16Sample(shadowPos);
}

float GetShadowDensity(ObjectPixelVertex p)
{
    return refShadowModel.GetShadowDensityFromPos(p.shadowPos);
}

float4 BlendColourWithLightAndShadow(float4 colour, float shadowDensity, float I)
{
    colour.xyz *= I;
    colour.xyz *= light.colour.xyz;

    return lerp(float4(colour.xyz, 1.0f), float4(0.0f, 0.0f, 0.0f, 0.0f), shadowDensity);
}
