#include "mplat.api.hlsl"
#include "shadow.api.hlsl"
#include "lights.api.hlsl"

float4 main(ObjectPixelVertex p) : SV_TARGET
{
    float3 Ng = normalize(p.worldNormal.xyz);   // no bump map here, so the interpolated normal is the geometric normal
    float shadowDensity = GetShadowDensity(p, Ng);
	float4 texel = p.colour;
    float I = GetDiffuseSpecularAndFoggedLighting(p);
    return BlendColourWithLightAndShadow(texel, shadowDensity, I);
}
