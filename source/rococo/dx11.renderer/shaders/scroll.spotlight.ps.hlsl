#include "mplat.api.hlsl"
#include "shadow.api.hlsl"
#include "lights.api.hlsl"

float4 main(ObjectPixelVertex p) : SV_TARGET
{
    float3 uvw = float3(p.uv_and_gloss.xy, p.textureIndices.x);
    float4 texel = GetFontPixel(uvw, p.colour);
    float shadowDensity = GetShadowDensity(p);
    float I = GetDiffuseSpecularAndFoggedLighting(p);
    return BlendColourWithLightAndShadow(texel, shadowDensity, I);
}

