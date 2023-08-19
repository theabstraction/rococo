#include "mplat.api.hlsl"

LandVertex main(ObjectVertex v)
{
	LandVertex sv;
	sv.position = Transform_World_To_Screen(float4(v.modelPosition.xyz, 1.0f));
	sv.normal = Transform_Instance_To_World(float4(v.modelNormal.xyz, 0.0f));
	sv.uv_material_and_gloss.xy = v.uv.xy;
	sv.uv_material_and_gloss.z = v.materialIndexAndGloss.x;
	sv.uv_material_and_gloss.w = v.materialIndexAndGloss.y;
	return sv;
}
