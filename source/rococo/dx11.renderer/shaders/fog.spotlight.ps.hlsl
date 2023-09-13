#include <mplat.api.hlsl>
#include "shadows.api.hlsl"
#include "lights.api.hlsl"

float4 main(ObjectPixelVertex p) : SV_TARGET
{
    float shadowDensity = GetShadowDensity(p);
    float4 texel = GetPointSpriteTexel(p.uv_material_and_gloss.xy, p.colour);
    float I = GetDiffuseSpecularAndFoggedLighting(p);
    return BlendColourWithLightAndShadow(texel, shadowDensity, I);
}

