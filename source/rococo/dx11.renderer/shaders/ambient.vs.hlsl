#include "mplat.api.hlsl"

ObjectPixelVertex main(ObjectVertex v)
{
	ObjectPixelVertex sv;

    float4 worldPosition = ComputeWorldPosition(v);
    sv.position = Project_World_To_Screen(worldPosition);
    sv.worldNormal = ComputeWorldNormal(v);
    sv.cameraSpacePosition = Transform_World_To_Camera(worldPosition);
	sv.shadowPos = float4(0.0f, 0.0f, 0.0f, 0.0f);
    sv.worldPosition = worldPosition.xyz;
	sv.uv_material_and_gloss.xy = v.uv.xy;
	sv.uv_material_and_gloss.zw = v.materialIndexAndGloss.xy;
	sv.colour = v.colour;
	return sv;
}