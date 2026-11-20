#include "mplat.api.hlsl"
#include "shadows.api.hlsl"
#include "lights.api.hlsl"

float4 main(ObjectPixelVertex p) : SV_TARGET
{
	float3 Ng = normalize(p.worldNormal.xyz);  
    p.worldNormal = GetBumpedWorldNormal(p);
    float shadowDensity = GetShadowDensity(p);
    float4 texel = SampleMaterial(p);

    float I = GetDiffuseSpecularAndFoggedLighting(p);
	float3 Ldir = normalize(GetLightToWorldPosition(p));
    I *= saturate(-dot(Ng, Ldir) * 4.0f); 

    return BlendColourWithLightAndShadow(texel, shadowDensity, I);
}
