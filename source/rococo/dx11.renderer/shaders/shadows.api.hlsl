#include "mplat.types.hlsl"

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