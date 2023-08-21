#include "mplat.api.hlsl"
#include "shadows.api.hlsl"

float4 main(ObjectPixelVertex p) : SV_TARGET
{
    float shadowDensity = GetShadowDensity(p);
    float4 texel = SampleMaterial(p);

    float I = GetDiffuseSpecularAndFoggedLighting(p);

	texel.xyz *= I;

    return lerp(texel * light.colour, float4(0, 0, 0, 0), shadowDensity);
}
