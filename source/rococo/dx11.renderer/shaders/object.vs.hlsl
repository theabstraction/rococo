#include "mplat.api.hlsl"

ObjectPixelVertex main(ObjectVertex v)
{
	ObjectPixelVertex sv;
	
    float4 worldPosition = ComputeWorldPosition(v);
	
    sv.position = Project_World_To_Screen(worldPosition);
    sv.worldNormal = ComputeWorldNormal(v);
    sv.worldPosition = worldPosition.xyz;
    sv.shadowPos = Transform_World_To_ShadowBuffer(worldPosition);
    sv.cameraSpacePosition = Transform_World_To_Camera(worldPosition);
	sv.uv_material_and_gloss.xy = v.uv.xy;
	sv.uv_material_and_gloss.z = v.materialIndexAndGloss.x;
	sv.uv_material_and_gloss.w = v.materialIndexAndGloss.y;
	sv.colour = v.colour;
	return sv;
}
