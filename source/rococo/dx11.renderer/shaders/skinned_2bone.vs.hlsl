#include "mplat.api.hlsl"

ObjectPixelVertex main(ObjectVertex v, BoneWeight_2Bones weights)
{
	ObjectPixelVertex sv;

	float4 modelPos = Transform_Model_Vertices_Via_Bone_Weights(float4(v.modelPosition.xyz, 1.0f), weights);
	float4 worldPos = Transform_Instance_To_World(modelPos);
	sv.position = Project_World_To_Screen(worldPos);
	sv.worldNormal = Transform_Instance_To_World(float4(v.modelNormal.xyz, 0.0f)).xyz;
	sv.worldPosition = worldPos.xyz;
	sv.shadowPos = Transform_World_To_ShadowBuffer(worldPos);
	sv.cameraSpacePosition = Transform_World_To_Camera(worldPos);
	sv.uv_material_and_gloss.xy = v.uv.xy;
	sv.uv_material_and_gloss.z = v.materialIndexAndGloss.x;
	sv.uv_material_and_gloss.w = v.materialIndexAndGloss.y;
	sv.colour = v.colour;
	return sv;
}
