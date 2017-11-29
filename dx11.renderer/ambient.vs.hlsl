#include "mplat.api.hlsl"

struct ScreenVertex
{
	float4 position : SV_POSITION0;
	float4 normal : NORMAL;
	float4 uv_material_gloss: TEXCOORD;
	float4 cameraSpacePosition: TEXCOORD1;
	float4 worldPosition: TEXCOORD2;
	float4 colour: COLOR0;
};

ScreenVertex main(ObjectVertex v)
{
	ScreenVertex sv;

	float4 instancePos = mul(instanceMatrix, v.position);
	sv.position = mul(worldMatrixAndProj, instancePos);
	sv.normal = v.normal;
	sv.cameraSpacePosition = mul(worldMatrix, instancePos);
	sv.worldPosition = instancePos;
	sv.uv_material_gloss.xy = v.uv.xy;
	sv.uv_material_gloss.zw = v.materialIndexAndGloss.xy;
	sv.colour = v.colour;
	return sv;
}