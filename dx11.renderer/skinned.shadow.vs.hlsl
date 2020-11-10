#include <mplat.api.hlsl>

struct ShadowOutVertex
{
	float4 position : SV_POSITION;
};

ShadowOutVertex main(ObjectVertex v, BoneWeight_2Bones weights)
{
	float4 modelPos = Transform_Model_Vertices_Via_Bone_Weights(v.position, weights);
	float4 worldPos = Transform_Instance_To_World(modelPos);

	ShadowOutVertex sv;
	sv.position = Transform_World_To_DepthBuffer(worldPos);
	return sv;
}