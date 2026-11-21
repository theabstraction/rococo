#ifndef SHADOW_API_HLSL
#define SHADOW_API_HLSL

#include "mplat.types.hlsl"

// Shadow map lookups with hardware percentage-closer filtering.
// Convention: density 1 = fully shadowed, 0 = fully lit.
//
// C++ REQUIREMENT: the sampler bound to s1 must be a comparison sampler:
//   Filter         = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT
//   ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL   (lit when receiver depth <= stored depth)
//   Address U/V/W  = D3D11_TEXTURE_ADDRESS_BORDER, BorderColor = {1,1,1,1} (off-map taps read as lit)
// tx_ShadowMap (t1) is the R32_FLOAT SRV of the R32_TYPELESS / D32_FLOAT shadow depth target.
// This replaces 'SamplerState shadowSampler : register(s1)', which has been removed from mplat.types.hlsl
SamplerComparisonState shadowCmpSampler : register(s1);

struct ShadowCoord
{
    float2 uv;
    float  z;
    bool   inside;
};

ShadowCoord ToShadowCoord(float4 shadowPos)
{
    ShadowCoord c;
    float3 ndc = shadowPos.xyz / shadowPos.w;          // one divide per pixel, not per tap
    c.uv = float2(0.5f + 0.5f * ndc.x, 0.5f - 0.5f * ndc.y);
    c.z = ndc.z;
    c.inside = shadowPos.w > 0.0f && c.z <= 1.0f && all(c.uv == saturate(c.uv));
    return c;
}

// Fraction lit at one tap, with hardware 2x2 bilinear PCF
float LitTap(ShadowCoord c, float2 offsetTexels)
{
    float2 uv = c.uv + offsetTexels * light.OOShadowTxWidth;
    return tx_ShadowMap.SampleCmpLevelZero(shadowCmpSampler, uv, c.z);
}

// Single hardware-filtered tap. Cheapest option, 2x2 bilinear edge only
float GetShadowDensity_PCF1(float4 shadowPos)
{
    ShadowCoord c = ToShadowCoord(shadowPos);
    if (!c.inside) return 0.0f;

    return 1.0f - LitTap(c, float2(0.0f, 0.0f));
}

// 3x3 taps x 2x2 bilinear = smooth 4x4 footprint at shadowFudge = 1. shadowFudge scales the tap spacing in texels
float GetShadowDensity_PCF9(float4 shadowPos)
{
    ShadowCoord c = ToShadowCoord(shadowPos);
    if (!c.inside) return 0.0f;

    float r = max(light.shadowFudge, 0.0f);
    float lit = 0.0f;

    [unroll] for (int y = -1; y <= 1; ++y)
    {
        [unroll] for (int x = -1; x <= 1; ++x)
        {
            lit += LitTap(c, float2(x, y) * r);
        }
    }

    return 1.0f - lit * (1.0f / 9.0f);
}

// Softer penumbra for shadowFudge >> 1: rotated Poisson disc, no banding.
// Needs SV_Position.xy for the per-pixel rotation, so it is not exposed as an IShadowModel class
static const float2 kPoisson12[12] =
{
    float2(-0.326f,-0.406f), float2(-0.840f,-0.074f), float2(-0.696f, 0.457f),
    float2(-0.203f, 0.621f), float2( 0.962f,-0.195f), float2( 0.473f,-0.480f),
    float2( 0.519f, 0.767f), float2( 0.185f,-0.893f), float2( 0.507f, 0.064f),
    float2( 0.896f, 0.412f), float2(-0.322f,-0.933f), float2(-0.792f,-0.598f)
};

float InterleavedGradientNoise(float2 pixel)   // Jimenez 2014
{
    return frac(52.9829189f * frac(dot(pixel, float2(0.06711056f, 0.00583715f))));
}

float GetShadowDensity_Poisson12(float4 shadowPos, float2 screenPixel)
{
    ShadowCoord c = ToShadowCoord(shadowPos);
    if (!c.inside) return 0.0f;

    float a = InterleavedGradientNoise(screenPixel) * 6.2831853f;
    float s, co;
    sincos(a, s, co);
    float2x2 rot = float2x2(co, -s, s, co);

    float r = max(light.shadowFudge, 1.0f) * 1.5f;
    float lit = 0.0f;

    [unroll] for (int i = 0; i < 12; ++i)
    {
        lit += LitTap(c, mul(rot, kPoisson12[i]) * r);
    }

    return 1.0f - lit * (1.0f / 12.0f);
}

// Normal-offset receiver: removes acne on grazing surfaces without peter-panning.
// worldNormal must be the normalized GEOMETRIC normal (not the bump-mapped one).
// light.fov is the full cone angle in radians (it feeds GetRHProjectionMatrix on the C++ side).
// light.direction is normalized here because the constant buffer carries it as supplied by the scene.
float4 GetShadowPosNormalOffset(float3 worldPos, float3 worldNormal)
{
    float3 toP = worldPos - light.position.xyz;
    float3 lightDir = normalize(light.direction.xyz);
    float d = max(dot(toP, lightDir), light.nearPlane);
    float texelWorld = 2.0f * d * tan(0.5f * light.fov) * light.OOShadowTxWidth;

    float NdotL = saturate(dot(worldNormal, -normalize(toP)));
    float k = 1.5f * (1.0f - NdotL);                    // more push at grazing angles

    return mul(light.worldToShadowBufferMatrix, float4(worldPos + worldNormal * texelWorld * k, 1.0f));
}

// Shadow models selectable at runtime via class linkage, e.g. in config_mplat.sxy:
//   (shaders.Set "refShadowModel" "ShadowModelPCF9")
interface IShadowModel
{
    float GetShadowDensityFromPos(float4 shadowPos);
};

class ShadowModelPCF1 : IShadowModel
{
    float GetShadowDensityFromPos(float4 shadowPos)
    {
        return GetShadowDensity_PCF1(shadowPos);
    }
};

class ShadowModelPCF9 : IShadowModel
{
    float GetShadowDensityFromPos(float4 shadowPos)
    {
        return GetShadowDensity_PCF9(shadowPos);
    }
};

IShadowModel refShadowModel;

float GetShadowDensityDirect(float4 shadowPos)
{
    return refShadowModel.GetShadowDensityFromPos(shadowPos);
}

// Uses the interpolated shadowPos. For geometry without a meaningful normal (fog sprites, text)
float GetShadowDensity(ObjectPixelVertex p)
{
    return GetShadowDensityDirect(p.shadowPos);
}

// Normal-offset version. Ng is the normalized geometric normal captured before bumping
float GetShadowDensity(ObjectPixelVertex p, float3 Ng)
{
    return GetShadowDensityDirect(GetShadowPosNormalOffset(p.worldPosition, Ng));
}

float4 BlendColourWithLightAndShadow(float4 colour, float shadowDensity, float I)
{
    colour.xyz *= I;
    colour.xyz *= light.colour.xyz;

    return lerp(float4(colour.xyz, 1.0f), float4(0.0f, 0.0f, 0.0f, 1.0f), shadowDensity);
}

#endif