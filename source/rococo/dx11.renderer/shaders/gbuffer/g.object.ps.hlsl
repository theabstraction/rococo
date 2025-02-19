#include "..\mplat.api.hlsl"
#include "..\shadows.api.hlsl"
#include "..\lights.api.hlsl"

GBufferOutput main(ObjectPixelVertex p)
{
    GBufferOutput output;
	//float shadowDensity = GetShadowDensity(p);
    float4 texel = SampleMaterial(p);
    output.normal = float4(p.worldNormal.xyz, 0.0f);
    output.depth = float4(1.0f, 0.0f, 0.0f, 0.0f);
    output.colour = texel.xyzw;
    return output;
    //float I = GetDiffuseSpecularAndFoggedLighting(p);
    //return BlendColourWithLightAndShadow(texel, shadowDensity, I);
}
