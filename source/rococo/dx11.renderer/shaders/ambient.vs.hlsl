#include "mplat.api.hlsl"

ObjectPixelVertex main(ObjectVertex v)
{
	ObjectPixelVertex sv;

	float4 instancePos = Transform_Instance_To_World_Scaled(v.position);
	sv.position = Transform_World_To_Screen(float4(instancePos.xyz, 1.0f));
	sv.worldNormal = Transform_Instance_To_World(float4(v.normal.xyz, 0.0f)).xyz;
	sv.cameraSpacePosition = Transform_World_To_Camera(instancePos);
	sv.shadowPos = float4(0.0f, 0.0f, 0.0f, 0.0f);
	sv.worldPosition = instancePos.xyz;
	sv.uv_material_and_gloss.xy = v.uv.xy;
	sv.uv_material_and_gloss.zw = v.materialIndexAndGloss.xy;
	sv.colour = v.colour;
	return sv;
}