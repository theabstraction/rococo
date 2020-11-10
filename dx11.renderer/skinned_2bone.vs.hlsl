#include "mplat.api.hlsl"

ScreenVertex main(ObjectVertex v, BoneWeight_2Bones weights)
{
	ScreenVertex sv;

	float4 modelPos = Transform_Model_Vertices_Via_Bone_Weights(v.position, weights);
	float4 worldPos = Transform_Instance_To_World(modelPos);
	sv.position = Transform_World_To_Screen(worldPos);
	sv.normal = Transform_Instance_To_World(float4(v.normal.xyz, 0.0f));
	sv.worldPosition = worldPos;
	sv.shadowPos = Transform_World_To_ShadowBuffer(worldPos);
	sv.cameraSpacePosition = Transform_World_To_Camera(worldPos);
	sv.uv_material_and_gloss.xy = v.uv.xy;
	sv.uv_material_and_gloss.z = v.materialIndexAndGloss.x;
	sv.uv_material_and_gloss.w = v.materialIndexAndGloss.y;
	sv.colour = v.colour;
	return sv;
}
