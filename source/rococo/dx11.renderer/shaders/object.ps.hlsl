#include "mplat.api.hlsl"
#include "shadows.api.hlsl"
#include "lights.api.hlsl"

float4 main(ObjectPixelVertex p) : SV_TARGET
{
    float3 Ng = normalize(p.worldNormal.xyz);   // geometric normal, before bumping
    p.worldNormal = GetBumpedWorldNormal(p);

    float shadowDensity = GetShadowDensity(p);
    float4 texel = SampleMaterial(p);

    float2 ds = GetDiffuseAndSpecular(p, Ng);   // x = diffuse, y = specular

    // Diffuse is tinted by the surface colour, specular is not
    float3 lit = (texel.xyz * ds.x + ds.y) * light.colour.xyz;

    return lerp(float4(lit, 1.0f), float4(0.0f, 0.0f, 0.0f, 1.0f), shadowDensity);
}