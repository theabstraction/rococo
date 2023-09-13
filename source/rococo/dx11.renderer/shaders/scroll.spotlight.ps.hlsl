#include "mplat.api.hlsl"
#include "shadows.api.hlsl"
#include "lights.api.hlsl"

float4 main(ObjectPixelVertex p) : SV_TARGET
{
	float4 texel = GetFontPixel(p.uv_material_and_gloss.xyw, p.colour);
    float shadowDensity = GetShadowDensity(p);
    float I = GetDiffuseSpecularAndFoggedLighting(p);
    return BlendColourWithLightAndShadow(texel, shadowDensity, I);
}

