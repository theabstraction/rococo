#include "mplat.api.hlsl"

ObjectPixelVertex main(ObjectVertex v)
{
	ObjectPixelVertex sv;
	float4 instancePos = Transform_Instance_To_World_Scaled(v.modelPosition);
	sv.position = Transform_World_To_Screen(float4(instancePos.xyz, 1.0f));
	sv.worldNormal = Transform_Instance_To_World(float4(v.modelNormal.xyz,0.0f)).xyz;
	sv.worldPosition = instancePos.xyz;
	sv.shadowPos = Transform_World_To_ShadowBuffer(instancePos);
	sv.cameraSpacePosition = Transform_World_To_Camera(instancePos);
	sv.uv_material_and_gloss.xy = v.uv.xy;
	sv.uv_material_and_gloss.z = v.materialIndexAndGloss.x;
	sv.uv_material_and_gloss.w = v.materialIndexAndGloss.y;
	sv.colour = v.colour;
	return sv;
}
