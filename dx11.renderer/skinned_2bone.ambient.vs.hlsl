#include "mplat.api.hlsl"

struct AmbientVertex
{
	float4 position : SV_POSITION0;
	float4 normal : NORMAL;
	float4 uv_material_gloss: TEXCOORD;
	float4 cameraSpacePosition: TEXCOORD1;
	float4 worldPosition: TEXCOORD2;
	float4 colour: COLOR0;
};

AmbientVertex main(ObjectVertex v, BoneWeight_2Bones weights)
{
	AmbientVertex sv;

	float4 modelPos = Transform_Model_Via_Skinned(v.position, weights);
	float4 instancePos = Transform_Instance_To_World(modelPos);
	sv.position = Transform_World_To_Screen(instancePos);
	sv.normal = Transform_Instance_To_World(v.normal);
	sv.cameraSpacePosition = Transform_World_To_Camera(instancePos);
	sv.worldPosition = instancePos;
	sv.uv_material_gloss.xy = v.uv.xy;
	sv.uv_material_gloss.zw = v.materialIndexAndGloss.xy;
	sv.colour = v.colour;
	return sv;
}