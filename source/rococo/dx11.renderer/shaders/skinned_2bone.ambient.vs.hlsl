#include "mplat.api.hlsl"

struct AmbientVertex
{
	float4 position : SV_POSITION0;
	float4 normal : NORMAL;
	float3 uv_and_gloss: TEXCOORD;
	float4 cameraSpacePosition: TEXCOORD1;
	float4 worldPosition: TEXCOORD2;
	float2 textureIndices: TEXCOORD5;
	float4 colour: COLOR0;
};

AmbientVertex main(ObjectVertex v, BoneWeight_2Bones weights)
{
	AmbientVertex sv;

	float4 modelPos = Transform_Model_Vertices_Via_Bone_Weights(float4(v.modelPosition,1.0f), weights);
	float4 worldPos = Transform_Instance_To_World(modelPos);
	sv.position = Project_World_To_Screen(worldPos);
	sv.normal = Transform_Instance_To_World(float4(v.modelNormal, 0.0f));
	sv.cameraSpacePosition = Transform_World_To_Camera(worldPos);
	sv.worldPosition = worldPos;
	sv.uv_and_gloss.xy = v.uv.xy;
	sv.uv_and_gloss.z = v.colourNormalAndGloss.z;
	sv.textureIndices.x = v.colourNormalAndGloss.x;
	sv.textureIndices.y = v.colourNormalAndGloss.y;
	sv.colour = v.colour;
	return sv;
}