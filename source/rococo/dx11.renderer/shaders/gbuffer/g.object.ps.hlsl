#include "..\mplat.api.hlsl"
#include "..\shadows.api.hlsl"
#include "..\lights.api.hlsl"

GBufferOutput main(ObjectPixelVertex p)
{
    GBufferOutput output;
	output.colour = float4(SampleMaterial(p).xyz, p.uv_material_and_gloss.w);
    output.normal = float4(p.worldNormal.xyz, 0.0f);
    output.depth = float4(p.position.w, 0.0f, 0.0f, 0.0f);
	output.position = float4(p.worldPosition.xyz, 1.0f);
    return output;
}
