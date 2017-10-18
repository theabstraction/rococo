struct ObjectVertex
{
	float4 position : POSITION;
	float4 normal : NORMAL;	
	float4 emissiveColour: COLOR0;
	float4 diffuseColour: COLOR1;
	float2 uv: TEXCOORD;
};

struct ScreenVertex
{
	float4 position : SV_POSITION;
	float4 normal : NORMAL;
	float2 uv: TEXCOORD;
	float4 worldPosition: TEXCOORD1;
};

#pragma pack_matrix(row_major)

cbuffer globalState
{
	float4x4 worldMatrixAndProj;
	float4x4 worldMatrix;
	float4 lightDir;
	float4 lightPos;
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

	v.normal.w = 0;

	sv.position = mul(worldMatrixAndProj, instancePos);
	sv.normal = mul(instanceMatrix, v.normal);
	sv.worldPosition = instancePos;
	sv.uv = v.uv;

	return sv;
}