#include "mplat.api.hlsl"
#include "shadow.api.hlsl"
#include "lights.api.hlsl"

float4 main(ObjectPixelVertex p) : SV_TARGET
{
    float3 Ng = normalize(p.worldNormal.xyz);   // geometric normal, before bumping
    p.worldNormal = GetBumpedWorldNormal(p);

    float shadowDensity = GetShadowDensity(p, Ng);   // refShadowModel (PCF9 by default) with normal offset along the geometric normal
    float4 texel = SampleMaterial(p);

    float2 ds = GetDiffuseAndSpecular(p, Ng);   // x = diffuse, y = specular

    // Diffuse is tinted by the surface colour, specular is not
    float3 lit = (texel.xyz * ds.x + ds.y) * light.colour.xyz;
    float alpha = texel.a * light.colour.a;     // keeps this shader's original alpha behaviour

    return lerp(float4(lit, alpha), float4(0.0f, 0.0f, 0.0f, 0.0f), shadowDensity);
}