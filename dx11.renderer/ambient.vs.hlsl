struct ObjectVertex
{
	float4 position : POSITION;
	float4 normal : NORMAL;
	float2 uv: TEXCOORD0;
	float4 colour: COLOR0;
	float  materialIndex : TEXCOORD1;
};

struct ScreenVertex
{
	float4 position : SV_POSITION0;
	float4 normal : NORMAL;
	float3 uv_material: TEXCOORD;
	float4 cameraSpacePosition: TEXCOORD1;
	float4 worldPosition: TEXCOORD2;
	float4 colour: COLOR0;
};

#pragma pack_matrix(row_major)

cbuffer globalState
{
	float4x4 worldMatrixAndProj;
	float4x4 worldMatrix;
};

cbuffer perInstanceData
{
	float4x4 instanceMatrix;
	float4 highlightColour;
}

ScreenVertex main(ObjectVertex v)
{
	ScreenVertex sv;

	float4 instancePos = mul(instanceMatrix, v.position);
	sv.position = mul(worldMatrixAndProj, instancePos);
	sv.normal = v.normal;
	sv.cameraSpacePosition = mul(worldMatrix, instancePos);
	sv.worldPosition = instancePos;
	sv.uv_material.xy = v.uv.xy;
	sv.uv_material.z = v.materialIndex;
	sv.colour = v.colour;
	return sv;
}