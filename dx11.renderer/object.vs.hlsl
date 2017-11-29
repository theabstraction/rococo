#include "mplat.api.hlsl"

struct ScreenVertex
{
	float4 position : SV_POSITION0;
	float4 uv_material_and_gloss: TEXCOORD;
	float4 worldPosition: TEXCOORD1;
	float4 normal : TEXCOORD2;
	float4 shadowPos: TEXCOORD3;
	float4 cameraSpacePosition: TEXCOORD4;
	float4 colour: COLOR0;
};

ScreenVertex main(ObjectVertex v)
{
	ScreenVertex sv;
	float4 instancePos = mul(instanceMatrix, v.position);
	sv.position = mul(worldMatrixAndProj, instancePos);
	sv.normal = mul(instanceMatrix, float4(v.normal.xyz,0.0f));
	sv.worldPosition = instancePos;
	sv.shadowPos = mul(light.worldToShadowBuffer, instancePos);
	sv.cameraSpacePosition = mul(worldMatrix, instancePos);
	sv.uv_material_and_gloss.xy = v.uv.xy;
	sv.uv_material_and_gloss.z = v.materialIndexAndGloss.x;
	sv.uv_material_and_gloss.w = v.materialIndexAndGloss.y;
	sv.colour = v.colour;
	return sv;
}
