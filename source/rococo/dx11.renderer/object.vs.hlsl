#include "mplat.api.hlsl"

ScreenVertex main(ObjectVertex v)
{
	ScreenVertex sv;
	float4 instancePos = Transform_Instance_To_World(v.position);
	sv.position = Transform_World_To_Screen(instancePos);
	sv.normal = Transform_Instance_To_World(float4(v.normal.xyz,0.0f));
	sv.worldPosition = instancePos;
	sv.shadowPos = Transform_World_To_ShadowBuffer(instancePos);
	sv.cameraSpacePosition = Transform_World_To_Camera(instancePos);
	sv.uv_material_and_gloss.xy = v.uv.xy;
	sv.uv_material_and_gloss.z = v.materialIndexAndGloss.x;
	sv.uv_material_and_gloss.w = v.materialIndexAndGloss.y;
	sv.colour = v.colour;
	return sv;
}